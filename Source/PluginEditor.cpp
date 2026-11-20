#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace ui
{
namespace
{
    // Materials are generated once per session and shared by every editor instance.
    const juce::Image& texture (int which)
    {
        static const juce::Image chassis = makeMaterial (Material::Chassis, 1280, 800, 29u);
        static const juce::Image plate   = makeMaterial (Material::Plate, 820, 600, 13u);
        static const juce::Image rust    = makeMaterial (Material::Chassis, 700, 520, 41u);
        return which == 0 ? chassis : (which == 1 ? plate : rust);
    }

    void drawWordmark (juce::Graphics& g)
    {
        // UNDER as bent glass tube, TOW as a rust-eaten stencil plate
        juce::GlyphArrangement under, tow;
        const auto f = Fonts::get().heavy (40.0f).withExtraKerningFactor (0.04f);
        under.addLineOfText (f, "UNDER", 20.0f, 41.0f);
        juce::Path pu; under.createPath (pu);
        neonStroke (g, pu, C::teal, 1.3f, 1.2f);
        const float x = 20.0f + juce::GlyphArrangement::getStringWidth (f, "UNDER") + 5.0f;
        tow.addLineOfText (f, "TOW", x, 41.0f);
        juce::Path pt; tow.createPath (pt);
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (pt);
            g.setGradientFill (juce::ColourGradient (C::bone, 0, 12, C::boneDim, 0, 44, false));
            g.fillRect (pt.getBounds().expanded (2));
            g.setOpacity (0.45f);
            g.drawImage (texture (2), pt.getBounds().expanded (30), juce::RectanglePlacement::fillDestination);
            g.setOpacity (1.0f);
            g.setColour (juce::Colours::black.withAlpha (0.9f));             // stencil bridges
            for (float bx = pt.getBounds().getX() + 9.0f; bx < pt.getBounds().getRight(); bx += 17.0f)
                g.fillRect (juce::Rectangle<float> (2.0f, 9.0f).withPosition (bx, 22.0f));
        }
        // a wave under the word
        juce::Path wave;
        const float w0 = 20.0f, w1 = x + juce::GlyphArrangement::getStringWidth (f, "TOW");
        for (float wx = w0; wx <= w1; wx += 2.0f)
        {
            const float wy = 49.0f + 2.2f * std::sin ((wx - w0) * 0.11f);
            if (wx == w0) wave.startNewSubPath (wx, wy); else wave.lineTo (wx, wy);
        }
        neonStroke (g, wave, C::sea, 0.9f, 0.6f);
        g.setFont (Fonts::get().mono (10.0f));
        g.setColour (C::amber.withAlpha (0.9f));
        const float cx = w1 + 12.0f;
        g.drawText ("AMBIENT", juce::Rectangle<float> (cx, 16.0f, 100.0f, 12.0f), juce::Justification::left, false);
        g.drawText ("SPACE", juce::Rectangle<float> (cx, 28.0f, 100.0f, 12.0f), juce::Justification::left, false);
        g.setColour (C::boneDim);
        g.drawText ("RC-02", juce::Rectangle<float> (cx, 40.0f, 100.0f, 12.0f), juce::Justification::left, false);
    }

    // A small engraved section label with a rule running to the right.
    class Caption : public juce::Component
    {
    public:
        Caption (juce::String t, juce::Colour c) : text (std::move (t)), colour (c) { setInterceptsMouseClicks (false, false); }
        void paint (juce::Graphics& g) override
        {
            const auto f = Fonts::get().mono (10.5f).withExtraKerningFactor (0.12f);
            g.setFont (f);
            g.setColour (colour.withAlpha (0.85f));
            g.drawText (text, getLocalBounds(), juce::Justification::centredLeft, false);
            const float x = juce::GlyphArrangement::getStringWidth (f, text) + 8.0f;
            g.setColour (colour.withAlpha (0.22f));
            g.fillRect (juce::Rectangle<float> (x, (float) getHeight() * 0.5f, (float) getWidth() - x, 1.0f));
        }
    private:
        juce::String text;
        juce::Colour colour;
    };

    float hashUnit (uint64_t x)
    {
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x ^= x >> 31;
        return (float) (x >> 40) / 16777215.0f;       // 0..1
    }
}

//==============================================================================
void PlateLayer::paint (juce::Graphics& g)
{
    const float sc = g.getInternalContext().getPhysicalPixelScaleFactor();
    const int w = juce::roundToInt ((float) getWidth() * sc), h = juce::roundToInt ((float) getHeight() * sc);
    if (cache.getWidth() != w || cache.getHeight() != h)
    {
        cache = juce::Image (juce::Image::RGB, std::max (1, w), std::max (1, h), true);
        juce::Graphics cg (cache);
        cg.addTransform (juce::AffineTransform::scale (sc));
        cg.drawImage (texture (0), getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination);
        cg.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.55f), 0, 0, juce::Colours::black.withAlpha (0.25f), 0, 58, false));
        cg.fillRect (0, 0, getWidth(), 58);
        cg.setColour (juce::Colours::black.withAlpha (0.8f));
        cg.drawHorizontalLine (57, 0.0f, (float) getWidth());
        drawWordmark (cg);
        for (const auto& p : plates)
            drawPlate (cg, p.r, p.style, texture (1), texture (2), p.seed);
    }
    g.drawImage (cache, getLocalBounds().toFloat());
}

//==============================================================================
FreezeButton::FreezeButton (UndertowProcessor& p) : ParamControl (p, "void_freeze", "FREEZE") { colour = C::magenta; }

FreezeButton::~FreezeButton()
{
    if (pressed) { setNorm (0.0f); endEdit(); }
}

bool FreezeButton::holdMode() const { return (bool) proc.state.state.getProperty ("freezeHold", false); }

void FreezeButton::paint (juce::Graphics& g)
{
    const bool on = norm() > 0.5f;
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.fillRoundedRectangle (r, 6.0f);
    const auto b = r.reduced (5.0f);
    if (on) { g.setColour (C::magenta.withAlpha (0.32f)); g.fillRoundedRectangle (b.expanded (4.0f), 7.0f); }
    g.setGradientFill (juce::ColourGradient (on ? C::magenta.brighter (0.2f) : juce::Colour (0xff2c1620), b.getTopLeft(),
                                             on ? C::magenta.darker (0.5f) : juce::Colour (0xff120a0e), b.getBottomRight(), false));
    g.fillRoundedRectangle (b, 5.0f);
    // six-armed frost mark
    const auto c = b.getCentre().translated (0, -9.0f);
    juce::Path frost;
    for (int k = 0; k < 6; ++k)
    {
        const float a = juce::MathConstants<float>::pi * (float) k / 3.0f;
        const juce::Point<float> d (std::sin (a), -std::cos (a));
        frost.startNewSubPath (c);
        frost.lineTo (c + d * 15.0f);
        const juce::Point<float> e (std::sin (a + 0.6f), -std::cos (a + 0.6f)), f (std::sin (a - 0.6f), -std::cos (a - 0.6f));
        frost.startNewSubPath (c + d * 9.0f); frost.lineTo (c + d * 9.0f + e * 5.0f);
        frost.startNewSubPath (c + d * 9.0f); frost.lineTo (c + d * 9.0f + f * 5.0f);
    }
    neonStroke (g, frost, on ? juce::Colours::white : C::magenta, 1.4f, on ? 1.0f : 0.4f);
    neonText (g, holdMode() ? "HOLD" : "FREEZE", b.withTrimmedTop (b.getHeight() - 26.0f),
              Fonts::get().heavy (17.0f).withExtraKerningFactor (0.1f), on ? juce::Colours::white : C::magenta,
              juce::Justification::centred, on ? 1.0f : 0.4f);
}

void FreezeButton::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu m;
        m.addSectionHeader ("FREEZE");
        m.addItem (1, "Click to freeze, click to release", true, ! holdMode());
        m.addItem (2, "Freeze only while held", true, holdMode());
        juce::Component::SafePointer<FreezeButton> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r)
        {
            if (safe != nullptr && r > 0) { safe->proc.state.state.setProperty ("freezeHold", r == 2, nullptr); safe->repaint(); }
        });
        return;
    }
    if (holdMode()) { pressed = true; beginEdit(); setNorm (1.0f); }
    else setNormComplete (norm() > 0.5f ? 0.0f : 1.0f);
}

void FreezeButton::mouseUp (const juce::MouseEvent&)
{
    if (pressed) { setNorm (0.0f); endEdit(); pressed = false; }
}

//==============================================================================
template <class T, class... Args>
T* Panel::add (juce::Rectangle<int> r, Args&&... args)
{
    auto c = std::make_unique<T> (std::forward<Args> (args)...);
    auto* raw = c.get();
    raw->setBounds (r);
    addAndMakeVisible (raw);
    if (auto* pc = dynamic_cast<ParamControl*> (raw)) controls.push_back (pc);
    owned.push_back (std::move (c));
    return raw;
}

Knob* Panel::knob (const char* id, const juce::String& label, Knob::Size s, int x, int y, juce::Colour c, int width)
{
    return add<Knob> ({ x, y, width > 0 ? width : Knob::preferredWidth (s), Knob::preferredHeight (s) }, proc, id, label, s, c);
}

LitButton* Panel::button (juce::Rectangle<int> r, const juce::String& text, juce::Colour c, std::function<void()> click, const juce::String& tip)
{
    auto* b = add<LitButton> (r, text, c);
    b->onClick = std::move (click);
    if (tip.isNotEmpty()) b->setTooltip (tip);
    b->setTitle (text);
    return b;
}

void Panel::plate (juce::Rectangle<float> r, juce::String title, juce::Colour c, float rust)
{
    PlateStyle st; st.title = std::move (title); st.titleColour = c; st.rustEdge = rust;
    const uint32_t seed = (uint32_t) (r.getX() * 31.0f + r.getY() * 7.0f + 1.0f);
    base.plates.push_back ({ r, st, seed });
}

Panel::Panel (UndertowProcessor& p) : proc (p)
{
    setSize (W, H);
    base.setBounds (getLocalBounds());
    addAndMakeVisible (base);
    frame = proc.extraState();
    buildSpace();
    buildPad();
    buildMutate();
    buildPulse();
    buildRail();
    buildTopBar();
    reduceMotion = (bool) p.state.state.getProperty ("reduceMotion", false);
    selectBand (1);
    showPage ((int) p.state.state.getProperty ("rightPage", 0));
    tick();
}

Panel::~Panel()
{
    if (recording) finishRecording (false);
    if (cornerLearning >= 0) cornerLearning = -1;
    sceneBEditing = false;
}

void Panel::buildTopBar()
{
    button ({ 252, 14, 26, 30 }, "<", C::teal, [this] { stepPreset (-1); }, "Previous preset");
    presetName = button ({ 282, 14, 210, 30 }, "", C::teal, [this] { showPresets(); }, "Factory presets by category. Save your own with Logic's preset menu.");
    presetName->fontHeight = 15.0f;
    button ({ 496, 14, 26, 30 }, ">", C::teal, [this] { stepPreset (1); }, "Next preset");
    editedBadge = button ({ 526, 18, 58, 22 }, "EDITED", C::amber, nullptr, "Changed since the preset was loaded.");
    editedBadge->fontHeight = 12.0f;

    editB = button ({ 598, 14, 76, 30 }, "EDIT B", C::amber, [this] { setEditB (! sceneBEditing); },
                    "While lit, every knob shows and edits Scene B (amber). BLOOM morphs from A to B.");
    aToB = button ({ 678, 14, 46, 30 }, "A>B", C::amber, [this] { sceneAction ("Copy A to B", [this] { proc.copySceneAToB(); }); },
                   "Copy Scene A into Scene B.");
    bToA = button ({ 728, 14, 46, 30 }, "B>A", C::amber, [this] { sceneAction ("Copy B to A", [this] { proc.copySceneBToA(); }); },
                   "Copy Scene B into Scene A.");
    swapB = button ({ 778, 14, 52, 30 }, "SWAP", C::amber, [this] { sceneAction ("Swap scenes", [this] { proc.swapScenes(); }); },
                    "Swap Scenes A and B.");
    add<BarSlider> ({ 840, 13, 290, 32 }, proc, "bloom", "BLOOM");
    undoB = button ({ 1136, 14, 44, 30 }, "UNDO", C::teal, [this] { proc.undoManager().undo(); }, "Undo (Cmd-Z)");
    redoB = button ({ 1182, 14, 44, 30 }, "REDO", C::teal, [this] { proc.undoManager().redo(); }, "Redo (Shift-Cmd-Z)");
    undoB->fontHeight = redoB->fontHeight = 12.5f;
    auto* setup = button ({ 1228, 14, 44, 30 }, "SIZE", C::teal, [this] { showSettings(); }, "Interface size and options");
    setup->fontHeight = 12.5f;
}

void Panel::buildSpace()
{
    using S = Knob::Size;
    // ---- DRIFT
    plate ({ 10, 64, 392, 140 }, "DRIFT");
    add<Switch> ({ 300, 68, 92, 20 }, proc, "drift_on", "ON");
    auto* driftSync = add<Switch> ({ 230, 68, 70, 20 }, proc, "drift_sync", "SYNC", C::amber);
    auto* driftRate = knob ("drift_rate", "RATE", S::Small, 18, 104, C::amber, 62);
    auto* driftDiv = add<ListSelector> ({ 16, 122, 66, 40 }, proc, "drift_div", "RATE", C::amber);
    ratePairs.push_back ({ driftSync, driftRate, driftDiv });
    knob ("drift_depth", "DEPTH", S::Small, 82, 104, C::teal, 62);
    knob ("drift_wow", "WOW", S::Small, 144, 104, C::teal, 62);
    knob ("drift_flutter", "FLUTTER", S::Small, 206, 104, C::teal, 62);
    knob ("drift_width", "WIDTH", S::Small, 268, 104, C::teal, 62);
    knob ("drift_mix", "MIX", S::Small, 330, 104, C::teal, 62);

    // ---- ECHO
    plate ({ 10, 210, 392, 310 }, "ECHO");
    add<Switch> ({ 300, 214, 92, 20 }, proc, "echo_on", "ON");
    auto* echoSync = add<Switch> ({ 230, 214, 70, 20 }, proc, "echo_sync", "SYNC", C::amber);
    add<Segments> ({ 22, 244, 262, 28 }, proc, "echo_mode", "", juce::StringArray { "TAPE", "PING-PONG", "REVERSE" });
    auto* echoTime = knob ("echo_time", "TIME", S::Small, 298, 236, C::amber, 92);
    auto* echoDiv = add<ListSelector> ({ 292, 250, 100, 40 }, proc, "echo_div", "TIME", C::amber);
    ratePairs.push_back ({ echoSync, echoTime, echoDiv });
    knob ("echo_feedback", "FEEDBACK", S::Large, 22, 294);
    knob ("echo_tone", "TONE", S::Large, 110, 294);
    knob ("echo_lowcut", "LOW CUT", S::Large, 198, 294);
    knob ("echo_drive", "DRIVE", S::Large, 286, 294);
    knob ("echo_age", "AGE", S::Large, 66, 408);
    knob ("echo_level", "LEVEL", S::Large, 242, 408);

    // ---- VOID | MUTATE share the right-hand plate; FREEZE stays on both pages
    plate ({ 878, 64, 392, 456 }, "");
    voidTab = button ({ 888, 67, 64, 22 }, "VOID", C::amber, [this] { showPage (0); }, "Reverb: size, decay, gravity and freeze.");
    mutTab = button ({ 956, 67, 84, 22 }, "MUTATE", C::magenta, [this] { showPage (1); },
                     "Sound-design page: harmonics, the pitch-climbing HALO, and EVOLVE.");
    voidTab->fontHeight = mutTab->fontHeight = 13.0f;
    add<FreezeButton> ({ 1160, 98, 100, 92 }, proc);
    const size_t from = owned.size();
    add<Switch> ({ 1168, 68, 92, 20 }, proc, "void_on", "ON");
    knob ("void_size", "SIZE", S::Large, 892, 96);
    knob ("void_decay", "DECAY", S::Large, 980, 96);
    knob ("void_gravity", "GRAVITY", S::Large, 1068, 96, C::amber);
    knob ("void_predelay", "PRE-DELAY", S::Medium, 896, 208);
    knob ("void_damp", "DAMP", S::Medium, 988, 208);
    knob ("void_lowcut", "LOW CUT", S::Medium, 1080, 208);
    knob ("void_diffusion", "DIFFUSION", S::Medium, 1172, 208);
    knob ("void_mod", "MOD", S::Medium, 896, 302);
    knob ("void_width", "WIDTH", S::Medium, 988, 302);
    knob ("void_level", "LEVEL", S::Medium, 1080, 302);
    tail = std::make_unique<TailView> (proc);
    tail->setBounds (892, 396, 364, 112);
    addAndMakeVisible (*tail);
    for (size_t i = from; i < owned.size(); ++i) voidPage.push_back (owned[i].get());
    voidPage.push_back (tail.get());
}

void Panel::buildMutate()
{
    using S = Knob::Size;
    const size_t from = owned.size();
    add<Switch> ({ 1168, 68, 92, 20 }, proc, "mut_on", "ON", C::magenta);
    add<Caption> ({ 892, 94, 256, 14 }, "HARMONICS", C::amber);
    knob ("harm_octave", "OCTAVE", S::Small, 892, 110, C::teal, 62);
    knob ("harm_fifth", "FIFTH", S::Small, 954, 110, C::teal, 62);
    knob ("harm_odd", "ODD", S::Small, 1016, 110, C::teal, 62);
    knob ("harm_sub", "SUB", S::Small, 1078, 110, C::teal, 62);
    add<Caption> ({ 892, 194, 368, 14 }, "HALO", C::amber);
    knob ("halo_climb", "CLIMB", S::Medium, 892, 208, C::magenta, 72);
    knob ("halo_spread", "SPREAD", S::Small, 968, 212, C::teal, 62);
    add<ListSelector> ({ 1036, 226, 150, 40 }, proc, "halo_interval", "INTERVAL", C::amber);
    add<Caption> ({ 892, 294, 250, 14 }, "EVOLVE", C::amber);
    add<Switch> ({ 1150, 291, 110, 20 }, proc, "evolve_follow", "FOLLOW PAD", C::amber);
    knob ("evolve_amount", "AMOUNT", S::Medium, 892, 308, C::magenta, 72);
    knob ("evolve_size", "SIZE", S::Small, 968, 312, C::teal, 62);
    knob ("evolve_tone", "TONE", S::Small, 1030, 312, C::teal, 62);
    knob ("evolve_formant", "FORMANT", S::Small, 1092, 312, C::teal, 66);
    add<ListSelector> ({ 1164, 326, 94, 40 }, proc, "evolve_cycle", "CYCLE", C::amber);
    mutView = std::make_unique<MutateView> (proc);
    mutView->setBounds (892, 396, 364, 112);
    addAndMakeVisible (*mutView);
    for (size_t i = from; i < owned.size(); ++i) mutPage.push_back (owned[i].get());
    mutPage.push_back (mutView.get());
}

void Panel::showPage (int p)
{
    page = juce::jlimit (0, 1, p);
    for (auto* c : voidPage) c->setVisible (page == 0);
    for (auto* c : mutPage) c->setVisible (page == 1);
    voidTab->setLit (page == 0);
    mutTab->setLit (page == 1);
    proc.state.state.setProperty ("rightPage", page, nullptr);
    if (page == 1) mutView->tick (reduceMotion);
}

void Panel::buildPad()
{
    using S = Knob::Size;
    plate ({ 410, 64, 460, 456 }, "XY", C::teal);
    xy = std::make_unique<XYPad> (proc);
    xy->setBounds (500, 90, 280, 280);
    addAndMakeVisible (*xy);
    xy->onUserMove = [this] { if (recording) recPoints.reserve (1024); };

    const char* names[] { "A", "B", "C", "D" };
    const char* flips[] { "xy_flip_a", "xy_flip_b", "xy_flip_c", "xy_flip_d" };
    for (int c = 0; c < 4; ++c)
    {
        const int x = (c == 0 || c == 3) ? 420 : 788;
        const int y = (c < 2) ? 90 : 236;
        cornerTitle[(size_t) c] = button ({ x, y, 72, 24 }, juce::String ("CORNER ") + names[c], C::teal, nullptr);
        cornerTitle[(size_t) c]->fontHeight = 12.5f;
        add<Switch> ({ x + 2, y + 28, 70, 20 }, proc, flips[c], "FLIP", C::amber);
        learnB[(size_t) c] = button ({ x, y + 52, 72, 24 }, "LEARN", C::magenta, [this, c] { learnCorner (c); },
                                     "Arm this corner, move any knobs, then press DONE. What you moved becomes this corner's variation.");
        clearB[(size_t) c] = button ({ x, y + 80, 34, 24 }, "CLR", C::teal, [this, c] { clearCorner (c); }, "Clear this corner's variation.");
        randB[(size_t) c] = button ({ x + 38, y + 80, 34, 24 }, "RND", C::teal, [this, c] { randomiseCorner (c); },
                                    "A small random variation on decay, size, gravity, feedback, drift and pulse.");
        for (auto* b : { learnB[(size_t) c], clearB[(size_t) c], randB[(size_t) c] }) b->fontHeight = 12.0f;
    }

    add<Segments> ({ 420, 380, 236, 40 }, proc, "xy_current", "CURRENT", juce::StringArray { "OFF", "ORBIT", "TIDE", "WANDER" });
    knob ("xy_radius", "RADIUS", S::Small, 660, 372, C::teal, 60);
    add<ListSelector> ({ 722, 380, 84, 40 }, proc, "xy_rate", "CYCLE", C::amber);
    knob ("xy_angle", "ANGLE", S::Small, 808, 372, C::teal, 56);

    recB = button ({ 420, 456, 64, 30 }, "REC", C::magenta, [this] { toggleRecord(); },
                   "Record the puck for the chosen number of bars, then it loops in time with the song.");
    add<Switch> ({ 490, 461, 76, 20 }, proc, "xy_motion_play", "PLAY", C::amber);
    add<ListSelector> ({ 566, 458, 84, 26 }, proc, "xy_motion_bars", "BARS", C::amber);
    returnB = button ({ 656, 456, 80, 30 }, "RETURN", C::teal, [this]
    {
        // a host parameter, so Logic can record and draw it
        auto& r = proc.parameter (ut::xy_return);
        r.beginChangeGesture();
        r.setValueNotifyingHost (r.getValue() > 0.5f ? 0.0f : 1.0f);
        r.endChangeGesture();
    }, "After you let go, the puck glides back to the centre over one bar (recorded as automation in Latch or Touch).");
    returnB->fontHeight = 13.0f;
    knob ("xy_amount", "AMOUNT", S::Small, 800, 440, C::teal, 62);
}

void Panel::buildPulse()
{
    using S = Knob::Size;
    plate ({ 10, 526, 1260, 170 }, "PULSE", C::teal);
    add<Switch> ({ 120, 530, 70, 20 }, proc, "pulse_on", "ON");
    add<Switch> ({ 22, 560, 120, 20 }, proc, "pulse_sub", "PULSE LOWS", C::amber);
    add<Segments> ({ 22, 586, 228, 40 }, proc, "pulse_position", "WHERE", juce::StringArray { "BEFORE", "AFTER", "WET ONLY" });
    add<ListSelector> ({ 22, 636, 120, 40 }, proc, "pulse_bands", "BANDS", C::amber);
    knob ("pulse_smooth", "SMOOTH", S::Small, 168, 616, C::teal, 70);

    spec = std::make_unique<Spectrum> (proc);
    spec->setBounds (258, 540, 382, 146);
    addAndMakeVisible (*spec);
    spec->onSelect = [this] (int b) { selectBand (b); };

    for (int b = 0; b < 6; ++b)
    {
        bandTabs[(size_t) b] = button ({ 650 + 59 * b, 540, 55, 24 }, "BAND " + juce::String (b + 1), C::amber, [this, b] { selectBand (b); });
        bandTabs[(size_t) b]->fontHeight = 12.5f;
        const int base = ut::pulse_b1_sync + 7 * b;
        auto id = [base] (int k) { return ut::specs[(size_t) (base + k)].id; };
        auto& set = bandControls[(size_t) b];
        auto* sync = add<Switch> ({ 650, 572, 70, 20 }, proc, id (0), "SYNC", C::amber);
        auto* rate = knob (id (2), "RATE", S::Small, 712, 600, C::amber, 64);
        auto* div = add<ListSelector> ({ 714, 616, 84, 40 }, proc, id (1), "RATE", C::amber);
        ratePairs.push_back ({ sync, rate, div, b });
        auto* depth = knob (id (3), "DEPTH", S::Small, 650, 600, C::teal, 60);
        auto* phase = knob (id (5), "PHASE", S::Small, 800, 600, C::teal, 60);
        auto* source = add<Segments> ({ 722, 570, 120, 24 }, proc, id (6), "", juce::StringArray { "LFO", "STEPS" });
        auto* shape = add<ListSelector> ({ 848, 570, 166, 24 }, proc, id (4), "SHAPE", C::teal);
        set = { sync, rate, div, depth, phase, source, shape };
    }
    steps = std::make_unique<StepGrid> (proc);
    steps->setBounds (866, 608, 148, 70);
    addAndMakeVisible (*steps);

    // FOCUS
    add<Switch> ({ 1030, 540, 90, 20 }, proc, "pulse_focus_on", "FOCUS");
    add<ListSelector> ({ 1030, 570, 112, 24 }, proc, "pulse_focus_note", "NOTE", C::amber);
    add<ListSelector> ({ 1148, 570, 112, 24 }, proc, "pulse_focus_harm", "HARM", C::amber);
    knob ("pulse_focus_q", "Q", S::Small, 1026, 604, C::teal, 60);
    knob ("pulse_focus_depth", "DEPTH", S::Small, 1086, 604, C::teal, 60);
    add<ListSelector> ({ 1152, 618, 108, 40 }, proc, "pulse_focus_div", "RATE", C::amber);
}

void Panel::buildRail()
{
    using S = Knob::Size;
    plate ({ 10, 702, 470, 92 }, "MACROS", C::teal);
    knob ("macro_depth", "DEPTH", S::Small, 112, 712, C::teal, 72);
    knob ("macro_motion", "MOTION", S::Small, 185, 712, C::teal, 72);
    knob ("macro_glow", "GLOW", S::Small, 258, 712, C::teal, 72);
    knob ("macro_chaos", "CHAOS", S::Small, 331, 712, C::magenta, 72);
    knob ("macro_mutate", "MUTATE", S::Small, 404, 712, C::magenta, 72);

    plate ({ 486, 702, 330, 92 }, "ANCHOR + DUCK", C::teal);
    add<Switch> ({ 498, 734, 70, 20 }, proc, "anchor_on", "ON");
    add<Switch> ({ 498, 760, 76, 20 }, proc, "anchor_mono", "MONO");
    knob ("anchor_freq", "FREQ", S::Small, 586, 712, C::teal, 72);
    knob ("duck_amount", "DUCK", S::Small, 662, 712, C::amber, 68);
    knob ("duck_release", "RELEASE", S::Small, 734, 712, C::amber, 74);

    plate ({ 822, 702, 448, 92 }, "OUTPUT", C::teal);
    knob ("in_gain", "IN", S::Small, 900, 712, C::teal, 70);
    knob ("mix", "MIX", S::Small, 976, 712, C::teal, 80);
    knob ("out_gain", "OUT", S::Small, 1062, 712, C::teal, 70);
    meters = std::make_unique<Meters> (proc);
    meters->setBounds (1150, 710, 108, 78);
    addAndMakeVisible (*meters);
}

//==============================================================================
void Panel::setEditB (bool on)
{
    if (on && cornerLearning >= 0) return;
    sceneBEditing = on;
    editB->setLit (on);
    editB->setText (on ? "EDITING B" : "EDIT B");
    for (auto* c : controls) c->repaint();
}

void Panel::selectBand (int b)
{
    band = juce::jlimit (0, 5, b);
    for (int i = 0; i < 6; ++i)
    {
        bandTabs[(size_t) i]->setLit (i == band);
        for (auto* c : bandControls[(size_t) i]) c->setVisible (i == band);
    }
    spec->selected = band;
    spec->repaint();
    steps->band = band;
    steps->repaint();
    for (auto& rp : ratePairs) if (rp.band >= 0 && rp.band != band) { rp.rate->setVisible (false); rp.div->setVisible (false); }
}

void Panel::sceneAction (const juce::String& name, std::function<void()> action)
{
    const auto params = proc.normalizedValues();
    const auto extra = proc.extraState();
    action();
    pushSnapshot (proc, name, params, extra);
}

void Panel::learnCorner (int c)
{
    if (cornerLearning == c)
    {
        // DONE: what moved becomes the corner; the knobs go back to where they were.
        const auto now = proc.normalizedValues();
        auto next = learnExtra != nullptr ? *learnExtra : proc.extraState();
        auto& corner = next.corners[(size_t) c];
        corner.fill (0.0f);
        int moved = 0;
        for (int k = 0; k < ut::count; ++k)
            if (ut::specs[(size_t) k].kind == 0 && k != ut::xy_x && k != ut::xy_y)
                if (const float d = now[(size_t) k] - learnParams[(size_t) k]; std::abs (d) > 1.0e-4f)
                {
                    corner[(size_t) k] = juce::jlimit (-1.0f, 1.0f, d);
                    ++moved;
                }
        next.edited = true;
        cornerLearning = -1;
        if (moved > 0)
        {
            proc.applySnapshot (learnParams, next);
            pushSnapshot (proc, "Learn corner " + juce::String::charToString ((juce::juce_wchar) ('A' + c)), learnParams, *learnExtra);
        }
        else proc.applySnapshot (learnParams, *learnExtra);
        learnExtra.reset();
    }
    else
    {
        if (cornerLearning >= 0) learnCorner (cornerLearning);   // finish the other one first
        if (sceneBEditing) setEditB (false);
        learnParams = proc.normalizedValues();
        learnExtra = std::make_unique<ut::ExtraState> (proc.extraState());
        cornerLearning = c;
    }
    for (int k = 0; k < 4; ++k)
    {
        learnB[(size_t) k]->setText (cornerLearning == k ? "DONE" : "LEARN");
        learnB[(size_t) k]->setLit (cornerLearning == k);
    }
    editB->setEnabled (cornerLearning < 0);
}

void Panel::clearCorner (int c)
{
    if (cornerLearning >= 0) return;
    const auto before = proc.extraState();
    auto next = before;
    next.corners[(size_t) c].fill (0.0f);
    next.edited = true;
    proc.setExtraState (next);
    pushSnapshot (proc, "Clear corner", proc.normalizedValues(), before);
}

void Panel::randomiseCorner (int c)
{
    if (cornerLearning >= 0) return;
    const auto before = proc.extraState();
    auto next = before;
    auto& corner = next.corners[(size_t) c];
    corner.fill (0.0f);
    // Deterministic: the same corner and press count always give the same result.
    const uint64_t seed = (uint64_t) (c + 1) * 0x9E3779B97F4A7C15ULL + (uint64_t) (++randCount[(size_t) c]) * 0xD1B54A32D192ED03ULL;
    std::vector<int> pool { ut::void_decay, ut::void_size, ut::void_gravity, ut::echo_feedback, ut::drift_depth, ut::void_damp };
    auto plain = [this] (int k) { auto& p = proc.parameter (k); return p.convertFrom0to1 (p.getValue()); };
    if (plain (ut::pulse_on) > 0.5f)
        for (int b = 0; b < juce::roundToInt (plain (ut::pulse_bands)); ++b) pool.push_back (ut::pulse_b1_depth + 7 * b);
    int chosen = 0;
    for (size_t i = 0; i < pool.size(); ++i)
    {
        const float pick = hashUnit (seed + i * 3);
        if (pick < 0.45f || (chosen < 2 && i + 2 >= pool.size()))
        {
            const float magnitude = 0.10f + 0.25f * hashUnit (seed + i * 3 + 1);
            const float sign = hashUnit (seed + i * 3 + 2) < 0.5f ? -1.0f : 1.0f;
            corner[(size_t) pool[i]] = sign * magnitude;
            ++chosen;
        }
    }
    next.edited = true;
    proc.setExtraState (next);
    pushSnapshot (proc, "Randomise corner", proc.normalizedValues(), before);
}

void Panel::toggleRecord()
{
    if (recording) { finishRecording (true); return; }
    recParams = proc.normalizedValues();
    recExtra = std::make_unique<ut::ExtraState> (proc.extraState());
    auto& play = proc.parameter (ut::xy_motion_play);
    if (play.getValue() > 0.5f) { play.beginChangeGesture(); play.setValueNotifyingHost (0.0f); play.endChangeGesture(); }
    auto& bars = proc.parameter (ut::xy_motion_bars);
    recLength = (double) (1 << juce::roundToInt (bars.convertFrom0to1 (bars.getValue())));
    recLastBeat = proc.lastBeat();
    recElapsed = 0;
    recPoints.clear();
    recording = true;
    recB->setLit (true);
}

void Panel::tickRecording()
{
    const double beat = proc.lastBeat(), barBeats = std::max (1.0, proc.lastBarBeats());
    const double step = beat - recLastBeat;
    recLastBeat = beat;
    if (step > 0 && step < 4) recElapsed += step;                 // ignore loop jumps
    const double phase = beat / (barBeats * recLength);
    const float bar = (float) ((phase - std::floor (phase)) * recLength);
    ut::MotionPoint pt { bar, proc.parameter (ut::xy_x).getValue(), proc.parameter (ut::xy_y).getValue() };
    if (recPoints.empty() || std::abs (recPoints.back().bar - pt.bar) > (float) recLength / 1000.0f)
        recPoints.push_back (pt);
    const int total = juce::roundToInt (recLength);
    recB->setText ("REC " + juce::String (std::min (total, (int) (recElapsed / barBeats) + 1)) + "/" + juce::String (total));
    if (recElapsed >= recLength * barBeats) finishRecording (true);
}

void Panel::finishRecording (bool keep)
{
    recording = false;
    recB->setLit (false);
    recB->setText ("REC");
    if (! keep || recPoints.size() < 2 || recExtra == nullptr) { recPoints.clear(); return; }
    std::sort (recPoints.begin(), recPoints.end(), [] (const auto& a, const auto& b) { return a.bar < b.bar; });
    std::vector<ut::MotionPoint> kept;
    const size_t stride = (recPoints.size() + 1023) / 1024;
    for (size_t i = 0; i < recPoints.size(); i += std::max<size_t> (1, stride))
        if (kept.empty() || recPoints[i].bar > kept.back().bar) kept.push_back (recPoints[i]);
    auto next = proc.extraState();
    next.motionCount = (int) std::min<size_t> (1024, kept.size());
    for (int k = 0; k < next.motionCount; ++k) next.motion[(size_t) k] = kept[(size_t) k];
    next.edited = true;
    proc.setExtraState (next);
    auto& play = proc.parameter (ut::xy_motion_play);
    play.beginChangeGesture(); play.setValueNotifyingHost (1.0f); play.endChangeGesture();
    pushSnapshot (proc, "Record motion", recParams, *recExtra);
    recPoints.clear();
    recExtra.reset();
}

void Panel::stepPreset (int delta)
{
    const int n = proc.getNumPrograms();
    loadPreset ((proc.getCurrentProgram() + delta + n) % n);
}

void Panel::loadPreset (int index)
{
    if (cornerLearning >= 0) learnCorner (cornerLearning);
    if (recording) finishRecording (false);
    const auto params = proc.normalizedValues();
    const auto extra = proc.extraState();
    proc.setCurrentProgram (index);
    pushSnapshot (proc, "Load preset", params, extra);
}

void Panel::showPresets()
{
    juce::PopupMenu m;
    const auto& bank = ut::factoryPresets();
    juce::StringArray cats;
    for (const auto& d : bank) cats.addIfNotAlreadyThere (d.category);
    const int cur = proc.getCurrentProgram();
    for (const auto& cat : cats)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < (int) bank.size(); ++i)
            if (cat == bank[(size_t) i].category) sub.addItem (i + 1, bank[(size_t) i].name, true, i == cur);
        m.addSubMenu (cat, sub, true, nullptr, cat == bank[(size_t) juce::jlimit (0, (int) bank.size() - 1, cur)].category);
    }
    juce::Component::SafePointer<Panel> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetName), [safe] (int r)
    {
        if (safe != nullptr && r > 0) safe->loadPreset (r - 1);
    });
}

void Panel::showSettings()
{
    juce::PopupMenu m, sizes;
    const float cur = (float) frame.width / (float) W;
    const float scales[] { 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
    for (int i = 0; i < 5; ++i)
        sizes.addItem (10 + i, juce::String (juce::roundToInt (scales[i] * 100)) + " %", true, std::abs (cur - scales[i]) < 0.01f);
    m.addSubMenu ("Interface size", sizes);
    m.addItem (2, "Reduce motion", true, reduceMotion);
    m.addItem (3, "Show all parameters");
    m.addSeparator();
    m.addItem (4, "Fonts: Rajdhani, Share Tech Mono (SIL Open Font License)", false);
    juce::Component::SafePointer<Panel> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options(), [safe, scales] (int r)
    {
        if (safe == nullptr) return;
        if (r >= 10 && r < 15 && safe->onScale) safe->onScale (scales[r - 10]);
        if (r == 2) { safe->reduceMotion = ! safe->reduceMotion; safe->proc.state.state.setProperty ("reduceMotion", safe->reduceMotion, nullptr); }
        if (r == 3) safe->showAllParameters (true);
    });
}

void Panel::showAllParameters (bool show)
{
    if (! show) { genericClose.reset(); genericView.reset(); generic.reset(); return; }
    generic = std::make_unique<juce::GenericAudioProcessorEditor> (proc);
    genericView = std::make_unique<juce::Viewport>();
    genericView->setViewedComponent (generic.get(), false);
    genericView->setBounds (10, 64, 1260, 728);
    generic->setSize (1240, generic->getHeight());
    addAndMakeVisible (*genericView);
    genericClose = std::make_unique<LitButton> ("CLOSE", C::magenta);
    genericClose->setBounds (1180, 70, 80, 26);
    genericClose->onClick = [this] { juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<Panel> (this)]
                                     { if (safe != nullptr) safe->showAllParameters (false); }); };
    addAndMakeVisible (*genericClose);
}

void Panel::tick()
{
    frame = proc.extraState();
    for (auto* c : controls) if (c->isShowing()) c->refreshEffective();
    for (auto& rp : ratePairs)
    {
        if (rp.band >= 0 && rp.band != band) continue;
        const bool synced = rp.sync->norm() > 0.5f;
        if (rp.div->isVisible() != synced) rp.div->setVisible (synced);
        if (rp.rate->isVisible() == synced) rp.rate->setVisible (! synced);
    }
    xy->tick (frame, reduceMotion);
    spec->tick (reduceMotion);
    steps->tick (frame);
    if (page == 0) tail->tick(); else mutView->tick (reduceMotion);
    meters->tick();
    // the MUTATE tab glows when mutation is running and its page is hidden
    if (page == 0) mutTab->setLit (proc.engine.mutateGate.load (std::memory_order_relaxed) > 0.5f);
    if (recording) tickRecording();
    for (int c = 0; c < 4; ++c)
    {
        bool any = false;
        for (float v : frame.corners[(size_t) c]) any = any || v != 0.0f;
        cornerTitle[(size_t) c]->setLit (any);
        clearB[(size_t) c]->setEnabled (any && cornerLearning < 0);
        randB[(size_t) c]->setEnabled (cornerLearning < 0);
    }
    returnB->setLit (proc.parameter (ut::xy_return).getValue() > 0.5f);
    undoB->setLit (proc.undoManager().canUndo());
    redoB->setLit (proc.undoManager().canRedo());
    const int preset = proc.getCurrentProgram();
    const bool edited = proc.isEdited();
    if (preset != lastPreset || presetName->text.isEmpty())
    {
        lastPreset = preset;
        const auto& d = ut::factoryPresets()[(size_t) juce::jlimit (0, proc.getNumPrograms() - 1, preset)];
        presetName->setText (juce::String (d.category) + "  /  " + juce::String (d.name).toUpperCase());
    }
    editedBadge->setVisible (edited);
    lastEdited = edited;
}
} // namespace ui

//==============================================================================
UndertowEditor::UndertowEditor (UndertowProcessor& p)
    : AudioProcessorEditor (p), proc (p), panel (p)
{
    addAndMakeVisible (panel);
    panel.onScale = [this] (float s)
    {
        auto next = proc.extraState();
        next.width = juce::roundToInt (ui::Panel::W * s);
        next.height = juce::roundToInt (ui::Panel::H * s);
        proc.setExtraState (next);
        setSize (next.width, next.height);
    };
    const auto saved = p.extraState();
    const float s = juce::jlimit (0.75f, 2.0f, (float) saved.width / (float) ui::Panel::W);
    setResizable (true, true);
    setResizeLimits (960, 600, 2560, 1600);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) ui::Panel::W / ui::Panel::H);
    setSize (juce::roundToInt (ui::Panel::W * s), juce::roundToInt (ui::Panel::H * s));
    setWantsKeyboardFocus (true);
    startTimerHz (30);
}

UndertowEditor::~UndertowEditor() { stopTimer(); }

void UndertowEditor::resized()
{
    const float s = (float) getWidth() / (float) ui::Panel::W;
    panel.setTransform (juce::AffineTransform::scale (s));
    panel.setBounds (0, 0, ui::Panel::W, ui::Panel::H);
    // remember a drag-resize too
    auto saved = proc.extraState();
    if (saved.width != getWidth() || saved.height != getHeight())
    {
        saved.width = getWidth();
        saved.height = getHeight();
        proc.setExtraState (saved);
    }
}

bool UndertowEditor::keyPressed (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && k.getKeyCode() == 'Z')
    {
        if (k.getModifiers().isShiftDown()) proc.undoManager().redo(); else proc.undoManager().undo();
        return true;
    }
    return false;
}

void UndertowEditor::timerCallback() { panel.tick(); }
