// UNDERTOW core: small helpers the engine needs, with no dependencies beyond
// the C++ standard library. MIT licence (see LICENSE in this folder).
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

#if defined(__SSE__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP > 0)
#include <xmmintrin.h>
#define UT_X86_DENORMALS 1
#endif

namespace ut {

// Clamp with the bounds first: limit(lo, hi, value).
template <typename T> constexpr T limit(T lo, T hi, T v) noexcept {
  return v < lo ? lo : (hi < v ? hi : v);
}

template <typename T> struct MathConstants {
  static constexpr T pi = static_cast<T>(3.141592653589793238L);
  static constexpr T twoPi = static_cast<T>(2 * 3.141592653589793238L);
};

// Flushes denormals to zero while in scope (FTZ/DAZ on x86, FZ on ARM64).
class NoDenormals {
public:
  NoDenormals() noexcept {
#if defined(UT_X86_DENORMALS)
    saved = _mm_getcsr();
    _mm_setcsr(saved | 0x8040);
#elif defined(__aarch64__) && (defined(__GNUC__) || defined(__clang__))
    asm volatile("mrs %0, fpcr" : "=r"(saved));
    const uint64_t on = saved | (uint64_t(1) << 24);
    asm volatile("msr fpcr, %0" : : "r"(on));
#endif
  }
  ~NoDenormals() noexcept {
#if defined(UT_X86_DENORMALS)
    _mm_setcsr(saved);
#elif defined(__aarch64__) && (defined(__GNUC__) || defined(__clang__))
    asm volatile("msr fpcr, %0" : : "r"(saved));
#endif
  }
  NoDenormals(const NoDenormals &) = delete;
  NoDenormals &operator=(const NoDenormals &) = delete;

private:
#if defined(UT_X86_DENORMALS)
  unsigned int saved = 0;
#else
  uint64_t saved = 0;
#endif
};

// A non-owning view of planar audio: channel pointers, channel count, length.
struct AudioView {
  float *const *channels = nullptr;
  int numChannels = 0, numSamples = 0;

  int getNumSamples() const noexcept { return numSamples; }
  int getNumChannels() const noexcept { return numChannels; }
  float getSample(int ch, int i) const noexcept { return channels[ch][i]; }
  void setSample(int ch, int i, float x) noexcept { channels[ch][i] = x; }
  void copyFrom(int destCh, int destStart, const AudioView &src, int srcCh,
                int srcStart, int n) noexcept {
    std::copy_n(src.channels[srcCh] + srcStart, n, channels[destCh] + destStart);
  }
};

// Linkwitz-Riley 24 dB/oct crossover for two channels, built from two
// Butterworth state-variable stages (topology-preserving transform).
// lowpass and highpass are 4th order; allpass is the matching 2nd-order
// allpass (the phase of low + high), used to keep bands aligned.
enum class CrossoverType { lowpass, highpass, allpass };

class Crossover {
public:
  void prepare(double rate) noexcept {
    sampleRate = rate;
    update();
    reset();
  }
  void reset() noexcept {
    s1.fill(0);
    s2.fill(0);
    s3.fill(0);
    s4.fill(0);
  }
  void setType(CrossoverType t) noexcept { type = t; }
  void setCutoffFrequency(float hz) noexcept {
    cutoff = hz;
    update();
  }
  float getCutoffFrequency() const noexcept { return cutoff; }

  float processSample(int ch, float x) noexcept {
    const size_t c = (size_t)ch;
    float yB, yL;
    const float yH = stage(x, s1[c], s2[c], yB, yL);
    if (type == CrossoverType::allpass)
      return yL - root2 * yB + yH;
    float yB2, yL2;
    const float yH2 = stage(type == CrossoverType::lowpass ? yL : yH, s3[c],
                            s4[c], yB2, yL2);
    return type == CrossoverType::lowpass ? yL2 : yH2;
  }

  // Both bands at once: low is the 4th-order lowpass, high is the allpass
  // minus low, which is the matching 4th-order highpass. low + high is flat
  // in level.
  void processSample(int ch, float x, float &low, float &high) noexcept {
    const size_t c = (size_t)ch;
    float yB, yL;
    const float yH = stage(x, s1[c], s2[c], yB, yL);
    float yB2, yL2;
    stage(yL, s3[c], s4[c], yB2, yL2);
    low = yL2;
    high = yL - root2 * yB + yH - yL2;
  }

private:
  float stage(float x, float &a, float &b, float &band, float &low) const noexcept {
    const float high = (x - (root2 + g) * a - b) * h;
    band = g * high + a;
    a = g * high + band;
    low = g * band + b;
    b = g * band + low;
    return high;
  }
  void update() noexcept {
    g = (float)std::tan(MathConstants<double>::pi * (double)cutoff / sampleRate);
    h = (float)(1.0 / (1.0 + (double)(root2 * g) + (double)(g * g)));
  }

  static constexpr float root2 = 1.41421356237309504880f;
  CrossoverType type = CrossoverType::lowpass;
  double sampleRate = 44100;
  float cutoff = 2000, g = 0, h = 0;
  std::array<float, 2> s1{}, s2{}, s3{}, s4{};
};

} // namespace ut
