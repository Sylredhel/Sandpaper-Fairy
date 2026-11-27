#pragma once
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "SidechainFilter.h"

namespace sylredhel::sandpaper
{

enum class EngineMode : int
{
    Rider  = 0, // Crisp VCA Crest-Factor Compressor (linear ratio + optional crest-driven Grit!)
    VariMu = 1  // Progressive Soft-Ratio Vari-Mu Compression + Crest-Driven Tube Saturation
};

enum class AuditionMode : int
{
    Normal    = 0,
    Shavings  = 1, // Hear ONLY the exact shaved-off spike dust (Dry - Wet)
    ScMonitor = 2  // Hear ONLY the filtered Sidechain Detector signal (for dialing SC Freq!)
};

struct SandpaperParams
{
    EngineMode   engineMode    = EngineMode::Rider;
    ScFilterMode scFilterMode  = ScFilterMode::Off;
    double       scFreqHz      = 400.0;  // Sidechain filter frequency (40 Hz .. 12 kHz)
    double       crestThreshDb = 7.0;    // Crest Factor threshold (0.0 .. 18.0 dB)
    double       rmsWinMs      = 180.0;  // Slow phrase power-RMS window (40 .. 500 ms)
    double       floorDb       = -48.0;  // Noise floor guard (-72 .. -18 dBFS)
    double       ratio         = 6.0;    // Compression ratio (1.0 .. 100.0; 1.0 = pure sat!)
    double       kneeDb        = 2.5;    // Soft-knee width (0.0 .. 10.0 dB)
    double       attackMs      = 1.2;    // Attack time (0.1 .. 30.0 ms)
    double       releaseMs     = 22.0;   // Release time (5.0 .. 400.0 ms)
    bool         autoRelease   = false;  // Program-adaptive dual-stage release ON/OFF
    double       lookaheadMs   = 2.0;    // Pre-peak onset lookahead (0.0 .. 15.0 ms)
    double       gritDriveDb   = 0.0;    // Crest-Followed Saturator Drive (0.0 .. 24.0 dB, works in BOTH modes!)
    double       maxGrDb       = 24.0;   // Maximum gain reduction ceiling
    double       stereoLink01  = 1.0;    // Continuous stereo link (0.0 .. 1.0)
};

class DriftFreeBoxcar
{
public:
    static constexpr double kScale    = 16777216.0; // 2^24
    static constexpr double kInvScale = 1.0 / kScale;

    void prepare(int maxLength)
    {
        int sz = 16;
        while (sz < maxLength + 16)
            sz <<= 1;
        ringMask = sz - 1;
        ring.assign(static_cast<size_t>(sz), 0);
        currentLen = std::max(1, std::min(16, maxLength));
        invLen = 1.0 / static_cast<double>(currentLen);
        reset();
    }

    void reset()
    {
        std::fill(ring.begin(), ring.end(), 0);
        writePos = 0;
        sumFixed = 0;
    }

    void setLength(int newLen) noexcept
    {
        const int clamped = std::clamp(newLen, 1, ringMask - 2);
        if (clamped == currentLen)
            return;

        currentLen = clamped;
        invLen = 1.0 / static_cast<double>(currentLen);

        int64_t acc = 0;
        for (int k = 1; k <= currentLen; ++k)
            acc += ring[static_cast<size_t>((writePos - k) & ringMask)];
        sumFixed = acc;
    }

    inline double process(double x) noexcept
    {
        const int64_t q = static_cast<int64_t>(std::llround(std::clamp(x, -128.0, 128.0) * kScale));
        const int oldIdx = (writePos - currentLen) & ringMask;
        sumFixed += q - ring[static_cast<size_t>(oldIdx)];
        ring[static_cast<size_t>(writePos)] = q;
        writePos = (writePos + 1) & ringMask;
        return static_cast<double>(sumFixed) * (kInvScale * invLen);
    }

private:
    int ringMask = 1023;
    int writePos = 0;
    int currentLen = 16;
    double invLen = 1.0 / 16.0;
    int64_t sumFixed = 0;
    std::vector<int64_t> ring;
};

class CircularSlidingMax
{
public:
    void prepare(int maxWindow)
    {
        int sz = 16;
        while (sz < maxWindow + 16)
            sz <<= 1;
        mask = sz - 1;
        idxRing.assign(static_cast<size_t>(sz), 0);
        valRing.assign(static_cast<size_t>(sz), 0.0);
        reset();
    }

    void reset()
    {
        head = 0;
        tail = 0;
        sampleCount = 0;
    }

    inline double process(double x, int windowLen) noexcept
    {
        const int W = std::max(1, windowLen);
        const int64_t now = sampleCount++;

        while (tail != head)
        {
            const int back = (tail - 1) & mask;
            if (valRing[static_cast<size_t>(back)] <= x)
                tail = back;
            else
                break;
        }

        idxRing[static_cast<size_t>(tail)] = now;
        valRing[static_cast<size_t>(tail)] = x;
        tail = (tail + 1) & mask;

        const int64_t expire = now - static_cast<int64_t>(W);
        while (head != tail && idxRing[static_cast<size_t>(head)] <= expire)
            head = (head + 1) & mask;

        return (head != tail) ? valRing[static_cast<size_t>(head)] : x;
    }

private:
    int mask = 2047;
    int head = 0;
    int tail = 0;
    int64_t sampleCount = 0;
    std::vector<int64_t> idxRing;
    std::vector<double>  valRing;
};

/**
 * Oversampled Output Stage:
 *   1. Clean VCA / Progressive Vari-Mu Wideband Gain Reduction (`grDb`)
 *   2. Crest-Factor-Envelope-Driven Asymmetric Tube Saturator (`liveDriveDb = gritDriveDb * crestEnv`)
 *      NOW ACTIVE IN BOTH RIDER AND VARI-MU MODES!
 *      - Set GRIT DRIVE = 0.0 dB for 100% pure zero-distortion VCA riding.
 *      - Dial +2..+6 dB in RIDER mode to keep transients sparkling & un-muffled!
 *      - Set RATIO = 1.0:1 + GRIT DRIVE > 0 dB for a 100% pure Crest-Followed Saturator!
 */
class SandpaperOutputStage
{
public:
    void prepareForRate(double effectiveRate)
    {
        const double fs = std::max(effectiveRate, 8000.0);
        if (std::abs(fs - currentFs) < 1.0)
            return;
        currentFs = fs;
        dcAlpha = std::exp(-6.283185307179586 * 12.0 / fs);
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            dcState[ch]  = 0.0;
            dcPrevIn[ch] = 0.0;
        }
    }

    inline double processSample(double x, double grDb, double crestEnv01, double refCeilLin,
                                double gritDriveDb, double floorLin, int ch) noexcept
    {
        // 1. Wideband VCA / Vari-Mu Gain Reduction (0 dB when RATIO == 1.0:1)
        const double gain  = std::pow(10.0, -std::clamp(grDb, 0.0, 24.0) * 0.05);
        const double yComp = x * gain;

        // 2. Crest-Followed Asymmetric Tube Saturator (freely available in BOTH RIDER & VARI-MU!)
        if (gritDriveDb > 0.05 && crestEnv01 > 1e-4)
        {
            const double liveDrvDb = gritDriveDb * std::clamp(crestEnv01, 0.0, 1.0);
            const double drvLin    = std::pow(10.0, liveDrvDb * 0.05);
            const double cRef      = std::max(refCeilLin, std::max(floorLin, 1e-5));

            const double u = (yComp / cRef) * drvLin;

            // Warm asymmetric triode curve (rich 2nd + 3rd harmonics, fixed ceiling at +/- cRef)
            constexpr double bias   = 0.14;
            constexpr double tb     = 0.1390924479221749;
            constexpr double invPos = 1.0 / (1.0 - tb);
            constexpr double invNeg = 1.0 / (1.0 + tb);

            const double raw  = std::tanh(u + bias) - tb;
            const double norm = (raw >= 0.0) ? (raw * invPos) : (raw * invNeg);
            const double ySat = cRef * norm;

            // Smooth crossfade over first 1.8 dB of live drive so 0 dB is exact dry wire
            const double ub    = std::clamp(liveDrvDb * (1.0 / 1.8), 0.0, 1.0);
            const double blend = ub * ub * (3.0 - 2.0 * ub);
            const double rawOut = yComp + blend * (ySat - yComp);

            const double hp = dcAlpha * (dcState[ch] + rawOut - dcPrevIn[ch]);
            dcPrevIn[ch] = rawOut;
            dcState[ch]  = hp;
            return hp;
        }
        else
        {
            dcPrevIn[ch] = yComp;
            dcState[ch]  = yComp;
            return yComp;
        }
    }

private:
    double currentFs   = 0.0;
    double dcAlpha     = 0.9998;
    double dcState[2]  = { 0.0, 0.0 };
    double dcPrevIn[2] = { 0.0, 0.0 };
};

/**
 * Per-Channel Crisp Crest-Factor Detector & Fast-Breathing Ballistics Core.
 *
 * Fixed constant frontend latency = 75 ms (zero FIR crossover overhead, zero PDC shift!):
 *   - dCtx   = 60 ms (future sustained power-RMS over [t+35..60ms] protects silence->phrase starts!)
 *   - dLaMax = 15 ms (user adjustable pre-peak onset lookahead 0 .. 15 ms!)
 *
 * Anti-Muffling Improvements:
 *   - Fast 2.2 ms Peak Detector Release (was 12 ms!) -> drops cleanly right after each spike!
 *   - Tight Micro-Hold (<= 2.0 ms, only when Release < 16 ms) -> no bloated release tail!
 *   - Refined Auto-Release (only engages when sustained GR > 3.0 dB, max 2.2x) -> drums breathe!
 */
class CrestDetectorChannel
{
public:
    void prepare(double sampleRate)
    {
        fs = std::max(sampleRate, 8000.0);

        dCtx   = std::max(32, static_cast<int>(std::round(0.060 * fs))); // 60 ms gapped context guard
        lFut   = std::max(16, static_cast<int>(std::round(0.025 * fs))); // 25 ms future sustained RMS [t+35..60ms]
        dLaMax = std::max(16, static_cast<int>(std::round(0.015 * fs))); // 15 ms max onset lookahead
        totalRiderLatency = dCtx + dLaMax;                               // 75 ms exact!

        futMsqBox.prepare(lFut + 16);
        futMsqBox.setLength(lFut);

        // Crisp 0.15 ms attack / 2.2 ms release peak follower -> releases immediately after sharp spikes!
        pkAtkAlpha = 1.0 - std::exp(-1.0 / (0.00015 * fs));
        pkRelAlpha = 1.0 - std::exp(-1.0 / (0.00250 * fs));

        // Refined Program-Adaptive integrator (65 ms attack, 200 ms decay -> ignores isolated spikes!)
        intAtkAlpha = 1.0 - std::exp(-1.0 / (0.065 * fs));
        intRelAlpha = 1.0 - std::exp(-1.0 / (0.200 * fs));

        // Smooth 8 ms reference RMS smoother for main-audio saturator ceiling C_ref
        ceilSmoothAlpha = 1.0 - std::exp(-1.0 / (0.008 * fs));

        pkLinDelay.prepare(dCtx + 16);
        scXSqDelay.prepare(dCtx + 16);
        audioXSqDelay.prepare(dCtx + 16);
        rmsAlignDelay.prepare(dLaMax + 16);

        maxSlideHold = dLaMax + static_cast<int>(std::ceil(0.004 * fs)) + 16;
        slideMaxGr.prepare(maxSlideHold);
        slideMaxEnv.prepare(maxSlideHold);
        extraGrDelay.prepare(dLaMax + 16);
        extraEnvDelay.prepare(dLaMax + 16);

        lastRmsMs = -1.0; lastAtkMs = -1.0; lastRelMs = -1.0; lastLaMs = -1.0;
        updateTiming(180.0, 1.2, 22.0, 2.0);
        reset();
    }

    void reset()
    {
        ePeak        = 1e-7;
        slowScMsq    = 1e-14;
        slowAudioMsq = 1e-14;
        smoothAudRms = 1e-4;
        gf = 0.0; gs = 0.0; sust = 0.0;
        ef = 0.0; es = 0.0;

        futMsqBox.reset();
        pkLinDelay.reset();
        scXSqDelay.reset();
        audioXSqDelay.reset();
        rmsAlignDelay.reset();
        slideMaxGr.reset();
        slideMaxEnv.reset();
        extraGrDelay.reset();
        extraEnvDelay.reset();
    }

    int getRiderLatencySamples() const noexcept { return totalRiderLatency; }

    void updateTiming(double rmsWinMs, double attackMs, double releaseMs, double lookaheadMs) noexcept
    {
        if (std::abs(rmsWinMs - lastRmsMs) > 0.5)
        {
            lastRmsMs = rmsWinMs;
            rmsAlpha = 1.0 - std::exp(-1.0 / (std::clamp(rmsWinMs, 40.0, 500.0) * 0.001 * fs));
        }

        if (std::abs(attackMs - lastAtkMs) > 0.02)
        {
            lastAtkMs = attackMs;
            atkAlpha = 1.0 - std::exp(-1.0 / (std::clamp(attackMs, 0.1, 35.0) * 0.001 * fs));
        }

        if (std::abs(releaseMs - lastRelMs) > 0.1 || std::abs(lookaheadMs - lastLaMs) > 0.05)
        {
            lastRelMs = releaseMs;
            lastLaMs  = lookaheadMs;

            const double relClamped = std::clamp(releaseMs, 5.0, 400.0);
            relFastAlpha = 1.0 - std::exp(-1.0 / (relClamped * 0.001 * fs));
            // Gentler Auto-Release slow branch (2.2x, floor 50 ms) so grooves breathe!
            relSlowAlpha = 1.0 - std::exp(-1.0 / (std::max(relClamped * 2.2, 50.0) * 0.001 * fs));

            curLaSamp = std::clamp(static_cast<int>(std::round(std::clamp(lookaheadMs, 0.0, 15.0) * 0.001 * fs)),
                                   0, dLaMax);
            // Tight micro-hold (<= 2.0 ms, only active when Release < 16 ms) prevents bass chatter without muffling!
            const double holdMs = std::clamp((16.0 - relClamped) * (1.0 / 11.0), 0.0, 1.0) * 2.0;
            const int holdSamp = static_cast<int>(std::round(holdMs * 0.001 * fs));
            curSlideWin = std::max(1, curLaSamp + holdSamp);
            curExtraDelay = std::clamp(dLaMax - curLaSamp, 0, dLaMax);
        }
    }

    /**
     * Step 1: Analyze filtered sidechain `scSample` for Crest Factor, while tracking
     *         main-audio RMS from `audioSample` so the wideband saturator ceiling C_ref
     *         stays properly calibrated even when SC HPF/LPF/BandPass is active!
     */
    inline void analyzeCrest(double scSample, double audioSample, const SandpaperParams& p,
                             double& outTargetGrDb, double& outTargetCrestEnv01,
                             double& outAlignedAudioRmsLin, double& outCrestDb) noexcept
    {
        const double axSc  = std::abs(scSample);
        const double scXSq = scSample * scSample;
        const double auXSq = audioSample * audioSample;

        ePeak += ((axSc > ePeak) ? pkAtkAlpha : pkRelAlpha) * (axSc - ePeak);

        // Future sustained sidechain power-RMS over [t + 35 ms .. t + 60 ms]
        const double futScMsq = std::max(futMsqBox.process(scXSq), 1e-14);

        // Align sidechain peak, sidechain x^2, and main audio x^2 to t = now - 60 ms
        const double evalPkLin = std::max(pkLinDelay.pushAndRead(ePeak, dCtx), 1e-7);
        const double evalScXSq = scXSqDelay.pushAndRead(scXSq, dCtx);
        const double evalAuXSq = audioXSqDelay.pushAndRead(auXSq, dCtx);

        // Sidechain Crest Factor calculation
        slowScMsq += rmsAlpha * (evalScXSq - slowScMsq);
        const double refScMsq = std::max(std::max(slowScMsq, futScMsq), 1e-14);

        // Wideband Audio RMS (for accurate wideband saturator ceiling C_ref even when SC is filtered!)
        slowAudioMsq += rmsAlpha * (evalAuXSq - slowAudioMsq);
        const double audRms = std::sqrt(std::max(slowAudioMsq, 1e-14));

        const double evalPkDb = 20.0 * std::log10(evalPkLin);
        const double refScDb  = 10.0 * std::log10(refScMsq);
        const double cfDb     = std::max(0.0, evalPkDb - refScDb);
        outCrestDb = cfDb;

        // Soft-knee excess above CREST THRESH (0.0 .. 18.0 dB)
        const double k = std::max(p.kneeDb, 0.20);
        const double diff = cfDb - p.crestThreshDb;
        double overDb = 0.0;
        if (diff >= 0.5 * k)
            overDb = diff;
        else if (diff > -0.5 * k)
        {
            const double tk = diff + 0.5 * k;
            overDb = (tk * tk) / (2.0 * k);
        }

        // Smooth Floor Gate over [floorDb - 6, floorDb + 6]
        const double uFl = std::clamp((evalPkDb - (p.floorDb - 6.0)) * (1.0 / 12.0), 0.0, 1.0);
        const double floorGate = uFl * uFl * (3.0 - 2.0 * uFl);
        const double overGated = overDb * floorGate;

        const double ratioSafe = std::max(p.ratio, 1.0);
        if (p.engineMode == EngineMode::Rider)
        {
            const double slope = 1.0 - (1.0 / ratioSafe);
            outTargetGrDb = std::min(p.maxGrDb, overGated * slope);
        }
        else // Progressive Vari-Mu
        {
            const double rEff = 1.0 + (ratioSafe - 1.0) * (overGated / (overGated + 2.5));
            const double slope = 1.0 - (1.0 / std::max(rEff, 1.0));
            outTargetGrDb = std::min(p.maxGrDb, overGated * slope);
        }

        // Normalized Crest Envelope Follower [0..1] for Grit Drive (full 1.0 at +8.0 dB crest excess)
        outTargetCrestEnv01 = std::clamp(overGated * (1.0 / 8.0), 0.0, 1.0);

        // Align wideband audio RMS to output audio (delay by dLaMax) with gentle 8 ms smoothing
        const double delAudRms = rmsAlignDelay.pushAndRead(audRms, dLaMax);
        smoothAudRms += ceilSmoothAlpha * (delAudRms - smoothAudRms);
        outAlignedAudioRmsLin = smoothAudRms;
    }

    inline void processBallistics(double targetGrDb, double targetCrestEnv01, bool autoRelease,
                                  double& outAlignedGrDb, double& outAlignedCrestEnv01) noexcept
    {
        const double heldGr  = slideMaxGr.process(targetGrDb, curSlideWin);
        const double heldEnv = slideMaxEnv.process(targetCrestEnv01, curSlideWin);

        gf += ((heldGr  > gf) ? atkAlpha : relFastAlpha) * (heldGr  - gf);
        gs += ((heldGr  > gs) ? atkAlpha : relSlowAlpha) * (heldGr  - gs);

        ef += ((heldEnv > ef) ? atkAlpha : relFastAlpha) * (heldEnv - ef);
        es += ((heldEnv > es) ? atkAlpha : relSlowAlpha) * (heldEnv - es);

        // Refined Auto-Release: isolated spikes release at fast user Release; only dense sustained GR > 3 dB slows!
        const double actMetric = std::max(heldGr, heldEnv * 8.0);
        sust += ((actMetric > sust) ? intAtkAlpha : intRelAlpha) * (actMetric - sust);
        const double blend = autoRelease ? std::clamp((sust - 3.0) * (1.0 / 6.0), 0.0, 0.75) : 0.0;

        const double envGr  = gf + blend * (gs - gf);
        const double envDrv = ef + blend * (es - ef);

        outAlignedGrDb       = extraGrDelay.pushAndRead(envGr, curExtraDelay);
        outAlignedCrestEnv01 = extraEnvDelay.pushAndRead(envDrv, curExtraDelay);
    }

private:
    double fs = 48000.0;
    int dCtx = 2880;
    int lFut = 1200;
    int dLaMax = 720;
    int totalRiderLatency = 3600;
    int maxSlideHold = 960;

    int curLaSamp = 96;
    int curSlideWin = 96;
    int curExtraDelay = 624;

    double lastRmsMs = -1.0, lastAtkMs = -1.0, lastRelMs = -1.0, lastLaMs = -1.0;

    double pkAtkAlpha = 0.13, pkRelAlpha = 0.0094;
    double rmsAlpha = 0.0001;
    double atkAlpha = 0.017, relFastAlpha = 0.0009, relSlowAlpha = 0.0004;
    double intAtkAlpha = 0.0003, intRelAlpha = 0.0001;
    double ceilSmoothAlpha = 0.0026;

    double ePeak        = 1e-7;
    double slowScMsq    = 1e-14;
    double slowAudioMsq = 1e-14;
    double smoothAudRms = 1e-4;
    double gf = 0.0, gs = 0.0, sust = 0.0;
    double ef = 0.0, es = 0.0;

    DriftFreeBoxcar    futMsqBox;
    IntDelayLine       pkLinDelay;
    IntDelayLine       scXSqDelay;
    IntDelayLine       audioXSqDelay;
    IntDelayLine       rmsAlignDelay;
    CircularSlidingMax slideMaxGr;
    CircularSlidingMax slideMaxEnv;
    IntDelayLine       extraGrDelay;
    IntDelayLine       extraEnvDelay;
};

/**
 * Host-Rate Frontend for Sandpaper Fairy v3.2:
 *   - Pure Wideband Audio Path (Zero Multiband Phase/Tone Splitting!)
 *   - Sidechain-Only 12 dB/oct TPT SVF (OFF / HPF / LPF / BAND FOCUS) + SC MONITOR Feed!
 *   - Fast-Breathing Crest-Factor Detector + Continuous Stereo Link (0..100%)
 */
class SandpaperFrontend
{
public:
    void prepare(double sampleRate)
    {
        fs = std::max(sampleRate, 8000.0);
        scFilter.prepare(fs);

        for (int ch = 0; ch < 2; ++ch)
        {
            det[ch].prepare(fs);
            riderLat = det[ch].getRiderLatencySamples();
            audioDel[ch].prepare(riderLat + 16);
            scMonDel[ch].prepare(riderLat + 16);
        }

        frontendLatencySamples = riderLat; // 75.0 ms exact (no FIR crossover latency!)
        reset();
    }

    void reset()
    {
        scFilter.reset();
        for (int ch = 0; ch < 2; ++ch)
        {
            det[ch].reset();
            audioDel[ch].reset();
            scMonDel[ch].reset();
        }
    }

    int getFrontendLatencySamples() const noexcept { return frontendLatencySamples; }

    struct FrameOut
    {
        double audioL   = 0.0, audioR   = 0.0; // Exact aligned wideband main audio
        double scMonL   = 0.0, scMonR   = 0.0; // Exact aligned filtered sidechain (for SC MONITOR)
        double grDbL    = 0.0, grDbR    = 0.0; // Smoothed gain reduction (dB)
        double envL     = 0.0, envR     = 0.0; // Smoothed Crest-Factor Envelope Follower [0..1]
        double refCeilL = 1.0, refCeilR = 1.0; // Stable wideband RMS-derived saturator ceiling C_ref
        double crestDb  = 0.0;                 // Live real-time Crest Factor (dB)
    };

    inline FrameOut processStereoSample(double inL, double inR, const SandpaperParams& p) noexcept
    {
        det[0].updateTiming(p.rmsWinMs, p.attackMs, p.releaseMs, p.lookaheadMs);
        det[1].updateTiming(p.rmsWinMs, p.attackMs, p.releaseMs, p.lookaheadMs);

        // 1. Sidechain-Only Filter (12 dB/oct TPT SVF - main audio is 100% untouched!)
        double scL = inL, scR = inR;
        scFilter.processStereo(inL, inR, p.scFilterMode, p.scFreqHz, scL, scR);

        // 2. Crisp Crest Factor Analysis on sidechain, while tracking wideband audio RMS for C_ref
        double rawGrL = 0.0, rawGrR = 0.0, rawEnvL = 0.0, rawEnvR = 0.0;
        double audRmsL = 1e-4, audRmsR = 1e-4, cfL = 0.0, cfR = 0.0;
        det[0].analyzeCrest(scL, inL, p, rawGrL, rawEnvL, audRmsL, cfL);
        det[1].analyzeCrest(scR, inR, p, rawGrR, rawEnvR, audRmsR, cfR);

        // 3. Continuous Stereo Link (0% dual-mono .. 100% fully linked)
        const double link   = std::clamp(p.stereoLink01, 0.0, 1.0);
        const double maxGr  = std::max(rawGrL,  rawGrR);
        const double maxEnv = std::max(rawEnvL, rawEnvR);
        const double maxRms = std::max(audRmsL, audRmsR);

        const double linkedGrL  = rawGrL  + link * (maxGr  - rawGrL);
        const double linkedGrR  = rawGrR  + link * (maxGr  - rawGrR);
        const double linkedEnvL = rawEnvL + link * (maxEnv - rawEnvL);
        const double linkedEnvR = rawEnvR + link * (maxEnv - rawEnvR);
        const double linkedRmsL = audRmsL + link * (maxRms - audRmsL);
        const double linkedRmsR = audRmsR + link * (maxRms - audRmsR);

        // 4. Crisp Attack/Release Ballistics
        FrameOut out;
        det[0].processBallistics(linkedGrL, linkedEnvL, p.autoRelease, out.grDbL, out.envL);
        det[1].processBallistics(linkedGrR, linkedEnvR, p.autoRelease, out.grDbR, out.envR);

        const double threshMult = std::pow(10.0, std::max(p.crestThreshDb, 0.5) * 0.05);
        out.refCeilL = linkedRmsL * threshMult;
        out.refCeilR = linkedRmsR * threshMult;

        // 5. Align wideband main audio & sidechain monitor to exact riderLat
        out.audioL  = audioDel[0].pushAndRead(inL, riderLat);
        out.audioR  = audioDel[1].pushAndRead(inR, riderLat);
        out.scMonL  = scMonDel[0].pushAndRead(scL, riderLat);
        out.scMonR  = scMonDel[1].pushAndRead(scR, riderLat);
        out.crestDb = std::max(cfL, cfR);

        return out;
    }

private:
    double fs = 48000.0;
    int riderLat = 3600;
    int frontendLatencySamples = 3600;

    SidechainSvf         scFilter;
    CrestDetectorChannel det[2];
    IntDelayLine         audioDel[2];
    IntDelayLine         scMonDel[2];
};

} // namespace sylredhel::sandpaper
