#pragma once
#include "Specs.h"
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
// Spaces for games (GamePresets.cpp); not part of the plug-in's bank.
const std::vector<PresetDef> &gamePresets();
inline int band(int n, int first) { return first + 7 * (n - 1); }
} // namespace ut
