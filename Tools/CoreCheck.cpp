// Builds and runs the UNDERTOW core with no JUCE: every factory preset's
// scene A through the engine, checking the output stays finite and bounded.
#include "Engine.h"
#include "Modulation.h"
#include "Presets.h"
#include <cstdio>
#include <memory>
#include <vector>

int main() {
  using namespace ut;
  int failures = 0;
  const double rate = 48000;
  const int block = 256;
  auto engine = std::make_unique<Engine>();
  Modulation modulation;
  auto extra = std::make_unique<ExtraState>();
  for (size_t k = 0; k < count; ++k)
    extra->sceneB[k] = Range::of(specs[k]).convertTo0to1(specs[k].initial);
  const auto &bank = factoryPresets();
  for (size_t p = 0; p < bank.size(); ++p) {
    engine->prepare(rate, block);
    Transport transport;
    transport.prepare(rate);
    std::array<float, count> values{};
    for (size_t k = 0; k < count; ++k)
      values[k] = specs[k].initial;
    for (const auto &v : bank[p].a)
      values[(size_t)v.param] = v.value;
    std::vector<float> left(block), right(block);
    float *channels[2]{left.data(), right.data()};
    float peak = 0;
    bool finite = true;
    uint32_t seed = 1;
    for (int pos = 0; pos < (int)rate * 3; pos += block) {
      for (int i = 0; i < block; ++i) {
        seed = seed * 1664525u + 1013904223u;
        const float x = pos < rate * 2 ? ((float)(seed >> 9) / 4194304.f - 1) * .25f : 0;
        left[(size_t)i] = x;
        right[(size_t)i] = x;
      }
      const auto time = transport.begin(block, 120, 0, false, false);
      const auto snapshot = modulation.evaluate(values, *extra, time);
      AudioView view{channels, 2, block};
      engine->process(view, 2, snapshot.values, *extra, time);
      for (int i = 0; i < block; ++i) {
        finite = finite && std::isfinite(left[(size_t)i]) && std::isfinite(right[(size_t)i]);
        peak = std::max({peak, std::abs(left[(size_t)i]), std::abs(right[(size_t)i])});
      }
    }
    const bool ok = finite && peak <= 1.f;
    if (!ok) {
      ++failures;
      std::printf("FAIL preset %zu %s: finite %d peak %.3f\n", p, bank[p].name, finite, peak);
    }
  }
  std::printf("core check: %zu presets, %d failures\nRESULT %d failures\n", bank.size(), failures, failures);
  return failures ? 1 : 0;
}
