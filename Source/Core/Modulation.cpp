#include "Modulation.h"
namespace ut {
namespace {
float hash(int64_t index, uint64_t seed) noexcept {
  uint64_t x = (uint64_t)index + seed;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  x ^= x >> 31;
  return (float)(x >> 40) / 8388607.5f - 1.f;
}
float wander(double phase, uint64_t seed) noexcept {
  const auto index = (int64_t)std::floor(phase);
  const float t = (float)fraction(phase), s = t * t * (3 - 2 * t);
  const float a = hash(index, seed);
  return a + s * (hash(index + 1, seed) - a);
}
} // namespace
std::array<float, 4> Modulation::weights(float x, float y) noexcept {
  // Centre-neutral bilinear quadrants: A upper left, then clockwise.
  const float l = std::max(0.f, 1 - 2 * x), r = std::max(0.f, 2 * x - 1),
              up = std::max(0.f, 2 * y - 1), down = std::max(0.f, 1 - 2 * y);
  return {l * up, r * up, r * down, l * down};
}
EffectiveSnapshot Modulation::evaluate(const std::array<float, count> &a,
                                       const ExtraState &frame,
                                       const Timeline &time) const noexcept {
  EffectiveSnapshot out;
  out.values = a;
  std::array<float, count> base{}, offset{}, controlNormal{};
  for (size_t k = 0; k < count; ++k)
    base[k] = ranges[k].convertTo0to1(a[k]);
  auto morph = [&](float amount) {
    for (int k = 0; k < count; ++k) {
      const size_t i = (size_t)k;
      out.normalized[i] =
          snapshotParameter(k) && amount != 0
              ? (amount == 1 ? frame.sceneB[i]
                             : base[i] + amount * (frame.sceneB[i] - base[i]))
              : base[i];
    }
  };
  auto macro = [&](int k) {
    return ranges[(size_t)k].convertFrom0to1(
               std::clamp(out.normalized[(size_t)k], 0.f, 1.f)) /
           100;
  };
  auto point = [&](const std::array<float, count> &c, float chaos) {
    float x = c[xy_x], y = c[xy_y];
    const double cycle = time.beat / beats[(size_t)(int)c[xy_rate]];
    if (c[xy_motion_play] > .5f && frame.motionCount > 1) {
      const int length = 1 << (int)c[xy_motion_bars];
      const float bar =
          (float)(fraction(time.beat / (time.barBeats * length)) * length);
      const int points = std::clamp(frame.motionCount, 2, 1024);
      int next = 0;
      while (next < points && frame.motion[(size_t)next].bar <= bar)
        ++next;
      const int left = next == 0 ? points - 1 : next - 1,
                right = next == points ? 0 : next;
      const auto &p = frame.motion[(size_t)left];
      const auto &q = frame.motion[(size_t)right];
      const float begin = p.bar - (next == 0 ? length : 0),
                  end = q.bar + (next == points ? length : 0);
      const float t =
          end > begin ? std::clamp((bar - begin) / (end - begin), 0.f, 1.f) : 0;
      x = p.x + t * (q.x - p.x) + (c[xy_x] - a[xy_x]);
      y = p.y + t * (q.y - p.y) + (c[xy_y] - a[xy_y]);
    } else {
      const float radius = c[xy_radius] * .005f,
                  angle = c[xy_angle] * MathConstants<float>::pi / 180;
      switch ((int)c[xy_current]) {
      case 1:
        x += radius *
             std::cos(MathConstants<double>::twoPi * fraction(cycle));
        y += radius *
             std::sin(MathConstants<double>::twoPi * fraction(cycle));
        break;
      case 2: {
        const float s =
            std::sin(MathConstants<double>::twoPi * fraction(cycle));
        x += radius * s * std::cos(angle);
        y += radius * s * std::sin(angle);
        break;
      }
      case 3:
        x += radius * wander(cycle, 731);
        y += radius * wander(cycle, 1907);
        break;
      default:
        break;
      }
      if (chaos != 0) {
        x += .12f * chaos * wander(cycle, 419);
        y += .12f * chaos * wander(cycle, 2341);
      }
    }
    return std::array<float, 2>{std::clamp(x, 0.f, 1.f),
                                std::clamp(y, 0.f, 1.f)};
  };
  morph(std::clamp(a[bloom] / 100, 0.f, 1.f));
  const float initialChaos = macro(macro_chaos);
  const auto sourcePoint = point(a, initialChaos);
  const auto w = weights(sourcePoint[0], sourcePoint[1]);
  for (int k = 0; k < count; ++k)
    if (specs[(size_t)k].kind == 0)
      for (int corner = 0; corner < 4; ++corner)
        offset[(size_t)k] +=
            w[(size_t)corner] *
            (a[(size_t)(xy_flip_a + corner)] > .5f ? -1.f : 1.f) *
            frame.corners[(size_t)corner][(size_t)k];
  auto isControl = [](int k) {
    return k == bloom || k == xy_x || k == xy_y || k == xy_amount ||
           k == xy_radius || k == xy_angle;
  };
  auto controls = a;
  controlNormal = base;
  // Routing controls take one prepass. Their weights are sampled before their
  // own offsets, avoiding recursive geometry or morph feedback.
  for (int k = 0; k < count; ++k)
    if (isControl(k)) {
      const size_t i = (size_t)k;
      controlNormal[i] =
          std::clamp(base[i] + offset[i] * a[xy_amount] / 100, 0.f, 1.f);
      if (controlNormal[i] != base[i])
        controls[i] = ranges[i].convertFrom0to1(controlNormal[i]);
    }
  morph(std::clamp(controls[bloom] / 100, 0.f, 1.f));
  for (int k = 0; k < count; ++k)
    if (specs[(size_t)k].kind == 0) {
      const size_t i = (size_t)k;
      out.normalized[i] =
          isControl(k)
              ? controlNormal[i]
              : out.normalized[i] + offset[i] * controls[xy_amount] / 100;
    }
  auto add = [&](int k, float delta) { out.normalized[(size_t)k] += delta; };
  const float depth = macro(macro_depth), motion = macro(macro_motion),
              glow = macro(macro_glow), chaos = macro(macro_chaos);
  add(void_size, .20f * depth);
  add(void_decay, .30f * depth);
  add(void_predelay, .12f * depth);
  add(echo_feedback, .18f * depth);
  add(drift_depth, .35f * motion);
  add(drift_wow, .25f * motion);
  add(void_mod, .35f * motion);
  for (int band = 0; band < 6; ++band) {
    add(pulse_b1_depth + 7 * band, .35f * motion);
    add(pulse_b1_phase + 7 * band, .30f * chaos * hash(band, 697));
  }
  add(void_damp, -.30f * glow);
  add(echo_tone, .25f * glow);
  add(echo_lowcut, .18f * glow);
  add(void_lowcut, .18f * glow);
  add(echo_age, .35f * chaos);
  const float mutate = macro(macro_mutate);
  add(harm_octave, .30f * mutate);
  add(harm_odd, .20f * mutate);
  add(halo_climb, .35f * mutate);
  add(evolve_amount, .45f * mutate);
  add(evolve_formant, .25f * mutate);
  for (int k = 0; k < count; ++k) {
    const size_t i = (size_t)k;
    out.normalized[i] = std::clamp(out.normalized[i], 0.f, 1.f);
    if (specs[i].kind == 0 && out.normalized[i] != base[i])
      out.values[i] = ranges[i].convertFrom0to1(out.normalized[i]);
  }
  const bool changed = controls[xy_x] != a[xy_x] || controls[xy_y] != a[xy_y] ||
                       controls[xy_radius] != a[xy_radius] ||
                       controls[xy_angle] != a[xy_angle] ||
                       chaos != initialChaos;
  const auto finalPoint = changed ? point(controls, chaos) : sourcePoint;
  out.x = finalPoint[0];
  out.y = finalPoint[1];
  out.values[xy_x] = out.normalized[xy_x] = out.x;
  out.values[xy_y] = out.normalized[xy_y] = out.y;
  return out;
}
} // namespace ut
