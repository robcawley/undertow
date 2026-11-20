#pragma once
#include <algorithm>
#include <cmath>
namespace ut {
inline double fraction(double x) noexcept { return x - std::floor(x); }
struct Timeline {
  double beat = 0, increment = 120.0 / (60 * 48000), bpm = 120, barBeats = 4;
  bool discontinuity = false, playing = false;
  double at(int sample) const noexcept { return beat + sample * increment; }
};
class Transport {
public:
  void prepare(double rate) noexcept {
    sr = rate;
    end = 0;
    tempo = 120;
    bars = 4;
    valid = false;
    wasPlaying = false;
  }
  Timeline begin(int samples, double bpm, double ppq, bool playing,
                 bool hasPosition, double barBeats = 4) noexcept {
    if (std::isfinite(bpm))
      bpm = std::clamp(bpm, 20., 400.);
    else
      bpm = tempo;
    Timeline t;
    t.bpm = bpm;
    t.increment = bpm / (60 * sr);
    t.barBeats = std::isfinite(barBeats) ? std::clamp(barBeats, 1., 32.) : 4;
    t.playing = playing;
    t.beat = playing && hasPosition && std::isfinite(ppq) ? ppq : end;
    t.discontinuity = valid && (std::abs(t.beat - end) > 1.e-7 ||
                                bpm != tempo || playing != wasPlaying);
    end = t.at(samples);
    tempo = bpm;
    bars = t.barBeats;
    valid = true;
    wasPlaying = playing;
    return t;
  }

private:
  double sr = 48000, end = 0, tempo = 120, bars = 4;
  bool valid = false, wasPlaying = false;
};
} // namespace ut
