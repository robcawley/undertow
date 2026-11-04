#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"
#include "../Parameters.h"
#include "../Core/State.h"
#include <map>

class UndertowProcessor;

namespace ui
{
// Common behaviour for every control bound to one parameter.
class ParamControl : public juce::Component, public juce::SettableTooltipClient
{
public:
    ParamControl (UndertowProcessor& p, const char* paramId, juce::String label);
    ~ParamControl() override;

    // Called by the editor's timer: picks up macro-modulated (effective) values.
    void refreshEffective();
    float norm() const;
    float effectiveNorm() const { return param.convertTo0to1 (effective); }
    bool modulated() const { return ! editsSceneB() && std::abs (norm() - effectiveNorm()) > 0.004f; }
    juce::String valueText() const;
    bool editsSceneB() const;                 // EDIT B is on and this parameter morphs
    juce::Colour drawColour() const;
    juce::String effectiveText() const { return param.getText (effectiveNorm(), 0); }
    juce::RangedAudioParameter& parameter() { return param; }
    bool isLearning() const;
    int learnedCC() const;

    juce::String label;
    juce::Colour colour = C::teal;

protected:
    void beginEdit();
    void setNorm (float n);              // during a gesture
    void endEdit();
    void setNormComplete (float n);      // one-shot change, with undo
    void showMenu();
    void typeValue();
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    virtual void valueChanged() { repaint(); }

    UndertowProcessor& proc;
    juce::RangedAudioParameter& param;
    int index = 0;
    float value = 0.0f, effective = 0.0f;
    bool editing = false, hovering = false;

    void mouseEnter (const juce::MouseEvent&) override { hovering = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hovering = false; repaint(); }

private:
    juce::ParameterAttachment attachment;
    float gestureStart = 0.0f, shownB = -1.0f;
    bool editingB = false;
    std::unique_ptr<ut::ExtraState> extraBefore;
    std::unique_ptr<juce::TextEditor> editor;
};

// Rotary knob: machined cap, neon value arc, outer ring for the effective (macro) value.
class Knob : public ParamControl
{
public:
    enum class Size { Small, Medium, Large, Macro };
    Knob (UndertowProcessor& p, const char* id, juce::String label, Size s = Size::Medium, juce::Colour c = C::teal);
    void paint (juce::Graphics&) override;
    void resized() override { cap = {}; }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool bipolar = false;
    Size size;
    static int preferredHeight (Size s);
    static int preferredWidth (Size s);
private:
    juce::Rectangle<float> dialArea() const;
    juce::Image cap;            // cached machined cap, redrawn only on resize
    float dragNorm = 0.0f, lastY = 0.0f;
    bool dragging = false;
};

// A row (or grid) of lit cells, one per choice: readable at a glance.
class Segments : public ParamControl
{
public:
    Segments (UndertowProcessor& p, const char* id, juce::String label, juce::StringArray cellNames = {},
              int columns = 0, juce::Colour c = C::teal);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    juce::StringArray names;
    int cols;
    juce::Rectangle<float> cell (int i) const;
};

// A display window showing the current choice; click for the full list, drag or scroll to step.
class ListSelector : public ParamControl
{
public:
    ListSelector (UndertowProcessor& p, const char* id, juce::String label, juce::Colour c = C::amber);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    void step (int delta);
};

// LED toggle for on/off parameters.
class Switch : public ParamControl
{
public:
    Switch (UndertowProcessor& p, const char* id, juce::String label, juce::Colour c = C::teal);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
};

// Momentary PUNCH under a hazard-striped guard, and latching HOLD.
class PerformButton : public ParamControl
{
public:
    PerformButton (UndertowProcessor& p, const char* id, juce::String label, bool momentary);
    ~PerformButton() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    bool momentary, pressed = false;
};

// Records a finished parameter change in the processor's undo history.
void pushUndo (UndertowProcessor& p, juce::RangedAudioParameter& param, float fromNorm, float toNorm);
// Records a change to many parameters and/or the scene, corner, step and motion
// state as one undo step. Pass the state from before the change; the current
// state is the "after".
void pushSnapshot (UndertowProcessor& p, const juce::String& name,
                   const std::array<float, ut::count>& paramsBefore, const ut::ExtraState& extraBefore);

// Editor-wide modes shared by every control.
inline bool sceneBEditing = false;   // knobs show and edit Scene B
inline int cornerLearning = -1;      // a corner is armed: knob moves are temporary
inline int repaintRequests = 0;      // repaints asked for by the 30 Hz tick (idle test)
inline std::map<juce::String, int>* repaintLog = nullptr;
inline void noteRepaint (const char* who) { ++repaintRequests; if (repaintLog != nullptr) ++(*repaintLog)[who]; }

// A small lit text button for bars and panels.
class LitButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    LitButton (juce::String t, juce::Colour c = C::teal) : text (std::move (t)), colour (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { if (isEnabled() && onClick) onClick(); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }
    void setLit (bool b) { if (b != lit) { lit = b; repaint(); } }
    void setText (const juce::String& t) { if (t != text) { text = t; repaint(); } }
    juce::String text;
    juce::Colour colour;
    std::function<void()> onClick;
    bool lit = false, hover = false;
    float fontHeight = 14.5f;
};

// A wide horizontal bar for one big continuous parameter (BLOOM).
class BarSlider : public ParamControl
{
public:
    BarSlider (UndertowProcessor& p, const char* id, juce::String label, juce::Colour c = C::amber);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
private:
    juce::Rectangle<float> track() const;
    bool dragging = false;
};

// Shared tooltip text per parameter ID: one plain sentence each.
juce::String describe (const juce::String& paramId);
} // namespace ui
