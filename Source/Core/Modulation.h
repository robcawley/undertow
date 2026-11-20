#pragma once
#include "State.h"
#include "Transport.h"
namespace ut {
struct EffectiveSnapshot {
  std::array<float, count> values{}, normalized{};
  float x = .5f, y = .5f;
};
class Modulation {
public:
  Modulation() noexcept {
    for (size_t k = 0; k < count; ++k)
      ranges[k] = Range::of(specs[k]);
  }
  EffectiveSnapshot evaluate(const std::array<float, count> &a,
                             const ExtraState &frame,
                             const Timeline &time) const noexcept;
  static std::array<float, 4> weights(float x, float y) noexcept;

private:
  std::array<Range, count> ranges;
};
} // namespace ut
