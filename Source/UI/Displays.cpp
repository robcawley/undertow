#include "Displays.h"
#include "Chassis.h"
#include "../PluginProcessor.h"

namespace ui
{
namespace
{
    double nowSeconds() { return juce::Time::getMillisecondCounterHiRes() * 0.001; }
    float eff (const UndertowProcessor& p, int k) { return p.effectiveValue (k); }
}

//==============================================================================
XYPad::XYPad (UndertowProcessor& p)
    : proc (p), px (*p.state.getParameter ("xy_x")), py (*p.state.getParameter ("xy_y")),
      ax (px, [this] (float v) { vx = px.convertTo0to1 (v); repaint(); }, nullptr),
      ay (py, [this] (float v) { vy = py.convertTo0to1 (v); repaint(); }, nullptr)
{
    ax.sendInitialUpdate();
    ay.sendInitialUpdate();
    for (auto& t : trail) t = { 0.5f, 0.5f };
    setTooltip ("Drag the puck. Each corner adds (or, flipped, subtracts) its own variation; the centre is the sound as set. "
                "Double-click to return to the centre.");
    setTitle ("XY pad");
}

XYPad::~XYPad()
{
    if (dragging || glide) { ax.endGesture(); ay.endGesture(); }
}

juce::Point<float> XYPad::toScreen (float x, float y) const
{
    const auto r = pad();
    return { r.getX() + x * r.getWidth(), r.getBottom() - y * r.getHeight() };
}

void XYPad::begin()
{
    paramsBefore = proc.normalizedValues();
    extraBefore = std::make_unique<ut::ExtraState> (proc.extraState());
    ax.beginGesture();
    ay.beginGesture();
}

void XYPad::set (juce::Point<float> pos)
{
    const auto r = pad();
    const float x = juce::jlimit (0.0f, 1.0f, (pos.x - r.getX()) / r.getWidth());
    const float y = juce::jlimit (0.0f, 1.0f, (r.getBottom() - pos.y) / r.getHeight());
    ax.setValueAsPartOfGesture (px.convertFrom0to1 (x));
    ay.setValueAsPartOfGesture (py.convertFrom0to1 (y));
}

void XYPad::finish()
{
    ax.endGesture();
    ay.endGesture();
    if (extraBefore != nullptr && (std::abs (paramsBefore[ut::xy_x] - px.getValue()) > 1.0e-6f
                                   || std::abs (paramsBefore[ut::xy_y] - py.getValue()) > 1.0e-6f))
        pushSnapshot (proc, "Move puck", paramsBefore, *extraBefore);
    extraBefore.reset();
}

void XYPad::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    if (glide) { glide = false; finish(); }
    dragging = true;
    begin();
    set (e.position);
    if (onUserMove) onUserMove();
}

void XYPad::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging) set (e.position);
}

void XYPad::mouseUp (const juce::MouseEvent&)
{
    if (! dragging) return;
    dragging = false;
    const bool away = std::abs (vx - 0.5f) > 1.0e-4f || std::abs (vy - 0.5f) > 1.0e-4f;
    if (proc.parameter (ut::xy_return).getValue() > 0.5f && away)
    {
        // Glide home over one bar, inside the same host gesture so Latch and
        // Touch automation record the return.
        glide = true;
        glideStart = nowSeconds();
        glideSeconds = juce::jlimit (0.25, 8.0, 60.0 / std::max (20.0, proc.lastTempo()) * proc.lastBarBeats());
        glideFromX = vx; glideFromY = vy;
        return;
    }
    finish();
}

void XYPad::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    if (glide) { glide = false; finish(); }
    begin();
    ax.setValueAsPartOfGesture (px.convertFrom0to1 (0.5f));
    ay.setValueAsPartOfGesture (py.convertFrom0to1 (0.5f));
    finish();
}

void XYPad::tick (const ut::ExtraState& extra, bool reduceMotion)
{
    bool changed = false;
    for (int c = 0; c < 4; ++c)
    {
        bool any = false;
        for (float v : extra.corners[(size_t) c]) any = any || v != 0.0f;
        const bool flip = eff (proc, ut::xy_flip_a + c) > 0.5f;
        if (any != hasVariation[(size_t) c] || flip != flipped[(size_t) c]) changed = true;
        hasVariation[(size_t) c] = any;
        flipped[(size_t) c] = flip;
    }
    // the recorded path, rebuilt only when it changes
    const int version = extra.motionCount * 7919 + (extra.motionCount > 0 ? juce::roundToInt (extra.motion[0].x * 1000 + extra.motion[(size_t) extra.motionCount - 1].y * 100000) : 0);
    if (version != pathVersion)
    {
        pathVersion = version;
        path.clear();
        for (int k = 0; k < extra.motionCount; ++k) path.push_back ({ extra.motion[(size_t) k].x, extra.motion[(size_t) k].y });
        changed = true;
    }
    const float nx = proc.effectiveX(), ny = proc.effectiveY();
    if (std::abs (nx - ex) > 1.0e-4f || std::abs (ny - ey) > 1.0e-4f)
    {
        ex = nx; ey = ny;
        const auto last = trail[(size_t) ((trailHead + (int) trail.size() - 1) % (int) trail.size())];
        if (! reduceMotion && last.getDistanceFrom ({ ex, ey }) > 0.004f)
        {
            trail[(size_t) trailHead] = { ex, ey };
            trailHead = (trailHead + 1) % (int) trail.size();
        }
        changed = true;
    }
    if (glide)
    {
        const double t = juce::jlimit (0.0, 1.0, (nowSeconds() - glideStart) / glideSeconds);
        const float s = (float) (t * t * (3.0 - 2.0 * t));
        ax.setValueAsPartOfGesture (px.convertFrom0to1 (glideFromX + (0.5f - glideFromX) * s));
        ay.setValueAsPartOfGesture (py.convertFrom0to1 (glideFromY + (0.5f - glideFromY) * s));
        if (t >= 1.0) { glide = false; finish(); }
        changed = true;
    }
    if (cornerLearning >= 0)
    {
        pulseGlow = reduceMotion ? 1.0f : 0.6f + 0.4f * std::sin ((float) nowSeconds() * 5.0f);
        changed = true;
    }
    else if (pulseGlow != 0.0f) { pulseGlow = 0.0f; changed = true; }
    if (changed) { noteRepaint ("xy pad"); repaint(); }
}

int XYPad::key() const
{
    int k = pathVersion * 31 + getWidth() * 7 + getHeight();
    for (int c = 0; c < 4; ++c) k = k * 5 + (hasVariation[(size_t) c] ? 1 : 0) + (flipped[(size_t) c] ? 2 : 0);
    return k + (eff (proc, ut::xy_motion_play) > 0.5f ? 1 : 0);
}

void XYPad::renderBackdrop (float sc)
{
    backdrop = juce::Image (juce::Image::ARGB, juce::roundToInt ((float) getWidth() * sc), juce::roundToInt ((float) getHeight() * sc), true);
    juce::Graphics g (backdrop);
    g.addTransform (juce::AffineTransform::scale (sc));
    const auto r = pad();
    drawDisplayWell (g, getLocalBounds().toFloat());
    for (int i = 1; i < 8; ++i)
    {
        g.setColour (juce::Colours::white.withAlpha (i == 4 ? 0.13f : 0.045f));
        const float fx = r.getX() + r.getWidth() * (float) i / 8.0f, fy = r.getY() + r.getHeight() * (float) i / 8.0f;
        g.drawVerticalLine (juce::roundToInt (fx), r.getY(), r.getBottom());
        g.drawHorizontalLine (juce::roundToInt (fy), r.getX(), r.getRight());
    }
    const juce::Point<float> corners[] { r.getTopLeft(), r.getTopRight(), r.getBottomRight(), r.getBottomLeft() };
    const char* names[] { "A", "B", "C", "D" };
    const auto half = juce::Rectangle<float> (r.getWidth() * 0.5f, r.getHeight() * 0.5f);
    for (int c = 0; c < 4; ++c)
    {
        const auto col = flipped[(size_t) c] ? C::amber : C::teal;
        // each corner only acts inside its own quadrant, so its glow lives there
        auto& img = glow[(size_t) c];
        img = juce::Image (juce::Image::ARGB, juce::roundToInt (half.getWidth() * sc), juce::roundToInt (half.getHeight() * sc), true);
        {
            juce::Graphics gg (img);
            gg.addTransform (juce::AffineTransform::scale (sc));
            const auto origin = juce::Point<float> (c == 1 || c == 2 ? half.getWidth() : 0.0f, c >= 2 ? half.getHeight() : 0.0f);
            gg.setGradientFill (juce::ColourGradient (col.withAlpha (0.55f), origin, col.withAlpha (0.0f),
                                                      origin + (half.getCentre() - origin) * 1.9f, true));
            gg.fillAll();
        }
        if (hasVariation[(size_t) c])
        {
            g.setGradientFill (juce::ColourGradient (col.withAlpha (0.10f), corners[c], col.withAlpha (0.0f),
                                                     corners[c] + (r.getCentre() - corners[c]) * 0.8f, true));
            g.fillRect (r);
        }
        auto label = juce::Rectangle<float> (40.0f, 22.0f);
        label.setCentre (corners[c] + (r.getCentre() - corners[c]) * 0.09f);
        const auto f = Fonts::get().heavy (20.0f);
        if (hasVariation[(size_t) c])
            neonText (g, juce::String (names[c]) + (flipped[(size_t) c] ? "-" : ""), label, f, col, juce::Justification::centred, 0.6f);
        else
        {
            g.setFont (f);
            g.setColour (C::boneDim.withAlpha (0.55f));
            g.drawText (names[c], label, juce::Justification::centred, false);
        }
    }
    if (path.size() > 1)
    {
        juce::Path p;
        p.startNewSubPath (toScreen (path[0].x, path[0].y));
        for (size_t k = 1; k < path.size(); ++k) p.lineTo (toScreen (path[k].x, path[k].y));
        g.setColour (C::amber.withAlpha (eff (proc, ut::xy_motion_play) > 0.5f ? 0.55f : 0.25f));
        g.strokePath (p, juce::PathStrokeType (1.2f));
    }
}

void XYPad::paint (juce::Graphics& g)
{
    const float sc = g.getInternalContext().getPhysicalPixelScaleFactor();
    const int k = key();
    if (backdrop.isNull() || k != backdropKey || backdrop.getWidth() != juce::roundToInt ((float) getWidth() * sc))
    {
        renderBackdrop (sc);
        backdropKey = k;
    }
    g.drawImage (backdrop, getLocalBounds().toFloat());
    const auto r = pad();
    // the active corner's glow, weighted by where the sound actually is
    const auto w = ut::Modulation::weights (ex, ey);
    for (int c = 0; c < 4; ++c)
        if (hasVariation[(size_t) c] && w[(size_t) c] > 0.01f)
        {
            const auto quad = juce::Rectangle<float> (r.getWidth() * 0.5f, r.getHeight() * 0.5f)
                                  .withPosition (c == 1 || c == 2 ? r.getCentreX() : r.getX(), c >= 2 ? r.getCentreY() : r.getY());
            g.setOpacity (juce::jlimit (0.0f, 1.0f, std::sqrt (w[(size_t) c])));
            g.drawImage (glow[(size_t) c], quad);
            g.setOpacity (1.0f);
        }
    if (cornerLearning >= 0)
    {
        const char* names[] { "A", "B", "C", "D" };
        const juce::Point<float> corners[] { r.getTopLeft(), r.getTopRight(), r.getBottomRight(), r.getBottomLeft() };
        g.setGradientFill (juce::ColourGradient (C::magenta.withAlpha (0.35f * pulseGlow), corners[cornerLearning], C::magenta.withAlpha (0.0f),
                                                 corners[cornerLearning] + (r.getCentre() - corners[cornerLearning]) * 0.95f, true));
        g.fillRect (r);
        g.setColour (C::magenta.withAlpha (0.4f + 0.5f * pulseGlow));
        g.drawRect (r, 1.5f);
        const auto banner = r.reduced (36.0f, 0).withHeight (34.0f).translated (0, 6.0f);
        neonText (g, "LEARNING " + juce::String (names[cornerLearning]), banner.withHeight (16.0f),
                  Fonts::get().heavy (15.0f).withExtraKerningFactor (0.08f), C::magenta, juce::Justification::centred, 0.5f);
        g.setFont (Fonts::get().mono (10.5f));
        g.setColour (C::magenta.withAlpha (0.85f));
        g.drawText ("move knobs, then DONE", banner.withTrimmedTop (18.0f), juce::Justification::centred, false);
    }
    for (int i = 0; i < (int) trail.size(); ++i)
    {
        const int idx = (trailHead + i) % (int) trail.size();
        const float age = (float) (i + 1) / (float) trail.size();
        g.setColour (C::sea.withAlpha (0.35f * age));
        g.fillEllipse (juce::Rectangle<float> (3.0f + 3.0f * age, 3.0f + 3.0f * age).withCentre (toScreen (trail[(size_t) idx].x, trail[(size_t) idx].y)));
    }
    const auto puck = toScreen (vx, vy), where = toScreen (ex, ey);
    if (puck.getDistanceFrom (where) > 2.0f)
    {
        g.setColour (C::teal.withAlpha (0.35f));
        g.drawLine ({ puck, where }, 1.0f);
    }
    g.setColour (C::teal.withAlpha (0.25f));
    g.fillEllipse (juce::Rectangle<float> (22.0f, 22.0f).withCentre (where));
    g.setColour (C::teal);
    g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (where));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (juce::Rectangle<float> (21.0f, 21.0f).withCentre (puck), 4.0f);
    g.setColour (dragging || glide ? C::bone : C::bone.withAlpha (0.8f));
    g.drawEllipse (juce::Rectangle<float> (20.0f, 20.0f).withCentre (puck), 1.8f);
}

//==============================================================================
Spectrum::Spectrum (UndertowProcessor& p) : proc (p)
{
    level.fill (-90.0f);
    shown.fill (-90.0f);
    bandLevel.fill (1.0f);
    setTooltip ("The space after the effects. Coloured bands are the PULSE bands, brightening as they throb. "
                "Click a band to edit it; drag an orange line to move a crossover.");
}

float Spectrum::xFor (float hz) const
{
    const auto r = getLocalBounds().toFloat().reduced (6.0f, 4.0f);
    return r.getX() + r.getWidth() * std::log (juce::jlimit (20.0f, 20000.0f, hz) / 20.0f) / std::log (1000.0f);
}

float Spectrum::hzFor (float x) const
{
    const auto r = getLocalBounds().toFloat().reduced (6.0f, 4.0f);
    return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (x - r.getX()) / r.getWidth()));
}

std::array<float, 5> Spectrum::crossovers() const
{
    std::array<float, 5> x {};
    for (int i = 0; i < 5; ++i) x[(size_t) i] = proc.engine.pulse.crossoverTargets[(size_t) i].load (std::memory_order_relaxed);
    // Before audio has run, fall back to the parameters.
    if (x[0] <= 0.0f)
        for (int i = 0; i < 5; ++i) x[(size_t) i] = eff (proc, ut::pulse_xover1 + i);
    return x;
}

int Spectrum::lineAt (float x) const
{
    const int bands = juce::roundToInt (eff (proc, ut::pulse_bands));
    const auto xs = crossovers();
    int best = -1; float dist = 7.0f;
    for (int i = 0; i < bands - 1; ++i)
        if (const float d = std::abs (xFor (xs[(size_t) i]) - x); d < dist) { dist = d; best = i; }
    return best;
}

int Spectrum::bandAt (float x) const
{
    const int bands = juce::roundToInt (eff (proc, ut::pulse_bands));
    const auto xs = crossovers();
    for (int i = 0; i < bands - 1; ++i)
        if (x < xFor (xs[(size_t) i])) return i;
    return bands - 1;
}

void Spectrum::tick (bool reduceMotion)
{
    const uint32_t w = proc.engine.scopeWrite.load (std::memory_order_acquire);
    bool changed = false;
    if (w != lastWrite)
    {
        lastWrite = w;
        for (int i = 0; i < size; ++i)
            work[(size_t) i] = proc.engine.scope[(size_t) ((w - (uint32_t) size + (uint32_t) i) & (uint32_t) (ut::Engine::scopeSize - 1))].load (std::memory_order_relaxed);
        std::fill (work.begin() + size, work.end(), 0.0f);
        window.multiplyWithWindowingTable (work.data(), (size_t) size);
        fft.performFrequencyOnlyForwardTransform (work.data());
        const double sr = proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000.0;
        for (int b = 0; b < bins; ++b)
        {
            const double f0 = 20.0 * std::pow (1000.0, (double) b / bins), f1 = 20.0 * std::pow (1000.0, (double) (b + 1) / bins);
            const int i0 = juce::jlimit (1, size / 2 - 1, (int) (f0 / sr * size)), i1 = juce::jlimit (i0, size / 2 - 1, (int) (f1 / sr * size));
            float m = 0.0f;
            for (int i = i0; i <= i1; ++i) m = std::max (m, work[(size_t) i]);
            level[(size_t) b] = juce::jlimit (-90.0f, 6.0f, juce::Decibels::gainToDecibels (m / (float) (size / 4), -90.0f));
        }
    }
    else
        for (auto& l : level) l = std::max (-90.0f, l - 3.0f);       // transport stopped: let it fall away
    for (int b = 0; b < bins; ++b)
    {
        auto& s = shown[(size_t) b];
        const float target = level[(size_t) b];
        const float next = target > s ? (reduceMotion ? target : s + 0.6f * (target - s)) : std::max (target, s - 1.5f);
        if (std::abs (next - s) > 0.2f) changed = true;
        s = next;
    }
    for (int i = 0; i < 6; ++i)
    {
        const float l = proc.engine.pulse.levels[(size_t) i].load (std::memory_order_relaxed);
        if (std::abs (l - bandLevel[(size_t) i]) > 0.01f) { bandLevel[(size_t) i] = l; changed = true; }
    }
    // The spectrum is analysed every frame but drawn at 15 fps: plenty for a
    // slowly breathing display, and half the drawing cost.
    pending = pending || changed;
    if (pending && (++frameCount & 1) == 0) { pending = false; noteRepaint ("spectrum"); repaint(); }
}

void Spectrum::paint (juce::Graphics& g)
{
    const auto all = getLocalBounds().toFloat();
    const auto r = all.reduced (6.0f, 4.0f);
    const float sc = g.getInternalContext().getPhysicalPixelScaleFactor();
    const bool anchorOn = eff (proc, ut::anchor_on) > 0.5f;
    const float anchorHz = eff (proc, ut::anchor_freq);
    const int k = (anchorOn ? 1 : 0) + juce::roundToInt (anchorHz) * 2 + getWidth() * 4096;
    if (backdrop.isNull() || k != backdropKey || backdrop.getWidth() != juce::roundToInt ((float) getWidth() * sc))
    {
        backdropKey = k;
        backdrop = juce::Image (juce::Image::ARGB, juce::roundToInt (all.getWidth() * sc), juce::roundToInt (all.getHeight() * sc), true);
        juce::Graphics bg (backdrop);
        bg.addTransform (juce::AffineTransform::scale (sc));
        drawDisplayWell (bg, all);
        if (anchorOn)
        {
            const float ax = xFor (anchorHz);
            bg.setColour (juce::Colours::black.withAlpha (0.35f));
            bg.fillRect (juce::Rectangle<float> (r.getX(), r.getY(), ax - r.getX(), r.getHeight()));
            bg.setColour (C::boneDim.withAlpha (0.45f));
            for (float y = r.getY(); y < r.getBottom(); y += 6.0f) bg.drawVerticalLine (juce::roundToInt (ax), y, y + 3.0f);
            bg.setFont (Fonts::get().mono (9.5f));
            bg.drawText ("ANCHOR", juce::Rectangle<float> (60, 12).withPosition (r.getX() + 2.0f, r.getBottom() - 13.0f), juce::Justification::left, false);
        }
        bg.setFont (Fonts::get().mono (9.5f));
        for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
        {
            const float x = xFor (f);
            bg.setColour (juce::Colours::white.withAlpha (0.06f));
            bg.drawVerticalLine (juce::roundToInt (x), r.getY(), r.getBottom());
            bg.setColour (C::boneDim.withAlpha (0.6f));
            bg.drawText (f >= 1000.0f ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f),
                         juce::Rectangle<float> (30, 11).withCentre ({ x, r.getBottom() - 6.0f }), juce::Justification::centred, false);
        }
    }
    g.drawImage (backdrop, all);

    const bool on = eff (proc, ut::pulse_on) > 0.5f;
    const int bands = juce::roundToInt (eff (proc, ut::pulse_bands));
    const auto xs = crossovers();
    auto yFor = [&] (float db) { return r.getBottom() - r.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 84.0f) / 84.0f); };
    if (on)
        for (int b = 0; b < bands; ++b)
        {
            const float x0 = b == 0 ? r.getX() : xFor (xs[(size_t) b - 1]), x1 = b == bands - 1 ? r.getRight() : xFor (xs[(size_t) b]);
            const auto area = juce::Rectangle<float> (x0, r.getY(), x1 - x0, r.getHeight());
            const float throb = 1.0f - bandLevel[(size_t) b];
            g.setColour (C::teal.withAlpha (0.03f + 0.30f * throb));
            g.fillRect (area);
            if (b == selected)
            {
                g.setColour (C::amber.withAlpha (0.8f));
                g.drawRect (area.reduced (1.0f), 1.2f);
            }
            g.setFont (Fonts::get().mono (10.5f));
            g.setColour (b == selected ? C::amber : C::boneDim);
            g.drawText (juce::String (b + 1), area.withHeight (14.0f).translated (0, 2.0f), juce::Justification::centred, false);
        }
    juce::Path line, fill;
    for (int b = 0; b < bins; ++b)
    {
        const float x = r.getX() + r.getWidth() * ((float) b + 0.5f) / (float) bins, y = yFor (shown[(size_t) b]);
        if (b == 0) { line.startNewSubPath (x, y); fill.startNewSubPath (x, r.getBottom()); }
        else line.lineTo (x, y);
        fill.lineTo (x, y);
    }
    fill.lineTo (r.getRight(), r.getBottom());
    fill.closeSubPath();
    g.setColour (C::sea.withAlpha (0.20f));
    g.fillPath (fill);
    g.setColour (C::teal.withAlpha (on ? 0.95f : 0.6f));
    g.strokePath (line, juce::PathStrokeType (1.3f));
    if (on)
        for (int i = 0; i < bands - 1; ++i)
        {
            const float x = xFor (xs[(size_t) i]);
            const bool hot = i == hoverLine || i == dragLine;
            g.setColour (C::amber.withAlpha (hot ? 0.95f : 0.55f));
            g.fillRect (juce::Rectangle<float> (hot ? 2.0f : 1.2f, r.getHeight() - 14.0f).withPosition (x - (hot ? 1.0f : 0.6f), r.getY() + 14.0f));
            g.fillRoundedRectangle (juce::Rectangle<float> (8.0f, 12.0f).withCentre ({ x, r.getY() + 20.0f }), 2.0f);
            if (hot)
            {
                const float hz = xs[(size_t) i];
                g.setFont (Fonts::get().mono (11.0f));
                g.setColour (C::amber);
                g.drawText (hz >= 1000 ? juce::String (hz / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz",
                            juce::Rectangle<float> (80, 14).withCentre ({ juce::jlimit (r.getX() + 40, r.getRight() - 40, x), r.getY() + 36.0f }),
                            juce::Justification::centred, false);
            }
        }
    else
    {
        g.setFont (Fonts::get().heavy (15.0f));
        g.setColour (C::boneDim);
        g.drawText ("PULSE OFF", r.withHeight (22.0f).translated (0, 6.0f), juce::Justification::centred, false);
    }
}

void Spectrum::mouseMove (const juce::MouseEvent& e)
{
    const int l = eff (proc, ut::pulse_on) > 0.5f ? lineAt (e.position.x) : -1;
    if (l != hoverLine) { hoverLine = l; repaint(); }
    setMouseCursor (l >= 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::PointingHandCursor);
}

void Spectrum::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    dragLine = eff (proc, ut::pulse_on) > 0.5f ? lineAt (e.position.x) : -1;
    if (dragLine >= 0)
    {
        auto& p = proc.parameter (ut::pulse_xover1 + dragLine);
        dragStart = p.getValue();
        p.beginChangeGesture();
        return;
    }
    selected = bandAt (e.position.x);
    if (onSelect) onSelect (selected);
    repaint();
}

void Spectrum::mouseDrag (const juce::MouseEvent& e)
{
    if (dragLine < 0) return;
    auto& p = proc.parameter (ut::pulse_xover1 + dragLine);
    p.setValueNotifyingHost (p.convertTo0to1 (juce::jlimit (20.0f, 16000.0f, hzFor (e.position.x))));
    repaint();
}

void Spectrum::mouseUp (const juce::MouseEvent&)
{
    if (dragLine < 0) return;
    auto& p = proc.parameter (ut::pulse_xover1 + dragLine);
    p.endChangeGesture();
    pushUndo (proc, p, dragStart, p.getValue());
    dragLine = -1;
    repaint();
}

//==============================================================================
juce::Rectangle<float> StepGrid::cell (int i) const
{
    const auto r = getLocalBounds().toFloat();
    const float w = r.getWidth() / 8.0f, h = r.getHeight() / 2.0f;
    return { r.getX() + w * (float) (i % 8), r.getY() + h * (float) (i / 8), w, h };
}

void StepGrid::tick (const ut::ExtraState& extra)
{
    const uint16_t pat = extra.patterns[(size_t) band];
    const bool act = eff (proc, ut::pulse_on) > 0.5f && eff (proc, ut::pulse_b1_source + 7 * band) > 0.5f;
    const int ph = act ? proc.engine.pulse.stepIndex[(size_t) band].load (std::memory_order_relaxed) : -1;
    if (pat != pattern || act != active || ph != playhead) { pattern = pat; active = act; playhead = ph; noteRepaint ("step grid"); repaint(); }
}

void StepGrid::paint (juce::Graphics& g)
{
    for (int i = 0; i < 16; ++i)
    {
        const auto c = cell (i).reduced (1.5f);
        const bool on = (pattern >> i) & 1;
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (c, 2.0f);
        if (on)
        {
            g.setColour ((active ? C::teal : C::tealDim).withAlpha (active ? 0.75f : 0.6f));
            g.fillRoundedRectangle (c.reduced (2.0f), 1.5f);
        }
        g.setColour (i == playhead ? C::amber : juce::Colours::white.withAlpha (i % 4 == 0 ? 0.16f : 0.07f));
        g.drawRoundedRectangle (c, 2.0f, i == playhead ? 1.6f : 1.0f);
    }
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < 16; ++i)
        if (cell (i).contains (e.position))
        {
            const auto before = proc.extraState();
            auto next = before;
            next.patterns[(size_t) band] ^= (uint16_t) (1u << i);
            next.edited = true;
            proc.setExtraState (next);
            pushSnapshot (proc, "Step", proc.normalizedValues(), before);
            pattern = next.patterns[(size_t) band];
            repaint();
            return;
        }
}

//==============================================================================
void TailView::tick()
{
    const float d = eff (proc, ut::void_decay), gr = eff (proc, ut::void_gravity), pd = eff (proc, ut::void_predelay),
                sz = eff (proc, ut::void_size);
    const bool fr = eff (proc, ut::void_freeze) > 0.5f, o = eff (proc, ut::void_on) > 0.5f;
    if (std::abs (d - decay) > 1.0e-3f || std::abs (gr - gravity) > 0.05f || std::abs (pd - predelay) > 0.1f
        || std::abs (sz - size) > 0.05f || fr != frozen || o != on)
    {
        decay = d; gravity = gr; predelay = pd; size = sz; frozen = fr; on = o;
        noteRepaint ("tail");
        repaint();
    }
}

void TailView::paint (juce::Graphics& g)
{
    const auto all = getLocalBounds().toFloat();
    drawDisplayWell (g, all);
    const auto r = all.reduced (8.0f, 8.0f).withTrimmedBottom (10.0f);
    // time axis: long enough to show the whole decay, on a gently compressed scale
    const float span = std::max (1.0f, predelay * 0.001f + decay * 1.15f);
    auto xFor = [&] (float t) { return r.getX() + r.getWidth() * std::sqrt (juce::jlimit (0.0f, 1.0f, t / span)); };
    auto yFor = [&] (float db) { return r.getY() + r.getHeight() * juce::jlimit (0.0f, 1.0f, -db / 66.0f); };
    juce::Path p, f;
    const float pre = predelay * 0.001f, g01 = gravity / 100.0f;
    p.startNewSubPath (xFor (0), yFor (-66));
    p.lineTo (xFor (pre), yFor (-66));
    for (int i = 0; i <= 120; ++i)
    {
        const float t = pre + (span - pre) * (float) i / 120.0f, u = t - pre;
        float db = frozen ? 0.0f : -60.0f * u / std::max (0.1f, decay);
        if (! frozen)
        {
            if (g01 > 0) db -= g01 * 18.0f * (1.0f - std::exp (-u / (0.12f * decay + 0.05f)));
            if (g01 < 0) db += g01 * 28.0f * std::exp (-u / (0.3f * decay + 0.05f));
        }
        p.lineTo (xFor (t), yFor (std::max (-66.0f, db)));
    }
    f = p;
    f.lineTo (r.getRight(), r.getBottom());
    f.lineTo (r.getX(), r.getBottom());
    f.closeSubPath();
    const auto col = frozen ? C::magenta : C::teal;
    g.setGradientFill (juce::ColourGradient (col.withAlpha (on ? 0.30f : 0.08f), 0, r.getY(), col.withAlpha (0.0f), 0, r.getBottom(), false));
    g.fillPath (f);
    neonStroke (g, p, col.withAlpha (on ? 1.0f : 0.35f), 1.4f, on ? 0.6f : 0.0f);
    g.setFont (Fonts::get().mono (10.5f));
    g.setColour (C::boneDim);
    g.drawText ("TAIL", all.reduced (8.0f, 3.0f), juce::Justification::topLeft, false);
    juce::String info = ! on ? juce::String ("VOID OFF")
                     : frozen ? juce::String ("FROZEN")
                              : juce::String (decay, decay < 10 ? 1 : 0) + " s" + (gravity < -1 ? "  SWELL" : gravity > 1 ? "  DROP" : "");
    g.setColour (frozen ? C::magenta : C::teal);
    g.drawText (info, all.reduced (8.0f, 3.0f), juce::Justification::bottomRight, false);
}

//==============================================================================
void MutateView::tick (bool reduceMotion)
{
    if (! isVisible()) return;
    auto& e = proc.engine;
    const float g = e.mutateGate.load (std::memory_order_relaxed);
    const bool o = g > 0.001f;
    const bool fo = eff (proc, ut::evolve_follow) > 0.5f;
    const int cy = juce::roundToInt (eff (proc, ut::evolve_cycle));
    const float fa = eff (proc, ut::evolve_amount) * eff (proc, ut::evolve_formant) / 1.0e4f;
    bool changed = o != on || fo != follow || cy != cycle || std::abs (fa - formantAmt) > 0.01f || std::abs (g - gate) > 0.01f;
    on = o; follow = fo; cycle = cy; formantAmt = fa; gate = g;
    if (on)
    {
        const float nx = e.evolveX.load (std::memory_order_relaxed), ny = e.evolveY.load (std::memory_order_relaxed);
        if (std::abs (nx - ex) > 1.0e-3f || std::abs (ny - ey) > 1.0e-3f) changed = true;
        ex = nx; ey = ny;
        for (int f = 0; f < 3; ++f) hz[(size_t) f] = e.formantHz[(size_t) f].load (std::memory_order_relaxed);
        // history moves at a quarter of the frame rate; held still with reduced motion
        if (! reduceMotion && (++frames & 3) == 0)
        {
            hist[(size_t) head] = hz;
            head = (head + 1) % rows;
            filled = std::min (rows, filled + 1);
            trail[(size_t) trailHead] = { ex, ey };
            trailHead = (trailHead + 1) % (int) trail.size();
            trailFilled = std::min ((int) trail.size(), trailFilled + 1);
            changed = true;
        }
    }
    if (changed) { noteRepaint ("mutate"); repaint(); }
}

void MutateView::paint (juce::Graphics& g)
{
    const auto all = getLocalBounds().toFloat();
    drawDisplayWell (g, all);
    auto inner = all.reduced (8.0f, 16.0f);
    auto left = inner.removeFromLeft (inner.getWidth() * 0.66f).withTrimmedRight (10.0f);
    auto right = inner.withSizeKeepingCentre (std::min (inner.getWidth(), inner.getHeight()), std::min (inner.getWidth(), inner.getHeight()));
    const auto col = on ? C::magenta : C::boneDim;

    // formant waterfall on a log axis, 200 Hz to 3.2 kHz
    auto xFor = [&] (float f) { return left.getX() + left.getWidth() * juce::jlimit (0.0f, 1.0f, std::log2 (std::max (1.0f, f) / 200.0f) / 4.0f); };
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    for (float f : { 400.0f, 800.0f, 1600.0f })
        g.drawVerticalLine (juce::roundToInt (xFor (f)), left.getY(), left.getBottom());
    if (on && formantAmt > 0.0f)
    {
        const float rowH = left.getHeight() / (float) rows;
        for (int i = 0; i < filled; ++i)
        {
            const int idx = (head - filled + i + rows) % rows;
            const float age = (float) (filled - 1 - i) / (float) rows;           // 0 newest
            const float y = left.getBottom() - rowH * (float) (filled - i);
            for (int f = 0; f < 3; ++f)
            {
                const float x = xFor (hist[(size_t) idx][(size_t) f]);
                g.setColour ((f == 1 ? C::magenta : C::teal).withAlpha ((0.9f - 0.65f * age) * juce::jlimit (0.45f, 1.0f, formantAmt * 2.0f)));
                g.fillRect (juce::Rectangle<float> (4.0f, rowH + 0.5f).withCentre ({ x, y + rowH * 0.5f }));
            }
        }
        for (int f = 0; f < 3; ++f)
        {
            juce::Path line;
            const float x = xFor (hz[(size_t) f]);
            line.startNewSubPath (x, left.getBottom() - 10.0f);
            line.lineTo (x, left.getBottom());
            neonStroke (g, line, f == 1 ? C::magenta : C::teal, 2.0f, 0.8f);
        }
    }

    // EVOLVE wander: size across, tone up
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRect (right, 1.0f);
    g.drawVerticalLine (juce::roundToInt (right.getCentreX()), right.getY(), right.getBottom());
    g.drawHorizontalLine (juce::roundToInt (right.getCentreY()), right.getX(), right.getRight());
    auto pt = [&] (juce::Point<float> v)
    {
        return juce::Point<float> (right.getCentreX() + 0.5f * right.getWidth() * juce::jlimit (-1.0f, 1.0f, v.x) * 0.92f,
                                   right.getCentreY() - 0.5f * right.getHeight() * juce::jlimit (-1.0f, 1.0f, v.y) * 0.92f);
    };
    if (on)
    {
        if (trailFilled > 1)
        {
            juce::Path p;
            for (int i = 0; i < trailFilled; ++i)
            {
                const auto q = pt (trail[(size_t) ((trailHead - trailFilled + i + (int) trail.size()) % (int) trail.size())]);
                if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
            }
            neonStroke (g, p, C::magenta.withAlpha (0.55f), 1.1f, 0.3f);
        }
        const auto c = pt ({ ex, ey });
        g.setColour (C::magenta.withAlpha (0.3f * gate));
        g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre (c));
        g.setColour (juce::Colours::white.withAlpha (gate));
        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (c));
    }

    g.setFont (Fonts::get().mono (10.5f));
    g.setColour (C::boneDim);
    g.drawText ("FORMANTS", all.reduced (8.0f, 3.0f), juce::Justification::topLeft, false);
    g.drawText ("SIZE / TONE", all.reduced (8.0f, 3.0f), juce::Justification::topRight, false);
    static const char* cycles[] { "1 BAR", "2 BARS", "4 BARS", "8 BARS", "16 BARS" };
    const juce::String info = ! on ? juce::String ("MUTATE OFF")
                            : follow ? juce::String ("FOLLOWING THE PAD")
                                     : juce::String ("EVOLVING OVER ") + cycles[juce::jlimit (0, 4, cycle)];
    g.setColour (col);
    g.drawText (info, all.reduced (8.0f, 3.0f), juce::Justification::bottomRight, false);
}

//==============================================================================
void Meters::tick()
{
    auto decayTo = [] (float shown, float now) { const float v = now > shown ? now : shown * 0.86f; return v < 1.0e-4f ? 0.0f : v; };
    const float i = decayTo (in, proc.engine.meterIn.load (std::memory_order_relaxed));
    const float o = decayTo (out, proc.engine.meterOut.load (std::memory_order_relaxed));
    const float env = proc.engine.duckMeter.load (std::memory_order_relaxed);
    const float d = juce::Decibels::gainToDecibels (1.0f + env * eff (proc, ut::duck_amount) * 0.16f);
    auto hold = [] (float v, float& held, int& count)
    {
        const float before = held;
        if (v > held) { held = v; count = 30; }
        else if (count > 0) --count;
        else held = held < 1.0e-4f ? 0.0f : held * 0.9f;
        return std::abs (held - before) > 0.002f;
    };
    const bool holds = hold (i, inHold, holdIn) | hold (o, outHold, holdOut);
    if (std::abs (i - in) > 0.002f || std::abs (o - out) > 0.002f || std::abs (d - duck) > 0.05f || holds)
    {
        in = i; out = o; duck = d;
        noteRepaint ("meters");
        repaint();
    }
}

void Meters::paint (juce::Graphics& g)
{
    const auto all = getLocalBounds().toFloat();
    const float barW = (all.getWidth() - 12.0f) / 3.0f;
    auto yFor = [&] (float gain, juce::Rectangle<float> r)
    {
        const float db = juce::Decibels::gainToDecibels (gain, -60.0f);
        return r.getBottom() - r.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
    };
    const char* names[] { "IN", "OUT", "DUCK" };
    for (int k = 0; k < 3; ++k)
    {
        auto col = all.withWidth (barW).translated ((barW + 6.0f) * (float) k, 0);
        auto bar = col.withTrimmedBottom (13.0f);
        g.setColour (C::display);
        g.fillRoundedRectangle (bar, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawRoundedRectangle (bar, 2.0f, 1.0f);
        const auto inner = bar.reduced (2.0f);
        if (k < 2)
        {
            const float v = k == 0 ? in : out, hold = k == 0 ? inHold : outHold;
            const float y = yFor (v, inner);
            g.setGradientFill (juce::ColourGradient (C::magenta, 0, inner.getY(), C::teal, 0, inner.getY() + inner.getHeight() * 0.25f, false));
            g.fillRect (inner.withTop (y));
            g.setColour (hold >= 0.99f ? C::magenta : C::bone.withAlpha (0.7f));
            g.fillRect (juce::Rectangle<float> (inner.getWidth(), 1.5f).withPosition (inner.getX(), yFor (hold, inner)));
        }
        else
        {
            const float h = inner.getHeight() * juce::jlimit (0.0f, 1.0f, duck / 18.0f);
            g.setColour (C::amber.withAlpha (0.85f));
            g.fillRect (inner.withHeight (h));
        }
        g.setFont (Fonts::get().mono (9.5f));
        g.setColour (C::boneDim);
        g.drawText (names[k], col.withTop (col.getBottom() - 12.0f), juce::Justification::centred, false);
    }
}
} // namespace ui
