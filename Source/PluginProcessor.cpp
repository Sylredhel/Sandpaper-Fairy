#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

SandpaperFairyAudioProcessor::SandpaperFairyAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SandpaperFairyV32Params", createParameterLayout())
{
    for (size_t i = 0; i < 3; ++i)
    {
        const size_t order = i + 1; // 1->2x, 2->4x, 3->8x
        osBank[i] = std::make_unique<juce::dsp::Oversampling<float>>(
            2, order, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true);
    }

    pEngineMode   = apvts.getRawParameterValue("engineMode");
    pScFilterMode = apvts.getRawParameterValue("scFilterMode");
    pOversampling = apvts.getRawParameterValue("oversampling");
    pScFreqHz     = apvts.getRawParameterValue("scFreqHz");
    pCrestThresh  = apvts.getRawParameterValue("crestThreshDb");
    pRmsWinMs     = apvts.getRawParameterValue("rmsWinMs");
    pFloorDb      = apvts.getRawParameterValue("floorDb");
    pRatio        = apvts.getRawParameterValue("ratio");
    pKneeDb       = apvts.getRawParameterValue("kneeDb");
    pAttackMs     = apvts.getRawParameterValue("attackMs");
    pReleaseMs    = apvts.getRawParameterValue("releaseMs");
    pAutoRelease  = apvts.getRawParameterValue("autoRelease");
    pLookaheadMs  = apvts.getRawParameterValue("lookaheadMs");
    pGritDriveDb  = apvts.getRawParameterValue("gritDriveDb");
    pStereoLink   = apvts.getRawParameterValue("stereoLink");
    pMixPct       = apvts.getRawParameterValue("mixPct");
    pOutGainDb    = apvts.getRawParameterValue("outGainDb");
    pAudition     = apvts.getRawParameterValue("audition");
    pBypass       = apvts.getRawParameterValue("bypass");
}

SandpaperFairyAudioProcessor::~SandpaperFairyAudioProcessor()
{
    cancelPendingUpdate();
}

juce::AudioProcessorValueTreeState::ParameterLayout SandpaperFairyAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 1. Engine, Sidechain Filter & Oversampling
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "engineMode", 1 }, "Sanding Engine",
        juce::StringArray {
            "RIDER (Clean VCA)",
            "VARI-MU (Soft Tube)"
        }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "scFilterMode", 1 }, "Sidechain Filter",
        juce::StringArray {
            "SC: OFF (Full)",
            "SC: HPF (Low-Safe)",
            "SC: LPF (Air-Safe)",
            "SC: BAND (Focus)"
        }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "oversampling", 1 }, "Oversampling",
        juce::StringArray { "OFF (1x)", "2x FIR", "4x FIR", "8x FIR" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "scFreqHz", 1 }, "Sidechain Frequency",
        juce::NormalisableRange<float>(40.0f, 12000.0f, 1.0f, 0.30f), 400.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "crestThreshDb", 1 }, "Crest Threshold",
        juce::NormalisableRange<float>(0.0f, 18.0f, 0.1f, 0.78f), 7.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "rmsWinMs", 1 }, "RMS Window",
        juce::NormalisableRange<float>(40.0f, 500.0f, 1.0f, 0.55f), 180.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "floorDb", 1 }, "Noise Floor Guard",
        juce::NormalisableRange<float>(-72.0f, -18.0f, 0.5f), -48.0f));

    // 2. Dynamics & Crest-Followed Saturator Drive (Active in BOTH RIDER & VARI-MU!)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ratio", 1 }, "Ratio",
        juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f, 0.34f), 6.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "kneeDb", 1 }, "Knee",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), 2.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attackMs", 1 }, "Attack",
        juce::NormalisableRange<float>(0.1f, 30.0f, 0.1f, 0.42f), 1.2f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "releaseMs", 1 }, "Release",
        juce::NormalisableRange<float>(5.0f, 400.0f, 0.5f, 0.42f), 22.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "autoRelease", 1 }, "Auto Release", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lookaheadMs", 1 }, "Lookahead",
        juce::NormalisableRange<float>(0.0f, 15.0f, 0.1f, 0.70f), 2.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "gritDriveDb", 1 }, "Grit Drive",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 0.0f));

    // 3. Link, Monitor & Output
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stereoLink", 1 }, "Stereo Link",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mixPct", 1 }, "Parallel Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outGainDb", 1 }, "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "audition", 1 }, "Audition",
        juce::StringArray { "NORMAL", "SHAVINGS (DELTA)", "SC MONITOR" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return { params.begin(), params.end() };
}

double SandpaperFairyAudioProcessor::computeOsLatency(int osIdx) const noexcept
{
    if (osIdx >= 1 && osIdx <= 3)
        return static_cast<double>(osBank[static_cast<size_t>(osIdx - 1)]->getLatencyInSamples());
    return 0.0;
}

void SandpaperFairyAudioProcessor::handleAsyncUpdate()
{
    setLatencySamples(pendingLatencyAtomic.load(std::memory_order_relaxed));
}

void SandpaperFairyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    hostFs = std::max(sampleRate, 8000.0);
    currentFsAtomic.store(hostFs, std::memory_order_relaxed);

    const int maxBlock = std::max(4096, samplesPerBlock);
    audioBuf.setSize(2, maxBlock, false, true, true);
    dryBuf.setSize(2, maxBlock, false, true, true);
    scMonBuf.setSize(2, maxBlock, false, true, true);
    grCtrlBuf.setSize(2, maxBlock, false, true, true);
    envCtrlBuf.setSize(2, maxBlock, false, true, true);
    ceilCtrlBuf.setSize(2, maxBlock, false, true, true);

    for (size_t i = 0; i < 3; ++i)
    {
        osBank[i]->initProcessing(static_cast<size_t>(maxBlock));
        osBank[i]->reset();
    }

    frontend.prepare(hostFs);

    activeOsIdx = juce::jlimit(0, 3, static_cast<int>(std::round(pOversampling->load())));
    const double osFactor = (activeOsIdx == 0) ? 1.0 : static_cast<double>(1 << activeOsIdx);
    outputStage.prepareForRate(hostFs * osFactor);
    outputStage.reset();

    currentOsLatency = computeOsLatency(activeOsIdx);
    const int reportedLat = frontend.getFrontendLatencySamples() + static_cast<int>(std::round(currentOsLatency));
    latencyAtomic.store(reportedLat, std::memory_order_relaxed);
    pendingLatencyAtomic.store(reportedLat, std::memory_order_relaxed);
    setLatencySamples(reportedLat);

    const int maxOsLat = static_cast<int>(std::ceil(computeOsLatency(3))) + 32;
    dryAlignDelay.prepare(maxOsLat);
    scMonAlignDelay.prepare(maxOsLat);
    grHalfDelay.prepare(maxOsLat);
    envHalfDelay.prepare(maxOsLat);
    ceilHalfDelay.prepare(maxOsLat);

    prevGrL   = 0.0; prevGrR   = 0.0;
    prevEnvL  = 0.0; prevEnvR  = 0.0;
    prevCeilL = 1.0; prevCeilR = 1.0;

    scopeDecimSamples = std::max(32, static_cast<int>(std::round(0.005 * hostFs)));
    scopeCounter = 0;
    scopePeakCf  = 0.0;
    scopePeakGr  = 0.0;
    crestScope.reset();

    const double rampSec = 0.015;
    smScFreq.reset(hostFs, rampSec);
    smCrestThresh.reset(hostFs, rampSec);
    smRmsWinMs.reset(hostFs, rampSec);
    smFloorDb.reset(hostFs, rampSec);
    smRatio.reset(hostFs, rampSec);
    smKneeDb.reset(hostFs, rampSec);
    smAttackMs.reset(hostFs, rampSec);
    smReleaseMs.reset(hostFs, rampSec);
    smLookaheadMs.reset(hostFs, rampSec);
    smGritDriveDb.reset(hostFs, rampSec);
    smStereoLink01.reset(hostFs, rampSec);
    smMix.reset(hostFs, rampSec);
    smOutGainLin.reset(hostFs, rampSec);

    smScFreq.setCurrentAndTargetValue(pScFreqHz->load());
    smCrestThresh.setCurrentAndTargetValue(pCrestThresh->load());
    smRmsWinMs.setCurrentAndTargetValue(pRmsWinMs->load());
    smFloorDb.setCurrentAndTargetValue(pFloorDb->load());
    smRatio.setCurrentAndTargetValue(pRatio->load());
    smKneeDb.setCurrentAndTargetValue(pKneeDb->load());
    smAttackMs.setCurrentAndTargetValue(pAttackMs->load());
    smReleaseMs.setCurrentAndTargetValue(pReleaseMs->load());
    smLookaheadMs.setCurrentAndTargetValue(pLookaheadMs->load());
    smGritDriveDb.setCurrentAndTargetValue(pGritDriveDb->load());
    smStereoLink01.setCurrentAndTargetValue(pStereoLink->load() * 0.01);
    smMix.setCurrentAndTargetValue(pMixPct->load() * 0.01);
    smOutGainLin.setCurrentAndTargetValue(std::pow(10.0, pOutGainDb->load() * 0.05));
}

void SandpaperFairyAudioProcessor::releaseResources()
{
    cancelPendingUpdate();
    for (size_t i = 0; i < 3; ++i)
        osBank[i]->reset();
    frontend.reset();
    outputStage.reset();
    dryAlignDelay.reset();
    scMonAlignDelay.reset();
    grHalfDelay.reset();
    envHalfDelay.reset();
    ceilHalfDelay.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SandpaperFairyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto outSet = layouts.getMainOutputChannelSet();
    const auto inSet  = layouts.getMainInputChannelSet();
    if (outSet != juce::AudioChannelSet::mono() && outSet != juce::AudioChannelSet::stereo())
        return false;
    return outSet == inSet;
}
#endif

void SandpaperFairyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numInCh    = getTotalNumInputChannels();
    const int numOutCh   = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = numInCh; i < numOutCh; ++i)
        buffer.clear(i, 0, numSamples);

    if (numSamples <= 0 || numInCh <= 0)
        return;

    if (audioBuf.getNumSamples() < numSamples)
    {
        audioBuf.setSize(2, numSamples, false, true, true);
        dryBuf.setSize(2, numSamples, false, true, true);
        scMonBuf.setSize(2, numSamples, false, true, true);
        grCtrlBuf.setSize(2, numSamples, false, true, true);
        envCtrlBuf.setSize(2, numSamples, false, true, true);
        ceilCtrlBuf.setSize(2, numSamples, false, true, true);
    }

    // Live RT-safe Oversampling switch (OFF / 2x / 4x / 8x)
    const int reqOsIdx = juce::jlimit(0, 3, static_cast<int>(std::round(pOversampling->load())));
    if (reqOsIdx != activeOsIdx)
    {
        activeOsIdx = reqOsIdx;
        if (activeOsIdx >= 1)
            osBank[static_cast<size_t>(activeOsIdx - 1)]->reset();

        const double osFactor = (activeOsIdx == 0) ? 1.0 : static_cast<double>(1 << activeOsIdx);
        outputStage.prepareForRate(hostFs * osFactor);

        currentOsLatency = computeOsLatency(activeOsIdx);
        const int newLat = frontend.getFrontendLatencySamples() + static_cast<int>(std::round(currentOsLatency));
        latencyAtomic.store(newLat, std::memory_order_relaxed);
        pendingLatencyAtomic.store(newLat, std::memory_order_relaxed);
        triggerAsyncUpdate();
    }

    const auto engineMode   = static_cast<sylredhel::sandpaper::EngineMode>(static_cast<int>(std::round(pEngineMode->load())));
    const auto scFilterMode = static_cast<sylredhel::sandpaper::ScFilterMode>(static_cast<int>(std::round(pScFilterMode->load())));
    const auto audition     = static_cast<sylredhel::sandpaper::AuditionMode>(static_cast<int>(std::round(pAudition->load())));
    const bool autoRel      = pAutoRelease->load() > 0.5f;
    const bool bypassed     = pBypass->load() > 0.5f;

    smScFreq.setTargetValue(pScFreqHz->load());
    smCrestThresh.setTargetValue(pCrestThresh->load());
    smRmsWinMs.setTargetValue(pRmsWinMs->load());
    smFloorDb.setTargetValue(pFloorDb->load());
    smRatio.setTargetValue(pRatio->load());
    smKneeDb.setTargetValue(pKneeDb->load());
    smAttackMs.setTargetValue(pAttackMs->load());
    smReleaseMs.setTargetValue(pReleaseMs->load());
    smLookaheadMs.setTargetValue(pLookaheadMs->load());
    smGritDriveDb.setTargetValue(pGritDriveDb->load());
    smStereoLink01.setTargetValue(pStereoLink->load() * 0.01);
    smMix.setTargetValue(pMixPct->load() * 0.01);
    smOutGainLin.setTargetValue(std::pow(10.0, pOutGainDb->load() * 0.05));

    const bool isMono = (numInCh == 1);
    float* ioL = buffer.getWritePointer(0);
    float* ioR = isMono ? nullptr : buffer.getWritePointer(1);

    float* audL  = audioBuf.getWritePointer(0);
    float* audR  = audioBuf.getWritePointer(1);
    float* dryL  = dryBuf.getWritePointer(0);
    float* dryR  = dryBuf.getWritePointer(1);
    float* scML  = scMonBuf.getWritePointer(0);
    float* scMR  = scMonBuf.getWritePointer(1);
    float* gCtlL = grCtrlBuf.getWritePointer(0);
    float* gCtlR = grCtrlBuf.getWritePointer(1);
    float* eCtlL = envCtrlBuf.getWritePointer(0);
    float* eCtlR = envCtrlBuf.getWritePointer(1);
    float* cCtlL = ceilCtrlBuf.getWritePointer(0);
    float* cCtlR = ceilCtrlBuf.getWritePointer(1);

    sylredhel::sandpaper::SandpaperParams sp;
    sp.engineMode   = engineMode;
    sp.scFilterMode = scFilterMode;
    sp.autoRelease  = autoRel;
    sp.maxGrDb      = 24.0;

    const double halfOsLat = 0.5 * currentOsLatency;
    double blockMaxCf  = 0.0;
    double blockMaxEnv = 0.0;

    // -------------------------------------------------------------------------
    // STAGE 1 (Host Rate): Pure Wideband Audio Alignment + Sidechain-Filtered
    //                      Crisp Crest Detector + Continuous Stereo Link
    // -------------------------------------------------------------------------
    for (int i = 0; i < numSamples; ++i)
    {
        sp.scFreqHz      = smScFreq.getNextValue();
        sp.crestThreshDb = smCrestThresh.getNextValue();
        sp.rmsWinMs      = smRmsWinMs.getNextValue();
        sp.floorDb       = smFloorDb.getNextValue();
        sp.ratio         = smRatio.getNextValue();
        sp.kneeDb        = smKneeDb.getNextValue();
        sp.attackMs      = smAttackMs.getNextValue();
        sp.releaseMs     = smReleaseMs.getNextValue();
        sp.lookaheadMs   = smLookaheadMs.getNextValue();
        sp.gritDriveDb   = smGritDriveDb.getNextValue();
        sp.stereoLink01  = smStereoLink01.getNextValue();

        const double inL = static_cast<double>(ioL[i]);
        const double inR = isMono ? inL : static_cast<double>(ioR[i]);

        const auto fr = frontend.processStereoSample(inL, inR, sp);
        blockMaxCf  = std::max(blockMaxCf,  fr.crestDb);
        blockMaxEnv = std::max(blockMaxEnv, std::max(fr.envL, fr.envR));

        audL[i] = static_cast<float>(fr.audioL);
        audR[i] = static_cast<float>(fr.audioR);

        // Align dry & sidechain monitor by full currentOsLatency
        double aDryL = 0.0, aDryR = 0.0, aScL = 0.0, aScR = 0.0;
        dryAlignDelay.pushAndRead(fr.audioL, fr.audioR, currentOsLatency, aDryL, aDryR);
        scMonAlignDelay.pushAndRead(fr.scMonL, fr.scMonR, currentOsLatency, aScL, aScR);
        dryL[i] = static_cast<float>(aDryL);
        dryR[i] = static_cast<float>(aDryR);
        scML[i] = static_cast<float>(aScL);
        scMR[i] = static_cast<float>(aScR);

        // Align GR, Crest-Envelope & Saturator Ceiling to midpoint INSIDE oversampler
        double aGrL = 0.0, aGrR = 0.0, aEnvL = 0.0, aEnvR = 0.0, aCeilL = 1.0, aCeilR = 1.0;
        grHalfDelay.pushAndRead(fr.grDbL, fr.grDbR, halfOsLat, aGrL, aGrR);
        envHalfDelay.pushAndRead(fr.envL, fr.envR, halfOsLat, aEnvL, aEnvR);
        ceilHalfDelay.pushAndRead(fr.refCeilL, fr.refCeilR, halfOsLat, aCeilL, aCeilR);

        gCtlL[i] = static_cast<float>(aGrL);
        gCtlR[i] = static_cast<float>(aGrR);
        eCtlL[i] = static_cast<float>(aEnvL);
        eCtlR[i] = static_cast<float>(aEnvR);
        cCtlL[i] = static_cast<float>(aCeilL);
        cCtlR[i] = static_cast<float>(aCeilR);

        scopePeakCf = std::max(scopePeakCf, fr.crestDb);
    }

    // -------------------------------------------------------------------------
    // STAGE 2 (Oversampled or 1x): Wideband VCA / Vari-Mu + Unlocked Crest Grit
    // -------------------------------------------------------------------------
    const double curGritDb   = smGritDriveDb.getTargetValue();
    const double curFloorLin = std::pow(10.0, smFloorDb.getTargetValue() * 0.05);

    auto runOutputLoop = [&](float* bL, float* bR, size_t count, size_t factor)
    {
        const double invFactor = 1.0 / static_cast<double>(factor);
        for (size_t hostIdx = 0; hostIdx < static_cast<size_t>(numSamples); ++hostIdx)
        {
            const double gEndL = static_cast<double>(gCtlL[hostIdx]);
            const double gEndR = static_cast<double>(gCtlR[hostIdx]);
            const double eEndL = static_cast<double>(eCtlL[hostIdx]);
            const double eEndR = static_cast<double>(eCtlR[hostIdx]);
            const double cEndL = static_cast<double>(cCtlL[hostIdx]);
            const double cEndR = static_cast<double>(cCtlR[hostIdx]);

            for (size_t sub = 0; sub < factor; ++sub)
            {
                const size_t k = hostIdx * factor + sub;
                if (k >= count) break;

                const double t  = static_cast<double>(sub + 1) * invFactor;
                const double gL = prevGrL   + t * (gEndL - prevGrL);
                const double gR = prevGrR   + t * (gEndR - prevGrR);
                const double eL = prevEnvL  + t * (eEndL - prevEnvL);
                const double eR = prevEnvR  + t * (eEndR - prevEnvR);
                const double cL = prevCeilL + t * (cEndL - prevCeilL);
                const double cR = prevCeilR + t * (cEndR - prevCeilR);

                bL[k] = static_cast<float>(outputStage.processSample(
                    static_cast<double>(bL[k]), gL, eL, cL, curGritDb, curFloorLin, 0));
                bR[k] = static_cast<float>(outputStage.processSample(
                    static_cast<double>(bR[k]), gR, eR, cR, curGritDb, curFloorLin, 1));
            }

            prevGrL   = gEndL; prevGrR   = gEndR;
            prevEnvL  = eEndL; prevEnvR  = eEndR;
            prevCeilL = cEndL; prevCeilR = cEndR;
        }
    };

    if (activeOsIdx == 0)
    {
        runOutputLoop(audL, audR, static_cast<size_t>(numSamples), 1);
    }
    else
    {
        auto& os = *osBank[static_cast<size_t>(activeOsIdx - 1)];
        auto block2  = juce::dsp::AudioBlock<float>(audioBuf).getSubBlock(0, static_cast<size_t>(numSamples));
        auto osBlock = os.processSamplesUp(block2);
        runOutputLoop(osBlock.getChannelPointer(0), osBlock.getChannelPointer(1),
                      osBlock.getNumSamples(), os.getOversamplingFactor());
        os.processSamplesDown(block2);
    }

    // -------------------------------------------------------------------------
    // STAGE 3: Audition (Normal / Shavings / SC Monitor), Parallel Mix & Scope
    // -------------------------------------------------------------------------
    double blockMaxGr = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        const double mix  = smMix.getNextValue();
        const double outG = smOutGainLin.getNextValue();

        const double dL   = static_cast<double>(dryL[i]);
        const double dR   = static_cast<double>(dryR[i]);
        const double wetL = static_cast<double>(audL[i]);
        const double wetR = static_cast<double>(audR[i]);

        double outL = dL, outR = dR;
        double instGr = 0.0;

        if (!bypassed)
        {
            const double blendL = (1.0 - mix) * dL + mix * wetL;
            const double blendR = (1.0 - mix) * dR + mix * wetR;

            if (audition == sylredhel::sandpaper::AuditionMode::Shavings)
            {
                outL = (dL - blendL) * outG;
                outR = (dR - blendR) * outG;
            }
            else if (audition == sylredhel::sandpaper::AuditionMode::ScMonitor)
            {
                outL = static_cast<double>(scML[i]);
                outR = static_cast<double>(scMR[i]);
            }
            else
            {
                outL = blendL * outG;
                outR = blendR * outG;
            }

            const double ctrlGr = 0.5 * (static_cast<double>(gCtlL[i]) + static_cast<double>(gCtlR[i])) * mix;
            instGr = ctrlGr;
            if (curGritDb > 0.05)
            {
                const double inM  = std::max(std::abs(dL), std::abs(dR));
                const double outM = std::max(std::abs(blendL), std::abs(blendR));
                if (inM > curFloorLin && outM < inM)
                    instGr = std::max(instGr, std::clamp(20.0 * std::log10(inM / std::max(outM, 1e-6)), 0.0, 20.0));
            }
            blockMaxGr = std::max(blockMaxGr, instGr);
        }

        ioL[i] = static_cast<float>(outL);
        if (!isMono)
            ioR[i] = static_cast<float>(outR);

        scopePeakGr = std::max(scopePeakGr, instGr);
        if (++scopeCounter >= scopeDecimSamples)
        {
            crestScope.pushFrame(static_cast<float>(std::clamp(scopePeakCf, 0.0, 22.0)),
                                 static_cast<float>(std::clamp(scopePeakGr, 0.0, 20.0)));
            scopeCounter = 0;
            scopePeakCf  = 0.0;
            scopePeakGr  = 0.0;
        }
    }

    const float prevGr = grDbAtomic.load(std::memory_order_relaxed);
    const float nextGr = static_cast<float>(blockMaxGr);
    grDbAtomic.store((nextGr > prevGr) ? nextGr : (prevGr * 0.85f + nextGr * 0.15f),
                     std::memory_order_relaxed);

    const float prevCf = crestDbAtomic.load(std::memory_order_relaxed);
    const float nextCf = static_cast<float>(blockMaxCf);
    crestDbAtomic.store((nextCf > prevCf) ? nextCf : (prevCf * 0.85f + nextCf * 0.15f),
                        std::memory_order_relaxed);

    const float liveGrit = bypassed ? 0.0f : static_cast<float>(blockMaxEnv * curGritDb);
    const float prevGrit = liveGritDbAtomic.load(std::memory_order_relaxed);
    liveGritDbAtomic.store((liveGrit > prevGrit) ? liveGrit : (prevGrit * 0.80f + liveGrit * 0.20f),
                           std::memory_order_relaxed);
}

juce::AudioProcessorEditor* SandpaperFairyAudioProcessor::createEditor()
{
    return new SandpaperFairyAudioProcessorEditor(*this);
}

void SandpaperFairyAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SandpaperFairyAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SandpaperFairyAudioProcessor();
}
