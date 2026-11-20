#include "undertow_effect.h"

#include "Processor.h"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cstring>

namespace godot {

namespace {

String preset_hint() {
  String hint;
  for (int i = 0; i < ut::Processor::presetCount(); ++i) {
    if (i > 0)
      hint += ",";
    String name = String::utf8(ut::Processor::presetName(i)).replace(",", " ");
    if (i >= ut::Processor::gamePresetCount())
      name = String::utf8(ut::Processor::presetCategory(i)) + " / " + name;
    hint += name;
  }
  return hint;
}

} // namespace

// ---- Effect

AudioEffectUndertow::AudioEffectUndertow() { take_preset_values(0); }

AudioEffectUndertow::~AudioEffectUndertow() {
  std::lock_guard<std::mutex> guard(lock);
  instances.clear();
}

Ref<AudioEffectInstance> AudioEffectUndertow::_instantiate() {
  Ref<AudioEffectUndertowInstance> instance;
  instance.instantiate();
  instance->setup(this);
  return instance;
}

void AudioEffectUndertow::attach(AudioEffectUndertowInstance *p_instance) {
  auto *p = p_instance->processor();
  p->loadPreset(preset);
  p->setTempo(tempo);
  for (const auto &o : overrides)
    p->setParameter(o.first, o.second);
  std::lock_guard<std::mutex> guard(lock);
  instances.push_back(p_instance);
}

void AudioEffectUndertow::detach(AudioEffectUndertowInstance *p_instance) {
  std::lock_guard<std::mutex> guard(lock);
  instances.erase(std::remove(instances.begin(), instances.end(), p_instance), instances.end());
}

int AudioEffectUndertow::resolve_preset(const Variant &p_preset) const {
  if (p_preset.get_type() == Variant::INT || p_preset.get_type() == Variant::FLOAT) {
    const int index = (int)p_preset;
    return index >= 0 && index < ut::Processor::presetCount() ? index : -1;
  }
  return find_preset(String(p_preset));
}

int AudioEffectUndertow::find_preset(const String &p_name) const {
  return ut::Processor::findPreset(p_name.utf8().get_data());
}

void AudioEffectUndertow::take_preset_values(int p_index) {
  float v[ut::count];
  ut::Processor::presetValues(p_index, v);
  mix = v[ut::mix];
  freeze = v[ut::void_freeze] > .5f;
  depth = v[ut::macro_depth];
  motion = v[ut::macro_motion];
  glow = v[ut::macro_glow];
  chaos = v[ut::macro_chaos];
  mutate = v[ut::macro_mutate];
}

void AudioEffectUndertow::notify_headline() {
  // The headline properties changed under the inspector (preset load or
  // morph); have it redraw them.
  notify_property_list_changed();
  emit_changed();
}

void AudioEffectUndertow::send(int p_parameter, float p_value) {
  bool replaced = false;
  for (auto &o : overrides)
    if (o.first == p_parameter) {
      o.second = p_value;
      replaced = true;
    }
  if (!replaced)
    overrides.emplace_back(p_parameter, p_value);
  std::lock_guard<std::mutex> guard(lock);
  for (auto *i : instances)
    i->processor()->setParameter(p_parameter, p_value);
}

void AudioEffectUndertow::set_preset(int p_index) {
  p_index = std::clamp(p_index, 0, ut::Processor::presetCount() - 1);
  preset = p_index;
  overrides.clear();
  take_preset_values(p_index);
  {
    std::lock_guard<std::mutex> guard(lock);
    for (auto *i : instances)
      i->processor()->loadPreset(p_index);
  }
  notify_headline();
}

bool AudioEffectUndertow::morph_to(const Variant &p_preset, float p_seconds) {
  const int index = resolve_preset(p_preset);
  if (index < 0) {
    UtilityFunctions::push_warning("UNDERTOW: no preset called ", p_preset);
    return false;
  }
  preset = index;
  overrides.clear();
  take_preset_values(index);
  bool ok = true;
  {
    std::lock_guard<std::mutex> guard(lock);
    for (auto *i : instances)
      ok = i->processor()->morphTo(index, std::max(0.f, p_seconds)) && ok;
  }
  notify_headline();
  return ok;
}

bool AudioEffectUndertow::is_morphing() const {
  std::lock_guard<std::mutex> guard(lock);
  for (auto *i : instances)
    if (i->processor()->isMorphing())
      return true;
  return false;
}

void AudioEffectUndertow::set_mix(float v) {
  mix = std::clamp(v, 0.f, 100.f);
  send(ut::mix, mix);
}
void AudioEffectUndertow::set_freeze(bool on) {
  freeze = on;
  send(ut::void_freeze, on ? 1.f : 0.f);
}
void AudioEffectUndertow::set_depth(float v) {
  depth = std::clamp(v, -100.f, 100.f);
  send(ut::macro_depth, depth);
}
void AudioEffectUndertow::set_motion(float v) {
  motion = std::clamp(v, -100.f, 100.f);
  send(ut::macro_motion, motion);
}
void AudioEffectUndertow::set_glow(float v) {
  glow = std::clamp(v, -100.f, 100.f);
  send(ut::macro_glow, glow);
}
void AudioEffectUndertow::set_chaos(float v) {
  chaos = std::clamp(v, -100.f, 100.f);
  send(ut::macro_chaos, chaos);
}
void AudioEffectUndertow::set_mutate(float v) {
  mutate = std::clamp(v, -100.f, 100.f);
  send(ut::macro_mutate, mutate);
}
void AudioEffectUndertow::set_tempo(float bpm) {
  tempo = std::clamp(bpm, 20.f, 400.f);
  std::lock_guard<std::mutex> guard(lock);
  for (auto *i : instances)
    i->processor()->setTempo(tempo);
}

bool AudioEffectUndertow::set_parameter(const String &p_id, float p_value) {
  const int k = ut::Processor::findParameter(p_id.utf8().get_data());
  if (k < 0) {
    UtilityFunctions::push_warning("UNDERTOW: no parameter called ", p_id);
    return false;
  }
  const auto &s = ut::Processor::parameterSpec(k);
  p_value = std::clamp(p_value, s.lo, s.hi);
  send(k, p_value);
  switch (k) {
  case ut::mix: mix = p_value; break;
  case ut::void_freeze: freeze = p_value > .5f; break;
  case ut::macro_depth: depth = p_value; break;
  case ut::macro_motion: motion = p_value; break;
  case ut::macro_glow: glow = p_value; break;
  case ut::macro_chaos: chaos = p_value; break;
  case ut::macro_mutate: mutate = p_value; break;
  default: return true;
  }
  notify_headline();
  return true;
}

float AudioEffectUndertow::get_parameter(const String &p_id) const {
  const int k = ut::Processor::findParameter(p_id.utf8().get_data());
  if (k < 0)
    return 0;
  {
    std::lock_guard<std::mutex> guard(lock);
    if (!instances.empty())
      return instances.front()->processor()->parameter(k);
  }
  for (const auto &o : overrides)
    if (o.first == k)
      return o.second;
  float v[ut::count];
  ut::Processor::presetValues(preset, v);
  return v[k];
}

PackedStringArray AudioEffectUndertow::get_preset_names() const {
  PackedStringArray out;
  for (int i = 0; i < ut::Processor::presetCount(); ++i)
    out.push_back(String::utf8(ut::Processor::presetName(i)));
  return out;
}

PackedStringArray AudioEffectUndertow::get_game_preset_names() const {
  PackedStringArray out;
  for (int i = 0; i < ut::Processor::gamePresetCount(); ++i)
    out.push_back(String::utf8(ut::Processor::presetName(i)));
  return out;
}

PackedStringArray AudioEffectUndertow::get_parameter_ids() const {
  PackedStringArray out;
  for (int k = 0; k < ut::count; ++k)
    out.push_back(String::utf8(ut::Processor::parameterSpec(k).id));
  return out;
}

void AudioEffectUndertow::_bind_methods() {
  ClassDB::bind_method(D_METHOD("set_preset", "index"), &AudioEffectUndertow::set_preset);
  ClassDB::bind_method(D_METHOD("get_preset"), &AudioEffectUndertow::get_preset);
  ClassDB::bind_method(D_METHOD("set_mix", "percent"), &AudioEffectUndertow::set_mix);
  ClassDB::bind_method(D_METHOD("get_mix"), &AudioEffectUndertow::get_mix);
  ClassDB::bind_method(D_METHOD("set_freeze", "on"), &AudioEffectUndertow::set_freeze);
  ClassDB::bind_method(D_METHOD("get_freeze"), &AudioEffectUndertow::get_freeze);
  ClassDB::bind_method(D_METHOD("set_depth", "value"), &AudioEffectUndertow::set_depth);
  ClassDB::bind_method(D_METHOD("get_depth"), &AudioEffectUndertow::get_depth);
  ClassDB::bind_method(D_METHOD("set_motion", "value"), &AudioEffectUndertow::set_motion);
  ClassDB::bind_method(D_METHOD("get_motion"), &AudioEffectUndertow::get_motion);
  ClassDB::bind_method(D_METHOD("set_glow", "value"), &AudioEffectUndertow::set_glow);
  ClassDB::bind_method(D_METHOD("get_glow"), &AudioEffectUndertow::get_glow);
  ClassDB::bind_method(D_METHOD("set_chaos", "value"), &AudioEffectUndertow::set_chaos);
  ClassDB::bind_method(D_METHOD("get_chaos"), &AudioEffectUndertow::get_chaos);
  ClassDB::bind_method(D_METHOD("set_mutate", "value"), &AudioEffectUndertow::set_mutate);
  ClassDB::bind_method(D_METHOD("get_mutate"), &AudioEffectUndertow::get_mutate);
  ClassDB::bind_method(D_METHOD("set_tempo", "bpm"), &AudioEffectUndertow::set_tempo);
  ClassDB::bind_method(D_METHOD("get_tempo"), &AudioEffectUndertow::get_tempo);

  ClassDB::bind_method(D_METHOD("morph_to", "preset", "seconds"), &AudioEffectUndertow::morph_to, DEFVAL(2.0f));
  ClassDB::bind_method(D_METHOD("is_morphing"), &AudioEffectUndertow::is_morphing);
  ClassDB::bind_method(D_METHOD("set_parameter", "id", "value"), &AudioEffectUndertow::set_parameter);
  ClassDB::bind_method(D_METHOD("get_parameter", "id"), &AudioEffectUndertow::get_parameter);
  ClassDB::bind_method(D_METHOD("get_preset_names"), &AudioEffectUndertow::get_preset_names);
  ClassDB::bind_method(D_METHOD("get_game_preset_names"), &AudioEffectUndertow::get_game_preset_names);
  ClassDB::bind_method(D_METHOD("get_parameter_ids"), &AudioEffectUndertow::get_parameter_ids);
  ClassDB::bind_method(D_METHOD("find_preset", "name"), &AudioEffectUndertow::find_preset);

  ADD_PROPERTY(PropertyInfo(Variant::INT, "preset", PROPERTY_HINT_ENUM, preset_hint()), "set_preset", "get_preset");
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mix", PROPERTY_HINT_RANGE, "0,100,1,suffix:%"), "set_mix", "get_mix");
  ADD_PROPERTY(PropertyInfo(Variant::BOOL, "freeze"), "set_freeze", "get_freeze");
  ADD_GROUP("Macros", "");
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "depth", PROPERTY_HINT_RANGE, "-100,100,1,suffix:%"), "set_depth", "get_depth");
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "motion", PROPERTY_HINT_RANGE, "-100,100,1,suffix:%"), "set_motion", "get_motion");
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "glow", PROPERTY_HINT_RANGE, "-100,100,1,suffix:%"), "set_glow", "get_glow");
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "chaos", PROPERTY_HINT_RANGE, "-100,100,1,suffix:%"), "set_chaos", "get_chaos");
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mutate", PROPERTY_HINT_RANGE, "-100,100,1,suffix:%"), "set_mutate", "get_mutate");
  ADD_GROUP("Timing", "");
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tempo", PROPERTY_HINT_RANGE, "20,400,0.1,suffix:bpm"), "set_tempo", "get_tempo");
}

// ---- Instance

AudioEffectUndertowInstance::AudioEffectUndertowInstance() = default;

AudioEffectUndertowInstance::~AudioEffectUndertowInstance() {
  if (base.is_valid())
    base->detach(this);
}

void AudioEffectUndertowInstance::setup(AudioEffectUndertow *p_base) {
  double rate = 48000;
  if (AudioServer *server = AudioServer::get_singleton())
    rate = server->get_mix_rate();
  engine = std::make_unique<ut::Processor>();
  engine->prepare(rate, 1024);
  base = Ref<AudioEffectUndertow>(p_base);
  p_base->attach(this);
}

void AudioEffectUndertowInstance::_process(const void *p_src, AudioFrame *p_dst, int32_t p_frame_count) {
  static_assert(sizeof(AudioFrame) == 2 * sizeof(float), "AudioFrame is two floats");
  if (p_frame_count <= 0)
    return;
  if (p_src != p_dst)
    std::memcpy(p_dst, p_src, sizeof(AudioFrame) * (size_t)p_frame_count);
  if (engine)
    engine->processInterleaved(reinterpret_cast<float *>(p_dst), p_frame_count);
}

} // namespace godot
