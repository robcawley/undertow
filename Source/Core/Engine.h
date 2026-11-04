#pragma once
#include "Specs.h"
#include "Pulse.h"
#include "Mutate.h"
#include "Util.h"
#include <atomic>
#include <vector>
namespace ut {
struct Delay {
  std::vector<float> data;
  int write = 0, filled = 0;
  void prepare(double sr, double seconds) {
    data.assign((size_t)(sr * seconds) + 8, 0);
    reset();
  }
  void reset() noexcept { write = filled = 0; }
  float read(double samples) const noexcept {
    const double d = limit(1., (double)data.size() - 3, samples);
    if (d + 1 > filled)
      return 0;
    double pos = (double)write - (double)d;
    if (pos < 0)
      pos += (double)data.size();
    const int i = (int)pos;
    const float t = (float)(pos - (double)i);
    return data[(size_t)i] +
           t * (data[(size_t)((i + 1) % (int)data.size())] - data[(size_t)i]);
  }
  void push(float x) noexcept {
    data[(size_t)write] = x;
    write = (write + 1) % (int)data.size();
    filled = std::min(filled + 1, (int)data.size());
  }
};
struct Filter {
  float low = 0, dc = 0;
  float run(float x, float lp, float hp) noexcept {
    low += lp * (x - low);
    dc += hp * (low - dc);
    return low - dc;
  }
};
class Engine {
public:
  // Brings the reverb return up to roughly the level of what feeds it.
  static constexpr float spaceMakeup = 6.2f;
  void prepare(double sampleRate, int block);
  void process(AudioView &, int inputs,
               const std::array<float, count> &, const ExtraState &,
               const Timeline &) noexcept;
  Pulse pulse;
  std::atomic<float> driftPhase{0}, echoPhase{0};
  std::atomic<float> meterIn{0}, meterOut{0}, duckMeter{0};
  // Mono wet signal for the editor's spectrum (written per sample, read by
  // the message thread; relaxed atomics so the audio thread never waits).
  // MUTATE telemetry for the editor
  std::array<std::atomic<float>, 3> formantHz{};
  std::atomic<float> evolveX{0}, evolveY{0}, mutateGate{0};
  static constexpr int scopeSize = 2048;
  std::array<std::atomic<float>, scopeSize> scope{};
  std::atomic<uint32_t> scopeWrite{0};

private:
  double sr = 48000, phase = 0;
  uint32_t scopeAt = 0;
  float smoothing = 0, duckEnv = 0, freeze = 0, gravityEnv = 0;
  std::array<float, count> smooth{};
  bool first = true;
  float anchorCutoff = -1, bypassBlend = 0;
  std::array<Delay, 2> drift, echo, pre, late;
  std::array<Delay, 8> tank, diffusers;
  std::array<float, 8> loopGains{};
  std::array<float, 5> filterCoefficients{};
  std::array<Filter, 2> echoFilters, wetFilters;
  std::array<float, 8> damp{};
  std::array<float, 8> frozenLengths{};
  Crossover anchor, wetHigh;
  float gates[3]{};
  double echoTime = 0, reversePhase = 0, wowPhase = 0, flutterPhase = 0,
         agePhase = 0;
  std::array<double, 8> modPhases{};
  int previousMode = 0, oldMode = 0;
  float modeFade = 1;
  double lastReadDelay = 0;
  double previousEchoDelay = 0, previousReversePhase = 0, previousAgePhase = 0,
         oldEchoIncrement = 0, oldAgeIncrement = 0, echoIncrement = 0,
         lastBeatIncrement = 0;
  int retimeMode = 0, lastEchoDivision = -1;
  bool lastEchoSync = false;
  float echoRetime = 1, driftDelay[2]{}, driftSlew = 0;
  // ---- MUTATE
  std::array<Shifter, 2> fifthShift, haloShift;
  std::array<std::array<Band, 3>, 2> formants;
  float haloEnv = 0, haloAttack = 0, haloRelease = 0;
  float spaceGuard = 1, guardRelease = 0;
  static constexpr float spaceCeiling = .708f; // -3 dBFS
  float formEnvIn[2]{}, formEnvOut[2]{}, formEnvCoef = 0;
  float frozenPow = 0, frozenRef = 0, frozenCoef = 0, frozenGain = 1, frozenGlide = 0;
  int frozenCount = 0;
  static constexpr float haloCeiling = .035f;
  float mutGate = 0, harmEnv[2]{}, harmDc[2]{}, harmLp[2]{}, subPrev[2]{},
        subLp[2]{}, haloPrev[2]{}, haloLp[2]{};
  bool subFlip[2]{};
  float envAttack = 0, envRelease = 0, dcCoef = 0, subTrack = 0, subSmooth = 0,
        haloTone = 0, evolveGlide = 0;
  float evolveSize = 0, evolveTone = 0, evolveForm = .5f, targetSize = 0,
        targetTone = 0, targetForm = .5f;
};
} // namespace ut
