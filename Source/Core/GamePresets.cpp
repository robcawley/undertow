#include "Presets.h"

namespace ut {
namespace {

// Spaces for games. Unlike the plug-in bank, nothing here follows a song
// tempo: echoes and pulses are set in milliseconds and hertz, so they sound
// the same whatever the game's clock is doing.
std::vector<PresetDef> build() {
  std::vector<PresetDef> p;
  const char *game = "GAME";

  // Wet stone, close walls, a short slap off the far side.
  p.push_back({game, "Cave",
               {{mix, 50}, {anchor_freq, 90}, {drift_on, 0},
                {echo_sync, 0}, {echo_time, 180}, {echo_feedback, 30}, {echo_tone, 3000},
                {echo_lowcut, 250}, {echo_drive, 5}, {echo_age, 10}, {echo_level, 22},
                {void_size, 72}, {void_decay, 3.5f}, {void_predelay, 25}, {void_damp, 55},
                {void_lowcut, 150}, {void_diffusion, 70}, {void_mod, 15}, {void_width, 90},
                {void_level, 85}},
               {}, {}});

  // Muffled and slow-moving: the space wobbles, the highs are gone.
  p.push_back({game, "Underwater",
               {{mix, 85}, {anchor_freq, 150},
                {drift_rate, .6f}, {drift_depth, 55}, {drift_wow, 60}, {drift_flutter, 20},
                {drift_width, 40}, {drift_mix, 75}, {echo_on, 0},
                {void_size, 60}, {void_decay, 2.5f}, {void_predelay, 10}, {void_damp, 92},
                {void_lowcut, 80}, {void_diffusion, 90}, {void_mod, 70}, {void_width, 70},
                {void_level, 95},
                {pulse_on, 1}, {pulse_position, 2}, {pulse_bands, 2}, {pulse_xover1, 400},
                {pulse_b2_sync, 0}, {pulse_b2_rate, .35f}, {pulse_b2_depth, 30}, {pulse_b2_shape, 0}},
               {}, {}});

  // Huge stone and a faint choir an octave up.
  p.push_back({game, "Cathedral",
               {{mix, 50}, {anchor_freq, 100},
                {drift_depth, 10}, {drift_wow, 5}, {drift_flutter, 0}, {drift_mix, 20},
                {echo_on, 0},
                {void_size, 100}, {void_decay, 12}, {void_predelay, 70}, {void_damp, 35},
                {void_lowcut, 140}, {void_diffusion, 95}, {void_mod, 25}, {void_width, 100},
                {void_level, 95},
                {mut_on, 1}, {harm_octave, 8}, {halo_climb, 15}, {halo_interval, 0}, {halo_spread, 50}},
               {}, {}});

  // A metal pipe: tight, ringing flutter echoes.
  p.push_back({game, "Tunnel",
               {{mix, 55}, {anchor_freq, 110}, {drift_on, 0},
                {echo_sync, 0}, {echo_time, 70}, {echo_feedback, 60}, {echo_tone, 5000},
                {echo_lowcut, 300}, {echo_drive, 20}, {echo_age, 5}, {echo_level, 40},
                {void_size, 40}, {void_decay, 2.2f}, {void_predelay, 5}, {void_damp, 30},
                {void_lowcut, 200}, {void_diffusion, 40}, {void_mod, 10}, {void_width, 70},
                {void_level, 70}},
               {}, {}});

  // Open air: a long, clean echo off distant rock and very little room.
  p.push_back({game, "Canyon",
               {{mix, 45}, {anchor_freq, 100}, {drift_on, 0},
                {echo_sync, 0}, {echo_time, 650}, {echo_feedback, 35}, {echo_tone, 4000},
                {echo_lowcut, 200}, {echo_drive, 0}, {echo_age, 10}, {echo_level, 45},
                {void_size, 45}, {void_decay, 1.8f}, {void_predelay, 40}, {void_damp, 60},
                {void_lowcut, 200}, {void_diffusion, 60}, {void_mod, 10}, {void_width, 100},
                {void_level, 45}},
               {}, {}});

  // Soft focus: wide, smeared, with a slow shimmer that comes and goes.
  p.push_back({game, "Dream",
               {{mix, 60},
                {drift_rate, .25f}, {drift_depth, 45}, {drift_wow, 40}, {drift_flutter, 10},
                {drift_width, 90}, {drift_mix, 60},
                {echo_mode, 1}, {echo_sync, 0}, {echo_time, 380}, {echo_feedback, 45},
                {echo_tone, 3500}, {echo_age, 30}, {echo_level, 35},
                {void_size, 85}, {void_decay, 9}, {void_damp, 50}, {void_mod, 50},
                {void_width, 100}, {void_level, 90},
                {mut_on, 1}, {halo_climb, 25}, {halo_interval, 0}, {halo_spread, 60},
                {evolve_amount, 30}, {evolve_cycle, 3}},
               {}, {}});

  // A flashback: worn tape, wow, echoes that arrive backwards.
  p.push_back({game, "Memory",
               {{mix, 60},
                {drift_rate, .3f}, {drift_depth, 35}, {drift_wow, 60}, {drift_flutter, 30},
                {drift_mix, 55},
                {echo_mode, 2}, {echo_sync, 0}, {echo_time, 600}, {echo_feedback, 40},
                {echo_tone, 3000}, {echo_lowcut, 200}, {echo_drive, 25}, {echo_age, 70},
                {echo_level, 45},
                {void_size, 80}, {void_decay, 6}, {void_damp, 60}, {void_level, 80}},
               {}, {}});

  // Everything hangs in the air: the space freezes what was playing.
  // Morph to it when the game pauses; morph back to carry on.
  p.push_back({game, "Pause",
               {{mix, 90},
                {drift_depth, 20}, {drift_mix, 40}, {echo_on, 0},
                {void_size, 90}, {void_decay, 30}, {void_predelay, 20}, {void_damp, 60},
                {void_mod, 40}, {void_width, 100}, {void_level, 100}, {void_freeze, 1}},
               {}, {}});

  // A slow heartbeat throb above the low end, a small dull room, a little sway.
  p.push_back({game, "Low Health",
               {{mix, 55}, {anchor_freq, 120},
                {drift_rate, .5f}, {drift_depth, 30}, {drift_wow, 30}, {drift_mix, 40},
                {echo_on, 0},
                {void_size, 50}, {void_decay, 2}, {void_damp, 80}, {void_level, 70},
                {pulse_on, 1}, {pulse_position, 1}, {pulse_bands, 2}, {pulse_xover1, 500},
                {pulse_b2_sync, 0}, {pulse_b2_rate, 1.2f}, {pulse_b2_depth, 65}, {pulse_b2_shape, 4}},
               {}, {}});

  // Dark and heavy, a space that swells and rumbles.
  p.push_back({game, "Storm",
               {{mix, 55}, {anchor_freq, 70},
                {drift_rate, .1f}, {drift_depth, 50}, {drift_wow, 30}, {drift_mix, 50},
                {echo_sync, 0}, {echo_time, 900}, {echo_feedback, 50}, {echo_tone, 2000},
                {echo_age, 40}, {echo_level, 30},
                {void_size, 95}, {void_decay, 10}, {void_damp, 70}, {void_lowcut, 60},
                {void_mod, 60}, {void_gravity, 40}, {void_level, 95}},
               {}, {}});

  // Endless and dark, with an octave below that grows over time.
  p.push_back({game, "Void",
               {{mix, 75}, {anchor_freq, 90},
                {drift_depth, 25}, {drift_mix, 40},
                {echo_mode, 1}, {echo_sync, 0}, {echo_time, 1200}, {echo_feedback, 55},
                {echo_tone, 2500}, {echo_level, 30},
                {void_size, 100}, {void_decay, 60}, {void_predelay, 150}, {void_damp, 50},
                {void_lowcut, 120}, {void_diffusion, 100}, {void_mod, 45}, {void_gravity, -30},
                {void_level, 100},
                {mut_on, 1}, {halo_climb, 20}, {halo_interval, 2}, {halo_spread, 40},
                {evolve_amount, 40}, {evolve_cycle, 4}},
               {}, {}});

  // Weightless: the space circles slowly through four variations.
  p.push_back({game, "Orbit",
               {{mix, 60},
                {drift_depth, 30}, {drift_mix, 45},
                {echo_mode, 1}, {echo_sync, 0}, {echo_time, 500}, {echo_feedback, 45},
                {echo_tone, 6000}, {echo_level, 35},
                {void_size, 85}, {void_decay, 9}, {void_mod, 40}, {void_level, 90},
                {mut_on, 1}, {halo_climb, 20}, {halo_interval, 1}, {halo_spread, 50},
                {xy_current, 1}, {xy_radius, 40}, {xy_rate, 19}},
               {},
               {{{{void_size, .3f}, {void_decay, .25f}},
                 {{echo_feedback, .3f}, {echo_level, .2f}},
                 {{void_damp, -.3f}, {halo_climb, .3f}},
                 {{drift_depth, .4f}, {void_width, -.2f}}}}});
  return p;
}

} // namespace

const std::vector<PresetDef> &gamePresets() {
  static const std::vector<PresetDef> bank = build();
  return bank;
}

} // namespace ut
