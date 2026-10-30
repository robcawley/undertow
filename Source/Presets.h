#pragma once
#include "Parameters.h"
#include <vector>
namespace ut {
struct PresetValue {
  int param;
  float value; // plain units for scenes, normalised offset for corners
};
struct PresetDef {
  const char *category, *name;
  std::vector<PresetValue> a, b;
  std::array<std::vector<PresetValue>, 4> corners;
  std::array<uint16_t, 6> patterns{65535, 65535, 65535, 65535, 65535, 65535};
};
// Factory bank. Indices are permanent: add new presets at the end only.
const std::vector<PresetDef> &factoryPresets();
inline int band(int n, int first) { return first + 7 * (n - 1); }
} // namespace ut
