// UNDERTOW core: the whole effect behind one object, for games and other
// hosts. MIT licence (see LICENSE in this folder).
//
// Threads: prepare() and the constructor allocate; call them before audio
// starts. process() runs on the audio thread and never allocates or locks.
// The control calls (setParameter, loadPreset, morphTo, setTempo, setFreeze)
// may come from one other thread (the game thread) or from the audio thread
// itself; they are queued and take effect at the start of the next process().
#pragma once
#include "Engine.h"
#include "Modulation.h"
#include "Presets.h"
#include <atomic>
#include <memory>

namespace ut {

class Processor {
public:
  Processor();
  ~Processor();

  // Allocates. maxBlock is the largest block process() will be given at once;
  // longer blocks are split internally.
  void prepare(double sampleRate, int maxBlock);
  // Clears all tails and delay lines. Audio thread.
  void reset() noexcept;

  // In place, planar stereo. Non-finite input samples are treated as silence.
  void process(float *left, float *right, int frames) noexcept;
  // In place, interleaved stereo (L R L R ...).
  void processInterleaved(float *stereo, int frames) noexcept;

  // ---- Presets: the game bank first, then the 92 plug-in presets.
  static int presetCount() noexcept;
  static const char *presetName(int index) noexcept;
  static const char *presetCategory(int index) noexcept;
  static int findPreset(const char *name) noexcept; // -1 if not found
  static int gamePresetCount() noexcept;
  // The plain values a preset starts from (scene A), as the effect will use
  // them. Allocates nothing; any thread.
  static void presetValues(int index, float *out) noexcept;

  // ---- Parameters, in plain units (%, Hz, ms, s, dB; switches 0/1;
  // choices as their index).
  static int parameterCount() noexcept { return count; }
  static const Spec &parameterSpec(int index) noexcept;
  static int findParameter(const char *id) noexcept; // -1 if not found

  // ---- Control (queued; see the note at the top).
  bool setParameter(int index, float plainValue) noexcept;
  bool loadPreset(int index) noexcept;
  // Glides from the current settings to the preset over the given time.
  // Levels and amounts glide; a section that turns on fades in, one that turns
  // off fades out first; other switches change halfway.
  bool morphTo(int index, float seconds) noexcept;
  bool setTempo(double bpm) noexcept;
  bool setFreeze(bool on) noexcept;

  // Last values the audio thread used (any thread may read).
  float parameter(int index) const noexcept;
  int currentPreset() const noexcept { return preset.load(std::memory_order_relaxed); }
  bool isMorphing() const noexcept { return morphing.load(std::memory_order_relaxed); }

  // Engine meters, for interfaces.
  float inputPeak() const noexcept { return engine->meterIn.load(std::memory_order_relaxed); }
  float outputPeak() const noexcept { return engine->meterOut.load(std::memory_order_relaxed); }

  static constexpr int chunk = 256;

private:
  struct Command {
    enum Type : int { none, set, load, morph, tempo } type = none;
    int index = 0;
    float value = 0;
  };
  bool push(const Command &c) noexcept;
  void drain() noexcept;
  void apply(const Command &c) noexcept;
  void buildPreset(int index, std::array<float, count> &values, ExtraState &extra) const noexcept;
  void stepMorph(int frames) noexcept;
  void run(float *left, float *right, int frames) noexcept;

  std::unique_ptr<Engine> engine;
  Modulation modulation;
  Transport transport;
  std::unique_ptr<ExtraState> extra, fromExtra, toExtra;
  std::array<float, count> values{}, fromValues{}, toValues{}, finalValues{}, switchAt{};
  std::array<Range, count> ranges{};
  std::array<std::atomic<float>, count> published{};
  std::atomic<int> preset{-1};
  std::atomic<bool> morphing{false};
  double rate = 48000, bpm = 120;
  int maxBlock = 1024;
  double morphLength = 0, morphElapsed = 0;
  int morphTarget = -1;
  bool morphActive = false;

  static constexpr int queueSize = 1024; // power of two
  std::array<Command, queueSize> queue{};
  std::atomic<uint32_t> head{0}, tail{0};
};

} // namespace ut
