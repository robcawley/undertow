#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "UI/Displays.h"
#include "UI/Chassis.h"

class UndertowProcessor;

namespace ui
{
// Plates painted once per scale into an image, so dynamic repaints are a cheap blit.
struct PlateDef { juce::Rectangle<float> r; PlateStyle style; uint32_t seed; };

class PlateLayer : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    std::vector<PlateDef> plates;
    juce::Image cache;
};

// FREEZE: a big latching button; right-click to make it momentary.
class FreezeButton : public ParamControl
{
public:
    FreezeButton (UndertowProcessor& p);
    ~FreezeButton() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    bool holdMode() const;
    bool pressed = false;
};

class Panel : public juce::Component
{
public:
    static constexpr int W = 1280, H = 800;
    explicit Panel (UndertowProcessor& p);
    ~Panel() override;
    void tick();
    void resized() override {}
    std::function<void (float)> onScale;
    bool reduceMotion = false;

    // for the offscreen renderer and tests
    void setEditB (bool on);
    void selectBand (int band);
    void learnCorner (int corner);
    void clearCorner (int corner);
    void randomiseCorner (int corner);
    void toggleRecord();
    bool isRecording() const { return recording; }
    void showPage (int page);                 // 0 VOID, 1 MUTATE
    int currentPage() const { return page; }
    XYPad& pad() { return *xy; }
    Spectrum& spectrum() { return *spec; }
    std::vector<ParamControl*>& allControls() { return controls; }

private:
    template <class T, class... Args> T* add (juce::Rectangle<int> r, Args&&... args);
    Knob* knob (const char* id, const juce::String& label, Knob::Size s, int x, int y, juce::Colour c = C::teal, int width = 0);
    LitButton* button (juce::Rectangle<int> r, const juce::String& text, juce::Colour c, std::function<void()> click,
                       const juce::String& tip = {});
    void plate (juce::Rectangle<float> r, juce::String title, juce::Colour c = C::amber, float rust = 1.0f);
    void buildTopBar();
    void buildSpace();
    void buildPad();
    void buildPulse();
    void buildMutate();
    void buildRail();
    void showSettings();
    void showPresets();
    void stepPreset (int delta);
    void loadPreset (int index);
    void sceneAction (const juce::String& name, std::function<void()> action);
    void showAllParameters (bool show);
    void finishRecording (bool keep);
    void tickRecording();

    UndertowProcessor& proc;
    PlateLayer base;
    std::vector<ParamControl*> controls;
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::unique_ptr<XYPad> xy;
    std::unique_ptr<Spectrum> spec;
    std::unique_ptr<StepGrid> steps;
    std::unique_ptr<TailView> tail;
    std::unique_ptr<Meters> meters;
    std::unique_ptr<MutateView> mutView;
    std::vector<juce::Component*> voidPage, mutPage;
    LitButton *voidTab = nullptr, *mutTab = nullptr;
    int page = 0;
    ut::ExtraState frame;

    // swaps between a rate knob and a division list as SYNC changes
    struct RatePair { ParamControl* sync = nullptr; juce::Component* rate = nullptr; juce::Component* div = nullptr; int band = -1; };
    std::vector<RatePair> ratePairs;
    std::array<std::vector<juce::Component*>, 6> bandControls;
    std::array<LitButton*, 6> bandTabs {};
    int band = 1;

    LitButton *presetName = nullptr, *editedBadge = nullptr, *editB = nullptr, *undoB = nullptr, *redoB = nullptr,
              *recB = nullptr, *returnB = nullptr, *aToB = nullptr, *bToA = nullptr, *swapB = nullptr;
    std::array<LitButton*, 4> learnB {}, clearB {}, randB {};
    std::array<LitButton*, 4> cornerTitle {};
    int lastPreset = -1;
    bool lastEdited = false;

    // corner learning
    std::array<float, ut::count> learnParams {};
    std::unique_ptr<ut::ExtraState> learnExtra;
    std::array<int, 4> randCount {};

    // motion recording
    bool recording = false;
    double recLastBeat = 0, recElapsed = 0, recLength = 4;
    std::vector<ut::MotionPoint> recPoints;
    std::array<float, ut::count> recParams {};
    std::unique_ptr<ut::ExtraState> recExtra;

    std::unique_ptr<juce::GenericAudioProcessorEditor> generic;
    std::unique_ptr<juce::Viewport> genericView;
    std::unique_ptr<LitButton> genericClose;
};
} // namespace ui

class UndertowEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit UndertowEditor (UndertowProcessor&);
    ~UndertowEditor() override;
    void paint (juce::Graphics&) override {}
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    ui::Panel& getPanel() { return panel; }          // for the offscreen renderer
    void tickForTest() { panel.tick(); }

private:
    void timerCallback() override;
    UndertowProcessor& proc;
    ui::Panel panel;
    juce::TooltipWindow tips { this, 650 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UndertowEditor)
};
