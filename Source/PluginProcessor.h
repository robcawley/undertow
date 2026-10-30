#pragma once
#include "DSP/Engine.h"
#include "Modulation.h"
#include "Parameters.h"
#include "Presets.h"
#include "State.h"
#include <mutex>
class UndertowProcessor final : public juce::AudioProcessor {
public:
  UndertowProcessor();
  juce::AudioProcessorValueTreeState state;
  ut::Engine engine;
  ut::ExtraState extraState() const {
    std::lock_guard guard(extraMutex);
    return extra;
  }
  void setExtraState(const ut::ExtraState &next) {
    std::lock_guard guard(extraMutex);
    extra = next;
    bool offsets = false;
    for (const auto &corner : extra.corners)
      for (float value : corner)
        offsets |= value != 0;
    hasCornerOffsets.store(offsets, std::memory_order_relaxed);
    exchange.publish(extra);
  }

  void copySceneAToB();
  void copySceneBToA();
  void swapScenes();
  void setSceneBValue(int index, float normalized);
  float effectiveValue(int index) const noexcept {
    return effective[(size_t)index].load(std::memory_order_relaxed);
  }
  float effectiveX() const noexcept {
    return puckX.load(std::memory_order_relaxed);
  }
  float effectiveY() const noexcept {
    return puckY.load(std::memory_order_relaxed);
  }
  ut::Modulation modulation;

  // ---- Editor support (message thread).
  juce::UndoManager &undoManager() { return undo; }
  int parameterIndex(const juce::String &id) const;
  juce::RangedAudioParameter &parameter(int index) const {
    return *params[(size_t)index];
  }
  std::array<float, ut::count> normalizedValues() const;
  // Sets every differing parameter (each as its own host gesture) and the
  // non-parameter state. Used by undo, presets and scene actions.
  void applySnapshot(const std::array<float, ut::count> &normalized,
                     const ut::ExtraState &next);
  float sceneBValue(int index) const {
    std::lock_guard guard(extraMutex);
    return extra.sceneB[(size_t)index];
  }
  bool isEdited() const;
  // Song position as last seen by the audio thread, for motion recording.
  double lastBeat() const noexcept { return beatNow.load(std::memory_order_relaxed); }
  double lastBarBeats() const noexcept { return barNow.load(std::memory_order_relaxed); }
  double lastTempo() const noexcept { return tempoNow.load(std::memory_order_relaxed); }
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override { return "UNDERTOW"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override;
  int getNumPrograms() override { return (int)ut::factoryPresets().size(); }
  int getCurrentProgram() override;
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int) override;
  void changeProgramName(int, const juce::String &) override {}
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;

private:
  std::array<std::atomic<float> *, ut::count> raw{};
  std::array<juce::RangedAudioParameter *, ut::count> params{};
  std::vector<std::array<float, ut::count>> presetNormal;
  juce::UndoManager undo{0, 60};
  std::atomic<double> beatNow{0}, barNow{4}, tempoNow{120};
  double tempo = 120, barBeats = 4;
  ut::Transport transport;
  std::array<std::atomic<float>, ut::count> effective{};
  std::atomic<bool> hasCornerOffsets{false};
  std::atomic<float> puckX{.5f}, puckY{.5f};
  mutable std::mutex extraMutex;
  ut::ExtraState extra;
  ut::StateExchange exchange;
};
