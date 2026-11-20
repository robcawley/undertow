// UNDERTOW for Godot: an audio bus effect. MIT licence (see LICENSE).
#pragma once

#include <godot_cpp/classes/audio_effect.hpp>
#include <godot_cpp/classes/audio_effect_instance.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <memory>
#include <mutex>
#include <vector>

namespace ut {
class Processor;
}

namespace godot {

class AudioEffectUndertowInstance;

// The effect resource. Add it to an audio bus; every bus it sits on gets its
// own instance. Changing a property or calling a method here reaches every
// instance. Call these from the main thread (or one other thread at a time).
class AudioEffectUndertow : public AudioEffect {
  GDCLASS(AudioEffectUndertow, AudioEffect)

public:
  AudioEffectUndertow();
  ~AudioEffectUndertow() override;

  Ref<AudioEffectInstance> _instantiate() override;

  // Presets: the 12 game presets first, then the plug-in's 92.
  void set_preset(int p_index);
  int get_preset() const { return preset; }

  // The headline controls, in the units the inspector shows. Loading or
  // morphing to a preset sets them to the preset's values.
  void set_mix(float p_percent);
  float get_mix() const { return mix; }
  void set_freeze(bool p_on);
  bool get_freeze() const { return freeze; }
  void set_depth(float p_value);
  float get_depth() const { return depth; }
  void set_motion(float p_value);
  float get_motion() const { return motion; }
  void set_glow(float p_value);
  float get_glow() const { return glow; }
  void set_chaos(float p_value);
  float get_chaos() const { return chaos; }
  void set_mutate(float p_value);
  float get_mutate() const { return mutate; }
  void set_tempo(float p_bpm);
  float get_tempo() const { return tempo; }

  // Glides to a preset (by name or index) over the given seconds.
  bool morph_to(const Variant &p_preset, float p_seconds = 2.0f);
  bool is_morphing() const;

  // Any of the 132 parameters, by id ("void_decay", "echo_time" ...).
  bool set_parameter(const String &p_id, float p_value);
  float get_parameter(const String &p_id) const;

  PackedStringArray get_preset_names() const;
  PackedStringArray get_game_preset_names() const;
  PackedStringArray get_parameter_ids() const;
  int find_preset(const String &p_name) const;

  // Instances register here so control calls reach them.
  void attach(AudioEffectUndertowInstance *p_instance);
  void detach(AudioEffectUndertowInstance *p_instance);

protected:
  static void _bind_methods();

private:
  int resolve_preset(const Variant &p_preset) const;
  void take_preset_values(int p_index);
  void send(int p_parameter, float p_value);
  void notify_headline();

  int preset = 0;
  float mix = 50, depth = 0, motion = 0, glow = 0, chaos = 0, mutate = 0, tempo = 120;
  bool freeze = false;
  // Changes made through set_parameter, replayed on new instances after the
  // preset loads.
  std::vector<std::pair<int, float>> overrides;

  mutable std::mutex lock;
  std::vector<AudioEffectUndertowInstance *> instances;
};

class AudioEffectUndertowInstance : public AudioEffectInstance {
  GDCLASS(AudioEffectUndertowInstance, AudioEffectInstance)

public:
  AudioEffectUndertowInstance();
  ~AudioEffectUndertowInstance() override;

  void _process(const void *p_src, AudioFrame *p_dst, int32_t p_frame_count) override;
  bool _process_silence() const override { return true; }

  void setup(AudioEffectUndertow *p_base);
  ut::Processor *processor() { return engine.get(); }

protected:
  static void _bind_methods() {}

private:
  Ref<AudioEffectUndertow> base;
  std::unique_ptr<ut::Processor> engine;
};

} // namespace godot
