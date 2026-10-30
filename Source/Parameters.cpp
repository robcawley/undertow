#include "Parameters.h"
#include <locale>
#include <sstream>
namespace ut {
constexpr const char *divisionNames =
    "1/64|1/32|1/32 triplet|1/16 triplet|1/32 dotted|1/16|1/8 triplet|1/16 "
    "dotted|1/8|1/4 triplet|1/8 dotted|1/4|1/2 triplet|1/4 dotted|1/2|1 bar "
    "triplet|1/2 dotted|1 bar|2 bars|4 bars|8 bars";
const std::array<Spec, count> specs{{
#include "ParameterSpecs.inc"
}};
namespace {
// Readable display text. Every string here parses back through
// Float::getValueForText.
juce::String display(float v, const juce::String &unit) {
  if (unit == "Hz")
    return v >= 1000 ? juce::String(v / 1000, v >= 10000 ? 1 : 2) + " kHz"
                     : juce::String(v, v < 100 ? 1 : 0) + " Hz";
  if (unit == "%")
    return juce::String(juce::roundToInt(v)) + " %";
  if (unit == "ms")
    return juce::String(v, v < 100 ? 1 : 0) + " ms";
  if (unit == "s")
    return juce::String(v, v < 10 ? 2 : 1) + " s";
  if (unit == "dB")
    return (v > 0.05f ? "+" : "") + juce::String(v, 1) + " dB";
  if (unit == "deg")
    return juce::String(juce::roundToInt(v)) + " deg";
  if (unit == "Q")
    return juce::String(v, 1) + " Q";
  return juce::String(v, 3) + (unit.isEmpty() ? "" : " " + unit);
}
} // namespace
class Float final : public juce::AudioParameterFloat {
public:
  Float(const Spec &s)
      : AudioParameterFloat(
            {s.id, 1}, s.name, rangeFor(s), s.initial,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction(
                [unit = juce::String(s.unit)](float v, int) {
                  return display(v, unit);
                })),
        unit(s.unit) {}
  float getValueForText(const juce::String &text) const override {
    std::istringstream in(text.trim().toStdString());
    in.imbue(std::locale::classic());
    float v;
    std::string suffix, extra;
    if (!(in >> v) || !std::isfinite(v))
      return static_cast<const juce::AudioProcessorParameter &>(*this)
          .getValue();
    in >> suffix;
    in >> extra;
    // "2.5k", "2.5 kHz": thousands, for frequencies.
    if (unit == "Hz" && (juce::String(suffix).equalsIgnoreCase("k") ||
                         juce::String(suffix).equalsIgnoreCase("kHz"))) {
      v *= 1000;
      suffix.clear();
    }
    if (unit == "deg" && suffix == "\u00b0")
      suffix.clear();
    if (!extra.empty() ||
        (!suffix.empty() && !juce::String(suffix).equalsIgnoreCase(unit)))
      return static_cast<const juce::AudioProcessorParameter &>(*this)
          .getValue();
    return convertTo0to1(juce::jlimit(range.start, range.end, v));
  }

private:
  juce::String unit;
  static juce::NormalisableRange<float> rangeFor(const Spec &s) {
    juce::NormalisableRange<float> r(s.lo, s.hi);
    if (s.centre > s.lo && s.centre < s.hi)
      r.setSkewForCentre(s.centre);
    return r;
  }
};
namespace {
std::unique_ptr<juce::RangedAudioParameter> make(const Spec &s) {
  if (s.kind == 0)
    return std::make_unique<Float>(s);
  if (s.kind == 1)
    return std::make_unique<juce::AudioParameterBool>(juce::ParameterID{s.id, 1}, s.name, s.initial != 0);
  if (s.kind == 3)
    return std::make_unique<juce::AudioParameterInt>(juce::ParameterID{s.id, 1}, s.name, (int)s.lo, (int)s.hi,
                                                     (int)s.initial);
  return std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{s.id, 1}, s.name,
                                                      juce::StringArray::fromTokens(s.choices, "|", ""),
                                                      (int)s.initial);
}
} // namespace
// Parameters are grouped so Logic's automation menu shows them in sub-menus.
// Hosts identify parameters by ID, never by position, so the grouping (and the
// order it gives) doesn't touch saved automation.
juce::AudioProcessorValueTreeState::ParameterLayout layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout l;
  struct Group { const char *id, *name; std::vector<std::pair<int, int>> ranges; };
  const Group groups[]{
      {"output", "OUTPUT", {{mix, duck_release}, {bloom, bloom}}},
      {"drift", "DRIFT", {{drift_on, drift_mix}}},
      {"echo", "ECHO", {{echo_on, echo_level}}},
      {"void", "VOID", {{void_on, void_level}}},
      {"pulse", "PULSE", {{pulse_on, pulse_focus_div}}},
      {"xy", "XY", {{xy_x, xy_motion_play}, {xy_return, xy_return}}},
      {"macros", "MACROS", {{macro_depth, macro_chaos}, {macro_mutate, macro_mutate}}},
      {"mutate", "MUTATE", {{mut_on, evolve_follow}}}};
  std::array<bool, count> placed{};
  for (auto &g : groups) {
    auto group = std::make_unique<juce::AudioProcessorParameterGroup>(g.id, g.name, " | ");
    for (auto [from, to] : g.ranges)
      for (int k = from; k <= to; ++k) {
        group->addChild(make(specs[(size_t)k]));
        placed[(size_t)k] = true;
      }
    l.add(std::move(group));
  }
  for (int k = 0; k < count; ++k) jassert(placed[(size_t)k]);
  return l;
}
} // namespace ut
