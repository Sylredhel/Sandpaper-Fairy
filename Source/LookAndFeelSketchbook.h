#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include <array>
#include <cmath>

namespace sylredhel::sketch
{
    static inline const juce::Colour kPaperTop    { 0xfff7f2e6 };
    static inline const juce::Colour kPaperBot    { 0xffece3cf };
    static inline const juce::Colour kCardFill    { 0xfffaf6ec };
    static inline const juce::Colour kPlotBg      { 0xfff3ecd9 };
    static inline const juce::Colour kGraphite    { 0xff2f2c28 };
    static inline const juce::Colour kGraphiteMid { 0xff5a544b };
    static inline const juce::Colour kGraphiteLt  { 0xff9b9283 };
    static inline const juce::Colour kGridLine    { 0xffdfd6c1 };

    static inline const juce::Colour kKraftBrown  { 0xffb2622e };
    static inline const juce::Colour kKraftLight  { 0xffd9a773 };
    static inline const juce::Colour kTerracotta  { 0xffc6523c };
    static inline const juce::Colour kOchre       { 0xffc0842c };
    static inline const juce::Colour kSlateBlue   { 0xff466987 };
    static inline const juce::Colour kSageTeal    { 0xff2f8578 };
    static inline const juce::Colour kTubeAmber   { 0xffc46b28 };

    static inline void drawSketchBox(juce::Graphics& g, juce::Rectangle<float> r,
                                     float corner = 6.0f, juce::Colour strokeCol = kGraphiteMid)
    {
        g.setColour(strokeCol.withAlpha(0.78f));
        g.drawRoundedRectangle(r, corner, 1.35f);
        g.setColour(strokeCol.withAlpha(0.24f));
        g.drawRoundedRectangle(r.translated(0.9f, 0.7f).reduced(0.4f), corner, 0.9f);
    }

    static inline void drawPencilHatch(juce::Graphics& g, juce::Rectangle<float> r,
                                       juce::Colour col, float step = 7.0f)
    {
        g.saveState();
        g.reduceClipRegion(r.toNearestInt());
        g.setColour(col);
        const float h = r.getHeight();
        for (float x = r.getX() - h; x < r.getRight() + h; x += step)
            g.drawLine(x, r.getBottom(), x + h, r.getY(), 0.75f);
        g.restoreState();
    }
}

class LookAndFeelSketchbook : public juce::LookAndFeel_V4
{
public:
    static constexpr const char* kAccentProp  = "sylAccent";
    static constexpr const char* kBipolarProp = "sylBipolar";
    static constexpr const char* kLivePosProp = "sylLivePos";

    LookAndFeelSketchbook()
    {
        using namespace sylredhel::sketch;
        setColour(juce::ComboBox::backgroundColourId, kCardFill);
        setColour(juce::ComboBox::textColourId,       kGraphite);
        setColour(juce::ComboBox::outlineColourId,    kGraphiteMid);
        setColour(juce::ComboBox::arrowColourId,      kGraphite);

        setColour(juce::PopupMenu::backgroundColourId,            kPaperTop);
        setColour(juce::PopupMenu::textColourId,                  kGraphite);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, kKraftBrown.withAlpha(0.22f));
        setColour(juce::PopupMenu::highlightedTextColourId,       kGraphite);

        setColour(juce::TextButton::buttonColourId,   kCardFill);
        setColour(juce::TextButton::buttonOnColourId, kKraftBrown.withAlpha(0.25f));
        setColour(juce::TextButton::textColourOffId,  kGraphite);
        setColour(juce::TextButton::textColourOnId,   kGraphite);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        using namespace sylredhel::sketch;
        auto b = juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f);
        g.setColour(box.isMouseOver() ? juce::Colour(0xfffffbf2) : kCardFill);
        g.fillRoundedRectangle(b, 4.5f);
        drawSketchBox(g, b, 4.5f, box.isMouseOver() ? kGraphite : kGraphiteMid);

        const float cx = static_cast<float>(buttonX) + static_cast<float>(buttonW) * 0.5f;
        const float cy = static_cast<float>(buttonY) + static_cast<float>(buttonH) * 0.5f;
        juce::Path arrow;
        arrow.startNewSubPath(cx - 4.0f, cy - 2.0f);
        arrow.lineTo(cx, cy + 2.8f);
        arrow.lineTo(cx + 4.0f, cy - 2.0f);
        g.setColour(kGraphite);
        g.strokePath(arrow, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour&, bool isHighlighted, bool isDown) override
    {
        using namespace sylredhel::sketch;
        auto b = btn.getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(isDown ? kGridLine : (isHighlighted ? juce::Colour(0xfffffbf2) : kCardFill));
        g.fillRoundedRectangle(b, 4.5f);
        drawSketchBox(g, b, 4.5f, isHighlighted ? kGraphite : kGraphiteMid);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider& slider) override
    {
        using namespace sylredhel::sketch;
        const auto b = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                              static_cast<float>(width), static_cast<float>(height));
        const float cx = b.getCentreX();
        const float cy = b.getCentreY() + 1.0f;
        const float r  = juce::jmin(b.getWidth(), b.getHeight()) * 0.34f;
        const float angle = startAngle + sliderPos * (endAngle - startAngle);

        const uint32_t colHex = static_cast<uint32_t>(
            static_cast<int>(slider.getProperties().getWithDefault(kAccentProp, static_cast<int>(0xffb2622e))));
        const bool bipolar = static_cast<bool>(slider.getProperties().getWithDefault(kBipolarProp, false));
        const juce::Colour accent(colHex);

        const float tickR1 = r + 3.5f;
        const float tickR2 = r + 6.5f;
        g.setColour(kGraphiteLt.withAlpha(0.75f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (static_cast<float>(i) / 10.0f) * (endAngle - startAngle) - juce::MathConstants<float>::halfPi;
            g.drawLine(cx + std::cos(a) * tickR1, cy + std::sin(a) * tickR1,
                       cx + std::cos(a) * tickR2, cy + std::sin(a) * tickR2, (i == 0 || i == 5 || i == 10) ? 1.4f : 0.9f);
        }

        const float arcR = r + 5.0f;
        const float midAngle = 0.5f * (startAngle + endAngle);
        if (bipolar)
        {
            if (std::abs(angle - midAngle) > 0.01f)
            {
                juce::Path valArc;
                valArc.addCentredArc(cx, cy, arcR, arcR, 0.0f,
                                     std::min(midAngle, angle), std::max(midAngle, angle), true);
                g.setColour(accent.withAlpha(0.88f));
                g.strokePath(valArc, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour(accent.withAlpha(0.28f));
                g.strokePath(valArc, juce::PathStrokeType(4.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            g.setColour(kGraphite);
            g.drawVerticalLine(static_cast<int>(cx), cy - arcR - 4.0f, cy - arcR + 1.5f);
        }
        else if (sliderPos > 0.002f)
        {
            const float livePos = static_cast<float>(slider.getProperties().getWithDefault(kLivePosProp, -1.0));
            juce::Path valArc;
            valArc.addCentredArc(cx, cy, arcR, arcR, 0.0f, startAngle, angle, true);
            g.setColour(accent.withAlpha(livePos >= 0.0f ? 0.30f : 0.88f));
            g.strokePath(valArc, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            if (livePos < 0.0f)
            {
                g.setColour(accent.withAlpha(0.28f));
                g.strokePath(valArc, juce::PathStrokeType(4.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            else
            {
                const float clampedLive = juce::jlimit(0.0f, 1.0f, livePos);
                const float liveAngle = startAngle + clampedLive * (endAngle - startAngle);
                if (clampedLive > 0.003f)
                {
                    juce::Path liveArc;
                    liveArc.addCentredArc(cx, cy, arcR, arcR, 0.0f, startAngle, liveAngle, true);
                    g.setColour(kTerracotta.withAlpha(0.92f));
                    g.strokePath(liveArc, juce::PathStrokeType(3.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                }
                const float dotA = liveAngle - juce::MathConstants<float>::halfPi;
                g.setColour(kTerracotta);
                g.fillEllipse(cx + std::cos(dotA) * arcR - 3.0f, cy + std::sin(dotA) * arcR - 3.0f, 6.0f, 6.0f);
            }
        }

        g.setColour(kGraphite.withAlpha(0.10f));
        g.fillEllipse(cx - r + 1.8f, cy - r + 2.4f, r * 2.0f, r * 2.0f);

        juce::ColourGradient cap(juce::Colour(0xfffffcf5), cx - r * 0.4f, cy - r * 0.5f,
                                 juce::Colour(0xffe9dfc9), cx + r * 0.6f, cy + r * 0.6f, false);
        g.setGradientFill(cap);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

        g.setColour(kGraphite.withAlpha(0.85f));
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);
        g.setColour(kGraphite.withAlpha(0.28f));
        g.drawEllipse(cx - r + 0.8f, cy - r - 0.5f, r * 2.0f - 0.5f, r * 2.0f + 0.3f, 0.9f);

        const float ptrA = angle - juce::MathConstants<float>::halfPi;
        g.setColour(accent);
        g.drawLine(cx + std::cos(ptrA) * (r * 0.22f), cy + std::sin(ptrA) * (r * 0.22f),
                   cx + std::cos(ptrA) * (r * 0.84f), cy + std::sin(ptrA) * (r * 0.84f), 2.4f);
        g.setColour(kGraphite);
        g.fillEllipse(cx - 2.0f, cy - 2.0f, 4.0f, 4.0f);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& btn,
                          bool isHighlighted, bool) override
    {
        using namespace sylredhel::sketch;
        auto b = btn.getLocalBounds().toFloat().reduced(1.5f);
        const bool isOn = btn.getToggleState();
        const bool isBypass = btn.getButtonText().containsIgnoreCase("BYPASS");

        const juce::Colour accent = isBypass ? kTerracotta : kKraftBrown;
        const juce::Colour fillCol = isOn
            ? accent.withAlpha(0.24f)
            : (isHighlighted ? juce::Colour(0xfffffbf2) : kCardFill);
        const juce::Colour rimCol = isOn
            ? accent
            : (isHighlighted ? kGraphite : kGraphiteMid);

        g.setColour(fillCol);
        g.fillRoundedRectangle(b, 4.5f);
        if (isOn)
            drawPencilHatch(g, b.reduced(1.5f), rimCol.withAlpha(0.18f), 6.0f);
        drawSketchBox(g, b, 4.5f, rimCol);

        g.setColour(isOn ? accent.darker(0.32f) : kGraphite);
        g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
        g.drawText(btn.getButtonText(), b.toNearestInt(), juce::Justification::centred);
    }
};

/**
 * Calm, Focused "Turtle-Neck" Sandpaper Fairy Mascot!
 *
 * Design rules:
 *   - Head & eyes stay rock-steady (ZERO frantic head/eye shaking!).
 *   - When sanding active spikes, the fairy smoothly leans forward/downward into a
 *     cute focused "turtle-neck" posture (shoulders hunch up slightly, head cranes
 *     forward & closer to the spike) while its expression turns intensely focused.
 *   - ONLY the arms, hands, and kraft sandpaper strip scrub rapidly left-and-right!
 *   - A genuine sharp, jagged, crystalline triangular STALAGMITE SPIKE shoots up
 *     through a cracked sketchbook floor (wide faceted rocky base -> razor-sharp apex),
 *     never looking like a blunt pipe!
 */
class SandpaperFairyMascot : public juce::Component
{
public:
    void updateActivity(float grDbIn)
    {
        const float target = juce::jlimit(0.0f, 1.0f, grDbIn / 7.5f);
        // Smooth posture envelope (no jittery snapping!)
        activity = (target > activity) ? (activity * 0.60f + target * 0.40f)
                                       : (activity * 0.88f + target * 0.12f);

        // Brisk arm/sandpaper scrubbing oscillation (~5..9 Hz @ 60 Hz)
        scrubPhase += 0.36f + 0.56f * activity;
        if (scrubPhase > 628.3185f) scrubPhase -= 628.3185f;

        dustPhase += 0.042f + 0.080f * activity;
        if (dustPhase > 100.0f) dustPhase -= 100.0f;

        idlePhase += 0.035f;
        if (idlePhase > 6.283185f) idlePhase -= 6.283185f;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace sylredhel::sketch;
        auto b = getLocalBounds().toFloat();

        g.setColour(kPlotBg);
        g.fillRoundedRectangle(b, 6.0f);
        drawSketchBox(g, b.reduced(0.6f), 6.0f, kGraphiteMid);

        // Little kraft masking tape strip at top
        g.setColour(kKraftBrown.withAlpha(0.24f));
        g.fillRect(b.getCentreX() - 22.0f, b.getY() - 1.0f, 44.0f, 10.0f);

        const float cx     = b.getCentreX();
        const float floorY = b.getBottom() - 32.0f;

        // =====================================================================
        // 1. FAIRY BODY & STEADY "TURTLE-NECK" (GEOBUKMOK) POSTURE
        //    (Positioned slightly above the floor spike; head NEVER shakes!)
        // =====================================================================
        const float breathe    = std::sin(idlePhase) * 1.1f;
        // Focused turtle-neck hunch: head cranes slightly down & forward as activity rises
        const float hunchDown  = 8.5f * activity;
        const float shoulderUp = 3.2f * activity;

        const float bodyX = cx;
        const float bodyY = b.getY() + 78.0f + breathe + hunchDown * 0.35f;

        // Soft fluttery fairy wings behind shoulders (calm, steady)
        const float wingFlap = std::sin(idlePhase * 2.2f) * (1.8f + 2.2f * activity);
        juce::Path wL, wR;
        wL.addEllipse(bodyX - 39.0f, bodyY - 8.0f - shoulderUp - wingFlap, 24.0f, 14.5f);
        wR.addEllipse(bodyX + 15.0f, bodyY - 8.0f - shoulderUp - wingFlap, 24.0f, 14.5f);
        g.setColour(kCardFill.withAlpha(0.92f));
        g.fillPath(wL); g.fillPath(wR);
        g.setColour(kKraftLight.withAlpha(0.28f));
        g.fillPath(wL); g.fillPath(wR);
        g.setColour(kGraphiteMid);
        g.strokePath(wL, juce::PathStrokeType(1.2f));
        g.strokePath(wR, juce::PathStrokeType(1.2f));

        // Hunched shoulders + rounded workshop torso
        juce::Path torso;
        torso.startNewSubPath(bodyX - 18.0f, bodyY + 26.0f);
        torso.quadraticTo(bodyX - 20.0f, bodyY - 4.0f - shoulderUp, bodyX - 10.0f, bodyY - 6.0f - shoulderUp);
        torso.lineTo(bodyX + 10.0f, bodyY - 6.0f - shoulderUp);
        torso.quadraticTo(bodyX + 20.0f, bodyY - 4.0f - shoulderUp, bodyX + 18.0f, bodyY + 26.0f);
        torso.closeSubPath();

        g.setColour(juce::Colour(0xfffffbf0));
        g.fillPath(torso);
        g.setColour(kGraphite);
        g.strokePath(torso, juce::PathStrokeType(1.55f));

        // Kraft workshop apron
        juce::Rectangle<float> apron(bodyX - 12.0f, bodyY + 3.0f, 24.0f, 21.0f);
        g.setColour(kKraftBrown.withAlpha(0.26f));
        g.fillRoundedRectangle(apron, 4.0f);
        g.setColour(kKraftBrown.darker(0.15f));
        g.drawRoundedRectangle(apron, 4.0f, 1.15f);
        g.drawLine(bodyX - 8.0f, bodyY + 13.0f, bodyX + 8.0f, bodyY + 13.0f, 0.95f);

        // Steady Head (cranes slightly forward/downward into cute turtle-neck focus - ZERO horizontal shake!)
        const float hx = bodyX;
        const float hy = bodyY - 20.0f + hunchDown * 0.72f;
        const float headScale = 1.0f + 0.045f * activity; // Subtle perspective enlargement as head leans closer!
        const float headRx = 19.5f * headScale;
        const float headRy = 16.5f * headScale;

        // Tiny curved neck shadow line visible when leaning forward
        if (activity > 0.15f)
        {
            g.setColour(kGraphite.withAlpha(0.14f * activity));
            g.fillEllipse(hx - 14.0f, hy + headRy - 5.0f, 28.0f, 7.0f);
        }

        g.setColour(juce::Colour(0xfffffcf5));
        g.fillEllipse(hx - headRx, hy - headRy, headRx * 2.0f, headRy * 2.0f);
        g.setColour(kGraphite);
        g.drawEllipse(hx - headRx, hy - headRy, headRx * 2.0f, headRy * 2.0f, 1.55f);
        g.setColour(kGraphite.withAlpha(0.25f));
        g.drawEllipse(hx - headRx + 0.7f, hy - headRy + 0.4f, headRx * 2.0f - 0.5f, headRy * 2.0f - 0.6f, 0.9f);

        // Folded kraft bandana on forehead (moves smoothly with head, zero shake)
        juce::Path band;
        band.startNewSubPath(hx - headRx * 0.88f, hy - headRy * 0.48f);
        band.quadraticTo(hx, hy - headRy * 1.06f, hx + headRx * 0.88f, hy - headRy * 0.48f);
        band.lineTo(hx + headRx * 0.76f, hy - headRy * 0.88f);
        band.quadraticTo(hx, hy - headRy * 1.32f, hx - headRx * 0.76f, hy - headRy * 0.88f);
        band.closeSubPath();
        g.setColour(kKraftBrown.withAlpha(0.32f));
        g.fillPath(band);
        g.setColour(kGraphite);
        g.strokePath(band, juce::PathStrokeType(1.3f));
        g.drawLine(hx + headRx * 0.76f, hy - headRy * 0.74f, hx + headRx + 3.5f, hy - headRy * 0.92f, 1.2f);
        g.drawLine(hx + headRx * 0.82f, hy - headRy * 0.60f, hx + headRx + 4.0f, hy - headRy * 0.48f, 1.2f);

        // Steady Focused Eyes (NO left-right jitter! Calmly narrow & gaze downward at spike!)
        const float eyeH = 8.4f - 2.2f * activity; // Slightly narrowed in deep craftsman concentration!
        const float eyeY = hy - 0.5f + 2.0f * activity;
        const float pupilY = eyeY + 1.4f + 0.9f * activity;

        g.setColour(juce::Colours::white);
        g.fillEllipse(hx - 11.5f, eyeY - eyeH * 0.5f, 8.2f, eyeH);
        g.fillEllipse(hx + 3.3f,  eyeY - eyeH * 0.5f, 8.2f, eyeH);
        g.setColour(kGraphite);
        g.drawEllipse(hx - 11.5f, eyeY - eyeH * 0.5f, 8.2f, eyeH, 1.2f);
        g.drawEllipse(hx + 3.3f,  eyeY - eyeH * 0.5f, 8.2f, eyeH, 1.2f);

        // Steady centered-downward pupils (ZERO horizontal oscillation!)
        g.fillEllipse(hx - 9.1f, pupilY - 1.7f, 3.4f, 3.4f);
        g.fillEllipse(hx + 5.7f, pupilY - 1.7f, 3.4f, 3.4f);

        // Expressive focused eyebrows (gentle neutral -> concentrated inward tilt)
        g.setColour(kGraphite);
        const float browTilt = 2.6f * activity;
        const float browY    = eyeY - eyeH * 0.5f - 2.6f;
        g.drawLine(hx - 12.2f, browY - browTilt * 0.35f, hx - 4.2f, browY + browTilt * 0.85f, 1.25f);
        g.drawLine(hx + 4.2f,  browY + browTilt * 0.85f, hx + 12.2f, browY - browTilt * 0.35f, 1.25f);

        // Soft blush cheeks
        g.setColour(kTerracotta.withAlpha(0.20f + 0.20f * activity));
        g.fillEllipse(hx - 15.8f, eyeY + 5.2f, 6.4f, 3.6f);
        g.fillEllipse(hx + 9.4f,  eyeY + 5.2f, 6.4f, 3.6f);

        // Expression changes cleanly: calm gentle smile -> concentrated pursed mouth!
        g.setColour(kGraphite);
        const float mouthY = eyeY + 8.0f;
        if (activity > 0.38f)
        {
            // Determined scrunched 'w' craftsman mouth
            juce::Path m;
            m.startNewSubPath(hx - 3.6f, mouthY + 0.8f);
            m.lineTo(hx - 1.2f, mouthY - 0.8f);
            m.lineTo(hx + 1.2f, mouthY + 0.8f);
            m.lineTo(hx + 3.6f, mouthY - 0.8f);
            g.strokePath(m, juce::PathStrokeType(1.25f));
        }
        else if (activity > 0.10f)
        {
            // Focused straight line
            g.drawLine(hx - 3.2f, mouthY, hx + 3.2f, mouthY, 1.25f);
        }
        else
        {
            // Calm subtle idle smile
            juce::Path m;
            m.startNewSubPath(hx - 3.2f, mouthY - 0.5f);
            m.quadraticTo(hx, mouthY + 1.8f, hx + 3.2f, mouthY - 0.5f);
            g.strokePath(m, juce::PathStrokeType(1.2f));
        }

        // Single calm sweat drop on forehead only during heavy spikes
        if (activity > 0.52f)
        {
            const float sx = hx - headRx - 3.5f;
            const float sy = hy - 6.0f + std::fmod(dustPhase * 6.0f, 5.0f);
            g.setColour(kSlateBlue.withAlpha(0.75f * (activity - 0.4f) * 1.6f));
            g.drawEllipse(sx, sy, 3.2f, 4.8f, 1.05f);
        }

        // =====================================================================
        // 2. CRACKED SKETCHBOOK FLOOR & TRUE SHARP POINTED STALAGMITE SPIKE!
        //    (Wide jagged triangular rock base -> razor-sharp needle apex, NOT a pipe!)
        // =====================================================================
        g.setColour(kGraphiteMid.withAlpha(0.78f));
        g.drawLine(b.getX() + 12.0f, floorY, b.getRight() - 12.0f, floorY, 1.45f);
        g.setColour(kGraphiteLt.withAlpha(0.45f));
        g.drawLine(b.getX() + 18.0f, floorY + 2.5f, b.getRight() - 20.0f, floorY + 2.5f, 0.9f);

        // Spike geometry: shoots dramatically UPWARD from floor when GR occurs!
        const float spikeH     = 11.0f + 46.0f * activity; // Tall dramatic pointed spike!
        const float baseHalfW  = 16.0f + 16.0f * activity; // Wide rocky base (32..64 px)
        const float fullTipY   = floorY - spikeH;
        // Only the very razor tip (top ~20%) is actively shaved down slightly by the sandpaper!
        const float shavedCutH = 3.0f + 11.0f * activity;
        const float contactY   = fullTipY + shavedCutH * 0.75f;
        const float tipHalfW   = std::max(1.2f, (shavedCutH / std::max(spikeH, 12.0f)) * baseHalfW * 0.65f);

        // Small cracked floor fracture lines & tiny jagged rock barbs flanking the main spike!
        if (activity > 0.05f)
        {
            const float crackA = std::min(0.80f, 0.25f + activity * 0.75f);
            g.setColour(kGraphite.withAlpha(crackA));
            g.drawLine(cx - baseHalfW - 8.0f, floorY + 1.0f, cx - baseHalfW - 2.0f, floorY + 5.5f, 1.0f);
            g.drawLine(cx + baseHalfW + 8.0f, floorY + 1.0f, cx + baseHalfW + 3.0f, floorY + 5.0f, 1.0f);

            // Miniature sharp side stalagmite barbs (makes silhouette unmistakably spiky/rocky!)
            auto drawMiniBarb = [&](float bx, float bw, float bh)
            {
                juce::Path barb;
                barb.startNewSubPath(bx - bw, floorY);
                barb.lineTo(bx + bw * 0.15f, floorY - bh);
                barb.lineTo(bx + bw, floorY);
                barb.closeSubPath();
                g.setColour(kGraphiteLt.withAlpha(0.32f));
                g.fillPath(barb);
                g.setColour(kGraphite.withAlpha(0.82f));
                g.strokePath(barb, juce::PathStrokeType(1.2f));
            };
            drawMiniBarb(cx - baseHalfW * 0.88f, 6.5f + 3.0f * activity, 7.0f + 12.0f * activity);
            drawMiniBarb(cx + baseHalfW * 0.86f, 6.0f + 2.8f * activity, 6.0f + 10.0f * activity);
        }

        // Main Faceted Crystalline Stalagmite Body (Left light facet + Right shaded facet!)
        juce::Path spikeBody;
        spikeBody.startNewSubPath(cx - baseHalfW, floorY);
        spikeBody.lineTo(cx - baseHalfW * 0.56f, floorY - spikeH * 0.36f);
        spikeBody.lineTo(cx - tipHalfW, contactY);
        spikeBody.lineTo(cx + tipHalfW, contactY);
        spikeBody.lineTo(cx + baseHalfW * 0.52f, floorY - spikeH * 0.42f);
        spikeBody.lineTo(cx + baseHalfW, floorY);
        spikeBody.closeSubPath();

        // Left lit stone facet
        g.setColour(kGraphiteLt.withAlpha(0.24f));
        g.fillPath(spikeBody);

        // Right darker stone shadow facet (gives crisp 3D crystalline stalagmite volume!)
        juce::Path rightFacet;
        rightFacet.startNewSubPath(cx, contactY);
        rightFacet.lineTo(cx + tipHalfW, contactY);
        rightFacet.lineTo(cx + baseHalfW * 0.52f, floorY - spikeH * 0.42f);
        rightFacet.lineTo(cx + baseHalfW, floorY);
        rightFacet.lineTo(cx + baseHalfW * 0.08f, floorY);
        rightFacet.closeSubPath();
        g.setColour(kGraphiteMid.withAlpha(0.26f));
        g.fillPath(rightFacet);

        // Crisp central angular ridge line + bold graphite outline
        g.setColour(kGraphite.withAlpha(0.55f));
        g.drawLine(cx, contactY, cx + baseHalfW * 0.08f, floorY, 1.05f);
        g.setColour(kGraphite);
        g.strokePath(spikeBody, juce::PathStrokeType(1.55f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));

        // Sharp Pointed Tip Outline (intact when idle, red-dashed shaved apex when sanding!)
        if (activity > 0.06f)
        {
            // Razor-sharp needle apex protruding above sandpaper in terracotta dashes (shows what's being shaved off!)
            const float dashes[2] = { 2.8f, 2.2f };
            g.setColour(kTerracotta.withAlpha(std::min(0.90f, 0.35f + activity * 0.85f)));
            g.drawDashedLine(juce::Line<float>(cx - tipHalfW, contactY, cx, fullTipY), dashes, 2, 1.25f);
            g.drawDashedLine(juce::Line<float>(cx + tipHalfW, contactY, cx, fullTipY), dashes, 2, 1.25f);
        }
        else
        {
            // Tiny intact pointed pebble/thorn on the floor when idle
            juce::Path idleTip;
            idleTip.startNewSubPath(cx - tipHalfW, contactY);
            idleTip.lineTo(cx, fullTipY);
            idleTip.lineTo(cx + tipHalfW, contactY);
            g.setColour(kGraphite);
            g.strokePath(idleTip, juce::PathStrokeType(1.4f));
        }

        // =====================================================================
        // 3. FAST-SCRUBBING ARMS, HANDS & KRAFT SANDPAPER STRIP
        //    (ONLY hands & sandpaper scrub fast - head stays completely calm!)
        // =====================================================================
        const float scrubOsc   = std::sin(scrubPhase);
        const float scrubX     = (activity > 0.03f) ? (scrubOsc * (6.5f + 11.0f * activity)) : 0.0f;
        const float paperX     = cx + scrubX;
        const float paperY     = contactY - 1.5f;
        const float paperHalfW = 20.0f;

        // Subtle comic motion lines strictly beside the sandpaper strip
        if (activity > 0.14f)
        {
            const float blurDir = (std::cos(scrubPhase) >= 0.0f) ? -1.0f : 1.0f;
            const float bx = paperX + blurDir * (paperHalfW + 3.0f);
            g.setColour(kKraftBrown.withAlpha(std::min(0.60f, activity * 0.85f)));
            for (int s = 0; s < 2; ++s)
            {
                const float by = paperY - 2.0f + static_cast<float>(s) * 4.0f;
                const float len = 6.0f + 8.0f * activity;
                g.drawLine(bx, by, bx + blurDir * len, by, 1.1f);
            }
        }

        // Kraft Sandpaper Strip flexing across the sharp spike apex
        juce::Path spStrip;
        spStrip.startNewSubPath(paperX - paperHalfW, paperY - 4.0f);
        spStrip.quadraticTo(paperX, paperY + 2.0f, paperX + paperHalfW, paperY - 4.0f);
        spStrip.lineTo(paperX + paperHalfW, paperY + 2.5f);
        spStrip.quadraticTo(paperX, paperY + 8.0f, paperX - paperHalfW, paperY + 2.5f);
        spStrip.closeSubPath();

        g.setColour(kKraftLight.withAlpha(0.92f));
        g.fillPath(spStrip);
        g.setColour(kKraftBrown);
        g.strokePath(spStrip, juce::PathStrokeType(1.4f));

        // Grit dots on sandpaper
        g.setColour(kGraphite.withAlpha(0.62f));
        for (int i = -3; i <= 3; ++i)
        {
            const float gx = paperX + static_cast<float>(i) * 5.0f;
            const float gy = paperY + 1.2f - std::abs(static_cast<float>(i)) * 0.6f;
            g.fillEllipse(gx - 0.75f, gy - 0.75f, 1.5f, 1.5f);
        }

        // Arms reaching down from steady hunched shoulders to the briskly moving hands!
        const float shY = bodyY + 4.0f - shoulderUp * 0.5f;
        g.setColour(kGraphite);
        g.drawLine(bodyX - 15.0f, shY, paperX - paperHalfW + 2.0f, paperY - 1.0f, 1.55f);
        g.drawLine(bodyX + 15.0f, shY, paperX + paperHalfW - 2.0f, paperY - 1.0f, 1.55f);
        g.fillEllipse(paperX - paperHalfW - 0.5f, paperY - 3.3f, 4.6f, 4.6f);
        g.fillEllipse(paperX + paperHalfW - 4.1f, paperY - 3.3f, 4.6f, 4.6f);

        // =====================================================================
        // 4. FLYING GRAPHITE & SANDPAPER SHAVING DUST PARTICLES
        // =====================================================================
        if (activity > 0.06f)
        {
            const float dustA = std::min(0.92f, activity * 1.4f);
            for (int k = 0; k < 10; ++k)
            {
                const float side   = (k % 2 == 0) ? -1.0f : 1.0f;
                const float cycle  = std::fmod(dustPhase + static_cast<float>(k) * 0.217f, 1.0f);
                const float spread = 14.0f + (k % 4) * 7.0f + 14.0f * activity;
                const float arcH   = 7.0f + (k % 3) * 5.0f + 7.0f * activity;
                const float px = cx + side * (7.0f + cycle * spread);
                const float py = contactY - std::sin(cycle * 3.14159f) * arcH + cycle * 11.0f;
                const float sz = (1.5f + (k % 3) * 0.8f) * (1.0f - 0.45f * cycle);
                g.setColour((k % 3 == 0 ? kTerracotta : (k % 2 == 0 ? kKraftBrown : kGraphiteMid))
                                .withAlpha(dustA * (1.0f - cycle)));
                g.fillEllipse(px - sz * 0.5f, py - sz * 0.5f, sz, sz);
            }
        }

        // Caption
        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::italic)));
        g.setColour(kGraphiteMid);
        const juce::String mood = (activity > 0.48f) ? "filing sharp spike!"
                                : (activity > 0.06f) ? "sanding carefully..."
                                                     : "watching floor...";
        g.drawText(mood, static_cast<int>(b.getX() + 6), static_cast<int>(b.getBottom() - 19),
                   static_cast<int>(b.getWidth() - 12), 14, juce::Justification::centred);
    }

private:
    float activity   = 0.0f;
    float scrubPhase = 0.0f;
    float dustPhase  = 0.0f;
    float idlePhase  = 0.0f;
};

/**
 * Sketchbook Rolling Crest-Factor & Gain Reduction Scope!
 * Visualizes live Crest Factor (0 .. 24 dB) against your CREST THRESH line,
 * with terracotta fill whenever spikes exceed threshold and get sanded down!
 */
class SandpaperScopeView : public juce::Component
{
public:
    explicit SandpaperScopeView(SandpaperFairyAudioProcessor& proc) : processorRef(proc) {}

    void updateFrame(int engineModeIdx, int scModeIdx, int osModeIdx, float crestThreshVal, float gritSettingDb)
    {
        engIdx      = engineModeIdx;
        bandIdx     = scModeIdx;
        gritSetDb   = gritSettingDb;
        osIdx       = osModeIdx;
        crestThresh = crestThreshVal;
        curGrDb     = processorRef.getCurrentGrDb();
        curCfDb     = processorRef.getCurrentCrestDb();
        curDrvDb    = processorRef.getLiveGritDriveDb();

        const double fs = std::max(processorRef.getCurrentSampleRate(), 8000.0);
        latencyMs = static_cast<float>(1000.0 * static_cast<double>(processorRef.getEngineLatencySamples()) / fs);

        processorRef.getCrestScope().copyLatest(cfDbTrace.data(), grDbTrace.data(), kViewCols);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace sylredhel::sketch;
        auto b = getLocalBounds().toFloat();

        g.setColour(kPlotBg);
        g.fillRoundedRectangle(b, 6.0f);
        drawSketchBox(g, b.reduced(0.6f), 6.0f, kGraphiteMid);

        auto plot = b.reduced(12.0f, 10.0f);
        plot.removeFromTop(21.0f);

        constexpr float maxCf = 20.0f;
        auto cfToY = [&](float cf) -> float
        {
            const float norm = juce::jlimit(0.0f, 1.0f, cf / maxCf);
            return plot.getBottom() - norm * plot.getHeight();
        };

        // Graph-paper grid (6, 12, 18 dB Crest Factor lines)
        g.setColour(kGridLine);
        for (int i = 1; i < 8; ++i)
            g.drawVerticalLine(static_cast<int>(plot.getX() + plot.getWidth() * (static_cast<float>(i) / 8.0f)),
                               plot.getY(), plot.getBottom());

        g.setFont(juce::Font(juce::FontOptions(8.0f, juce::Font::plain)));
        for (float d = 5.0f; d <= 15.0f; d += 5.0f)
        {
            const float y = cfToY(d);
            g.setColour(kGridLine);
            g.drawHorizontalLine(static_cast<int>(y), plot.getX(), plot.getRight());
            g.setColour(kGraphiteLt);
            g.drawText(juce::String(static_cast<int>(d)) + "dB",
                       static_cast<int>(plot.getX() + 2), static_cast<int>(y - 10), 28, 10,
                       juce::Justification::centredLeft);
        }

        g.saveState();
        g.reduceClipRegion(plot.toNearestInt());

        // Build rolling Crest Factor trace (input CF vs sanded output CF = CF - GR)
        juce::Path pCfIn, pCfOut, pShaved;
        for (int i = 0; i < kViewCols; ++i)
        {
            const float x   = plot.getX() + (static_cast<float>(i) / static_cast<float>(kViewCols - 1)) * plot.getWidth();
            const float cIn = cfDbTrace[static_cast<size_t>(i)];
            const float gr  = grDbTrace[static_cast<size_t>(i)];
            const float cOut = std::max(0.0f, cIn - gr);
            const float yi  = cfToY(cIn);
            const float yo  = cfToY(cOut);
            if (i == 0) { pCfIn.startNewSubPath(x, yi); pCfOut.startNewSubPath(x, yo); pShaved.startNewSubPath(x, yi); }
            else        { pCfIn.lineTo(x, yi);          pCfOut.lineTo(x, yo);          pShaved.lineTo(x, yi); }
        }
        for (int i = kViewCols - 1; i >= 0; --i)
        {
            const float x    = plot.getX() + (static_cast<float>(i) / static_cast<float>(kViewCols - 1)) * plot.getWidth();
            const float cOut = std::max(0.0f, cfDbTrace[static_cast<size_t>(i)] - grDbTrace[static_cast<size_t>(i)]);
            pShaved.lineTo(x, cfToY(cOut));
        }
        pShaved.closeSubPath();

        // Soft cream/brown fill under sanded crest envelope
        juce::Path pFillOut(pCfOut);
        pFillOut.lineTo(plot.getRight(), plot.getBottom());
        pFillOut.lineTo(plot.getX(),     plot.getBottom());
        pFillOut.closeSubPath();
        g.setColour(kKraftBrown.withAlpha(0.11f));
        g.fillPath(pFillOut);

        // Shaved crest reduction ribbon in terracotta wash!
        g.setColour(kTerracotta.withAlpha(0.36f));
        g.fillPath(pShaved);

        // Dashed CREST THRESH horizontal reference line (Ochre)
        const float yTh = cfToY(crestThresh);
        const float dashes[2] = { 5.0f, 3.5f };
        g.setColour(kOchre.withAlpha(0.92f));
        g.drawDashedLine(juce::Line<float>(plot.getX(), yTh, plot.getRight(), yTh), dashes, 2, 1.35f);

        // Raw Crest Factor line (ochre) & Sanded Crest line (bold kraft brown)
        g.setColour(kOchre.withAlpha(0.72f));
        g.strokePath(pCfIn, juce::PathStrokeType(1.3f));

        g.setColour(kKraftBrown);
        g.strokePath(pCfOut, juce::PathStrokeType(2.0f));

        g.restoreState();

        // Top Telemetry Strip
        const auto top = juce::Rectangle<float>(b.getX() + 12.0f, b.getY() + 6.0f, b.getWidth() - 24.0f, 18.0f);
        static constexpr const char* engTags[2]  = { "RIDER", "VARI-MU" };
        static constexpr const char* bandTags[4] = { "SC OFF", "SC HPF", "SC LPF", "SC BAND" };
        static constexpr const char* osTags[4]   = { "1x", "2x", "4x", "8x" };

        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.setColour(kGraphite);
        g.drawText(juce::String(engTags[juce::jlimit(0, 1, engIdx)]) + " - " + juce::String(bandTags[juce::jlimit(0, 3, bandIdx)]),
                   static_cast<int>(top.getX()), static_cast<int>(top.getY()), 136, 18,
                   juce::Justification::centredLeft);

        g.setColour(kOchre.darker(0.20f));
        g.drawText("CF " + juce::String(curCfDb, 1) + "dB",
                   static_cast<int>(top.getX() + 138), static_cast<int>(top.getY()), 74, 18,
                   juce::Justification::centredLeft);

        g.setColour(curGrDb > 0.15f ? kTerracotta : kGraphiteLt);
        g.drawText("GR -" + juce::String(curGrDb, 1) + "dB",
                   static_cast<int>(top.getX() + 210), static_cast<int>(top.getY()), 70, 18,
                   juce::Justification::centredLeft);
        if (gritSetDb > 0.05f)
        {
            g.setColour(curDrvDb > 0.2f ? kTubeAmber : kGraphiteLt);
            g.drawText("DRV +" + juce::String(curDrvDb, 1) + "dB",
                       static_cast<int>(top.getX() + 282), static_cast<int>(top.getY()), 78, 18,
                       juce::Justification::centredLeft);
        }

        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
        g.setColour(kSlateBlue);
        g.drawText(juce::String(osTags[juce::jlimit(0, 3, osIdx)]) + " | PDC " + juce::String(latencyMs, 1) + "ms",
                   static_cast<int>(top.getRight() - 182), static_cast<int>(top.getY()), 108, 18,
                   juce::Justification::centredRight);

        g.setColour(kOchre);
        g.drawText("TH",  static_cast<int>(top.getRight() - 68), static_cast<int>(top.getY()), 22, 18, juce::Justification::centredRight);
        g.setColour(kKraftBrown);
        g.drawText("CF",  static_cast<int>(top.getRight() - 42), static_cast<int>(top.getY()), 20, 18, juce::Justification::centredRight);
        g.setColour(kTerracotta);
        g.drawText("GR",  static_cast<int>(top.getRight() - 20), static_cast<int>(top.getY()), 20, 18, juce::Justification::centredRight);
    }

private:
    static constexpr int kViewCols = 256;
    SandpaperFairyAudioProcessor& processorRef;
    std::array<float, kViewCols> cfDbTrace {};
    std::array<float, kViewCols> grDbTrace {};
    float crestThresh = 11.5f;
    float curGrDb     = 0.0f;
    float curCfDb     = 0.0f;
    float curDrvDb    = 0.0f;
    float gritSetDb   = 0.0f;
    float latencyMs   = 70.5f;
    int   engIdx      = 0;
    int   bandIdx     = 0;
    int   osIdx       = 2;
};

/**
 * Crest Factor Transfer Curve View:
 * X = Instantaneous Crest Factor (0 .. 24 dB)
 * Y = Applied Gain Reduction (0 .. -16 dB)
 * Shows linear VCA ratio in RIDER vs progressive Vari-Mu curve + Tube Grit aura in VARI-MU!
 */
class SpikeCurveView : public juce::Component
{
public:
    void updateParams(int engineModeIdx, float crestThreshIn, float ratioIn, float kneeIn,
                      float gritDriveDbIn, float liveCfIn, float liveGrIn)
    {
        engIdx      = engineModeIdx;
        crestThresh = crestThreshIn;
        ratio       = ratioIn;
        kneeDb      = kneeIn;
        tubeGrit01  = gritDriveDbIn / 24.0f;
        liveCfDb    = liveCfIn;
        liveGrDb    = liveGrIn;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace sylredhel::sketch;
        auto b = getLocalBounds().toFloat();

        g.setColour(kPlotBg);
        g.fillRoundedRectangle(b, 6.0f);
        drawSketchBox(g, b.reduced(0.6f), 6.0f, kGraphiteMid);

        g.setFont(juce::Font(juce::FontOptions(9.6f, juce::Font::bold)));
        g.setColour(kGraphite);
        g.drawText(engIdx == 0 ? "CREST RIDER CURVE" : "VARI-MU CREST CURVE",
                   static_cast<int>(b.getX() + 8), static_cast<int>(b.getY() + 6),
                   static_cast<int>(b.getWidth() - 16), 16, juce::Justification::centred);

        auto plot = b.reduced(16.0f, 14.0f);
        plot.removeFromTop(16.0f);
        plot.removeFromBottom(8.0f);

        auto mapX = [&](float cf) { return plot.getX() + juce::jlimit(0.0f, 1.0f, cf / 20.0f) * plot.getWidth(); };
        auto mapY = [&](float gr) { return plot.getY() + juce::jlimit(0.0f, 1.0f, gr / 20.0f) * plot.getHeight(); };

        g.setColour(kGridLine);
        for (float v = 6.0f; v <= 18.0f; v += 6.0f)
            g.drawVerticalLine(static_cast<int>(mapX(v)), plot.getY(), plot.getBottom());
        for (float v = 4.0f; v <= 12.0f; v += 4.0f)
            g.drawHorizontalLine(static_cast<int>(mapY(v)), plot.getX(), plot.getRight());

        g.setColour(kGraphiteLt.withAlpha(0.75f));
        g.drawHorizontalLine(static_cast<int>(plot.getY()), plot.getX(), plot.getRight());
        g.drawVerticalLine(static_cast<int>(plot.getX()), plot.getY(), plot.getBottom());

        // Dashed vertical CREST THRESH line
        const float dashes[2] = { 4.0f, 3.0f };
        g.setColour(kOchre.withAlpha(0.85f));
        g.drawDashedLine(juce::Line<float>(mapX(crestThresh), plot.getY(), mapX(crestThresh), plot.getBottom()), dashes, 2, 1.2f);

        // Evaluate exact VCA or Progressive Vari-Mu curve + optional Tube Grit warm halo
        juce::Path curve, gritHalo;
        constexpr int pts = 96;
        const float k = std::max(kneeDb, 0.25f);
        for (int i = 0; i <= pts; ++i)
        {
            const float cf   = (static_cast<float>(i) / static_cast<float>(pts)) * 20.0f;
            const float diff = cf - crestThresh;
            float over = 0.0f;
            if (diff >= 0.5f * k)       over = diff;
            else if (diff > -0.5f * k) { const float tk = diff + 0.5f * k; over = (tk * tk) / (2.0f * k); }

            float gr = 0.0f;
            if (engIdx == 0)
            {
                gr = std::min(20.0f, over * (1.0f - 1.0f / std::max(ratio, 1.0f)));
            }
            else
            {
                const float rEff = 1.0f + (std::max(ratio, 1.0f) - 1.0f) * (over / (over + 2.5f));
                gr = std::min(20.0f, over * (1.0f - 1.0f / std::max(rEff, 1.0f)));
            }

            const float px = mapX(cf);
            const float py = mapY(gr);
            if (i == 0) curve.startNewSubPath(px, py);
            else        curve.lineTo(px, py);

            // Show subtle Tube Grit saturation zone below curve when VARI-MU + Tube Grit > 0%
            const float gritExtra = (tubeGrit01 > 0.005f) ? std::min(20.0f - gr, (over / 8.0f) * tubeGrit01 * 6.5f) : 0.0f;
            const float pyG = mapY(gr + gritExtra);
            if (i == 0) gritHalo.startNewSubPath(px, pyG);
            else        gritHalo.lineTo(px, pyG);
        }

        if (tubeGrit01 > 0.01f)
        {
            g.setColour(kTubeAmber.withAlpha(0.55f));
            g.strokePath(gritHalo, juce::PathStrokeType(1.6f));
        }

        g.setColour(kKraftBrown);
        g.strokePath(curve, juce::PathStrokeType(2.2f));

        // Live moving terracotta dot
        if (liveCfDb > 0.2f || liveGrDb > 0.05f)
        {
            const float lx = mapX(liveCfDb);
            const float ly = mapY(liveGrDb);
            g.setColour(kTerracotta.withAlpha(0.28f));
            g.fillEllipse(lx - 7.0f, ly - 7.0f, 14.0f, 14.0f);
            g.setColour(kTerracotta);
            g.fillEllipse(lx - 3.5f, ly - 3.5f, 7.0f, 7.0f);
        }

        g.setFont(juce::Font(juce::FontOptions(8.2f, juce::Font::italic)));
        g.setColour(kGraphiteMid);
        g.drawText(tubeGrit01 > 0.01f ? "Crest (0..20 dB) -> Comp + Crest Grit"
                                      : "Crest Factor (0..20 dB) -> Clean VCA",
                   static_cast<int>(plot.getX()), static_cast<int>(plot.getBottom() + 2),
                   static_cast<int>(plot.getWidth()), 12, juce::Justification::centred);
    }

private:
    int   engIdx      = 0;
    float crestThresh = 11.5f;
    float ratio       = 4.5f;
    float kneeDb      = 4.0f;
    float tubeGrit01  = 0.50f;
    float liveCfDb    = 0.0f;
    float liveGrDb    = 0.0f;
};
