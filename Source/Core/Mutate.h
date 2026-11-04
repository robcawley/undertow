#pragma once
#include "Specs.h"
#include <cmath>
#include <vector>
namespace ut {
// Two crossfaded read heads sweeping a short delay line: a simple, cheap pitch
// shifter. Each head's window is a raised cosine and the two are half a
// window apart, so their gains always sum to one.
struct Shifter {
  std::vector<float> buffer;
  int write = 0, window = 2880;
  double phase = 0;
  void prepare(double sr, double seconds = .06) {
    window = std::max(64, (int)(sr * seconds));
    buffer.assign((size_t)(window * 2 + 8), 0.f);
    write = 0;
    phase = 0;
  }
  void reset() noexcept {
    std::fill(buffer.begin(), buffer.end(), 0.f);
    phase = 0;
  }
  float process(float x, double ratio) noexcept {
    const int size = (int)buffer.size();
    buffer[(size_t)write] = x;
    phase += (1.0 - ratio) / window;
    phase -= std::floor(phase);
    float out = 0;
    for (int h = 0; h < 2; ++h) {
      double p = phase + .5 * h;
      p -= std::floor(p);
      const double delay = 2 + p * window;
      double pos = write - delay;
      while (pos < 0) pos += size;
      const int i = (int)pos;
      const float t = (float)(pos - i);
      const float a = buffer[(size_t)i], b = buffer[(size_t)((i + 1) % size)];
      const float g = .5f - .5f * std::cos(6.2831853f * (float)p);
      out += g * (a + t * (b - a));
    }
    write = (write + 1) % size;
    return out;
  }
};
// State-variable band-pass for the moving formants.
struct Band {
  float g = 0, k = .1f, a1 = 0, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0;
  void set(float hz, float q, double sr) noexcept {
    g = std::tan(3.14159265f * std::min(hz, (float)sr * .45f) / (float)sr);
    k = 1.f / q;
    a1 = 1.f / (1.f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
  }
  float run(float x) noexcept {
    const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2 * v1 - ic1;
    ic2 = 2 * v2 - ic2;
    return v1;
  }
  void reset() noexcept { ic1 = ic2 = 0; }
};
// Smooth, repeatable random wander in -1..1 keyed to a phase.
inline float wanderAt(double phase, uint64_t seed) noexcept {
  auto hash = [](int64_t index, uint64_t s) {
    uint64_t x = (uint64_t)index + s;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return (float)(x >> 40) / 8388607.5f - 1.f;
  };
  const auto index = (int64_t)std::floor(phase);
  const float t = (float)(phase - std::floor(phase)), s = t * t * (3 - 2 * t);
  const float a = hash(index, seed);
  return a + s * (hash(index + 1, seed) - a);
}
} // namespace ut
