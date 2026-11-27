#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

struct SandpaperPreset
{
    juce::String name;
    juce::String tip;
    int   engineMode;    // 0=RIDER (Clean VCA), 1=VARI-MU (Soft Tube)
    int   scFilterMode;  // 0=OFF, 1=HPF (Low-Safe), 2=LPF (Air-Safe), 3=BAND (Focus)
    int   oversampling;  // 0=OFF, 1=2x, 2=4x, 3=8x
    float scFreqHz;      // 40 .. 12000 Hz
    float crestThreshDb; // 0.0 .. 18.0 dB
    float rmsWinMs;      // 40 .. 500 ms
    float floorDb;       // -72 .. -18 dBFS
    float ratio;         // 1.0 .. 100.0
    float kneeDb;        // 0.0 .. 10.0 dB
    float attackMs;      // 0.1 .. 30.0 ms
    float releaseMs;     // 5.0 .. 400.0 ms
    bool  autoRelease;
    float lookaheadMs;   // 0.0 .. 15.0 ms
    float gritDriveDb;   // 0.0 .. 24.0 dB (Works in BOTH RIDER & VARI-MU!)
    float stereoLinkPct; // 0.0 .. 100.0 %
    float mixPct;
    float outGainDb;
};

class SandpaperPresetManager
{
public:
    static const std::vector<SandpaperPreset>& getFactoryPresets()
    {
        static const std::vector<SandpaperPreset> presets = {
            { "01. Init (Crisp Crest Rider)",
              "Fast, open VCA crest rider (2ms lookahead, crisp release): shaves spikes without dulling the envelope.",
              0, 0, 2,  400.0f,  7.0f, 180.0f, -48.0f,  6.0f, 2.5f, 1.2f,  22.0f, false,  2.0f,  0.0f, 100.0f, 100.0f, 0.0f },

            { "02. Drum Break - Open Punch + Grit",
              "Crisp VCA RIDER + +5 dB crest-driven Grit sheen keeps drum break air & punch alive while taming peaks.",
              0, 1, 2,  220.0f,  3.5f, 160.0f, -44.0f, 16.0f, 2.0f, 1.5f,  18.0f, false,  1.5f,  5.0f,  85.0f, 100.0f, 0.0f },

            { "03. Drum Break - Pure Crest Saturator",
              "RATIO 1.0:1 (0 dB VCA comp) + +14 dB GRIT DRIVE driven purely by Crest Factor Envelope Follower!",
              1, 1, 3,  180.0f,  3.5f, 170.0f, -44.0f,  1.0f, 2.5f, 0.5f,  20.0f, false,  2.0f, 14.0f,  80.0f, 100.0f, 0.0f },

            { "04. Drum Bus - Low-Safe Vari-Mu Glue",
              "SC HPF @ 300 Hz ignores kick sub while progressive Vari-Mu + +10 dB Grit rounds snare & overhead pokes.",
              1, 1, 3,  300.0f,  4.5f, 180.0f, -44.0f,  8.0f, 3.0f, 1.8f,  26.0f, false,  1.5f, 10.0f,  75.0f, 100.0f, 0.0f },

            { "05. Squashed Break - Fast Spike Trim",
              "Low 1.5 dB threshold + 40:1 ratio + 10ms release trims stubborn peaks inside dense breaks without choking.",
              0, 0, 2,  300.0f,  1.5f, 150.0f, -42.0f, 40.0f, 1.5f, 0.4f,  10.0f, false,  2.5f,  3.0f,  90.0f, 100.0f, 0.0f },

            { "06. Vocal - Consonant SC Band Focus",
              "SC BAND @ 3.2 kHz detects sharp sibilants & consonant spikes while riding full-band cleanly (no multiband scoop).",
              0, 3, 2, 3200.0f,  6.5f, 170.0f, -50.0f,  7.0f, 3.0f, 0.6f,  25.0f, true,   3.0f,  0.0f, 100.0f, 100.0f, 0.0f },

            { "07. Vocal - Warm Crest Tube Polish",
              "Progressive Vari-Mu + +8 dB Crest Drive adds silky tube sheen on vocal peaks while leaving tails spotless.",
              1, 1, 2,  200.0f,  7.0f, 210.0f, -48.0f,  4.0f, 4.0f, 1.5f,  45.0f, true,   2.5f,  8.0f, 100.0f, 100.0f, 0.0f },

            { "08. Acoustic - Crisp Pick Shaver",
              "SC HPF @ 1.2 kHz focuses detection on bright pick spikes; fast 12ms release keeps guitar body breathing.",
              0, 1, 2, 1200.0f,  6.0f, 160.0f, -52.0f, 10.0f, 2.5f, 0.4f,  12.0f, false,  2.0f,  2.0f,  65.0f, 100.0f, 0.0f },

            { "09. Mix / Drums - Air-Safe Body Rider",
              "SC LPF @ 1.8 kHz ignores hi-hats & cymbals so wideband riding tracks kick/snare groove peaks only.",
              0, 2, 2, 1800.0f,  6.0f, 180.0f, -46.0f,  6.0f, 3.0f, 2.0f,  28.0f, false,  1.5f,  3.5f,  85.0f, 100.0f, 0.0f },

            { "10. Master - Transparent Peak Shaver",
              "4x oversampled mastering crest rider with +2.5 dB subtle peak-tip Grit to preserve brightness & punch.",
              0, 1, 2,  120.0f,  8.0f, 240.0f, -42.0f,  8.0f, 3.0f, 0.8f,  20.0f, true,   2.5f,  2.5f,  85.0f, 100.0f, 0.0f },

            { "11. Percussion - Wide Stereo (30% Link)",
              "30% Stereo Link + SC HPF @ 1.0 kHz tames panned percussion spikes independently with zero stereo collapse.",
              0, 1, 2, 1000.0f,  5.0f, 150.0f, -50.0f, 10.0f, 2.5f, 0.4f,  14.0f, false,  2.0f,  4.0f,  30.0f, 100.0f, 0.0f },

            { "12. Synth Pluck - Pure Crest Tube Bite",
              "RATIO 1.0:1 + +18 dB Crest Drive + 14ms release turns needle-sharp plucks into fat analog hits.",
              1, 0, 3,  400.0f,  5.0f, 160.0f, -50.0f,  1.0f, 3.0f, 0.4f,  14.0f, false,  2.0f, 18.0f,  75.0f, 100.0f, 0.0f }
        };
        return presets;
    }

    static void applyFactoryPreset(juce::AudioProcessorValueTreeState& apvts, int index)
    {
        const auto& list = getFactoryPresets();
        if (index < 0 || index >= static_cast<int>(list.size()))
            return;

        const auto& p = list[static_cast<size_t>(index)];
        auto setParam = [&](const juce::String& id, float plainVal)
        {
            if (auto* param = apvts.getParameter(id))
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost(param->convertTo0to1(plainVal));
                param->endChangeGesture();
            }
        };

        setParam("engineMode",    static_cast<float>(p.engineMode));
        setParam("scFilterMode",  static_cast<float>(p.scFilterMode));
        setParam("oversampling",  static_cast<float>(p.oversampling));
        setParam("scFreqHz",      p.scFreqHz);
        setParam("crestThreshDb", p.crestThreshDb);
        setParam("rmsWinMs",      p.rmsWinMs);
        setParam("floorDb",       p.floorDb);
        setParam("ratio",         p.ratio);
        setParam("kneeDb",        p.kneeDb);
        setParam("attackMs",      p.attackMs);
        setParam("releaseMs",     p.releaseMs);
        setParam("autoRelease",   p.autoRelease ? 1.0f : 0.0f);
        setParam("lookaheadMs",   p.lookaheadMs);
        setParam("gritDriveDb",   p.gritDriveDb);
        setParam("stereoLink",    p.stereoLinkPct);
        setParam("mixPct",        p.mixPct);
        setParam("outGainDb",     p.outGainDb);
        setParam("audition",      0.0f);
        setParam("bypass",        0.0f);
    }

    static juce::File getUserPresetDirectory()
    {
        auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                       .getChildFile("Sylredhel")
                       .getChildFile("Sandpaper Fairy")
                       .getChildFile("Presets");
        if (!dir.exists())
            dir.createDirectory();
        return dir;
    }

    static bool saveUserPreset(juce::AudioProcessorValueTreeState& apvts, const juce::File& file)
    {
        auto state = apvts.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        if (xml == nullptr)
            return false;
        return xml->writeTo(file);
    }

    static bool loadUserPreset(juce::AudioProcessorValueTreeState& apvts, const juce::File& file)
    {
        auto xml = juce::XmlDocument::parse(file);
        if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
            return true;
        }
        return false;
    }
};
