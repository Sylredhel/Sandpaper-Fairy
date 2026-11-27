#include "PluginEditor.h"

SandpaperFairyAudioProcessorEditor::SandpaperFairyAudioProcessorEditor(SandpaperFairyAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), scopeView(p)
{
    setLookAndFeel(&laf);
    setSize(1040, 646);

    addAndMakeVisible(scopeView);
    addAndMakeVisible(curveView);
    addAndMakeVisible(mascotView);

    // Header: Factory Preset Browser + Save/Load + Oversampling
    addAndMakeVisible(prevPresetBtn);
    addAndMakeVisible(presetBox);
    addAndMakeVisible(nextPresetBtn);
    addAndMakeVisible(savePresetBtn);
    addAndMakeVisible(loadPresetBtn);

    const auto& factories = SandpaperPresetManager::getFactoryPresets();
    for (int i = 0; i < static_cast<int>(factories.size()); ++i)
        presetBox.addItem(factories[static_cast<size_t>(i)].name, i + 1);
    presetBox.setSelectedId(1, juce::dontSendNotification);

    presetBox.onChange = [this]()
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0)
            loadFactoryPresetByIdx(idx);
    };
    prevPresetBtn.onClick = [this]()
    {
        const int n = static_cast<int>(SandpaperPresetManager::getFactoryPresets().size());
        loadFactoryPresetByIdx((currentFactoryPresetIdx - 1 + n) % n);
    };
    nextPresetBtn.onClick = [this]()
    {
        const int n = static_cast<int>(SandpaperPresetManager::getFactoryPresets().size());
        loadFactoryPresetByIdx((currentFactoryPresetIdx + 1) % n);
    };
    savePresetBtn.onClick = [this]() { openSavePresetDialog(); };
    loadPresetBtn.onClick = [this]() { openLoadPresetDialog(); };

    osBox.addItemList({ "OS: OFF (1x)", "OS: 2x FIR", "OS: 4x FIR", "OS: 8x FIR" }, 1);
    addAndMakeVisible(osBox);

    // Card 1: Engine & Sidechain-Filtered Crest Detector
    engineModeBox.addItemList({
        "RIDER (Clean VCA)",
        "VARI-MU (Soft Tube)"
    }, 1);
    scFilterBox.addItemList({
        "SC: OFF (Full)",
        "SC: HPF (Low-Safe)",
        "SC: LPF (Air-Safe)",
        "SC: BAND (Focus)"
    }, 1);
    addAndMakeVisible(engineModeBox);
    addAndMakeVisible(scFilterBox);

    setupKnob(scFreqKnob,      " Hz", 0xff2f8578, false,
              "SC FREQ: Sidechain filter frequency (40 Hz - 12 kHz). Filters detector ONLY; main audio stays pure wideband!");
    setupKnob(crestThreshKnob, " dB", 0xffc0842c, false,
              "CREST THRESH: Peak-to-RMS threshold (0-18 dB). Lower to 1.5-4 dB for dense/compressed drum breaks!");
    setupKnob(rmsWinKnob,      " ms", 0xffb2622e, false,
              "RMS WIN: Power-RMS averaging window (40-500 ms). Paired with gapped future guard to protect phrase starts.");
    setupKnob(floorKnob,       " dB", 0xff466987, false,
              "FLOOR GUARD: Ignores spikes below this level so breaths, room noise & quiet tails stay untouched.");

    // Card 2: Dynamics & Unlocked Crest-Followed Grit Drive
    setupKnob(ratioKnob, "", 0xffb2622e, false,
              "RATIO: 1.0:1 (pure Crest Saturator!) up to 100:1 (LIMIT). Linear slope in RIDER, progressive in VARI-MU.");
    ratioKnob.textFromValueFunction = [](double v)
    {
        if (v >= 90.0) return juce::String("LIMIT");
        if (v >= 20.0) return juce::String(static_cast<int>(std::round(v))) + ":1";
        return juce::String(v, 1) + ":1";
    };

    setupKnob(attackKnob,    " ms", 0xffc6523c, false,
              "ATTACK: Controls transient bite (0.1-30 ms). Open to 1.5-4 ms to let drum punch breathe cleanly!");
    setupKnob(releaseKnob,   " ms", 0xffc6523c, false,
              "RELEASE: Crisp recovery down to 5 ms! Fast 2.5ms peak detector eliminates sluggish blanket muffling.");
    setupKnob(lookaheadKnob, " ms", 0xff466987, false,
              "LOOKAHEAD: Pre-peak onset window (0-15 ms). Keep at 0-2 ms for punchy drums, higher for brickwall shaving.");
    setupKnob(kneeKnob,      " dB", 0xffb2622e, false,
              "KNEE: Soft-knee transition width around Crest Threshold (0-10 dB).");
    setupKnob(gritDriveKnob, " dB", 0xffc46b28, false,
              "GRIT DRIVE: Works in BOTH RIDER & VARI-MU! Crest envelope modulates tube drive (0-24 dB) to keep peaks open & vivid.");
    addAndMakeVisible(autoRelBtn);

    // Card 3: Link, Monitor & Output
    auditionBox.addItemList({ "NORMAL", "SHAVINGS (DELTA)", "SC MONITOR" }, 1);
    addAndMakeVisible(auditionBox);

    setupKnob(stereoLinkKnob, " %",  0xffb2622e, false,
              "STEREO LINK: Continuous 0% (dual-mono) to 100% (locked stereo) linking for GR, Crest Drive & Ceiling.");
    setupKnob(mixKnob,        " %",  0xff466987, false,
              "PARALLEL MIX: Exact phase-aligned blend between dry input and sanded output.");
    setupKnob(outGainKnob,    " dB", 0xff466987, true,
              "OUTPUT GAIN: Final transparent makeup trim.");
    addAndMakeVisible(bypassBtn);

    registerTip(presetBox,     "FACTORY PRESETS: 12 crisp wideband presets with Sidechain Filtering & unlocked Grit Drive.");
    registerTip(prevPresetBtn, "Previous factory preset.");
    registerTip(nextPresetBtn, "Next factory preset.");
    registerTip(savePresetBtn, "Save current settings as a .xml user preset.");
    registerTip(loadPresetBtn, "Load a saved .xml user preset from disk.");
    registerTip(osBox,         "OVERSAMPLING: Linear-phase equiripple anti-aliasing FIR (OFF/2x/4x/8x) for fast release & Grit.");

    registerTip(engineModeBox, "RIDER: Crisp linear-ratio VCA | VARI-MU: Progressive soft-ratio. Both support GRIT DRIVE!");
    registerTip(scFilterBox,   "PURE SIDECHAIN FILTER: Filters detector ONLY (HPF/LPF/BAND) -> cohesive wideband riding, zero multiband scoop!");
    registerTip(autoRelBtn,    "AUTO REL: Gently slows release only on heavy sustained GR (>3 dB); OFF gives fastest drum breathing.");
    registerTip(auditionBox,   "NORMAL: Full output | SHAVINGS: Solo shaved dust (Dry - Wet) | SC MONITOR: Hear filtered sidechain!");
    registerTip(bypassBtn,     "BYPASS: Exact latency-compensated A/B bypass with zero timing shift.");
    registerTip(scopeView,     "CREST SCOPE: Watch live Crest Factor (CF), Threshold (TH), GR cut & live Crest Grit Drive (DRV)!");
    registerTip(curveView,     "CREST CURVE: Maps Crest Factor (0..20 dB) to VCA / Vari-Mu GR and unlocked Crest Grit Drive.");
    registerTip(mascotView,    "Sandpaper Fairy (Sylredhel): Leans in calmly to file down sharp stalagmite spikes.");

    auto& a = processorRef.apvts;
    attEngineMode   = std::make_unique<CAtt>(a, "engineMode",    engineModeBox);
    attScFilter     = std::make_unique<CAtt>(a, "scFilterMode",  scFilterBox);
    attOversampling = std::make_unique<CAtt>(a, "oversampling",  osBox);
    attScFreq       = std::make_unique<SAtt>(a, "scFreqHz",      scFreqKnob);
    attCrestThresh  = std::make_unique<SAtt>(a, "crestThreshDb", crestThreshKnob);
    attRmsWin       = std::make_unique<SAtt>(a, "rmsWinMs",      rmsWinKnob);
    attFloor        = std::make_unique<SAtt>(a, "floorDb",       floorKnob);

    attRatio        = std::make_unique<SAtt>(a, "ratio",         ratioKnob);
    attKnee         = std::make_unique<SAtt>(a, "kneeDb",        kneeKnob);
    attAttack       = std::make_unique<SAtt>(a, "attackMs",      attackKnob);
    attRelease      = std::make_unique<SAtt>(a, "releaseMs",     releaseKnob);
    attAutoRel      = std::make_unique<BAtt>(a, "autoRelease",   autoRelBtn);
    attLookahead    = std::make_unique<SAtt>(a, "lookaheadMs",   lookaheadKnob);
    attGritDrive    = std::make_unique<SAtt>(a, "gritDriveDb",   gritDriveKnob);

    attAudition     = std::make_unique<CAtt>(a, "audition",      auditionBox);
    attStereoLink   = std::make_unique<SAtt>(a, "stereoLink",    stereoLinkKnob);
    attMix          = std::make_unique<SAtt>(a, "mixPct",        mixKnob);
    attOutGain      = std::make_unique<SAtt>(a, "outGainDb",     outGainKnob);
    attBypass       = std::make_unique<BAtt>(a, "bypass",        bypassBtn);

    addMouseListener(this, true);
    startTimerHz(60);
}

SandpaperFairyAudioProcessorEditor::~SandpaperFairyAudioProcessorEditor()
{
    stopTimer();
    removeMouseListener(this);
    setLookAndFeel(nullptr);
}

void SandpaperFairyAudioProcessorEditor::setupKnob(juce::Slider& s, const juce::String& suffix,
                                                   uint32_t accentHex, bool bipolar, const juce::String& tip)
{
    using namespace sylredhel::sketch;
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    s.setTextValueSuffix(suffix);
    s.setColour(juce::Slider::textBoxTextColourId,       kGraphite);
    s.setColour(juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    s.setColour(juce::Slider::textBoxBackgroundColourId, kPaperBot.withAlpha(0.65f));
    s.getProperties().set(LookAndFeelSketchbook::kAccentProp,  static_cast<int>(accentHex));
    s.getProperties().set(LookAndFeelSketchbook::kBipolarProp, bipolar);
    s.setRotaryParameters(juce::degreesToRadians(220.0f), juce::degreesToRadians(500.0f), true);
    addAndMakeVisible(s);
    registerTip(s, tip);
}

void SandpaperFairyAudioProcessorEditor::registerTip(juce::Component& c, const juce::String& tip)
{
    c.getProperties().set("hoverTip", tip);
}

void SandpaperFairyAudioProcessorEditor::mouseEnter(const juce::MouseEvent& e)
{
    auto* comp = e.eventComponent;
    while (comp != nullptr && comp != this)
    {
        if (comp->getProperties().contains("hoverTip"))
        {
            hoverTipText = comp->getProperties()["hoverTip"].toString();
            repaint(0, getHeight() - 32, getWidth(), 32);
            return;
        }
        comp = comp->getParentComponent();
    }
}

void SandpaperFairyAudioProcessorEditor::mouseExit(const juce::MouseEvent&)
{
    hoverTipText = "Hover over any control for a quick guide  |  Tip: Use SC MONITOR to dial SC FREQ, and add +2..+6 dB GRIT in RIDER for vivid punch!";
    repaint(0, getHeight() - 32, getWidth(), 32);
}

void SandpaperFairyAudioProcessorEditor::loadFactoryPresetByIdx(int idx)
{
    const auto& list = SandpaperPresetManager::getFactoryPresets();
    if (idx < 0 || idx >= static_cast<int>(list.size()))
        return;

    currentFactoryPresetIdx = idx;
    presetBox.setSelectedId(idx + 1, juce::dontSendNotification);
    SandpaperPresetManager::applyFactoryPreset(processorRef.apvts, idx);
    hoverTipText = "Preset: " + list[static_cast<size_t>(idx)].tip;
    repaint(0, getHeight() - 32, getWidth(), 32);
}

void SandpaperFairyAudioProcessorEditor::openSavePresetDialog()
{
    auto startDir = SandpaperPresetManager::getUserPresetDirectory();
    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Sandpaper Fairy Preset", startDir.getChildFile("MySandpaperPreset.xml"), "*.xml");

    constexpr int flags = juce::FileBrowserComponent::saveMode
                        | juce::FileBrowserComponent::canSelectFiles
                        | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f != juce::File {})
        {
            auto target = f.hasFileExtension(".xml") ? f : f.withFileExtension(".xml");
            if (SandpaperPresetManager::saveUserPreset(processorRef.apvts, target))
            {
                hoverTipText = "Saved preset: " + target.getFileName();
                repaint(0, getHeight() - 32, getWidth(), 32);
            }
        }
    });
}

void SandpaperFairyAudioProcessorEditor::openLoadPresetDialog()
{
    auto startDir = SandpaperPresetManager::getUserPresetDirectory();
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Sandpaper Fairy Preset", startDir, "*.xml");

    constexpr int flags = juce::FileBrowserComponent::openMode
                        | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f.existsAsFile())
        {
            if (SandpaperPresetManager::loadUserPreset(processorRef.apvts, f))
            {
                presetBox.setText(f.getFileNameWithoutExtension(), juce::dontSendNotification);
                hoverTipText = "Loaded user preset: " + f.getFileName();
                repaint(0, getHeight() - 32, getWidth(), 32);
            }
        }
    });
}

void SandpaperFairyAudioProcessorEditor::timerCallback()
{
    const int engIdx = engineModeBox.getSelectedItemIndex();
    const int scIdx  = scFilterBox.getSelectedItemIndex();
    const int osIdx  = osBox.getSelectedItemIndex();

    scFreqKnob.setAlpha(scIdx == 0 ? 0.42f : 1.0f);
    gritDriveKnob.setAlpha(1.0f); // Unlocked in BOTH RIDER & VARI-MU!

    const float grDb       = processorRef.getCurrentGrDb();
    const float cfDb       = processorRef.getCurrentCrestDb();
    const float liveDrvDb  = processorRef.getLiveGritDriveDb();
    const float gritSetDb  = static_cast<float>(gritDriveKnob.getValue());

    // Live terracotta modulated drive dot whenever GRIT DRIVE > 0.1 dB (in BOTH modes!)
    if (gritSetDb > 0.1f)
    {
        const double liveNorm = juce::jlimit(0.0, 1.0, static_cast<double>(liveDrvDb) / 24.0);
        gritDriveKnob.getProperties().set(LookAndFeelSketchbook::kLivePosProp, liveNorm);
        gritDriveKnob.repaint();
    }
    else if (static_cast<double>(gritDriveKnob.getProperties().getWithDefault(LookAndFeelSketchbook::kLivePosProp, -1.0)) >= 0.0)
    {
        gritDriveKnob.getProperties().set(LookAndFeelSketchbook::kLivePosProp, -1.0);
        gritDriveKnob.repaint();
    }

    scopeView.updateFrame(engIdx, scIdx, osIdx, static_cast<float>(crestThreshKnob.getValue()), gritSetDb);
    curveView.updateParams(engIdx,
                           static_cast<float>(crestThreshKnob.getValue()),
                           static_cast<float>(ratioKnob.getValue()),
                           static_cast<float>(kneeKnob.getValue()),
                           gritSetDb, cfDb, grDb);
    mascotView.updateActivity(std::max(grDb, liveDrvDb * 0.45f));
}

void SandpaperFairyAudioProcessorEditor::paint(juce::Graphics& g)
{
    using namespace sylredhel::sketch;
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());

    // 1. Warm Cream Sketchbook Paper Backdrop
    juce::ColourGradient paper(kPaperTop, 0.0f, 0.0f, kPaperBot, 0.0f, h, false);
    g.setGradientFill(paper);
    g.fillAll();

    g.setColour(kGridLine.withAlpha(0.35f));
    for (float y = 58.0f; y < h - 30.0f; y += 26.0f)
        g.drawHorizontalLine(static_cast<int>(y), 24.0f, w - 12.0f);

    // Sketchbook spiral binding holes
    g.setColour(kGraphiteMid.withAlpha(0.20f));
    g.drawVerticalLine(20, 0.0f, h);
    for (float y = 26.0f; y < h - 18.0f; y += 34.0f)
    {
        g.setColour(kGraphite.withAlpha(0.18f));
        g.fillEllipse(6.0f, y, 8.0f, 8.0f);
        g.setColour(kGraphiteMid.withAlpha(0.45f));
        g.drawEllipse(6.0f, y, 8.0f, 8.0f, 1.0f);
        g.drawLine(0.0f, y + 4.0f, 9.0f, y + 4.0f, 1.4f);
    }

    // 2. Top Sketchbook Header Strip
    g.setColour(kCardFill.withAlpha(0.85f));
    g.fillRect(21.0f, 0.0f, w - 21.0f, 52.0f);
    g.setColour(kGraphiteMid.withAlpha(0.55f));
    g.drawHorizontalLine(52, 21.0f, w);
    g.setColour(kGraphiteLt.withAlpha(0.30f));
    g.drawHorizontalLine(54, 21.0f, w);

    const juce::Rectangle<float> swatch(31.0f, 13.0f, 22.0f, 24.0f);
    g.setColour(kKraftLight.withAlpha(0.75f));
    g.fillRoundedRectangle(swatch, 3.0f);
    g.setColour(kGraphite);
    g.drawRoundedRectangle(swatch, 3.0f, 1.4f);
    g.setColour(kKraftBrown);
    for (int gy = 0; gy < 3; ++gy)
        for (int gx = 0; gx < 3; ++gx)
            g.fillEllipse(35.0f + gx * 5.5f, 17.5f + gy * 6.0f, 2.0f, 2.0f);

    g.setColour(kGraphiteMid);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::italic)));
    g.drawText("Sylredhel", 64, 7, 150, 14, juce::Justification::centredLeft);

    g.setColour(kGraphite);
    g.setFont(juce::Font(juce::FontOptions(19.5f, juce::Font::bold)));
    g.drawText("SANDPAPER FAIRY", 63, 21, 220, 24, juce::Justification::centredLeft);

    g.setColour(kKraftBrown);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("v3.2", 252, 26, 36, 16, juce::Justification::centredLeft);

    // 3. Three Sketchbook Control Cards
    auto drawCard = [&g](juce::Rectangle<float> r, const juce::String& title, juce::Colour accent)
    {
        g.setColour(kCardFill.withAlpha(0.92f));
        g.fillRoundedRectangle(r, 6.0f);
        drawSketchBox(g, r, 6.0f, kGraphiteMid);

        g.setColour(accent.withAlpha(0.30f));
        g.fillRoundedRectangle(r.getX() + 12.0f, r.getY() + 21.0f, 174.0f, 5.0f, 2.5f);

        g.setColour(kGraphite);
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText(title, static_cast<int>(r.getX() + 14), static_cast<int>(r.getY() + 8),
                   static_cast<int>(r.getWidth() - 28), 16, juce::Justification::centredLeft);
    };

    drawCard({ 32.0f,  284.0f, 360.0f, 316.0f }, "1. CREST DETECTOR & SC",       kOchre);
    drawCard({ 404.0f, 284.0f, 364.0f, 316.0f }, "2. DYNAMICS & CREST DRIVE",    kKraftBrown);
    drawCard({ 780.0f, 284.0f, 228.0f, 316.0f }, "3. LINK, MONITOR & OUTPUT",    kSlateBlue);

    auto lbl = [&g](const juce::String& txt, int x, int y, int width, juce::Colour col = kGraphiteMid)
    {
        g.setColour(col);
        g.setFont(juce::Font(juce::FontOptions(8.6f, juce::Font::bold)));
        g.drawText(txt, x, y, width, 13, juce::Justification::centred);
    };

    // Card 1 Labels
    lbl("ENGINE MODE",    46, 318, 160, kKraftBrown);
    lbl("SC FILTER",     218, 318, 160, kSageTeal.darker(0.15f));

    lbl("CREST THRESH",   44, 408,  84, kOchre.darker(0.20f));
    lbl("RMS WIN",       132, 408,  82, kKraftBrown);
    lbl("SC FREQ",       218, 408,  82, kSageTeal.darker(0.15f));
    lbl("FLOOR",         304, 408,  78, kSlateBlue);

    g.setColour(kGraphiteMid);
    g.setFont(juce::Font(juce::FontOptions(8.4f, juce::Font::italic)));
    g.drawFittedText("Pure wideband audio path with 12 dB/oct Sidechain Filter (HPF/LPF/BAND).\nFast 2.5ms peak detector + 60ms phrase guard keeps grooves open & punchy.",
                     44, 542, 336, 34, juce::Justification::centred, 2);

    // Card 2 Labels
    lbl("RATIO",       416, 318, 108, kKraftBrown);
    lbl("ATTACK",      532, 318, 108, kTerracotta);
    lbl("RELEASE",     648, 318, 108, kTerracotta);

    lbl("LOOKAHEAD",   416, 444, 108, kSlateBlue);
    lbl("KNEE",        532, 444, 108, kKraftBrown);
    lbl("GRIT DRIVE",  648, 444, 108, kTubeAmber);

    // Card 3 Labels
    lbl("AUDITION",  794, 318, 200, kTerracotta);
    lbl("ST LINK",   788, 386,  68, kKraftBrown);
    lbl("MIX",       860, 386,  68);
    lbl("OUT GAIN",  932, 386,  68);

    // 4. Bottom 1-Line Hover Guide Bar
    const auto bar = juce::Rectangle<float>(32.0f, h - 32.0f, w - 64.0f, 24.0f);
    g.setColour(kCardFill.withAlpha(0.90f));
    g.fillRoundedRectangle(bar, 4.0f);
    drawSketchBox(g, bar, 4.0f, kGraphiteLt);

    g.setColour(kKraftBrown);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("GUIDE:", static_cast<int>(bar.getX() + 10), static_cast<int>(bar.getY()), 48, 24,
               juce::Justification::centredLeft);

    g.setColour(kGraphite);
    g.setFont(juce::Font(juce::FontOptions(10.2f, juce::Font::plain)));
    g.drawText(hoverTipText, static_cast<int>(bar.getX() + 58), static_cast<int>(bar.getY()),
               static_cast<int>(bar.getWidth() - 68), 24, juce::Justification::centredLeft, true);
}

void SandpaperFairyAudioProcessorEditor::resized()
{
    // Header: Preset Bar + Oversampling Selector
    prevPresetBtn.setBounds(300, 13,  26, 26);
    presetBox.setBounds(330,     13, 270, 26);
    nextPresetBtn.setBounds(604, 13,  26, 26);
    savePresetBtn.setBounds(638, 13,  56, 26);
    loadPresetBtn.setBounds(698, 13,  56, 26);
    osBox.setBounds(862,         13, 146, 26);

    // Upper Visual Row
    scopeView.setBounds(32,   64, 548, 208);
    curveView.setBounds(592,  64, 232, 208);
    mascotView.setBounds(836, 64, 172, 208);

    // Card 1: Crest Detector & Sidechain Filter
    engineModeBox.setBounds(46,     334, 160, 28);
    scFilterBox.setBounds(218,      334, 160, 28);
    crestThreshKnob.setBounds(44,   426,  84, 102);
    rmsWinKnob.setBounds(132,       426,  82, 102);
    scFreqKnob.setBounds(218,       426,  82, 102);
    floorKnob.setBounds(304,        426,  78, 102);

    // Card 2: Dynamics & Unlocked Crest-Followed Grit Drive
    ratioKnob.setBounds(425,      334,  90, 98);
    attackKnob.setBounds(541,     334,  90, 98);
    releaseKnob.setBounds(657,    334,  90, 98);

    lookaheadKnob.setBounds(425,  460,  90, 98);
    kneeKnob.setBounds(541,       460,  90, 98);
    gritDriveKnob.setBounds(657,  460,  90, 98);

    autoRelBtn.setBounds(520,     566, 132, 24);

    // Card 3: Link, Monitor & Output
    auditionBox.setBounds(794,     334, 200, 28);
    stereoLinkKnob.setBounds(788,  404,  68, 100);
    mixKnob.setBounds(860,         404,  68, 100);
    outGainKnob.setBounds(932,     404,  68, 100);
    bypassBtn.setBounds(794,       534, 200,  34);
}
