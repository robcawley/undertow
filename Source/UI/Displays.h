#pragma once
#include "Controls.h"
#include <juce_dsp/juce_dsp.h>

namespace ui
{
// The XY pad: four corner variations around a centre-neutral puck.
// A is upper left, then clockwise. The puck is xy_x / xy_y; the moving dot is
// where Current, motion playback and macros actually put it.
class XYPad : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit XYPad (UndertowProcessor& p);
    ~XYPad() override;
    void tick (const ut::ExtraState& extra, bool reduceMotion);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    bool gliding() const { return glide; }
    std::function<void()> onUserMove;        // a drag started (stops a recording count-in etc.)

private:
    juce::Rectangle<float> pad() const { return getLocalBounds().toFloat().reduced (6.0f); }
    juce::Point<float> toScreen (float x, float y) const;
    void set (juce::Point<float> pos);
    void begin();
    void finish();
    UndertowProcessor& proc;
    juce::RangedAudioParameter &px, &py;
    juce::ParameterAttachment ax, ay;
    float vx = 0.5f, vy = 0.5f, ex = 0.5f, ey = 0.5f;
    std::array<juce::Point<float>, 32> trail {};
    int trailHead = 0;
    std::array<bool, 4> hasVariation {}, flipped {};
    std::vector<juce::Point<float>> path;
    int pathVersion = -1;
    bool dragging = false, glide = false;
    double glideStart = 0, glideSeconds = 1;
    float glideFromX = 0.5f, glideFromY = 0.5f, pulseGlow = 0;
    std::array<float, ut::count> paramsBefore {};
    std::unique_ptr<ut::ExtraState> extraBefore;
    // static layers, rebuilt only when the corners, the path or the scale change
    void renderBackdrop (float scale);
    juce::Image backdrop;
    std::array<juce::Image, 4> glow;
    int backdropKey = -1;
    int key() const;
};

// Live spectrum of the wet signal with the PULSE bands, their throb, and
// draggable crossover lines.
class Spectrum : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit Spectrum (UndertowProcessor& p);
    void tick (bool reduceMotion);
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hoverLine = -1; repaint(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    int selected = 1;                      // 0-based band shown in the band editor
    std::function<void (int)> onSelect;

private:
    static constexpr int order = 11, size = 1 << order, bins = 128;
    float xFor (float hz) const;
    float hzFor (float x) const;
    int lineAt (float x) const;
    int bandAt (float x) const;
    std::array<float, 5> crossovers() const;
    UndertowProcessor& proc;
    juce::dsp::FFT fft { order };
    juce::dsp::WindowingFunction<float> window { (size_t) size, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, size * 2> work {};
    std::array<float, bins> level {}, shown {};
    std::array<float, 6> bandLevel {};
    uint32_t lastWrite = 0;
    int hoverLine = -1, dragLine = -1;
    float dragStart = 0;
    std::array<float, ut::count> paramsBefore {};
    std::unique_ptr<ut::ExtraState> extraBefore;
    juce::Image backdrop;
    int backdropKey = -1, frameCount = 0;
    bool pending = false;
};

// Sixteen steps for one band; click to toggle.
class StepGrid : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit StepGrid (UndertowProcessor& p) : proc (p) { setTooltip ("Sixteen steps for this band. Used when the band's source is STEPS. Click to switch a step on or off."); }
    void tick (const ut::ExtraState& extra);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    int band = 1;
private:
    juce::Rectangle<float> cell (int i) const;
    UndertowProcessor& proc;
    uint16_t pattern = 0xffff;
    int playhead = -1;
    bool active = false;
};

// Shape of the reverb tail from pre-delay, decay, gravity and freeze.
class TailView : public juce::Component
{
public:
    explicit TailView (UndertowProcessor& p) : proc (p) { setOpaque (true); }
    void tick();
    void paint (juce::Graphics&) override;
private:
    UndertowProcessor& proc;
    float decay = -1, gravity = 0, predelay = 0, size = 0;
    bool frozen = false, on = true;
};

// MUTATE: the formants sliding through the tail (newest at the bottom) and the
// EVOLVE wander, size across and tone up.
class MutateView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit MutateView (UndertowProcessor& p) : proc (p)
    {
        setOpaque (true);
        setTooltip ("Left: the vowel-like formants sliding through the tail. Right: where EVOLVE has the space now, size across and tone up.");
    }
    void tick (bool reduceMotion);
    void paint (juce::Graphics&) override;
private:
    UndertowProcessor& proc;
    static constexpr int rows = 40;
    std::array<std::array<float, 3>, rows> hist {};
    std::array<juce::Point<float>, 32> trail {};
    int head = 0, filled = 0, trailHead = 0, trailFilled = 0, frames = 0;
    float gate = -1, ex = 0, ey = 0, formantAmt = 0;
    std::array<float, 3> hz {};
    bool follow = false, on = false;
    int cycle = -1;
};

// IN / OUT peak bars and the duck amount.
class Meters : public juce::Component
{
public:
    explicit Meters (UndertowProcessor& p) : proc (p) {}
    void tick();
    void paint (juce::Graphics&) override;
private:
    UndertowProcessor& proc;
    float in = 0, out = 0, duck = 0, inHold = 0, outHold = 0;
    int holdIn = 0, holdOut = 0;
};
} // namespace ui
