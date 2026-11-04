#include "undertow.h"
#include "Processor.h"
#include <new>

struct ut_instance {
  ut::Processor processor;
};

namespace {
bool validParameter(int index) { return index >= 0 && index < ut::count; }
} // namespace

extern "C" {

ut_instance *ut_create(double sampleRate, int maxBlock) {
  auto *instance = new (std::nothrow) ut_instance;
  if (instance != nullptr)
    instance->processor.prepare(sampleRate, maxBlock);
  return instance;
}
void ut_destroy(ut_instance *instance) { delete instance; }
void ut_prepare(ut_instance *instance, double sampleRate, int maxBlock) {
  if (instance != nullptr)
    instance->processor.prepare(sampleRate, maxBlock);
}

void ut_process(ut_instance *instance, float *left, float *right, int frames) {
  if (instance != nullptr && left != nullptr && right != nullptr && frames > 0)
    instance->processor.process(left, right, frames);
}
void ut_process_interleaved(ut_instance *instance, float *stereo, int frames) {
  if (instance != nullptr && stereo != nullptr && frames > 0)
    instance->processor.processInterleaved(stereo, frames);
}

int ut_preset_count(void) { return ut::Processor::presetCount(); }
int ut_game_preset_count(void) { return ut::Processor::gamePresetCount(); }
const char *ut_preset_name(int index) { return ut::Processor::presetName(index); }
const char *ut_preset_category(int index) { return ut::Processor::presetCategory(index); }
int ut_find_preset(const char *name) { return ut::Processor::findPreset(name); }

int ut_parameter_count(void) { return ut::count; }
const char *ut_parameter_id(int index) { return validParameter(index) ? ut::specs[(size_t)index].id : ""; }
const char *ut_parameter_name(int index) { return validParameter(index) ? ut::specs[(size_t)index].name : ""; }
const char *ut_parameter_unit(int index) { return validParameter(index) ? ut::specs[(size_t)index].unit : ""; }
float ut_parameter_min(int index) { return validParameter(index) ? ut::specs[(size_t)index].lo : 0; }
float ut_parameter_max(int index) { return validParameter(index) ? ut::specs[(size_t)index].hi : 0; }
float ut_parameter_default(int index) { return validParameter(index) ? ut::specs[(size_t)index].initial : 0; }
int ut_find_parameter(const char *id) { return ut::Processor::findParameter(id); }

int ut_set_parameter(ut_instance *instance, int index, float value) {
  return instance != nullptr && instance->processor.setParameter(index, value);
}
float ut_get_parameter(const ut_instance *instance, int index) {
  return instance != nullptr ? instance->processor.parameter(index) : 0;
}
int ut_load_preset(ut_instance *instance, int index) {
  return instance != nullptr && instance->processor.loadPreset(index);
}
int ut_morph_to(ut_instance *instance, int index, float seconds) {
  return instance != nullptr && instance->processor.morphTo(index, seconds);
}
int ut_set_tempo(ut_instance *instance, double bpm) {
  return instance != nullptr && instance->processor.setTempo(bpm);
}
int ut_set_freeze(ut_instance *instance, int on) {
  return instance != nullptr && instance->processor.setFreeze(on != 0);
}
int ut_current_preset(const ut_instance *instance) {
  return instance != nullptr ? instance->processor.currentPreset() : -1;
}
int ut_is_morphing(const ut_instance *instance) {
  return instance != nullptr && instance->processor.isMorphing();
}

} // extern "C"
