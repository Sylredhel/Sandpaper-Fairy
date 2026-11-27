#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "PresetManager.h"
#include "LookAndFeelSketchbook.h"

class SandpaperFairyAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit SandpaperFairyAudioProcessorEditor(SandpaperFairyAudioProcessor&);
    ~SandpaperFairyAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    void setupKnob(juce::Slider& s, const juce::String& suffix, uint32_t accentHex,
                   bool bipolar, const juce::String& tip);
    void registerTip(juce::Component& c, const juce::String& tip);
    void loadFactoryPresetByIdx(int idx);
    void openSavePresetDialog();
    void openLoadPresetDialog();

    SandpaperFairyAudioProcessor& processorRef;
    LookAndFeelSketchbook laf;
    SandpaperScopeView    scopeView;
    SpikeCurveView        curveView;
    SandpaperFairyMascot  mascotView;

    // Header: Preset Browser + Oversampling Selector
    juce::TextButton prevPresetBtn { "<" };
    juce::ComboBox   presetBox;
    juce::TextButton nextPresetBtn { ">" };
    juce::TextButton savePresetBtn { "SAVE" };
    juce::TextButton loadPresetBtn { "LOAD" };
    juce::ComboBox   osBox;

    // Card 1: Engine & Sidechain-Filtered Crest Detector
    juce::ComboBox engineModeBox, scFilterBox;
    juce::Slider   scFreqKnob, crestThreshKnob, rmsWinKnob, floorKnob;

    // Card 2: Compressor Dynamics & Unlocked Crest-Followed Grit Drive
    juce::Slider       ratioKnob, attackKnob, releaseKnob;
    juce::Slider       lookaheadKnob, kneeKnob, gritDriveKnob;
    juce::ToggleButton autoRelBtn { "AUTO REL" };

    // Card 3: Link, Monitor & Output
    juce::ComboBox     auditionBox;
    juce::Slider       stereoLinkKnob, mixKnob, outGainKnob;
    juce::ToggleButton bypassBtn { "BYPASS" };

    juce::String hoverTipText { "Hover over any control for a quick guide, or pick a factory preset above." };
    int currentFactoryPresetIdx = 0;
    std::unique_ptr<juce::FileChooser> fileChooser;

    using SAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<CAtt> attEngineMode, attScFilter, attOversampling, attAudition;
    std::unique_ptr<SAtt> attScFreq, attCrestThresh, attRmsWin, attFloor;
    std::unique_ptr<SAtt> attRatio, attKnee, attAttack, attRelease, attLookahead, attGritDrive;
    std::unique_ptr<SAtt> attStereoLink, attMix, attOutGain;
    std::unique_ptr<BAtt> attAutoRel, attBypass;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SandpaperFairyAudioProcessorEditor)
};
