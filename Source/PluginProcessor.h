#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/SandpaperEngine.h"
#include <array>
#include <atomic>
#include <memory>

template <int Capacity>
class LockFreeCrestScope
{
public:
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

    void reset() noexcept
    {
        cfDbBuf.fill(0.0f);
        grDbBuf.fill(0.0f);
        writePos.store(0, std::memory_order_relaxed);
    }

    inline void pushFrame(float cfDb, float grDb) noexcept
    {
        const int idx = writePos.load(std::memory_order_relaxed);
        cfDbBuf[static_cast<size_t>(idx)] = cfDb;
        grDbBuf[static_cast<size_t>(idx)] = grDb;
        writePos.store((idx + 1) & (Capacity - 1), std::memory_order_release);
    }

    void copyLatest(float* destCfDb, float* destGrDb, int count) const noexcept
    {
        const int end = writePos.load(std::memory_order_acquire);
        int pos = (end - count) & (Capacity - 1);
        for (int i = 0; i < count; ++i)
        {
            destCfDb[i] = cfDbBuf[static_cast<size_t>(pos)];
            destGrDb[i] = grDbBuf[static_cast<size_t>(pos)];
            pos = (pos + 1) & (Capacity - 1);
        }
    }

private:
    std::array<float, Capacity> cfDbBuf {};
    std::array<float, Capacity> grDbBuf {};
    std::atomic<int> writePos { 0 };
};

class SandpaperFairyAudioProcessor : public juce::AudioProcessor,
                                     private juce::AsyncUpdater
{
public:
    static constexpr int kScopeCapacity = 512;

    SandpaperFairyAudioProcessor();
    ~SandpaperFairyAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.14; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    double getCurrentSampleRate() const noexcept    { return currentFsAtomic.load(std::memory_order_relaxed); }
    int    getEngineLatencySamples() const noexcept { return latencyAtomic.load(std::memory_order_relaxed); }
    float  getCurrentGrDb() const noexcept          { return grDbAtomic.load(std::memory_order_relaxed); }
    float  getCurrentCrestDb() const noexcept       { return crestDbAtomic.load(std::memory_order_relaxed); }
    float  getLiveGritDriveDb() const noexcept      { return liveGritDbAtomic.load(std::memory_order_relaxed); }
    const LockFreeCrestScope<kScopeCapacity>& getCrestScope() const noexcept { return crestScope; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void handleAsyncUpdate() override;
    double computeOsLatency(int osIdx) const noexcept;

    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> osBank;

    sylredhel::sandpaper::SandpaperFrontend    frontend;
    sylredhel::sandpaper::SandpaperOutputStage outputStage;
    sylredhel::sandpaper::FractionalDelayLine  dryAlignDelay;
    sylredhel::sandpaper::FractionalDelayLine  scMonAlignDelay;
    sylredhel::sandpaper::FractionalDelayLine  grHalfDelay;
    sylredhel::sandpaper::FractionalDelayLine  envHalfDelay;
    sylredhel::sandpaper::FractionalDelayLine  ceilHalfDelay;

    juce::AudioBuffer<float> audioBuf;
    juce::AudioBuffer<float> dryBuf;
    juce::AudioBuffer<float> scMonBuf;
    juce::AudioBuffer<float> grCtrlBuf;
    juce::AudioBuffer<float> envCtrlBuf;
    juce::AudioBuffer<float> ceilCtrlBuf;

    // Smoothed continuous parameters
    juce::SmoothedValue<double> smScFreq, smCrestThresh, smRmsWinMs, smFloorDb;
    juce::SmoothedValue<double> smRatio, smKneeDb, smAttackMs, smReleaseMs;
    juce::SmoothedValue<double> smLookaheadMs, smGritDriveDb, smStereoLink01;
    juce::SmoothedValue<double> smMix, smOutGainLin;

    // Cached atomic parameter pointers
    std::atomic<float>* pEngineMode   = nullptr;
    std::atomic<float>* pScFilterMode = nullptr;
    std::atomic<float>* pOversampling = nullptr;
    std::atomic<float>* pScFreqHz     = nullptr;
    std::atomic<float>* pCrestThresh  = nullptr;
    std::atomic<float>* pRmsWinMs     = nullptr;
    std::atomic<float>* pFloorDb      = nullptr;
    std::atomic<float>* pRatio        = nullptr;
    std::atomic<float>* pKneeDb       = nullptr;
    std::atomic<float>* pAttackMs     = nullptr;
    std::atomic<float>* pReleaseMs    = nullptr;
    std::atomic<float>* pAutoRelease  = nullptr;
    std::atomic<float>* pLookaheadMs  = nullptr;
    std::atomic<float>* pGritDriveDb  = nullptr;
    std::atomic<float>* pStereoLink   = nullptr;
    std::atomic<float>* pMixPct       = nullptr;
    std::atomic<float>* pOutGainDb    = nullptr;
    std::atomic<float>* pAudition     = nullptr;
    std::atomic<float>* pBypass       = nullptr;

    double hostFs           = 48000.0;
    double currentOsLatency = 0.0;
    int    activeOsIdx      = 2; // Default 4x FIR

    double prevGrL   = 0.0, prevGrR   = 0.0;
    double prevEnvL  = 0.0, prevEnvR  = 0.0;
    double prevCeilL = 1.0, prevCeilR = 1.0;

    int    scopeDecimSamples = 240;
    int    scopeCounter      = 0;
    double scopePeakCf       = 0.0;
    double scopePeakGr       = 0.0;

    std::atomic<double> currentFsAtomic      { 48000.0 };
    std::atomic<int>    latencyAtomic        { 3616 };
    std::atomic<int>    pendingLatencyAtomic { 3616 };
    std::atomic<float>  grDbAtomic           { 0.0f };
    std::atomic<float>  crestDbAtomic        { 0.0f };
    std::atomic<float>  liveGritDbAtomic     { 0.0f };
    LockFreeCrestScope<kScopeCapacity> crestScope;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SandpaperFairyAudioProcessor)
};
