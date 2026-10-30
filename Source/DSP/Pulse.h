#pragma once
#include "../State.h"
#include "../Transport.h"
#include <juce_dsp/juce_dsp.h>
namespace ut {
class Pulse {
public:
  static std::array<float, 5>
  orderedCrossovers(const std::array<float, count> &values,
                    double rate) noexcept;
  static bool requested(const std::array<float, count> &values) noexcept;
  static double cycles(int band, const std::array<float, count> &values,
                       double beat, double freeCycles) noexcept;
  static float waveform(int shape, double phase) noexcept;
  void prepare(double rate, int block);
  void begin(const std::array<float, count> &values, const ExtraState &state,
             const Timeline &time) noexcept;
  void tick(const std::array<float, count> &smoothed, int sample) noexcept;
  void apply(float *stereo) noexcept;
  float subGain() const noexcept { return 1 + subBlend * (gains[0] - 1); }
  int position() const noexcept { return location; }
  std::array<std::atomic<float>, 5> crossoverTargets{};
  std::array<std::atomic<float>, 6> phases{}, levels{};
  std::array<std::atomic<int>, 6> stepIndex{};

private:
  struct Resonator {
    double one[2]{}, two[2]{}, g = 0, r = 1, h = 1;
    float run(int ch, float x) noexcept {
      const double high = (x - (r + g) * one[ch] - two[ch]) * h,
                   band = g * high + one[ch], low = g * band + two[ch];
      one[ch] = std::clamp(g * high + band, -128., 128.);
      two[ch] = std::clamp(g * band + low, -128., 128.);
      return (float)(r * band);
    }
    void reset() noexcept { one[0] = one[1] = two[0] = two[1] = 0; }
  };
  void resetFilters() noexcept;
  double sr = 48000;
  uint32_t controlSample = 0;
  Timeline timeline;
  const ExtraState *frame = nullptr;
  std::array<float, count> targets{};
  std::array<juce::dsp::LinkwitzRileyFilter<float>, 5> splits;
  std::array<std::array<juce::dsp::LinkwitzRileyFilter<float>, 5>, 5>
      compensation;
  std::array<float, 5> cutoffs{}, lastCutoffs{};
  std::array<Resonator, 40> focus;
  std::array<double, 6> freePhase{};
  std::array<float, 6> gains{1, 1, 1, 1, 1, 1};
  int bands = 3, desiredBands = 3, location = 1, desiredLocation = 1,
      harmonics = 4, desiredHarmonics = 4;
  float blend = 0, focusBlend = 0, subBlend = 0, focusGain = 0;
  float focusCompensation = 1, focusEnvelope[2]{}, envelopeRelease = 1;
  float smoothCoefficient = .01f, gateStep = .001f;
  bool enabled = false, needsBands = false, needsFocus = false,
       wasFiltering = false;
};
} // namespace ut
