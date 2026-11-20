#include "Processor.h"
#include <cstring>

namespace ut {
namespace {

const PresetDef &presetDef(int index) noexcept {
  const auto &game = gamePresets();
  if (index < (int)game.size())
    return game[(size_t)index];
  return factoryPresets()[(size_t)(index - (int)game.size())];
}

// The plain value the plug-in's audio thread sees once a host has set a
// parameter to a normalised value; reproduced so the same preset sounds the
// same in both.
float settle(const Range &r, const Spec &s, float normalised) noexcept {
  normalised = limit(0.f, 1.f, normalised);
  if (s.kind == 1)
    return normalised >= .5f ? 1.f : 0.f;
  if (s.kind == 0) {
    const float v = r.convertFrom0to1(normalised);
    return r.convertFrom0to1(r.convertTo0to1(v));
  }
  return std::round(r.convertFrom0to1(normalised));
}

// A section switch and the levels that bring it in. During a morph a section
// that turns on starts with these at zero; one that turns off takes them to
// zero first and switches off at the end.
struct Fade {
  int section;
  std::array<int, 6> levels;
  int used;
};
constexpr Fade fades[]{
    {drift_on, {drift_mix}, 1},
    {echo_on, {echo_level}, 1},
    {void_on, {void_level}, 1},
    {pulse_on, {pulse_b1_depth, pulse_b2_depth, pulse_b3_depth, pulse_b4_depth, pulse_b5_depth, pulse_b6_depth}, 6},
    {pulse_focus_on, {pulse_focus_depth}, 1},
    {mut_on, {harm_octave, harm_fifth, harm_odd, harm_sub, halo_climb, evolve_amount}, 6},
};

} // namespace

Processor::Processor()
    : engine(std::make_unique<Engine>()), extra(std::make_unique<ExtraState>()),
      fromExtra(std::make_unique<ExtraState>()), toExtra(std::make_unique<ExtraState>()) {
  for (size_t k = 0; k < count; ++k) {
    ranges[k] = Range::of(specs[k]);
    const float normal = ranges[k].convertTo0to1(specs[k].initial);
    values[k] = settle(ranges[k], specs[k], normal);
    extra->sceneB[k] = normal;
    published[k].store(values[k], std::memory_order_relaxed);
  }
  prepare(rate, maxBlock);
}

Processor::~Processor() = default;

void Processor::prepare(double sampleRate, int block) {
  rate = sampleRate > 0 ? sampleRate : 48000;
  maxBlock = std::max(1, block);
  engine->prepare(rate, chunk);
  transport.prepare(rate);
  morphActive = false;
  morphing.store(false, std::memory_order_relaxed);
}

// ---- Presets and parameters

int Processor::gamePresetCount() noexcept { return (int)gamePresets().size(); }
int Processor::presetCount() noexcept {
  return (int)(gamePresets().size() + factoryPresets().size());
}
const char *Processor::presetName(int index) noexcept {
  return index >= 0 && index < presetCount() ? presetDef(index).name : "";
}
const char *Processor::presetCategory(int index) noexcept {
  return index >= 0 && index < presetCount() ? presetDef(index).category : "";
}
void Processor::presetValues(int index, float *out) noexcept {
  if (out == nullptr)
    return;
  index = limit(0, presetCount() - 1, index);
  std::array<float, count> a{};
  for (size_t k = 0; k < count; ++k)
    a[k] = Range::of(specs[k]).convertTo0to1(specs[k].initial);
  for (const auto &v : presetDef(index).a)
    a[(size_t)v.param] = Range::of(specs[(size_t)v.param]).convertTo0to1(v.value);
  for (size_t k = 0; k < count; ++k)
    out[k] = settle(Range::of(specs[k]), specs[k], a[k]);
}

int Processor::findPreset(const char *name) noexcept {
  if (name == nullptr)
    return -1;
  for (int i = 0; i < presetCount(); ++i)
    if (std::strcmp(presetDef(i).name, name) == 0)
      return i;
  return -1;
}
const Spec &Processor::parameterSpec(int index) noexcept {
  return specs[(size_t)limit(0, count - 1, index)];
}
int Processor::findParameter(const char *id) noexcept {
  if (id == nullptr)
    return -1;
  for (int k = 0; k < count; ++k)
    if (std::strcmp(specs[(size_t)k].id, id) == 0)
      return k;
  return -1;
}
float Processor::parameter(int index) const noexcept {
  return index >= 0 && index < count ? published[(size_t)index].load(std::memory_order_relaxed) : 0;
}

void Processor::buildPreset(int index, std::array<float, count> &out,
                            ExtraState &state) const noexcept {
  const auto &def = presetDef(index);
  std::array<float, count> a{};
  for (size_t k = 0; k < count; ++k)
    a[k] = ranges[k].convertTo0to1(specs[k].initial);
  for (const auto &v : def.a)
    a[(size_t)v.param] = ranges[(size_t)v.param].convertTo0to1(v.value);
  // RETURN belongs to the player, not the preset.
  a[xy_return] = ranges[xy_return].convertTo0to1(values[xy_return]);
  for (size_t k = 0; k < count; ++k) {
    out[k] = settle(ranges[k], specs[k], a[k]);
    state.sceneB[k] = a[k];
  }
  for (const auto &v : def.b)
    state.sceneB[(size_t)v.param] = ranges[(size_t)v.param].convertTo0to1(v.value);
  for (auto &corner : state.corners)
    corner.fill(0);
  for (size_t c = 0; c < 4; ++c)
    for (const auto &v : def.corners[c])
      state.corners[c][(size_t)v.param] = limit(-1.f, 1.f, v.value);
  state.patterns = def.patterns;
  state.motionCount = 0;
  state.preset = index;
  state.edited = false;
}

// ---- Control queue (single producer, the audio thread consumes)

bool Processor::push(const Command &c) noexcept {
  const uint32_t h = head.load(std::memory_order_relaxed);
  if (h - tail.load(std::memory_order_acquire) >= (uint32_t)queueSize)
    return false;
  queue[h & (queueSize - 1)] = c;
  head.store(h + 1, std::memory_order_release);
  return true;
}

void Processor::drain() noexcept {
  uint32_t t = tail.load(std::memory_order_relaxed);
  const uint32_t h = head.load(std::memory_order_acquire);
  while (t != h) {
    apply(queue[t & (queueSize - 1)]);
    ++t;
  }
  tail.store(t, std::memory_order_release);
}

bool Processor::setParameter(int index, float value) noexcept {
  if (index < 0 || index >= count || !std::isfinite(value))
    return false;
  return push({Command::set, index, value});
}
bool Processor::loadPreset(int index) noexcept {
  return index >= 0 && index < presetCount() && push({Command::load, index, 0});
}
bool Processor::morphTo(int index, float seconds) noexcept {
  return index >= 0 && index < presetCount() && std::isfinite(seconds) &&
         push({Command::morph, index, std::max(0.f, seconds)});
}
bool Processor::setTempo(double tempo) noexcept {
  return std::isfinite(tempo) && push({Command::tempo, 0, (float)tempo});
}
bool Processor::setFreeze(bool on) noexcept { return setParameter(void_freeze, on ? 1.f : 0.f); }

void Processor::apply(const Command &c) noexcept {
  switch (c.type) {
  case Command::set: {
    const size_t k = (size_t)c.index;
    const float v = settle(ranges[k], specs[k], ranges[k].convertTo0to1(c.value));
    values[k] = fromValues[k] = toValues[k] = finalValues[k] = v;
    break;
  }
  case Command::load:
    buildPreset(c.index, values, *extra);
    morphActive = false;
    preset.store(c.index, std::memory_order_relaxed);
    break;
  case Command::morph: {
    if (c.value <= 0) {
      apply({Command::load, c.index, 0});
      break;
    }
    fromValues = values;
    *fromExtra = *extra;
    buildPreset(c.index, finalValues, *toExtra); // finalValues: the true end values
    toValues = finalValues;
    for (size_t k = 0; k < count; ++k)
      switchAt[k] = .5f;
    for (const auto &f : fades) {
      const bool fromOn = fromValues[(size_t)f.section] > .5f,
                 toOn = toValues[(size_t)f.section] > .5f;
      if (!fromOn && toOn) {
        switchAt[(size_t)f.section] = 0;
        for (int i = 0; i < f.used; ++i)
          fromValues[(size_t)f.levels[(size_t)i]] = specs[(size_t)f.levels[(size_t)i]].lo;
      } else if (fromOn && !toOn) {
        switchAt[(size_t)f.section] = 1;
        for (int i = 0; i < f.used; ++i)
          toValues[(size_t)f.levels[(size_t)i]] = specs[(size_t)f.levels[(size_t)i]].lo;
      }
    }
    morphLength = (double)c.value * rate;
    morphElapsed = 0;
    morphTarget = c.index;
    morphActive = true;
    break;
  }
  case Command::tempo:
    bpm = limit(20., 400., (double)c.value);
    break;
  default:
    break;
  }
}

void Processor::stepMorph(int frames) noexcept {
  morphElapsed += frames;
  const float t = (float)std::min(1.0, morphElapsed / morphLength),
              e = t * t * (3 - 2 * t);
  if (t >= 1) {
    values = finalValues;
    *extra = *toExtra;
    morphActive = false;
    preset.store(morphTarget, std::memory_order_relaxed);
    return;
  }
  for (size_t k = 0; k < count; ++k) {
    if (specs[k].kind == 0) {
      const float a = ranges[k].convertTo0to1(fromValues[k]),
                  b = ranges[k].convertTo0to1(toValues[k]);
      values[k] = ranges[k].convertFrom0to1(a + e * (b - a));
    } else
      values[k] = t >= switchAt[k] ? finalValues[k] : fromValues[k];
  }
  for (size_t k = 0; k < count; ++k)
    extra->sceneB[k] = fromExtra->sceneB[k] + e * (toExtra->sceneB[k] - fromExtra->sceneB[k]);
  for (size_t c = 0; c < 4; ++c)
    for (size_t k = 0; k < count; ++k)
      extra->corners[c][k] =
          fromExtra->corners[c][k] + e * (toExtra->corners[c][k] - fromExtra->corners[c][k]);
  extra->patterns = t >= .5f ? toExtra->patterns : fromExtra->patterns;
  extra->motionCount = 0;
}

// ---- Audio

void Processor::run(float *left, float *right, int frames) noexcept {
  for (int i = 0; i < frames; ++i) {
    if (!std::isfinite(left[i]))
      left[i] = 0;
    if (!std::isfinite(right[i]))
      right[i] = 0;
    left[i] = limit(-8.f, 8.f, left[i]);
    right[i] = limit(-8.f, 8.f, right[i]);
  }
  if (morphActive)
    stepMorph(frames);
  const auto time = transport.begin(frames, bpm, 0, false, false, 4);
  const auto snapshot = modulation.evaluate(values, *extra, time);
  float *channels[2]{left, right};
  AudioView view{channels, 2, frames};
  engine->process(view, 2, snapshot.values, *extra, time);
}

void Processor::process(float *left, float *right, int frames) noexcept {
  drain();
  for (int pos = 0; pos < frames; pos += chunk)
    run(left + pos, right + pos, std::min(chunk, frames - pos));
  for (size_t k = 0; k < count; ++k)
    published[k].store(values[k], std::memory_order_relaxed);
  morphing.store(morphActive, std::memory_order_relaxed);
}

void Processor::processInterleaved(float *stereo, int frames) noexcept {
  float left[chunk], right[chunk];
  drain();
  for (int pos = 0; pos < frames; pos += chunk) {
    const int n = std::min(chunk, frames - pos);
    for (int i = 0; i < n; ++i) {
      left[i] = stereo[2 * (pos + i)];
      right[i] = stereo[2 * (pos + i) + 1];
    }
    run(left, right, n);
    for (int i = 0; i < n; ++i) {
      stereo[2 * (pos + i)] = left[i];
      stereo[2 * (pos + i) + 1] = right[i];
    }
  }
  for (size_t k = 0; k < count; ++k)
    published[k].store(values[k], std::memory_order_relaxed);
  morphing.store(morphActive, std::memory_order_relaxed);
}

} // namespace ut
