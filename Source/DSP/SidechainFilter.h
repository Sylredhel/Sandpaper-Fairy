#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

namespace sylredhel::sandpaper
{

enum class ScFilterMode : int
{
    Off      = 0, // Full-band sidechain detector
    HighPass = 1, // 12 dB/oct TPT SVF HPF on sidechain (ignores kick/sub below SC Freq)
    LowPass  = 2, // 12 dB/oct TPT SVF LPF on sidechain (ignores hi-hats/air above SC Freq)
    BandPass = 3  // 12 dB/oct TPT SVF Bandpass on sidechain (focuses on spikes around SC Freq)
};

/**
 * Stereo Cytomic TPT Zero-Delay-Feedback State-Variable Filter (12 dB/oct)
 * Used exclusively on the sidechain detector path (zero phase coloration on main audio!).
 */
class SidechainSvf
{
public:
    void prepare(double sampleRate)
    {
        fs = std::max(sampleRate, 8000.0);
        lastFc = -1.0;
        updateCoeffs(1200.0);
        reset();
    }

    void reset() noexcept
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            ic1eq[ch] = 0.0;
            ic2eq[ch] = 0.0;
        }
    }

    inline void processStereo(double inL, double inR, ScFilterMode mode, double fcHz,
                              double& outL, double& outR) noexcept
    {
        if (mode == ScFilterMode::Off)
        {
            outL = inL;
            outR = inR;
            return;
        }

        const double fc = std::clamp(fcHz, 30.0, 0.42 * fs);
        if (std::abs(fc - lastFc) > fc * 0.004)
            updateCoeffs(fc);

        const double inp[2] = { inL, inR };
        double out[2] = { inL, inR };

        for (int ch = 0; ch < 2; ++ch)
        {
            const double v0 = inp[ch];
            const double v3 = v0 - ic2eq[ch];
            const double v1 = a1 * ic1eq[ch] + a2 * v3;
            const double v2 = ic2eq[ch] + a2 * ic1eq[ch] + a3 * v3;
            ic1eq[ch] = 2.0 * v1 - ic1eq[ch];
            ic2eq[ch] = 2.0 * v2 - ic2eq[ch];

            if (mode == ScFilterMode::HighPass)
                out[ch] = v0 - k * v1 - v2;
            else if (mode == ScFilterMode::LowPass)
                out[ch] = v2;
            else // ScFilterMode::BandPass (unity gain at center frequency: k * v1)
                out[ch] = k * v1;
        }

        outL = out[0];
        outR = out[1];
    }

private:
    void updateCoeffs(double fc) noexcept
    {
        lastFc = fc;
        constexpr double pi = 3.14159265358979323846;
        // Butterworth Q = 0.7071 for clean HPF/LPF & musical ~1.4-octave BandPass focus
        k = 1.4142135623730951;
        const double g = std::tan(pi * fc / fs);
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    double fs = 48000.0;
    double lastFc = -1.0;
    double k = 1.41421356, a1 = 0.0, a2 = 0.0, a3 = 0.0;
    double ic1eq[2] = { 0.0, 0.0 };
    double ic2eq[2] = { 0.0, 0.0 };
};

class IntDelayLine
{
public:
    void prepare(int maxDelaySamples)
    {
        int sz = 16;
        while (sz < maxDelaySamples + 16)
            sz <<= 1;
        bufMask = sz - 1;
        buf.assign(static_cast<size_t>(sz), 0.0);
        reset();
    }

    void reset()
    {
        std::fill(buf.begin(), buf.end(), 0.0);
        writePos = 0;
    }

    inline double pushAndRead(double x, int delaySamples) noexcept
    {
        buf[static_cast<size_t>(writePos)] = x;
        const int d = std::clamp(delaySamples, 0, bufMask - 1);
        const double y = buf[static_cast<size_t>((writePos - d) & bufMask)];
        writePos = (writePos + 1) & bufMask;
        return y;
    }

private:
    int bufMask = 2047;
    int writePos = 0;
    std::vector<double> buf;
};

class FractionalDelayLine
{
public:
    void prepare(int maxDelaySamples)
    {
        int sz = 16;
        while (sz < maxDelaySamples + 16)
            sz <<= 1;
        bufSize = sz;
        bufMask = sz - 1;
        for (int ch = 0; ch < 2; ++ch)
            buf[ch].assign(static_cast<size_t>(bufSize), 0.0);
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
            std::fill(buf[ch].begin(), buf[ch].end(), 0.0);
        writePos = 0;
    }

    inline void pushAndRead(double inL, double inR, double delaySamples,
                            double& outL, double& outR) noexcept
    {
        buf[0][static_cast<size_t>(writePos)] = inL;
        buf[1][static_cast<size_t>(writePos)] = inR;

        if (delaySamples < 1.0)
        {
            outL = inL;
            outR = inR;
            writePos = (writePos + 1) & bufMask;
            return;
        }

        const double d = std::clamp(delaySamples, 1.0, static_cast<double>(bufSize - 4));
        const double readPos = static_cast<double>(writePos) - d;
        const int i1 = static_cast<int>(std::floor(readPos));
        const double f = readPos - static_cast<double>(i1);

        const int idx0 = (i1 - 1) & bufMask;
        const int idx1 = (i1)     & bufMask;
        const int idx2 = (i1 + 1) & bufMask;
        const int idx3 = (i1 + 2) & bufMask;

        const double c0 = -f * (f - 1.0) * (f - 2.0) * (1.0 / 6.0);
        const double c1 =  (f + 1.0) * (f - 1.0) * (f - 2.0) * 0.5;
        const double c2 = -(f + 1.0) * f * (f - 2.0) * 0.5;
        const double c3 =  (f + 1.0) * f * (f - 1.0) * (1.0 / 6.0);

        outL = c0 * buf[0][static_cast<size_t>(idx0)] + c1 * buf[0][static_cast<size_t>(idx1)]
             + c2 * buf[0][static_cast<size_t>(idx2)] + c3 * buf[0][static_cast<size_t>(idx3)];
        outR = c0 * buf[1][static_cast<size_t>(idx0)] + c1 * buf[1][static_cast<size_t>(idx1)]
             + c2 * buf[1][static_cast<size_t>(idx2)] + c3 * buf[1][static_cast<size_t>(idx3)];

        writePos = (writePos + 1) & bufMask;
    }

private:
    int bufSize = 4096;
    int bufMask = 4095;
    int writePos = 0;
    std::vector<double> buf[2];
};

} // namespace sylredhel::sandpaper
