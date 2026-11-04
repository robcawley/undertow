/* UNDERTOW core: C interface, for game engines and other hosts.
   MIT licence (see LICENSE in this folder).

   Threads: ut_create, ut_prepare and ut_destroy allocate; call them before or
   after audio runs. ut_process and ut_process_interleaved run on the audio
   thread and never allocate or lock. The control calls (ut_set_parameter,
   ut_load_preset, ut_morph_to, ut_set_tempo, ut_set_freeze) may come from one
   other thread, or from the audio thread; they take effect at the start of the
   next process call and return 0 if the queue is full or the input invalid.

   Parameters use plain units: %, Hz, ms, s, dB; switches 0 or 1; choices as
   their index. Presets: the game bank first (ut_game_preset_count of them),
   then the plug-in presets. */
#ifndef UNDERTOW_H
#define UNDERTOW_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && defined(UNDERTOW_SHARED)
#define UT_API __declspec(dllexport)
#elif defined(__GNUC__) && defined(UNDERTOW_SHARED)
#define UT_API __attribute__((visibility("default")))
#else
#define UT_API
#endif

typedef struct ut_instance ut_instance;

UT_API ut_instance *ut_create(double sample_rate, int max_block);
UT_API void ut_destroy(ut_instance *);
UT_API void ut_prepare(ut_instance *, double sample_rate, int max_block);

/* In place, stereo. */
UT_API void ut_process(ut_instance *, float *left, float *right, int frames);
UT_API void ut_process_interleaved(ut_instance *, float *stereo, int frames);

UT_API int ut_preset_count(void);
UT_API int ut_game_preset_count(void);
UT_API const char *ut_preset_name(int index);
UT_API const char *ut_preset_category(int index);
UT_API int ut_find_preset(const char *name); /* -1 if not found */

UT_API int ut_parameter_count(void);
UT_API const char *ut_parameter_id(int index);
UT_API const char *ut_parameter_name(int index);
UT_API const char *ut_parameter_unit(int index);
UT_API float ut_parameter_min(int index);
UT_API float ut_parameter_max(int index);
UT_API float ut_parameter_default(int index);
UT_API int ut_find_parameter(const char *id); /* -1 if not found */

UT_API int ut_set_parameter(ut_instance *, int index, float value);
UT_API float ut_get_parameter(const ut_instance *, int index);
UT_API int ut_load_preset(ut_instance *, int index);
UT_API int ut_morph_to(ut_instance *, int index, float seconds);
UT_API int ut_set_tempo(ut_instance *, double bpm);
UT_API int ut_set_freeze(ut_instance *, int on);
UT_API int ut_current_preset(const ut_instance *);
UT_API int ut_is_morphing(const ut_instance *);

#ifdef __cplusplus
}
#endif
#endif
