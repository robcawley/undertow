#include "Pulse.h"
namespace ut {
std::array<float, 5> Pulse::orderedCrossovers(const std::array<float, count> &v,
                                              double rate) noexcept {
  std::array<float, 5> f{};
  const float top = std::min(16000.f, (float)rate * .45f);
  for (int i = 0; i < 5; ++i)
    f[(size_t)i] = std::clamp(v[(size_t)(pulse_xover1 + i)], 20.f, top);
  std::sort(f.begin(), f.end());
  for (int i = 3; i >= 0; --i)
    f[(size_t)i] = std::min(f[(size_t)i], f[(size_t)(i + 1)] - 20.f);
  f[0] = std::max(20.f, f[0]);
  for (int i = 1; i < 5; ++i)
    f[(size_t)i] = std::max(f[(size_t)i], f[(size_t)(i - 1)] + 20.f);
  return f;
}
bool Pulse::requested(const std::array<float, count> &v) noexcept {
  if (v[pulse_on] < .5f)
    return false;
  for (int i = 0; i < (int)v[pulse_bands]; ++i)
    if (v[(size_t)(pulse_b1_depth + 7 * i)] > 0)
      return true;
  return v[pulse_focus_on] > .5f && v[pulse_focus_depth] > 0;
}
double Pulse::cycles(int band, const std::array<float, count> &v, double beat,
                     double freeCycles) noexcept {
  const int base = pulse_b1_sync + 7 * band;
  return (v[(size_t)base] > .5f
              ? beat / beats[(size_t)(int)v[(size_t)(base + 1)]]
              : freeCycles) +
         v[(size_t)(base + 5)] / 360. * (v[(size_t)(base + 6)] > .5f ? 16 : 1);
}
float Pulse::waveform(int shape, double phase) noexcept {
  const float p = (float)fraction(phase);
  switch (shape) {
  case 1:
    return p;
  case 2:
    return 1 - p;
  case 3:
    return p < .5f ? 1.f : 0.f;
  case 4: {
    const float x = std::sin(MathConstants<float>::pi * p);
    return x * x * x * x;
  }
  default:
    return .5f - .5f * std::cos(MathConstants<float>::twoPi * p);
  }
}
void Pulse::prepare(double rate, int block) {
  sr = rate;
  controlSample = 0;
  focusEnvelope[0] = focusEnvelope[1] = 0;
  envelopeRelease = std::exp(-1.f / (float)(sr * .6));
  const double spec = sr;
  for (auto &filter : splits)
    filter.prepare(spec);
  for (auto &row : compensation)
    for (auto &filter : row) {
      filter.prepare(spec);
      filter.setType(CrossoverType::allpass);
    }
  lastCutoffs.fill(-1);
  cutoffs.fill(200);
  freePhase.fill(0);
  gains.fill(1);
  for (auto &f : focus) {
    f.reset();
    f.g = 0;
    f.r = 1;
    f.h = 1;
  }
  blend = focusBlend = subBlend = focusGain = 0;
  wasFiltering = false;
  gateStep = 1.f / (float)(sr * .02);
  bands = desiredBands = 3;
  location = desiredLocation = 1;
}
void Pulse::resetFilters() noexcept {
  for (auto &f : splits)
    f.reset();
  for (auto &row : compensation)
    for (auto &f : row)
      f.reset();
  for (auto &f : focus)
    f.reset();
  focusEnvelope[0] = focusEnvelope[1] = 0;
}
void Pulse::begin(const std::array<float, count> &v, const ExtraState &state,
                  const Timeline &time) noexcept {
  targets = v;
  const auto ordered = orderedCrossovers(v, sr);
  for (size_t i = 0; i < 5; ++i)
    crossoverTargets[i].store(ordered[i], std::memory_order_relaxed);
  frame = &state;
  timeline = time;
  enabled = v[pulse_on] > .5f;
  desiredBands = (int)v[pulse_bands];
  desiredLocation = (int)v[pulse_position];
  desiredHarmonics = (int)v[pulse_focus_harm];
  needsBands = false;
  for (int i = 0; i < desiredBands; ++i)
    needsBands |= v[(size_t)(pulse_b1_depth + 7 * i)] > 0;
  needsFocus = v[pulse_focus_on] > .5f && v[pulse_focus_depth] > 0;
  smoothCoefficient =
      1.f -
      std::exp(-1.f / (float)(sr * (.002 + .048 * v[pulse_smooth] / 100)));
  if (!wasFiltering && blend == 0 && focusBlend == 0) {
    bands = desiredBands;
    location = desiredLocation;
    harmonics = desiredHarmonics;
  }
}
void Pulse::tick(const std::array<float, count> &s, int sample) noexcept {
  if ((!enabled || (!needsBands && !needsFocus)) && blend == 0 &&
      focusBlend == 0) {
    gains.fill(1);
    subBlend = 0;
    return;
  }
  const bool moving = location != desiredLocation || bands != desiredBands;
  const bool bandOn = enabled && needsBands && !moving,
             focusOn = enabled && needsFocus && !moving &&
                       harmonics == desiredHarmonics;
  blend = std::clamp(blend + (bandOn ? gateStep : -gateStep), 0.f, 1.f);
  const float oldFocus = focusBlend;
  focusBlend =
      std::clamp(focusBlend + (focusOn ? gateStep : -gateStep), 0.f, 1.f);
  if (oldFocus > 0 && focusBlend == 0) {
    for (auto &f : focus)
      f.reset();
    focusEnvelope[0] = focusEnvelope[1] = 0;
  }
  if (harmonics != desiredHarmonics && focusBlend == 0) {
    for (auto &f : focus)
      f.reset();
    focusEnvelope[0] = focusEnvelope[1] = 0;
    harmonics = desiredHarmonics;
  }
  const float subTarget =
      enabled && needsBands && targets[pulse_sub] > .5f ? 1.f : 0.f;
  subBlend += std::clamp(subTarget - subBlend, -gateStep, gateStep);
  if (moving && blend == 0 && focusBlend == 0) {
    resetFilters();
    bands = desiredBands;
    location = desiredLocation;
    lastCutoffs.fill(-1);
  }
  const bool filtering = blend > 0 || focusBlend > 0;
  if (!filtering && wasFiltering)
    resetFilters();
  wasFiltering = filtering;
  const double beat = timeline.at(sample);
  for (int i = 0; i < 6; ++i) {
    const int base = pulse_b1_sync + 7 * i;
    const double phase = cycles(i, s, beat, freePhase[(size_t)i]);
    phases[(size_t)i].store((float)fraction(phase), std::memory_order_relaxed);
    float wave = waveform((int)targets[(size_t)(base + 4)], phase);
    if (targets[(size_t)(base + 6)] > .5f) {
      const int step = (int)((int64_t)std::floor(phase) % 16 + 16) % 16;
      stepIndex[(size_t)i].store(step, std::memory_order_relaxed);
      wave = (frame->patterns[(size_t)i] & (1u << step)) ? 1.f : 0.f;
    }
    const float depth = enabled && i < bands ? s[(size_t)(base + 3)] / 100 : 0;
    const float target = 1 - depth * (1 - wave);
    gains[(size_t)i] += smoothCoefficient * (target - gains[(size_t)i]);
    levels[(size_t)i].store(gains[(size_t)i], std::memory_order_relaxed);
    freePhase[(size_t)i] += s[(size_t)(base + 2)] / sr;
    // Bound the stopped/free clock without changing its sixteen-step pattern.
    if (freePhase[(size_t)i] >= 16)
      freePhase[(size_t)i] -= 16;
    if (targets[(size_t)base] > .5f)
      freePhase[(size_t)i] =
          std::fmod(beat / beats[(size_t)(int)targets[(size_t)(base + 1)]] +
                        timeline.increment /
                            beats[(size_t)(int)targets[(size_t)(base + 1)]],
                    16.);
  }
  const float fwave =
      waveform(0, beat / beats[(size_t)(int)targets[pulse_focus_div]]);
  const float g =
      enabled && needsFocus
          ? s[pulse_focus_depth] / 100 * (std::pow(10.f, 12.f / 20) - 1) * fwave
          : 0;
  focusGain += smoothCoefficient * (g - focusGain);
  const float effectiveFocus = focusGain * focusBlend;
  focusCompensation =
      1.f / std::sqrt(1 + effectiveFocus * effectiveFocus /
                              (10 * harmonics * s[pulse_focus_q]));
  if ((controlSample++ % 16) == 0 && filtering) {
    cutoffs = orderedCrossovers(s, sr);
    for (int i = 0; i < bands - 1; ++i)
      if (std::abs(cutoffs[(size_t)i] - lastCutoffs[(size_t)i]) > .001f) {
        splits[(size_t)i].setCutoffFrequency(cutoffs[(size_t)i]);
        for (int k = 0; k < i; ++k)
          compensation[(size_t)i][(size_t)k].setCutoffFrequency(
              cutoffs[(size_t)i]);
        lastCutoffs[(size_t)i] = cutoffs[(size_t)i];
      }
    if (focusBlend > 0) {
      const double root =
          32.70319566257483 * std::exp2(targets[pulse_focus_note] / 12.);
      for (int octave = 0; octave < 5; ++octave)
        for (int h = 0; h < harmonics; ++h) {
          auto &f = focus[(size_t)(octave * 8 + h)];
          const double hz = std::min(sr * .4, root * (1 << octave) * (h + 1));
          const double targetG =
                           std::tan(MathConstants<double>::pi * hz / sr),
                       targetR = 1. / s[pulse_focus_q];
          const double slew = 1 - std::exp(-16. / (sr * .02));
          f.g += slew * (targetG - f.g);
          f.r += slew * (targetR - f.r);
          f.h = 1 / (1 + f.r * f.g + f.g * f.g);
        }
    }
  }
}
void Pulse::apply(float *stereo) noexcept {
  if (blend == 0 && focusBlend == 0)
    return;
  for (int ch = 0; ch < 2; ++ch) {
    float x = stereo[ch];
    if (blend > 0) {
      std::array<float, 6> parts{};
      float remaining = x;
      for (int i = 0; i < bands - 1; ++i) {
        float lo, hi;
        splits[(size_t)i].processSample(ch, remaining, lo, hi);
        parts[(size_t)i] = lo;
        remaining = hi;
        for (int k = 0; k < i; ++k)
          parts[(size_t)k] = compensation[(size_t)i][(size_t)k].processSample(
              ch, parts[(size_t)k]);
      }
      parts[(size_t)(bands - 1)] = remaining;
      float sum = 0;
      for (int i = 0; i < bands; ++i)
        sum += parts[(size_t)i] * gains[(size_t)i];
      x += blend * (sum - x);
    }
    if (focusBlend > 0) {
      float sum = 0;
      for (int o = 0; o < 5; ++o)
        for (int h = 0; h < harmonics; ++h)
          sum += focus[(size_t)(o * 8 + h)].run(ch, x);
      const float g = focusGain * focusBlend;
      const float compensationGain = focusCompensation;
      focusEnvelope[ch] =
          std::max(std::abs(x), focusEnvelope[ch] * envelopeRelease);
      const float ceiling = 3.981071706f * focusEnvelope[ch];
      x = std::clamp((x + g * sum / (5 * harmonics)) * compensationGain,
                     -ceiling, ceiling);
    }
    stereo[ch] = x;
  }
}
} // namespace ut
