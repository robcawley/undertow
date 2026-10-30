#include "Presets.h"
namespace ut {
namespace {
// Division indices into the shared list.
enum Div { d16 = 5, d8t = 6, d8 = 8, d4t = 9, d8d = 10, d4 = 11, d4d = 13, d2 = 14, bar1 = 17, bar2 = 18, bar4 = 19, bar8 = 20 };
std::vector<PresetDef> build() {
  std::vector<PresetDef> p;
  // 0. The plain starting point.
  p.push_back({"INIT", "Default", {}, {}, {}});

  // ---- BASS SPACE: tight lows, a room that opens around the riff.
  p.push_back({"BASS SPACE", "Low Ceiling",
               {{mix, 50}, {anchor_freq, 110}, {duck_amount, 15}, {duck_release, 300},
                {drift_depth, 15}, {drift_wow, 10}, {drift_flutter, 5}, {drift_mix, 35},
                {echo_div, d8d}, {echo_feedback, 30}, {echo_tone, 3500}, {echo_lowcut, 200}, {echo_level, 35},
                {void_size, 55}, {void_decay, 3.5f}, {void_predelay, 20}, {void_damp, 50}, {void_lowcut, 120},
                {void_diffusion, 75}, {void_mod, 20}, {void_width, 85}, {void_level, 90}},
               {{void_size, 80}, {void_decay, 8}, {echo_feedback, 50}, {void_damp, 40}},
               {{{{void_decay, .3f}, {void_size, .25f}},
                 {{echo_feedback, .35f}, {echo_level, .3f}},
                 {{void_damp, .35f}, {echo_tone, -.3f}},
                 {{drift_depth, .45f}, {void_width, .15f}}}}});
  p.push_back({"BASS SPACE", "Iron Hall",
               {{mix, 55}, {anchor_freq, 100}, {duck_amount, 15}, {duck_release, 450},
                {echo_mode, 1}, {echo_div, d4}, {echo_feedback, 40}, {echo_tone, 4500}, {echo_lowcut, 180},
                {echo_drive, 35}, {echo_age, 25}, {echo_level, 40},
                {void_size, 85}, {void_decay, 7}, {void_predelay, 45}, {void_damp, 40}, {void_lowcut, 110},
                {void_mod, 30}, {void_gravity, 30}, {void_level, 95}},
               {{void_gravity, -40}, {void_decay, 14}, {echo_feedback, 55}, {void_level, 100}},
               {{{{void_gravity, -.4f}, {void_decay, .25f}},
                 {{echo_drive, .4f}, {echo_age, .35f}},
                 {{void_predelay, .35f}, {void_damp, .3f}},
                 {{mix, -.2f}, {void_size, -.25f}}}}});
  p.push_back({"BASS SPACE", "Undercurrent",
               {{mix, 55}, {anchor_freq, 100}, {duck_amount, 20}, {duck_release, 500},
                {drift_depth, 30}, {drift_wow, 30}, {drift_mix, 50},
                {echo_mode, 2}, {echo_div, d2}, {echo_feedback, 35}, {echo_tone, 4000}, {echo_lowcut, 200}, {echo_level, 45},
                {void_size, 70}, {void_decay, 5}, {void_gravity, -40}, {void_lowcut, 120}, {void_level, 90}},
               {{void_decay, 10}, {echo_level, 60}, {void_gravity, -70}},
               {{{{echo_level, .35f}, {echo_feedback, .25f}},
                 {{void_gravity, -.35f}, {void_size, .25f}},
                 {{drift_wow, .45f}, {drift_flutter, .35f}},
                 {{duck_amount, .3f}, {void_damp, .35f}}}}});

  // ---- DRONE: long, slow, made for FREEZE.
  p.push_back({"DRONE", "Salt Room",
               {{mix, 70}, {anchor_freq, 80}, {drift_rate, .15f}, {drift_depth, 40}, {drift_wow, 35},
                {drift_flutter, 10}, {drift_width, 80}, {drift_mix, 60}, {echo_on, 0},
                {void_size, 95}, {void_decay, 30}, {void_predelay, 80}, {void_damp, 35}, {void_lowcut, 90},
                {void_diffusion, 95}, {void_mod, 55}, {void_width, 100}, {void_gravity, -20}, {void_level, 100}},
               {{void_decay, 70}, {void_gravity, -60}, {void_mod, 70}},
               {{{{void_damp, -.35f}},
                 {{void_mod, .35f}, {drift_wow, .35f}},
                 {{void_lowcut, .35f}},
                 {{void_gravity, -.35f}}}}});
  p.push_back({"DRONE", "Tide Without Moon",
               {{mix, 65}, {anchor_freq, 90}, {drift_depth, 30}, {drift_mix, 50},
                {echo_div, bar1}, {echo_feedback, 60}, {echo_tone, 2500}, {echo_lowcut, 150}, {echo_age, 60}, {echo_level, 55},
                {void_size, 90}, {void_decay, 40}, {void_damp, 50}, {void_mod, 40}, {void_lowcut, 100}, {void_level, 95}},
               {{echo_feedback, 85}, {echo_age, 80}, {void_decay, 80}},
               {{{{echo_feedback, .25f}, {echo_level, .25f}},
                 {{echo_age, .4f}, {echo_tone, -.35f}},
                 {{void_mod, .4f}},
                 {{drift_depth, .4f}, {drift_wow, .35f}}}}});
  p.push_back({"DRONE", "Long Breath",
               {{mix, 60}, {anchor_freq, 90}, {drift_depth, 15}, {echo_on, 0},
                {void_size, 85}, {void_decay, 20}, {void_predelay, 120}, {void_gravity, -70}, {void_mod, 35},
                {void_lowcut, 100}, {void_level, 100}},
               {{void_gravity, 40}, {void_decay, 40}},
               {{{{void_predelay, .35f}},
                 {{void_size, .2f}, {void_decay, .25f}},
                 {{void_damp, .4f}},
                 {{drift_depth, .45f}}}}});

  // ---- PULSE: chosen frequencies throb in time.
  p.push_back({"PULSE", "Breathing Walls",
               {{mix, 60}, {echo_on, 0}, {void_size, 75}, {void_decay, 6}, {void_lowcut, 120}, {void_level, 90},
                {pulse_on, 1}, {pulse_position, 1}, {pulse_bands, 3}, {pulse_smooth, 40},
                {band(2, pulse_b1_div), d4}, {band(2, pulse_b1_depth), 70},
                {band(3, pulse_b1_div), d8}, {band(3, pulse_b1_depth), 55}, {band(3, pulse_b1_shape), 2},
                {band(3, pulse_b1_phase), 90}},
               {{band(2, pulse_b1_depth), 95}, {band(3, pulse_b1_depth), 80}, {void_decay, 10}},
               {{{{band(2, pulse_b1_depth), .3f}},
                 {{band(3, pulse_b1_depth), .35f}},
                 {{void_decay, .3f}},
                 {{pulse_smooth, .4f}}}}});
  {
    PresetDef d{"PULSE", "Signal Lamp",
                {{mix, 55}, {echo_mode, 1}, {echo_div, d8}, {echo_feedback, 35}, {echo_level, 40},
                 {void_size, 60}, {void_decay, 3.5f}, {void_lowcut, 120}, {void_level, 85},
                 {pulse_on, 1}, {pulse_position, 1}, {pulse_bands, 4}, {pulse_smooth, 20},
                 {band(2, pulse_b1_source), 1}, {band(2, pulse_b1_div), d16}, {band(2, pulse_b1_depth), 85},
                 {band(3, pulse_b1_source), 1}, {band(3, pulse_b1_div), d16}, {band(3, pulse_b1_depth), 75},
                 {band(4, pulse_b1_div), d16}, {band(4, pulse_b1_depth), 60}, {band(4, pulse_b1_shape), 3}},
                {{pulse_smooth, 60}, {void_decay, 7}, {echo_feedback, 55}},
                {{{{band(2, pulse_b1_depth), .15f}},
                  {{band(4, pulse_b1_depth), .4f}},
                  {{echo_level, .35f}},
                  {{pulse_smooth, .45f}}}}};
    d.patterns[1] = 0x9249;  // every third sixteenth
    d.patterns[2] = 0x0F0F;  // four on, four off
    p.push_back(d);
  }
  p.push_back({"PULSE", "Focus in D",
               {{mix, 55}, {echo_on, 0}, {void_size, 80}, {void_decay, 10}, {void_lowcut, 110}, {void_level, 90},
                {pulse_on, 1}, {pulse_position, 1}, {pulse_bands, 2},
                {band(2, pulse_b1_div), d4}, {band(2, pulse_b1_depth), 35},
                {pulse_focus_on, 1}, {pulse_focus_note, 2}, {pulse_focus_harm, 4}, {pulse_focus_q, 25},
                {pulse_focus_depth, 80}, {pulse_focus_div, d8d}},
               {{pulse_focus_depth, 100}, {pulse_focus_q, 45}, {void_decay, 18}},
               {{{{pulse_focus_depth, .2f}},
                 {{pulse_focus_q, .35f}},
                 {{band(2, pulse_b1_depth), .45f}},
                 {{void_decay, .35f}}}}});

  // ---- MOTION: the pad moves by itself.
  p.push_back({"MOTION", "Drift Engine",
               {{mix, 55}, {drift_depth, 30}, {drift_mix, 50}, {echo_div, d4d}, {echo_feedback, 40}, {echo_level, 40},
                {void_size, 70}, {void_decay, 6}, {void_lowcut, 120}, {void_level, 90},
                {xy_current, 1}, {xy_radius, 40}, {xy_rate, bar2}},
               {{void_decay, 12}, {void_size, 85}, {echo_feedback, 55}, {drift_depth, 45}},
               {{{{void_decay, .4f}, {void_size, .25f}},
                 {{echo_level, .45f}, {echo_feedback, .3f}},
                 {{drift_depth, .5f}, {drift_wow, .4f}},
                 {{void_damp, .4f}, {mix, -.2f}}}}});
  p.push_back({"MOTION", "Wander Field",
               {{mix, 55}, {echo_div, d8d}, {echo_feedback, 35}, {echo_level, 35},
                {void_size, 75}, {void_decay, 7}, {void_lowcut, 120}, {void_level, 90},
                {pulse_on, 1}, {pulse_bands, 3}, {band(2, pulse_b1_div), d8}, {band(2, pulse_b1_depth), 50},
                {xy_current, 3}, {xy_radius, 50}, {xy_rate, bar4}},
               {{void_decay, 14}, {void_mod, 55}, {echo_level, 55}, {band(2, pulse_b1_depth), 75}},
               {{{{band(2, pulse_b1_depth), .45f}},
                 {{void_gravity, -.4f}},
                 {{echo_age, .45f}, {echo_tone, -.3f}},
                 {{void_mod, .45f}, {drift_flutter, .35f}}}}});

  // ---- MUTATE: harmonics, the climbing HALO, and a space that evolves.
  p.push_back({"MUTATE", "Glass Choir",
               {{mix, 60}, {anchor_freq, 90}, {drift_depth, 20}, {drift_mix, 40}, {echo_on, 0},
                {void_size, 85}, {void_decay, 12}, {void_predelay, 40}, {void_damp, 35}, {void_lowcut, 140},
                {void_mod, 35}, {void_level, 95},
                {mut_on, 1}, {harm_octave, 35}, {harm_fifth, 20}, {halo_climb, 55}, {halo_interval, 0},
                {halo_spread, 35}, {evolve_amount, 45}, {evolve_cycle, 2}, {evolve_formant, 70}},
               {{halo_climb, 80}, {evolve_formant, 95}, {void_decay, 25}, {harm_octave, 55}},
               {{{{halo_climb, .3f}, {halo_spread, .3f}},
                 {{evolve_formant, .3f}, {evolve_amount, .3f}},
                 {{harm_fifth, .4f}, {void_damp, -.25f}},
                 {{void_predelay, .35f}, {harm_octave, -.3f}}}}});
  p.push_back({"MUTATE", "Rust Organ",
               {{mix, 55}, {anchor_freq, 100}, {duck_amount, 10},
                {echo_div, d4}, {echo_feedback, 35}, {echo_tone, 3000}, {echo_drive, 40}, {echo_age, 40}, {echo_level, 40},
                {void_size, 70}, {void_decay, 6}, {void_damp, 55}, {void_lowcut, 120}, {void_level, 90},
                {mut_on, 1}, {harm_fifth, 45}, {harm_odd, 55}, {harm_sub, 30}, {halo_climb, 30}, {halo_interval, 1},
                {evolve_amount, 40}, {evolve_cycle, 1}, {evolve_tone, 70}, {evolve_formant, 30}},
               {{harm_odd, 80}, {echo_drive, 65}, {halo_climb, 50}, {void_decay, 10}},
               {{{{harm_odd, .3f}, {echo_drive, .3f}},
                 {{harm_sub, .4f}, {harm_fifth, -.25f}},
                 {{evolve_tone, .3f}, {evolve_amount, .35f}},
                 {{echo_age, .4f}, {halo_climb, .3f}}}}});
  p.push_back({"MUTATE", "Sinking Cathedral",
               {{mix, 65}, {anchor_freq, 80}, {drift_depth, 25}, {drift_wow, 25}, {drift_mix, 45}, {echo_on, 0},
                {void_size, 95}, {void_decay, 20}, {void_predelay, 60}, {void_damp, 45}, {void_lowcut, 90},
                {void_gravity, -40}, {void_mod, 40}, {void_level, 100},
                {mut_on, 1}, {harm_sub, 40}, {harm_octave, 15}, {halo_climb, 50}, {halo_interval, 2}, {halo_spread, 25},
                {evolve_amount, 55}, {evolve_cycle, 3}, {evolve_size, 80}, {evolve_formant, 40}},
               {{halo_climb, 75}, {void_decay, 40}, {void_gravity, -70}, {harm_sub, 60}},
               {{{{void_gravity, -.35f}, {halo_climb, .25f}},
                 {{evolve_size, .2f}, {evolve_amount, .3f}},
                 {{void_damp, .35f}, {harm_sub, .3f}},
                 {{halo_spread, .45f}, {evolve_tone, .3f}}}}});
  p.push_back({"MUTATE", "Living Freeze",
               {{mix, 70}, {anchor_freq, 80}, {drift_depth, 30}, {drift_wow, 30}, {drift_mix, 50}, {echo_on, 0},
                {void_size, 90}, {void_decay, 30}, {void_damp, 40}, {void_lowcut, 100}, {void_diffusion, 95},
                {void_mod, 45}, {void_width, 100}, {void_level, 100},
                {mut_on, 1}, {harm_octave, 20}, {halo_climb, 35}, {halo_interval, 3}, {halo_spread, 40},
                {evolve_amount, 80}, {evolve_cycle, 4}, {evolve_size, 60}, {evolve_tone, 60}, {evolve_formant, 85}},
               {{evolve_amount, 100}, {halo_climb, 60}, {evolve_cycle, 2}},
               {{{{evolve_formant, .15f}, {halo_spread, .3f}},
                 {{evolve_tone, .35f}, {void_mod, .3f}},
                 {{halo_climb, .35f}},
                 {{harm_octave, .4f}, {evolve_size, .3f}}}}});

  // ---- RIOT PAIRS: one for a NEON RIOT patch of the same name. Every one runs
  // two slow clocks that never line up - EVOLVE on its cycle, the puck on
  // CURRENT through four corners that each push the space a different way -
  // so a repeated phrase never comes back the same.
  const char *pair = "RIOT PAIRS";
  p.push_back({pair, "Snarl Bass",
               {{mix, 50}, {anchor_freq, 110}, {duck_amount, 15}, {duck_release, 350},
                {drift_depth, 20}, {drift_mix, 35},
                {echo_mode, 1}, {echo_div, d8d}, {echo_feedback, 40}, {echo_tone, 3800}, {echo_lowcut, 220},
                {echo_drive, 35}, {echo_age, 30}, {echo_level, 40},
                {void_size, 70}, {void_decay, 8}, {void_damp, 50}, {void_lowcut, 130}, {void_level, 90},
                {pulse_on, 1}, {pulse_bands, 3}, {band(3, pulse_b1_div), d8}, {band(3, pulse_b1_depth), 40}, {band(3, pulse_b1_shape), 2},
                {mut_on, 1}, {harm_odd, 25}, {halo_climb, 20}, {halo_interval, 1},
                {evolve_amount, 90}, {evolve_cycle, 3}, {evolve_tone, 70}, {evolve_formant, 75},
                {xy_current, 3}, {xy_radius, 60}, {xy_rate, bar4}},
               {{echo_feedback, 60}, {harm_odd, 45}, {void_decay, 10}, {evolve_amount, 90}},
               {{{{echo_feedback, .35f}, {halo_climb, .3f}},
                 {{harm_odd, .35f}, {echo_drive, .3f}},
                 {{evolve_formant, .35f}, {void_decay, .25f}},
                 {{band(3, pulse_b1_depth), .4f}, {void_gravity, -.4f}}}}});
  p.push_back({pair, "Rust Drone",
               {{mix, 65}, {out_gain, -1.5f}, {anchor_freq, 80}, {drift_depth, 30}, {drift_wow, 35}, {drift_mix, 50}, {echo_on, 0},
                {void_size, 90}, {void_decay, 25}, {void_predelay, 60}, {void_damp, 45}, {void_lowcut, 90},
                {void_diffusion, 95}, {void_mod, 45}, {void_gravity, -40}, {void_level, 80},
                {mut_on, 1}, {halo_climb, 40}, {halo_interval, 2}, {halo_spread, 35},
                {evolve_amount, 90}, {evolve_cycle, 4}, {evolve_size, 80}, {evolve_tone, 75}, {evolve_formant, 95},
                {xy_current, 2}, {xy_radius, 75}, {xy_rate, bar8}},
               {{halo_climb, 65}, {void_decay, 45}, {evolve_amount, 100}, {harm_odd, 30}},
               {{{{halo_climb, .2f}, {void_mod, .3f}},
                 {{harm_odd, .4f}, {evolve_tone, .3f}},
                 {{void_gravity, -.35f}, {void_damp, .3f}},
                 {{evolve_formant, .2f}, {halo_spread, .45f}}}}});
  p.push_back({pair, "Low Orbit",
               {{mix, 60}, {anchor_freq, 100}, {duck_amount, 10}, {drift_depth, 25}, {drift_mix, 40},
                {echo_div, d2}, {echo_feedback, 50}, {echo_tone, 3000}, {echo_lowcut, 200}, {echo_age, 50}, {echo_level, 45},
                {void_size, 80}, {void_decay, 12}, {void_damp, 45}, {void_lowcut, 110}, {void_level, 95},
                {pulse_on, 1}, {pulse_bands, 3}, {band(2, pulse_b1_div), bar2}, {band(2, pulse_b1_depth), 50}, {band(2, pulse_b1_shape), 4},
                {mut_on, 1}, {halo_climb, 30}, {halo_interval, 1}, {evolve_amount, 75}, {evolve_cycle, 3}, {evolve_formant, 55},
                {xy_current, 1}, {xy_radius, 70}, {xy_rate, bar8}},
               {{echo_feedback, 70}, {halo_climb, 50}, {void_decay, 20}},
               {{{{echo_age, .4f}, {echo_feedback, .25f}},
                 {{halo_climb, .35f}, {void_mod, .3f}},
                 {{band(2, pulse_b1_depth), .4f}, {evolve_tone, .3f}},
                 {{void_gravity, -.45f}, {echo_tone, -.3f}}}}});
  p.push_back({pair, "Glass Moths",
               {{mix, 65}, {anchor_freq, 90}, {drift_depth, 30}, {drift_flutter, 40}, {drift_width, 100}, {drift_mix, 50},
                {echo_mode, 1}, {echo_div, d8t}, {echo_feedback, 35}, {echo_tone, 7000}, {echo_lowcut, 300}, {echo_level, 35},
                {void_size, 80}, {void_decay, 10}, {void_damp, 25}, {void_lowcut, 150}, {void_mod, 40}, {void_level, 95},
                {mut_on, 1}, {harm_octave, 30}, {halo_climb, 55}, {halo_interval, 0}, {halo_spread, 50},
                {evolve_amount, 80}, {evolve_cycle, 3}, {evolve_formant, 85}, {evolve_tone, 50},
                {xy_current, 3}, {xy_radius, 80}, {xy_rate, bar4}},
               {{halo_climb, 80}, {harm_octave, 50}, {void_decay, 18}, {drift_flutter, 60}},
               {{{{halo_climb, .3f}, {halo_spread, .35f}},
                 {{drift_flutter, .4f}, {echo_feedback, .3f}},
                 {{evolve_formant, .15f}, {harm_octave, .35f}},
                 {{void_damp, .45f}, {halo_climb, -.3f}}}}});
  p.push_back({pair, "Abandoned Chamber",
               {{mix, 65}, {anchor_freq, 90}, {drift_depth, 15}, {drift_mix, 30},
                {echo_mode, 2}, {echo_div, d2}, {echo_feedback, 30}, {echo_tone, 2800}, {echo_lowcut, 200}, {echo_level, 35},
                {void_size, 85}, {void_decay, 18}, {void_predelay, 120}, {void_damp, 60}, {void_lowcut, 100},
                {void_mod, 35}, {void_level, 100},
                {mut_on, 1}, {halo_climb, 30}, {halo_interval, 2}, {evolve_amount, 85}, {evolve_cycle, 4},
                {evolve_formant, 90}, {evolve_size, 60},
                {xy_current, 2}, {xy_radius, 70}, {xy_rate, bar8}},
               {{void_decay, 35}, {halo_climb, 50}, {echo_level, 55}},
               {{{{void_predelay, .35f}, {echo_level, .3f}},
                 {{halo_climb, .35f}, {void_damp, -.3f}},
                 {{void_gravity, .4f}, {evolve_tone, .3f}},
                 {{echo_feedback, .35f}, {evolve_size, .3f}}}}});
  p.push_back({pair, "Black Reservoir",
               {{mix, 65}, {anchor_freq, 80}, {drift_depth, 20}, {drift_wow, 30}, {drift_mix, 40}, {echo_on, 0},
                {void_size, 95}, {void_decay, 30}, {void_predelay, 40}, {void_damp, 35}, {void_lowcut, 80},
                {void_gravity, -60}, {void_mod, 40}, {void_level, 100},
                {mut_on, 1}, {harm_sub, 35}, {halo_climb, 45}, {halo_interval, 2},
                {evolve_amount, 95}, {evolve_cycle, 3}, {evolve_size, 90}, {evolve_tone, 85}, {evolve_formant, 90},
                {xy_current, 3}, {xy_radius, 70}, {xy_rate, bar8}},
               {{void_gravity, -85}, {halo_climb, 65}, {harm_sub, 50}},
               {{{{harm_sub, .3f}, {void_gravity, -.3f}},
                 {{halo_climb, .3f}, {evolve_formant, .35f}},
                 {{void_damp, .3f}, {void_mod, .35f}},
                 {{harm_odd, .35f}, {evolve_size, .2f}}}}});
  p.push_back({pair, "Hammer-On Lead",
               {{mix, 50}, {anchor_freq, 120}, {duck_amount, 20}, {duck_release, 500}, {drift_depth, 20}, {drift_mix, 35},
                {echo_mode, 1}, {echo_div, d4d}, {echo_feedback, 45}, {echo_tone, 4500}, {echo_lowcut, 250}, {echo_level, 45},
                {void_size, 75}, {void_decay, 8}, {void_damp, 40}, {void_lowcut, 150}, {void_level, 90},
                {mut_on, 1}, {harm_fifth, 25}, {halo_climb, 40}, {halo_interval, 3}, {halo_spread, 40},
                {evolve_amount, 85}, {evolve_cycle, 3}, {evolve_formant, 80}, {out_gain, -2},
                {xy_current, 1}, {xy_radius, 55}, {xy_rate, bar4}},
               {{echo_feedback, 65}, {halo_climb, 60}, {void_decay, 14}},
               {{{{echo_feedback, .3f}, {echo_age, .35f}},
                 {{halo_climb, .3f}, {harm_fifth, .3f}},
                 {{duck_amount, .35f}, {evolve_formant, .3f}},
                 {{void_gravity, -.4f}, {echo_level, .25f}}}}});
  p.push_back({pair, "Bowed Cable Drone",
               {{mix, 60}, {out_gain, -3.5f}, {anchor_freq, 80}, {drift_depth, 25}, {drift_mix, 45}, {echo_on, 0},
                {void_size, 90}, {void_decay, 25}, {void_damp, 40}, {void_lowcut, 90}, {void_diffusion, 90},
                {void_mod, 50}, {void_level, 100},
                {mut_on, 1}, {harm_odd, 30}, {halo_climb, 45}, {halo_interval, 1}, {halo_spread, 35},
                {evolve_amount, 90}, {evolve_cycle, 4}, {evolve_formant, 75}, {evolve_size, 60},
                {xy_current, 3}, {xy_radius, 80}, {xy_rate, bar8}},
               {{halo_climb, 70}, {harm_odd, 50}, {void_decay, 45}},
               {{{{harm_odd, .3f}, {void_mod, .3f}},
                 {{halo_climb, .3f}, {halo_spread, .4f}},
                 {{evolve_tone, .4f}, {void_gravity, -.35f}},
                 {{harm_fifth, .4f}, {evolve_formant, .2f}}}}});
  p.push_back({pair, "Sodium Horizon",
               {{mix, 65}, {anchor_freq, 90}, {drift_depth, 35}, {drift_width, 100}, {drift_mix, 55},
                {echo_div, d4d}, {echo_feedback, 35}, {echo_tone, 5000}, {echo_lowcut, 250}, {echo_level, 30},
                {void_size, 90}, {void_decay, 20}, {void_damp, 35}, {void_lowcut, 110}, {void_width, 100}, {void_level, 100},
                {mut_on, 1}, {harm_octave, 25}, {halo_climb, 45}, {halo_interval, 0}, {halo_spread, 60},
                {evolve_amount, 85}, {evolve_cycle, 4}, {evolve_size, 70}, {evolve_formant, 55},
                {xy_current, 2}, {xy_radius, 85}, {xy_rate, bar8}},
               {{halo_climb, 70}, {void_decay, 35}, {harm_octave, 40}},
               {{{{halo_climb, .3f}, {drift_depth, .3f}},
                 {{harm_octave, .35f}, {echo_feedback, .3f}},
                 {{void_gravity, -.4f}, {evolve_formant, .3f}},
                 {{halo_spread, .3f}, {void_damp, .35f}}}}});
  p.push_back({pair, "Swarm Choir",
               {{mix, 65}, {anchor_freq, 90}, {drift_depth, 25}, {drift_mix, 45},
                {echo_mode, 1}, {echo_div, d4}, {echo_feedback, 30}, {echo_tone, 4000}, {echo_lowcut, 250}, {echo_level, 30},
                {void_size, 85}, {void_decay, 14}, {void_damp, 40}, {void_lowcut, 120}, {void_level, 95},
                {mut_on, 1}, {harm_fifth, 20}, {halo_climb, 35}, {halo_interval, 3},
                {evolve_amount, 90}, {evolve_cycle, 3}, {evolve_formant, 95}, {evolve_tone, 55},
                {xy_current, 3}, {xy_radius, 75}, {xy_rate, bar4}},
               {{evolve_amount, 100}, {halo_climb, 55}, {void_decay, 24}},
               {{{{evolve_tone, .35f}, {halo_climb, .3f}},
                 {{harm_fifth, .35f}, {echo_feedback, .3f}},
                 {{void_mod, .4f}, {evolve_size, .3f}},
                 {{harm_octave, .35f}, {void_gravity, -.35f}}}}});
  p.push_back({pair, "Mantle Bloom",
               {{mix, 65}, {anchor_freq, 90}, {drift_depth, 25}, {drift_wow, 25}, {drift_mix, 45}, {echo_on, 0},
                {void_size, 90}, {void_decay, 18}, {void_damp, 40}, {void_lowcut, 100}, {void_gravity, -70}, {void_level, 100},
                {pulse_on, 1}, {pulse_bands, 3}, {band(3, pulse_b1_div), bar4}, {band(3, pulse_b1_depth), 40}, {band(3, pulse_b1_shape), 4},
                {mut_on, 1}, {harm_octave, 15}, {halo_climb, 40}, {halo_interval, 0},
                {evolve_amount, 80}, {evolve_cycle, 4}, {evolve_size, 60}, {evolve_formant, 60},
                {xy_current, 2}, {xy_radius, 70}, {xy_rate, bar8}},
               {{void_gravity, -90}, {halo_climb, 60}, {void_decay, 30}},
               {{{{halo_climb, .35f}, {band(3, pulse_b1_depth), .3f}},
                 {{harm_octave, .35f}, {void_mod, .3f}},
                 {{evolve_tone, .35f}, {void_damp, .3f}},
                 {{void_gravity, .5f}, {evolve_formant, .3f}}}}});
  p.push_back({pair, "Tidal Armature",
               {{mix, 60}, {out_gain, -2.5f}, {anchor_freq, 90}, {drift_depth, 20}, {drift_mix, 35},
                {echo_div, d2}, {echo_feedback, 45}, {echo_tone, 3500}, {echo_lowcut, 200}, {echo_age, 60}, {echo_level, 40},
                {void_size, 85}, {void_decay, 15}, {void_damp, 45}, {void_lowcut, 110}, {void_level, 95},
                {mut_on, 1}, {harm_odd, 35}, {halo_climb, 30}, {halo_interval, 1},
                {evolve_amount, 80}, {evolve_cycle, 3}, {evolve_tone, 70}, {evolve_formant, 50},
                {xy_current, 2}, {xy_radius, 80}, {xy_rate, bar8}},
               {{echo_age, 85}, {harm_odd, 55}, {halo_climb, 50}},
               {{{{echo_age, .35f}, {echo_feedback, .25f}},
                 {{harm_odd, .3f}, {evolve_tone, .3f}},
                 {{halo_climb, .35f}, {void_decay, .25f}},
                 {{void_gravity, -.4f}, {evolve_formant, .4f}}}}});
  p.push_back({pair, "Relay Cathedral",
               {{mix, 60}, {anchor_freq, 90}, {drift_depth, 15}, {drift_mix, 30}, {echo_on, 0},
                {void_size, 95}, {void_decay, 22}, {void_predelay, 50}, {void_damp, 35}, {void_lowcut, 110}, {void_level, 100},
                {pulse_on, 1}, {pulse_bands, 3},
                {band(2, pulse_b1_div), d8}, {band(2, pulse_b1_depth), 55}, {band(2, pulse_b1_source), 1},
                {band(3, pulse_b1_div), d16}, {band(3, pulse_b1_depth), 45}, {band(3, pulse_b1_source), 1},
                {mut_on, 1}, {halo_climb, 50}, {halo_interval, 0}, {halo_spread, 40},
                {evolve_amount, 75}, {evolve_cycle, 4}, {evolve_formant, 60},
                {xy_current, 1}, {xy_radius, 65}, {xy_rate, bar8}},
               {{halo_climb, 70}, {band(2, pulse_b1_depth), 75}, {void_decay, 35}},
               {{{{band(2, pulse_b1_depth), .35f}, {halo_climb, .3f}},
                 {{band(3, pulse_b1_depth), .4f}, {evolve_tone, .3f}},
                 {{void_gravity, -.4f}, {halo_spread, .35f}},
                 {{harm_octave, .35f}, {void_mod, .35f}}}},
               {65535, 0x9249, 0xb6db, 65535, 65535, 65535}});
  p.push_back({pair, "Nightwell Vapour",
               {{mix, 70}, {anchor_freq, 80}, {drift_depth, 25}, {drift_wow, 40}, {drift_mix, 45}, {echo_on, 0},
                {void_size, 90}, {void_decay, 24}, {void_damp, 40}, {void_lowcut, 90}, {void_mod, 45}, {void_level, 100},
                {mut_on, 1}, {harm_sub, 15}, {halo_climb, 35}, {halo_interval, 2},
                {evolve_amount, 95}, {evolve_cycle, 3}, {evolve_tone, 80}, {evolve_formant, 90}, {evolve_size, 70},
                {xy_current, 3}, {xy_radius, 70}, {xy_rate, bar8}},
               {{halo_climb, 55}, {void_decay, 40}, {evolve_amount, 100}},
               {{{{evolve_tone, .2f}, {void_damp, -.3f}},
                 {{halo_climb, .35f}, {drift_wow, .3f}},
                 {{harm_sub, .35f}, {void_gravity, -.35f}},
                 {{halo_spread, .4f}, {evolve_formant, .3f}}}}});
  p.push_back({pair, "Faultline Halo",
               {{mix, 65}, {anchor_freq, 90}, {drift_depth, 20}, {drift_mix, 35},
                {echo_mode, 2}, {echo_div, bar1}, {echo_feedback, 35}, {echo_tone, 3500}, {echo_lowcut, 200}, {echo_level, 30},
                {void_size, 90}, {void_decay, 18}, {void_damp, 40}, {void_lowcut, 110}, {void_gravity, -50}, {void_level, 100},
                {mut_on, 1}, {harm_fifth, 15}, {halo_climb, 55}, {halo_interval, 3}, {halo_spread, 45},
                {evolve_amount, 85}, {evolve_cycle, 3}, {evolve_formant, 60},
                {xy_current, 2}, {xy_radius, 80}, {xy_rate, bar8}},
               {{halo_climb, 80}, {void_gravity, -80}, {void_decay, 30}},
               {{{{halo_climb, .3f}, {echo_level, .3f}},
                 {{harm_fifth, .35f}, {void_mod, .35f}},
                 {{void_gravity, -.35f}, {evolve_tone, .35f}},
                 {{halo_spread, .4f}, {echo_feedback, .3f}}}}});
  p.push_back({pair, "Ghosts in the Busbar",
               {{mix, 60}, {anchor_freq, 90}, {drift_depth, 25}, {drift_flutter, 30}, {drift_mix, 40},
                {echo_mode, 1}, {echo_div, d4d}, {echo_feedback, 40}, {echo_tone, 4000}, {echo_lowcut, 250}, {echo_age, 40}, {echo_level, 35},
                {void_size, 85}, {void_decay, 14}, {void_damp, 45}, {void_lowcut, 120}, {void_level, 95},
                {pulse_on, 1}, {pulse_bands, 3}, {band(2, pulse_b1_div), d8}, {band(2, pulse_b1_depth), 40}, {band(2, pulse_b1_source), 1},
                {mut_on, 1}, {harm_odd, 40}, {halo_climb, 35}, {halo_interval, 1},
                {evolve_amount, 95}, {evolve_cycle, 3}, {evolve_formant, 80},
                {xy_current, 3}, {xy_radius, 85}, {xy_rate, bar4}},
               {{harm_odd, 60}, {echo_feedback, 60}, {halo_climb, 55}},
               {{{{harm_odd, .3f}, {echo_age, .35f}},
                 {{band(2, pulse_b1_depth), .45f}, {halo_climb, .3f}},
                 {{drift_flutter, .4f}, {evolve_tone, .35f}},
                 {{void_gravity, -.4f}, {echo_feedback, .25f}}}},
               {65535, 0xa5a5, 65535, 65535, 65535, 65535}});
  // ---- 0.6.0: CINEMATIC, SHIMMER, FROZEN, RHYTHMIC, ECHOES, TEXTURE, and more
  // BASS SPACE and DRONE (indices 32-79).
  p.push_back({"CINEMATIC", "Long Corridor",
               {{mix, 60}, {anchor_freq, 90}, {drift_depth, 20}, {drift_mix, 35}, {echo_mode, 2}, {echo_div, d2}, {echo_feedback, 30}, {echo_tone, 3000}, {echo_lowcut, 220}, {echo_level, 25}, {void_size, 85}, {void_decay, 14}, {void_predelay, 180}, {void_damp, 45}, {void_lowcut, 110}, {void_level, 95}, {halo_climb, 25}, {halo_interval, 2}, {mut_on, 1}, {evolve_amount, 70}, {evolve_cycle, 4}, {evolve_formant, 60}, {xy_current, 2}, {xy_radius, 70}, {xy_rate, bar8}},
               {{void_decay, 28}, {void_predelay, 300}, {halo_climb, 45}},
               {{{{void_predelay, 0.3f}, {echo_level, 0.3f}}, {{halo_climb, 0.35f}, {evolve_tone, 0.3f}}, {{void_gravity, -0.4f}, {void_damp, 0.25f}}, {{echo_feedback, 0.3f}, {evolve_formant, 0.3f}}}}});
  p.push_back({"CINEMATIC", "Ember Field",
               {{mix, 65}, {anchor_freq, 90}, {drift_depth, 25}, {drift_wow, 30}, {drift_mix, 45}, {echo_on, 0}, {void_size, 90}, {void_decay, 20}, {void_damp, 40}, {void_lowcut, 100}, {void_gravity, -40}, {void_level, 100}, {pulse_on, 1}, {pulse_bands, 3}, {band(3, pulse_b1_div), bar4}, {band(3, pulse_b1_depth), 35}, {band(3, pulse_b1_shape), 4}, {harm_odd, 20}, {halo_climb, 35}, {halo_interval, 1}, {mut_on, 1}, {evolve_amount, 80}, {evolve_cycle, 4}, {evolve_tone, 70}, {evolve_formant, 55}, {xy_current, 3}, {xy_radius, 75}, {xy_rate, bar8}},
               {{void_gravity, -70}, {harm_odd, 40}, {halo_climb, 55}},
               {{{{harm_odd, 0.35f}, {void_mod, 0.3f}}, {{halo_climb, 0.3f}, {band(3, pulse_b1_depth), 0.35f}}, {{evolve_tone, 0.25f}, {void_damp, -0.3f}}, {{void_gravity, -0.35f}, {drift_wow, 0.35f}}}}});
  p.push_back({"CINEMATIC", "Cold Open",
               {{mix, 55}, {anchor_freq, 100}, {echo_div, d4d}, {echo_feedback, 30}, {echo_tone, 2500}, {echo_lowcut, 200}, {echo_level, 30}, {void_size, 75}, {void_decay, 16}, {void_damp, 70}, {void_lowcut, 140}, {void_level, 90}, {harm_sub, 15}, {halo_climb, 30}, {halo_interval, 2}, {mut_on, 1}, {evolve_amount, 75}, {evolve_cycle, 3}, {evolve_formant, 50}, {evolve_tone, 60}, {xy_current, 2}, {xy_radius, 60}, {xy_rate, bar4}},
               {{void_decay, 30}, {halo_climb, 50}, {void_damp, 80}},
               {{{{echo_feedback, 0.3f}, {echo_age, 0.35f}}, {{halo_climb, 0.3f}, {harm_sub, 0.25f}}, {{void_damp, -0.35f}, {evolve_formant, 0.3f}}, {{void_predelay, 0.4f}, {void_gravity, -0.3f}}}}});
  p.push_back({"CINEMATIC", "Slow Reveal",
               {{mix, 65}, {anchor_freq, 90}, {drift_depth, 20}, {drift_mix, 35}, {echo_on, 0}, {void_size, 90}, {void_decay, 22}, {void_predelay, 60}, {void_damp, 40}, {void_lowcut, 100}, {void_gravity, -85}, {void_level, 100}, {halo_climb, 40}, {halo_interval, 0}, {halo_spread, 45}, {mut_on, 1}, {evolve_amount, 85}, {evolve_cycle, 4}, {evolve_size, 70}, {evolve_formant, 50}, {xy_current, 1}, {xy_radius, 65}, {xy_rate, bar8}},
               {{void_gravity, -100}, {void_decay, 35}, {halo_climb, 60}},
               {{{{halo_climb, 0.3f}, {halo_spread, 0.3f}}, {{void_gravity, 0.3f}, {void_mod, 0.35f}}, {{evolve_size, 0.25f}, {evolve_tone, 0.35f}}, {{void_damp, 0.35f}, {harm_octave, 0.3f}}}}});
  p.push_back({"CINEMATIC", "Last Light",
               {{mix, 65}, {anchor_freq, 90}, {drift_depth, 30}, {drift_mix, 45}, {echo_on, 0}, {void_size, 85}, {void_decay, 18}, {void_damp, 25}, {void_lowcut, 130}, {void_mod, 40}, {void_level, 95}, {harm_octave, 20}, {halo_climb, 50}, {halo_interval, 0}, {halo_spread, 40}, {mut_on, 1}, {evolve_amount, 80}, {evolve_cycle, 4}, {evolve_formant, 70}, {xy_current, 3}, {xy_radius, 80}, {xy_rate, bar8}},
               {{halo_climb, 70}, {void_decay, 30}, {harm_octave, 35}},
               {{{{halo_climb, 0.3f}, {drift_depth, 0.3f}}, {{harm_octave, 0.35f}, {void_damp, 0.3f}}, {{evolve_formant, 0.2f}, {void_gravity, -0.4f}}, {{halo_spread, 0.4f}, {evolve_tone, 0.3f}}}}});
  p.push_back({"CINEMATIC", "Night Exterior",
               {{mix, 55}, {anchor_freq, 100}, {duck_amount, 15}, {duck_release, 600}, {drift_depth, 20}, {drift_mix, 35}, {echo_mode, 1}, {echo_div, d8d}, {echo_feedback, 35}, {echo_tone, 3000}, {echo_lowcut, 250}, {echo_age, 40}, {echo_level, 30}, {void_size, 80}, {void_decay, 12}, {void_damp, 60}, {void_lowcut, 120}, {void_level, 90}, {halo_climb, 20}, {halo_interval, 2}, {mut_on, 1}, {evolve_amount, 70}, {evolve_cycle, 3}, {evolve_tone, 80}, {evolve_formant, 45}, {xy_current, 3}, {xy_radius, 60}, {xy_rate, bar4}},
               {{echo_feedback, 55}, {void_decay, 22}, {evolve_amount, 90}},
               {{{{echo_age, 0.35f}, {echo_feedback, 0.25f}}, {{halo_climb, 0.35f}, {void_damp, -0.3f}}, {{duck_amount, 0.35f}, {evolve_tone, 0.15f}}, {{void_gravity, -0.4f}, {echo_level, 0.3f}}}}});
  p.push_back({"CINEMATIC", "Underscore",
               {{mix, 45}, {anchor_freq, 110}, {duck_amount, 20}, {duck_release, 700}, {drift_depth, 15}, {drift_mix, 30}, {echo_on, 0}, {void_size, 70}, {void_decay, 9}, {void_damp, 40}, {void_lowcut, 140}, {void_level, 90}, {halo_climb, 15}, {halo_interval, 0}, {mut_on, 1}, {evolve_amount, 85}, {evolve_cycle, 3}, {evolve_formant, 70}, {evolve_tone, 65}, {xy_current, 2}, {xy_radius, 50}, {xy_rate, bar8}},
               {{void_decay, 16}, {evolve_amount, 100}, {halo_climb, 30}},
               {{{{void_decay, 0.25f}, {evolve_formant, 0.3f}}, {{halo_climb, 0.3f}, {void_mod, 0.3f}}, {{duck_amount, 0.3f}, {void_damp, 0.3f}}, {{drift_depth, 0.35f}, {evolve_tone, 0.3f}}}}});
  p.push_back({"CINEMATIC", "Title Card",
               {{mix, 70}, {anchor_freq, 80}, {drift_depth, 25}, {drift_mix, 40}, {echo_on, 0}, {void_size, 100}, {void_decay, 35}, {void_predelay, 80}, {void_damp, 40}, {void_lowcut, 90}, {void_gravity, -50}, {void_level, 100}, {harm_octave, 25}, {halo_climb, 55}, {halo_interval, 3}, {halo_spread, 45}, {mut_on, 1}, {evolve_amount, 85}, {evolve_cycle, 4}, {evolve_size, 60}, {evolve_formant, 60}, {xy_current, 2}, {xy_radius, 85}, {xy_rate, bar8}, {out_gain, -1.5f}},
               {{halo_climb, 75}, {void_decay, 55}, {void_gravity, -80}},
               {{{{halo_climb, 0.25f}, {harm_octave, 0.3f}}, {{void_mod, 0.4f}, {evolve_tone, 0.3f}}, {{void_gravity, -0.35f}, {evolve_formant, 0.3f}}, {{halo_spread, 0.4f}, {void_damp, 0.3f}}}}});
  p.push_back({"SHIMMER", "Ascent Glass",
               {{mix, 60}, {anchor_freq, 100}, {drift_depth, 20}, {drift_mix, 35}, {echo_on, 0}, {void_size, 90}, {void_decay, 20}, {void_damp, 25}, {void_lowcut, 150}, {void_level, 95}, {harm_octave, 30}, {halo_climb, 70}, {halo_interval, 0}, {halo_spread, 40}, {mut_on, 1}, {evolve_amount, 60}, {evolve_cycle, 4}, {evolve_formant, 40}, {xy_current, 2}, {xy_radius, 60}, {xy_rate, bar8}, {out_gain, -1.5f}},
               {{halo_climb, 90}, {void_decay, 35}, {harm_octave, 45}},
               {{{{halo_spread, 0.4f}, {void_mod, 0.3f}}, {{harm_octave, 0.3f}, {void_damp, 0.3f}}, {{evolve_formant, 0.35f}, {evolve_tone, 0.3f}}, {{halo_climb, -0.35f}, {void_gravity, -0.4f}}}}});
  p.push_back({"SHIMMER", "Choir Loft",
               {{mix, 60}, {anchor_freq, 100}, {drift_depth, 20}, {drift_mix, 35}, {echo_on, 0}, {void_size, 85}, {void_decay, 16}, {void_predelay, 50}, {void_damp, 35}, {void_lowcut, 140}, {void_level, 95}, {harm_fifth, 25}, {halo_climb, 55}, {halo_interval, 3}, {mut_on, 1}, {evolve_amount, 85}, {evolve_cycle, 3}, {evolve_formant, 95}, {xy_current, 3}, {xy_radius, 65}, {xy_rate, bar4}},
               {{halo_climb, 75}, {evolve_amount, 100}, {void_decay, 26}},
               {{{{harm_fifth, 0.35f}, {halo_climb, 0.25f}}, {{evolve_tone, 0.35f}, {void_mod, 0.3f}}, {{void_predelay, 0.4f}, {halo_spread, 0.35f}}, {{harm_octave, 0.35f}, {void_gravity, -0.35f}}}}});
  p.push_back({"SHIMMER", "Prism Tail",
               {{mix, 60}, {anchor_freq, 100}, {drift_depth, 25}, {drift_mix, 40}, {echo_on, 0}, {void_size, 85}, {void_decay, 18}, {void_damp, 30}, {void_lowcut, 140}, {void_level, 95}, {harm_odd, 15}, {halo_climb, 60}, {halo_interval, 1}, {halo_spread, 70}, {mut_on, 1}, {evolve_amount, 70}, {evolve_cycle, 3}, {evolve_tone, 60}, {evolve_formant, 50}, {xy_current, 3}, {xy_radius, 70}, {xy_rate, bar4}},
               {{halo_climb, 80}, {halo_spread, 90}, {harm_odd, 30}},
               {{{{halo_spread, 0.25f}, {evolve_tone, 0.3f}}, {{harm_odd, 0.35f}, {void_mod, 0.3f}}, {{halo_climb, 0.3f}, {void_damp, 0.3f}}, {{evolve_formant, 0.4f}, {void_gravity, -0.35f}}}}});
  p.push_back({"SHIMMER", "Upper Air",
               {{mix, 55}, {anchor_freq, 110}, {drift_depth, 20}, {drift_flutter, 30}, {drift_mix, 35}, {echo_mode, 1}, {echo_div, d4}, {echo_feedback, 30}, {echo_tone, 8000}, {echo_lowcut, 400}, {echo_level, 25}, {void_size, 80}, {void_decay, 14}, {void_damp, 25}, {void_lowcut, 160}, {void_level, 90}, {halo_climb, 55}, {halo_interval, 0}, {halo_spread, 50}, {mut_on, 1}, {evolve_amount, 65}, {evolve_cycle, 3}, {evolve_formant, 45}, {xy_current, 1}, {xy_radius, 60}, {xy_rate, bar4}},
               {{halo_climb, 75}, {echo_feedback, 50}, {void_decay, 22}},
               {{{{echo_feedback, 0.3f}, {drift_flutter, 0.3f}}, {{halo_climb, 0.3f}, {halo_spread, 0.3f}}, {{evolve_tone, 0.35f}, {void_mod, 0.35f}}, {{echo_level, 0.3f}, {void_gravity, -0.35f}}}}});
  p.push_back({"SHIMMER", "Octave Rain",
               {{mix, 55}, {anchor_freq, 110}, {drift_depth, 15}, {drift_mix, 30}, {echo_div, d8}, {echo_feedback, 35}, {echo_tone, 6000}, {echo_lowcut, 300}, {echo_level, 30}, {void_size, 75}, {void_decay, 12}, {void_damp, 30}, {void_lowcut, 150}, {void_level, 90}, {pulse_on, 1}, {pulse_bands, 3}, {band(3, pulse_b1_div), d16}, {band(3, pulse_b1_depth), 45}, {band(3, pulse_b1_shape), 2}, {halo_climb, 60}, {halo_interval, 0}, {mut_on, 1}, {evolve_amount, 60}, {evolve_cycle, 2}, {evolve_formant, 50}, {xy_current, 3}, {xy_radius, 55}, {xy_rate, bar4}},
               {{band(3, pulse_b1_depth), 70}, {halo_climb, 80}, {echo_feedback, 50}},
               {{{{band(3, pulse_b1_depth), 0.3f}, {echo_feedback, 0.25f}}, {{halo_climb, 0.3f}, {void_decay, 0.25f}}, {{evolve_formant, 0.35f}, {halo_spread, 0.35f}}, {{void_damp, 0.35f}, {echo_level, 0.3f}}}}});
  p.push_back({"SHIMMER", "Fifth Heaven",
               {{mix, 65}, {anchor_freq, 90}, {drift_depth, 20}, {drift_mix, 35}, {echo_on, 0}, {void_size, 90}, {void_decay, 25}, {void_damp, 35}, {void_lowcut, 120}, {void_gravity, -40}, {void_level, 95}, {harm_fifth, 30}, {halo_climb, 70}, {halo_interval, 1}, {halo_spread, 35}, {mut_on, 1}, {evolve_amount, 80}, {evolve_cycle, 4}, {evolve_formant, 60}, {xy_current, 1}, {xy_radius, 60}, {xy_rate, bar8}, {out_gain, -1}},
               {{halo_climb, 85}, {void_decay, 40}, {harm_fifth, 45}},
               {{{{harm_fifth, 0.3f}, {void_mod, 0.3f}}, {{halo_spread, 0.4f}, {evolve_tone, 0.3f}}, {{void_gravity, -0.35f}, {halo_climb, 0.2f}}, {{harm_octave, 0.35f}, {void_damp, 0.3f}}}}});
  p.push_back({"FROZEN", "Ice Shelf",
               {{mix, 70}, {anchor_freq, 80}, {drift_depth, 25}, {drift_mix, 40}, {echo_on, 0}, {void_size, 95}, {void_decay, 40}, {void_damp, 30}, {void_lowcut, 100}, {void_diffusion, 100}, {void_mod, 50}, {void_level, 100}, {halo_climb, 30}, {halo_interval, 0}, {mut_on, 1}, {evolve_amount, 85}, {evolve_cycle, 4}, {evolve_formant, 70}, {evolve_tone, 50}, {xy_current, 2}, {xy_radius, 60}, {xy_rate, bar8}},
               {{void_decay, 70}, {evolve_amount, 100}, {halo_climb, 50}},
               {{{{void_mod, 0.3f}, {evolve_formant, 0.25f}}, {{halo_climb, 0.35f}, {halo_spread, 0.3f}}, {{evolve_tone, 0.35f}, {void_damp, 0.3f}}, {{harm_octave, 0.4f}, {evolve_size, 0.3f}}}}});
  p.push_back({"FROZEN", "Held Breath",
               {{mix, 70}, {anchor_freq, 80}, {drift_depth, 20}, {drift_wow, 30}, {drift_mix, 40}, {echo_on, 0}, {void_size, 90}, {void_decay, 30}, {void_damp, 40}, {void_lowcut, 90}, {void_gravity, -60}, {void_level, 100}, {halo_climb, 25}, {halo_interval, 2}, {mut_on, 1}, {evolve_amount, 90}, {evolve_cycle, 3}, {evolve_formant, 90}, {evolve_size, 70}, {xy_current, 3}, {xy_radius, 55}, {xy_rate, bar8}},
               {{void_decay, 50}, {void_gravity, -85}, {halo_climb, 40}},
               {{{{evolve_size, 0.25f}, {void_mod, 0.35f}}, {{halo_climb, 0.35f}, {drift_wow, 0.3f}}, {{void_gravity, 0.4f}, {evolve_tone, 0.35f}}, {{harm_sub, 0.3f}, {void_damp, 0.3f}}}}});
  p.push_back({"FROZEN", "Still Water",
               {{mix, 65}, {anchor_freq, 80}, {drift_depth, 15}, {drift_mix, 30}, {echo_on, 0}, {void_size, 90}, {void_decay, 35}, {void_damp, 40}, {void_lowcut, 100}, {void_mod, 20}, {void_level, 100}, {harm_octave, 15}, {halo_climb, 20}, {halo_interval, 0}, {halo_spread, 20}, {mut_on, 1}, {evolve_amount, 70}, {evolve_cycle, 4}, {evolve_tone, 70}, {evolve_formant, 45}, {xy_current, 1}, {xy_radius, 45}, {xy_rate, bar8}},
               {{void_decay, 55}, {evolve_amount, 90}, {void_mod, 35}},
               {{{{void_mod, 0.3f}, {halo_climb, 0.25f}}, {{evolve_formant, 0.35f}, {harm_octave, 0.3f}}, {{evolve_tone, 0.25f}, {void_damp, 0.3f}}, {{halo_spread, 0.4f}, {drift_depth, 0.35f}}}}});
  p.push_back({"FROZEN", "Permafrost",
               {{mix, 70}, {anchor_freq, 80}, {drift_depth, 20}, {drift_mix, 35}, {echo_on, 0}, {void_size, 95}, {void_decay, 50}, {void_damp, 55}, {void_lowcut, 80}, {void_level, 95}, {harm_sub, 20}, {halo_climb, 40}, {halo_interval, 2}, {mut_on, 1}, {evolve_amount, 85}, {evolve_cycle, 4}, {evolve_size, 80}, {evolve_formant, 60}, {xy_current, 3}, {xy_radius, 70}, {xy_rate, bar8}, {out_gain, -1}},
               {{void_decay, 80}, {halo_climb, 60}, {harm_sub, 35}},
               {{{{harm_sub, 0.3f}, {halo_climb, 0.25f}}, {{void_damp, -0.35f}, {evolve_formant, 0.3f}}, {{evolve_tone, 0.4f}, {void_mod, 0.35f}}, {{harm_odd, 0.35f}, {evolve_size, 0.2f}}}}});
  p.push_back({"FROZEN", "Glass Lung",
               {{mix, 70}, {anchor_freq, 90}, {drift_depth, 25}, {drift_mix, 40}, {echo_on, 0}, {void_size, 85}, {void_decay, 30}, {void_damp, 20}, {void_lowcut, 120}, {void_level, 95}, {harm_octave, 30}, {halo_climb, 40}, {halo_interval, 3}, {halo_spread, 50}, {mut_on, 1}, {evolve_amount, 85}, {evolve_cycle, 3}, {evolve_formant, 85}, {xy_current, 2}, {xy_radius, 75}, {xy_rate, bar4}, {out_gain, -1}},
               {{halo_climb, 60}, {void_decay, 50}, {harm_octave, 45}},
               {{{{harm_octave, 0.3f}, {void_mod, 0.3f}}, {{halo_spread, 0.35f}, {evolve_tone, 0.35f}}, {{halo_climb, 0.3f}, {void_damp, 0.3f}}, {{evolve_size, 0.4f}, {void_gravity, -0.35f}}}}});
  p.push_back({"FROZEN", "Suspended",
               {{mix, 70}, {anchor_freq, 80}, {drift_depth, 30}, {drift_wow, 30}, {drift_mix, 45}, {echo_on, 0}, {void_size, 100}, {void_decay, 45}, {void_damp, 40}, {void_lowcut, 90}, {void_mod, 40}, {void_level, 95}, {harm_odd, 20}, {halo_climb, 35}, {halo_interval, 1}, {mut_on, 1}, {evolve_amount, 95}, {evolve_cycle, 4}, {evolve_formant, 80}, {evolve_size, 60}, {xy_current, 3}, {xy_radius, 85}, {xy_rate, bar8}},
               {{void_decay, 70}, {halo_climb, 55}, {harm_odd, 35}},
               {{{{harm_odd, 0.3f}, {drift_wow, 0.3f}}, {{halo_climb, 0.3f}, {halo_spread, 0.4f}}, {{evolve_tone, 0.4f}, {void_damp, 0.3f}}, {{void_mod, 0.35f}, {evolve_size, 0.3f}}}}});
  p.push_back({"RHYTHMIC", "Tremor Grid",
               {{mix, 55}, {anchor_freq, 110}, {drift_depth, 10}, {drift_mix, 25}, {echo_on, 0}, {void_size, 65}, {void_decay, 6}, {void_damp, 45}, {void_lowcut, 150}, {void_level, 95}, {pulse_on, 1}, {pulse_bands, 4}, {pulse_smooth, 25}, {band(2, pulse_b1_div), d16}, {band(2, pulse_b1_depth), 60}, {band(2, pulse_b1_shape), 3}, {band(3, pulse_b1_div), d8}, {band(3, pulse_b1_depth), 50}, {band(4, pulse_b1_div), d16}, {band(4, pulse_b1_depth), 55}, {band(4, pulse_b1_source), 1}, {mut_on, 1}, {evolve_amount, 40}, {evolve_cycle, 3}, {evolve_formant, 40}, {xy_current, 1}, {xy_radius, 45}, {xy_rate, bar4}},
               {{band(2, pulse_b1_depth), 85}, {band(3, pulse_b1_depth), 75}, {void_decay, 10}},
               {{{{band(2, pulse_b1_depth), 0.3f}, {pulse_smooth, -0.2f}}, {{band(4, pulse_b1_depth), 0.35f}, {void_decay, 0.25f}}, {{band(3, pulse_b1_depth), 0.35f}, {evolve_tone, 0.3f}}, {{void_mod, 0.35f}, {pulse_smooth, 0.35f}}}},
               {65535, 65535, 65535, 0xb6b6, 65535, 65535}});
  p.push_back({"RHYTHMIC", "Heartline",
               {{mix, 55}, {out_gain, 2.5f}, {anchor_freq, 100}, {drift_depth, 10}, {drift_mix, 25}, {echo_div, d4}, {echo_feedback, 25}, {echo_tone, 3000}, {echo_level, 25}, {void_size, 70}, {void_decay, 5}, {void_damp, 50}, {void_lowcut, 130}, {void_level, 95}, {pulse_on, 1}, {pulse_sub, 1}, {pulse_bands, 2}, {pulse_smooth, 35}, {band(1, pulse_b1_div), d4}, {band(1, pulse_b1_depth), 60}, {band(1, pulse_b1_shape), 4}, {band(2, pulse_b1_div), d4}, {band(2, pulse_b1_depth), 45}, {band(2, pulse_b1_shape), 4}, {band(2, pulse_b1_phase), 90}, {mut_on, 1}, {evolve_amount, 40}, {evolve_cycle, 3}, {evolve_tone, 50}, {xy_current, 2}, {xy_radius, 40}, {xy_rate, bar4}},
               {{band(1, pulse_b1_depth), 80}, {band(2, pulse_b1_depth), 65}, {void_decay, 8}},
               {{{{band(1, pulse_b1_depth), 0.25f}, {pulse_smooth, 0.25f}}, {{band(2, pulse_b1_phase), 0.3f}, {echo_level, 0.3f}}, {{void_decay, 0.25f}, {evolve_tone, 0.3f}}, {{band(2, pulse_b1_depth), 0.3f}, {void_mod, 0.3f}}}}});
  p.push_back({"RHYTHMIC", "Sidechain Tide",
               {{mix, 65}, {anchor_freq, 100}, {drift_depth, 15}, {drift_mix, 30}, {echo_on, 0}, {void_size, 85}, {void_decay, 10}, {void_damp, 40}, {void_lowcut, 130}, {void_level, 100}, {pulse_on, 1}, {pulse_position, 2}, {pulse_bands, 1}, {pulse_smooth, 20}, {band(1, pulse_b1_div), d4}, {band(1, pulse_b1_depth), 85}, {band(1, pulse_b1_shape), 1}, {halo_climb, 20}, {halo_interval, 0}, {mut_on, 1}, {evolve_amount, 50}, {evolve_cycle, 3}, {evolve_formant, 45}, {xy_current, 3}, {xy_radius, 45}, {xy_rate, bar4}},
               {{band(1, pulse_b1_depth), 100}, {void_decay, 16}, {halo_climb, 35}},
               {{{{band(1, pulse_b1_depth), 0.15f}, {void_decay, 0.25f}}, {{halo_climb, 0.3f}, {void_mod, 0.3f}}, {{pulse_smooth, 0.4f}, {evolve_tone, 0.3f}}, {{evolve_formant, 0.35f}, {void_damp, 0.3f}}}}});
  p.push_back({"RHYTHMIC", "Gated Cathedral",
               {{mix, 65}, {anchor_freq, 100}, {drift_depth, 10}, {drift_mix, 25}, {echo_on, 0}, {void_size, 95}, {void_decay, 15}, {void_damp, 35}, {void_lowcut, 120}, {void_level, 100}, {pulse_on, 1}, {pulse_position, 2}, {pulse_bands, 3}, {pulse_smooth, 10}, {band(2, pulse_b1_div), d16}, {band(2, pulse_b1_depth), 90}, {band(2, pulse_b1_source), 1}, {band(3, pulse_b1_div), d16}, {band(3, pulse_b1_depth), 90}, {band(3, pulse_b1_source), 1}, {mut_on, 1}, {evolve_amount, 45}, {evolve_cycle, 4}, {evolve_formant, 40}, {xy_current, 1}, {xy_radius, 40}, {xy_rate, bar8}},
               {{void_decay, 25}, {band(2, pulse_b1_depth), 100}, {band(3, pulse_b1_depth), 100}},
               {{{{pulse_smooth, 0.3f}, {void_decay, 0.25f}}, {{halo_climb, 0.35f}, {void_mod, 0.3f}}, {{band(2, pulse_b1_depth), -0.35f}, {evolve_tone, 0.3f}}, {{evolve_formant, 0.35f}, {void_damp, 0.3f}}}},
               {65535, 0xeeee, 0xeeee, 65535, 65535, 65535}});
  p.push_back({"RHYTHMIC", "Step Ghosts",
               {{mix, 55}, {anchor_freq, 110}, {drift_depth, 15}, {drift_mix, 30}, {echo_mode, 1}, {echo_div, d8d}, {echo_feedback, 40}, {echo_tone, 4000}, {echo_lowcut, 250}, {echo_level, 35}, {void_size, 75}, {void_decay, 8}, {void_damp, 45}, {void_lowcut, 140}, {void_level, 90}, {pulse_on, 1}, {pulse_bands, 3}, {pulse_smooth, 25}, {band(1, pulse_b1_div), d16}, {band(1, pulse_b1_depth), 50}, {band(1, pulse_b1_source), 1}, {band(2, pulse_b1_div), d16}, {band(2, pulse_b1_depth), 60}, {band(2, pulse_b1_source), 1}, {band(3, pulse_b1_div), d16}, {band(3, pulse_b1_depth), 55}, {band(3, pulse_b1_source), 1}, {halo_climb, 20}, {halo_interval, 1}, {mut_on, 1}, {evolve_amount, 55}, {evolve_cycle, 3}, {evolve_formant, 55}, {xy_current, 3}, {xy_radius, 55}, {xy_rate, bar4}},
               {{echo_feedback, 55}, {band(2, pulse_b1_depth), 80}, {halo_climb, 35}},
               {{{{band(1, pulse_b1_depth), 0.3f}, {echo_feedback, 0.25f}}, {{band(2, pulse_b1_depth), 0.3f}, {halo_climb, 0.3f}}, {{band(3, pulse_b1_depth), 0.35f}, {evolve_tone, 0.3f}}, {{echo_age, 0.4f}, {void_decay, 0.25f}}}},
               {0x8888, 0x2424, 0x5151, 65535, 65535, 65535}});
  p.push_back({"RHYTHMIC", "Dotted Machine",
               {{mix, 55}, {anchor_freq, 110}, {duck_amount, 15}, {drift_depth, 10}, {drift_mix, 25}, {echo_mode, 1}, {echo_div, d8d}, {echo_feedback, 55}, {echo_tone, 5000}, {echo_lowcut, 300}, {echo_drive, 25}, {echo_level, 45}, {void_size, 65}, {void_decay, 6}, {void_damp, 45}, {void_lowcut, 160}, {void_level, 85}, {pulse_on, 1}, {pulse_bands, 3}, {band(2, pulse_b1_div), d8d}, {band(2, pulse_b1_depth), 50}, {band(2, pulse_b1_shape), 3}, {mut_on, 1}, {evolve_amount, 45}, {evolve_cycle, 3}, {evolve_tone, 60}, {xy_current, 1}, {xy_radius, 50}, {xy_rate, bar4}},
               {{echo_feedback, 70}, {band(2, pulse_b1_depth), 70}, {echo_drive, 45}},
               {{{{echo_feedback, 0.25f}, {echo_drive, 0.3f}}, {{band(2, pulse_b1_depth), 0.35f}, {echo_level, 0.25f}}, {{echo_age, 0.4f}, {evolve_tone, 0.3f}}, {{void_decay, 0.3f}, {duck_amount, 0.3f}}}}});
  p.push_back({"RHYTHMIC", "Triplet Rain",
               {{mix, 55}, {anchor_freq, 110}, {drift_depth, 15}, {drift_mix, 30}, {echo_mode, 1}, {echo_div, d8t}, {echo_feedback, 45}, {echo_tone, 6000}, {echo_lowcut, 300}, {echo_level, 40}, {void_size, 75}, {void_decay, 9}, {void_damp, 35}, {void_lowcut, 150}, {void_level, 90}, {pulse_on, 1}, {pulse_bands, 3}, {band(3, pulse_b1_div), d8t}, {band(3, pulse_b1_depth), 55}, {band(3, pulse_b1_shape), 2}, {harm_octave, 15}, {mut_on, 1}, {evolve_amount, 50}, {evolve_cycle, 3}, {evolve_formant, 50}, {xy_current, 3}, {xy_radius, 50}, {xy_rate, bar4}},
               {{echo_feedback, 60}, {band(3, pulse_b1_depth), 75}, {harm_octave, 30}},
               {{{{band(3, pulse_b1_depth), 0.3f}, {echo_feedback, 0.25f}}, {{harm_octave, 0.35f}, {halo_climb, 0.3f}}, {{echo_tone, -0.3f}, {evolve_tone, 0.3f}}, {{void_decay, 0.3f}, {echo_level, 0.25f}}}}});
  p.push_back({"RHYTHMIC", "Morse Bloom",
               {{mix, 60}, {anchor_freq, 100}, {drift_depth, 15}, {drift_mix, 30}, {echo_on, 0}, {void_size, 85}, {void_decay, 12}, {void_damp, 40}, {void_lowcut, 130}, {void_gravity, -50}, {void_level, 95}, {pulse_on, 1}, {pulse_bands, 3}, {pulse_smooth, 20}, {band(2, pulse_b1_div), d16}, {band(2, pulse_b1_depth), 70}, {band(2, pulse_b1_source), 1}, {halo_climb, 25}, {halo_interval, 0}, {mut_on, 1}, {evolve_amount, 60}, {evolve_cycle, 3}, {evolve_formant, 60}, {xy_current, 2}, {xy_radius, 55}, {xy_rate, bar4}},
               {{band(2, pulse_b1_depth), 90}, {void_gravity, -80}, {halo_climb, 40}},
               {{{{band(2, pulse_b1_depth), 0.25f}, {void_gravity, -0.3f}}, {{halo_climb, 0.3f}, {void_mod, 0.3f}}, {{evolve_formant, 0.35f}, {pulse_smooth, 0.3f}}, {{void_decay, 0.3f}, {evolve_tone, 0.3f}}}},
               {65535, 0xb5a9, 65535, 65535, 65535, 65535}});
  p.push_back({"ECHOES", "Tape Memory",
               {{mix, 55}, {anchor_freq, 110}, {drift_depth, 20}, {drift_wow, 30}, {drift_mix, 35}, {echo_div, d4}, {echo_feedback, 60}, {echo_tone, 3000}, {echo_lowcut, 200}, {echo_drive, 30}, {echo_age, 70}, {echo_level, 50}, {void_size, 65}, {void_decay, 6}, {void_damp, 50}, {void_lowcut, 150}, {void_level, 80}, {mut_on, 1}, {evolve_amount, 50}, {evolve_cycle, 3}, {evolve_tone, 60}, {evolve_formant, 40}, {xy_current, 2}, {xy_radius, 50}, {xy_rate, bar4}},
               {{echo_feedback, 72}, {echo_age, 90}, {echo_drive, 45}},
               {{{{echo_age, 0.25f}, {drift_wow, 0.35f}}, {{echo_feedback, 0.2f}, {echo_tone, -0.3f}}, {{echo_drive, 0.35f}, {evolve_tone, 0.3f}}, {{void_decay, 0.3f}, {echo_level, 0.2f}}}}});
  p.push_back({"ECHOES", "Ping Field",
               {{mix, 55}, {anchor_freq, 110}, {drift_depth, 15}, {drift_mix, 30}, {echo_mode, 1}, {echo_div, d8d}, {echo_feedback, 55}, {echo_tone, 6000}, {echo_lowcut, 300}, {echo_level, 50}, {void_size, 70}, {void_decay, 8}, {void_damp, 40}, {void_lowcut, 150}, {void_level, 85}, {halo_climb, 20}, {halo_interval, 1}, {mut_on, 1}, {evolve_amount, 50}, {evolve_cycle, 3}, {evolve_formant, 45}, {xy_current, 3}, {xy_radius, 60}, {xy_rate, bar4}},
               {{echo_feedback, 70}, {halo_climb, 40}, {void_decay, 14}},
               {{{{echo_feedback, 0.2f}, {echo_age, 0.3f}}, {{halo_climb, 0.35f}, {echo_tone, -0.25f}}, {{void_decay, 0.3f}, {evolve_tone, 0.3f}}, {{echo_drive, 0.35f}, {drift_depth, 0.3f}}}}});
  p.push_back({"ECHOES", "Backwards Rain",
               {{mix, 60}, {anchor_freq, 110}, {drift_depth, 15}, {drift_mix, 30}, {echo_mode, 2}, {echo_div, d2}, {echo_feedback, 50}, {echo_tone, 4500}, {echo_lowcut, 250}, {echo_level, 50}, {void_size, 80}, {void_decay, 10}, {void_damp, 40}, {void_lowcut, 140}, {void_level, 90}, {halo_climb, 25}, {halo_interval, 0}, {mut_on, 1}, {evolve_amount, 60}, {evolve_cycle, 3}, {evolve_formant, 50}, {xy_current, 3}, {xy_radius, 55}, {xy_rate, bar4}},
               {{echo_feedback, 65}, {void_decay, 18}, {halo_climb, 40}},
               {{{{echo_feedback, 0.2f}, {halo_climb, 0.3f}}, {{echo_age, 0.4f}, {void_mod, 0.3f}}, {{evolve_tone, 0.35f}, {echo_tone, -0.3f}}, {{void_gravity, -0.4f}, {echo_level, 0.2f}}}}});
  p.push_back({"ECHOES", "Dub Siren",
               {{mix, 55}, {anchor_freq, 120}, {duck_amount, 20}, {drift_depth, 15}, {drift_mix, 25}, {echo_div, d4d}, {echo_feedback, 70}, {echo_tone, 2500}, {echo_lowcut, 300}, {echo_drive, 50}, {echo_age, 40}, {echo_level, 50}, {void_size, 60}, {void_decay, 5}, {void_damp, 55}, {void_lowcut, 180}, {void_level, 75}, {pulse_on, 1}, {pulse_bands, 3}, {band(3, pulse_b1_div), d8}, {band(3, pulse_b1_depth), 30}, {mut_on, 1}, {evolve_amount, 40}, {evolve_cycle, 3}, {evolve_tone, 70}, {xy_current, 1}, {xy_radius, 45}, {xy_rate, bar4}, {out_gain, -1.5f}},
               {{echo_feedback, 82}, {echo_drive, 65}, {echo_age, 60}},
               {{{{echo_feedback, 0.15f}, {echo_tone, 0.3f}}, {{echo_drive, 0.3f}, {band(3, pulse_b1_depth), 0.3f}}, {{echo_age, 0.35f}, {evolve_tone, 0.2f}}, {{echo_lowcut, 0.3f}, {void_decay, 0.3f}}}}});
  p.push_back({"ECHOES", "Worn Cassette",
               {{mix, 55}, {anchor_freq, 110}, {drift_depth, 30}, {drift_wow, 50}, {drift_flutter, 40}, {drift_mix, 45}, {echo_div, d8}, {echo_feedback, 45}, {echo_tone, 2200}, {echo_lowcut, 250}, {echo_drive, 40}, {echo_age, 90}, {echo_level, 45}, {void_size, 65}, {void_decay, 6}, {void_damp, 60}, {void_lowcut, 150}, {void_level, 80}, {mut_on, 1}, {evolve_amount, 50}, {evolve_cycle, 3}, {evolve_tone, 70}, {xy_current, 3}, {xy_radius, 55}, {xy_rate, bar4}},
               {{drift_wow, 75}, {echo_age, 100}, {echo_feedback, 60}},
               {{{{drift_wow, 0.25f}, {drift_flutter, 0.3f}}, {{echo_feedback, 0.25f}, {echo_drive, 0.3f}}, {{echo_tone, -0.3f}, {evolve_tone, 0.3f}}, {{void_decay, 0.3f}, {drift_depth, 0.3f}}}}});
  p.push_back({"ECHOES", "Long Return",
               {{mix, 60}, {anchor_freq, 100}, {drift_depth, 20}, {drift_mix, 35}, {echo_div, bar1}, {echo_feedback, 65}, {echo_tone, 3500}, {echo_lowcut, 200}, {echo_age, 50}, {echo_level, 45}, {void_size, 85}, {void_decay, 14}, {void_damp, 45}, {void_lowcut, 120}, {void_level, 90}, {halo_climb, 20}, {halo_interval, 2}, {mut_on, 1}, {evolve_amount, 70}, {evolve_cycle, 4}, {evolve_formant, 50}, {xy_current, 1}, {xy_radius, 60}, {xy_rate, bar8}},
               {{echo_feedback, 78}, {void_decay, 24}, {halo_climb, 35}},
               {{{{echo_feedback, 0.15f}, {echo_age, 0.3f}}, {{halo_climb, 0.35f}, {void_mod, 0.3f}}, {{evolve_formant, 0.35f}, {echo_tone, -0.3f}}, {{void_gravity, -0.4f}, {evolve_tone, 0.3f}}}}});
  p.push_back({"TEXTURE", "Rust Bloom",
               {{mix, 60}, {anchor_freq, 100}, {drift_depth, 20}, {drift_mix, 35}, {echo_div, d4d}, {echo_feedback, 40}, {echo_tone, 3000}, {echo_lowcut, 250}, {echo_drive, 50}, {echo_age, 50}, {echo_level, 35}, {void_size, 75}, {void_decay, 10}, {void_damp, 50}, {void_lowcut, 130}, {void_level, 90}, {harm_odd, 45}, {mut_on, 1}, {evolve_amount, 80}, {evolve_cycle, 3}, {evolve_formant, 60}, {evolve_tone, 60}, {xy_current, 3}, {xy_radius, 70}, {xy_rate, bar4}, {out_gain, -1}},
               {{harm_odd, 65}, {echo_drive, 70}, {void_decay, 18}},
               {{{{harm_odd, 0.25f}, {echo_drive, 0.3f}}, {{echo_age, 0.35f}, {evolve_tone, 0.3f}}, {{halo_climb, 0.4f}, {void_mod, 0.3f}}, {{evolve_formant, 0.3f}, {void_gravity, -0.35f}}}}});
  p.push_back({"TEXTURE", "Static Garden",
               {{mix, 55}, {anchor_freq, 110}, {drift_depth, 20}, {drift_flutter, 50}, {drift_mix, 40}, {echo_on, 0}, {void_size, 75}, {void_decay, 9}, {void_damp, 35}, {void_lowcut, 150}, {void_level, 90}, {pulse_on, 1}, {pulse_bands, 3}, {band(3, pulse_b1_div), d16}, {band(3, pulse_b1_depth), 40}, {band(3, pulse_b1_source), 1}, {harm_odd, 30}, {harm_octave, 20}, {mut_on, 1}, {evolve_amount, 75}, {evolve_cycle, 3}, {evolve_formant, 60}, {xy_current, 3}, {xy_radius, 65}, {xy_rate, bar4}},
               {{harm_odd, 50}, {band(3, pulse_b1_depth), 65}, {drift_flutter, 70}},
               {{{{drift_flutter, 0.3f}, {band(3, pulse_b1_depth), 0.3f}}, {{harm_odd, 0.3f}, {harm_octave, 0.3f}}, {{evolve_tone, 0.35f}, {void_mod, 0.35f}}, {{halo_climb, 0.4f}, {void_decay, 0.25f}}}},
               {65535, 65535, 0x5a5a, 65535, 65535, 65535}});
  p.push_back({"TEXTURE", "Folded Paper",
               {{mix, 55}, {anchor_freq, 110}, {drift_depth, 15}, {drift_mix, 30}, {echo_mode, 1}, {echo_div, d16}, {echo_feedback, 30}, {echo_tone, 7000}, {echo_lowcut, 400}, {echo_level, 25}, {void_size, 70}, {void_decay, 8}, {void_damp, 20}, {void_lowcut, 160}, {void_level, 85}, {harm_octave, 35}, {harm_odd, 25}, {halo_climb, 30}, {halo_interval, 1}, {mut_on, 1}, {evolve_amount, 70}, {evolve_cycle, 2}, {evolve_formant, 55}, {xy_current, 3}, {xy_radius, 60}, {xy_rate, bar4}, {out_gain, -1}},
               {{harm_octave, 50}, {halo_climb, 50}, {echo_feedback, 45}},
               {{{{harm_odd, 0.3f}, {echo_feedback, 0.25f}}, {{halo_climb, 0.3f}, {halo_spread, 0.35f}}, {{harm_octave, 0.3f}, {evolve_tone, 0.3f}}, {{void_decay, 0.3f}, {void_damp, 0.35f}}}}});
  p.push_back({"TEXTURE", "Dead Air",
               {{mix, 60}, {anchor_freq, 90}, {drift_depth, 25}, {drift_wow, 40}, {drift_mix, 40}, {echo_on, 0}, {void_size, 80}, {void_decay, 12}, {void_damp, 60}, {void_lowcut, 200}, {void_level, 95}, {harm_sub, 25}, {harm_odd, 30}, {mut_on, 1}, {evolve_amount, 95}, {evolve_cycle, 3}, {evolve_tone, 85}, {evolve_formant, 85}, {evolve_size, 70}, {xy_current, 2}, {xy_radius, 65}, {xy_rate, bar8}},
               {{harm_odd, 50}, {void_decay, 22}, {harm_sub, 40}},
               {{{{harm_sub, 0.3f}, {void_damp, -0.3f}}, {{harm_odd, 0.3f}, {drift_wow, 0.3f}}, {{evolve_formant, 0.35f}, {void_mod, 0.3f}}, {{halo_climb, 0.35f}, {evolve_size, 0.3f}}}}});
  p.push_back({"TEXTURE", "Copper Air",
               {{mix, 60}, {anchor_freq, 100}, {drift_depth, 30}, {drift_mix, 45}, {echo_on, 0}, {void_size, 80}, {void_decay, 12}, {void_damp, 40}, {void_lowcut, 130}, {void_level, 90}, {harm_fifth, 35}, {halo_climb, 40}, {halo_interval, 1}, {mut_on, 1}, {evolve_amount, 80}, {evolve_cycle, 3}, {evolve_formant, 80}, {xy_current, 3}, {xy_radius, 65}, {xy_rate, bar4}},
               {{harm_fifth, 50}, {halo_climb, 60}, {void_decay, 20}},
               {{{{harm_fifth, 0.3f}, {drift_depth, 0.3f}}, {{halo_climb, 0.3f}, {halo_spread, 0.4f}}, {{evolve_tone, 0.35f}, {void_damp, 0.3f}}, {{harm_odd, 0.35f}, {void_gravity, -0.35f}}}}});
  p.push_back({"TEXTURE", "Dust Halo",
               {{mix, 60}, {anchor_freq, 100}, {drift_depth, 20}, {drift_flutter, 35}, {drift_mix, 40}, {echo_on, 0}, {void_size, 85}, {void_decay, 16}, {void_damp, 30}, {void_lowcut, 140}, {void_level, 95}, {halo_climb, 45}, {halo_interval, 0}, {halo_spread, 80}, {mut_on, 1}, {evolve_amount, 85}, {evolve_cycle, 4}, {evolve_formant, 70}, {xy_current, 3}, {xy_radius, 80}, {xy_rate, bar8}},
               {{halo_climb, 65}, {halo_spread, 100}, {void_decay, 26}},
               {{{{halo_spread, 0.2f}, {drift_flutter, 0.3f}}, {{halo_climb, 0.3f}, {harm_octave, 0.3f}}, {{evolve_tone, 0.35f}, {void_mod, 0.35f}}, {{void_gravity, -0.4f}, {evolve_formant, 0.25f}}}}});
  p.push_back({"BASS SPACE", "Concrete Floor",
               {{mix, 45}, {anchor_freq, 120}, {duck_amount, 25}, {duck_release, 300}, {drift_depth, 10}, {drift_mix, 25}, {echo_on, 0}, {void_size, 50}, {void_decay, 2.5f}, {void_predelay, 15}, {void_damp, 55}, {void_lowcut, 180}, {void_level, 90}, {mut_on, 1}, {evolve_amount, 40}, {evolve_cycle, 3}, {evolve_tone, 50}, {evolve_formant, 30}, {xy_current, 1}, {xy_radius, 40}, {xy_rate, bar4}},
               {{void_decay, 4.5f}, {void_size, 70}, {duck_amount, 15}},
               {{{{void_size, 0.25f}, {void_decay, 0.2f}}, {{duck_amount, -0.3f}, {void_damp, -0.3f}}, {{evolve_tone, 0.3f}, {void_predelay, 0.3f}}, {{harm_odd, 0.3f}, {void_mod, 0.3f}}}}});
  p.push_back({"BASS SPACE", "Sub Cavern",
               {{mix, 50}, {anchor_freq, 90}, {duck_amount, 20}, {duck_release, 400}, {drift_depth, 15}, {drift_mix, 30}, {echo_div, d4}, {echo_feedback, 30}, {echo_tone, 2500}, {echo_lowcut, 150}, {echo_level, 30}, {void_size, 80}, {void_decay, 6}, {void_damp, 55}, {void_lowcut, 100}, {void_level, 90}, {harm_sub, 30}, {halo_climb, 20}, {halo_interval, 2}, {mut_on, 1}, {evolve_amount, 50}, {evolve_cycle, 3}, {evolve_size, 60}, {xy_current, 2}, {xy_radius, 45}, {xy_rate, bar4}},
               {{harm_sub, 45}, {void_decay, 10}, {halo_climb, 35}},
               {{{{harm_sub, 0.25f}, {void_decay, 0.2f}}, {{halo_climb, 0.3f}, {echo_feedback, 0.25f}}, {{void_damp, 0.3f}, {evolve_tone, 0.3f}}, {{duck_amount, 0.3f}, {void_gravity, -0.35f}}}}});
  p.push_back({"BASS SPACE", "Tight Room",
               {{mix, 40}, {anchor_freq, 130}, {duck_amount, 30}, {duck_release, 250}, {drift_on, 0}, {echo_div, d16}, {echo_feedback, 20}, {echo_tone, 4000}, {echo_lowcut, 250}, {echo_level, 20}, {void_size, 35}, {void_decay, 1.5f}, {void_predelay, 10}, {void_damp, 50}, {void_lowcut, 200}, {void_level, 90}, {mut_on, 1}, {evolve_amount, 30}, {evolve_cycle, 2}, {evolve_tone, 40}, {xy_current, 1}, {xy_radius, 35}, {xy_rate, bar4}},
               {{void_decay, 3}, {void_size, 55}, {echo_level, 35}},
               {{{{void_size, 0.2f}, {echo_level, 0.25f}}, {{void_decay, 0.25f}, {void_damp, -0.3f}}, {{echo_feedback, 0.3f}, {duck_amount, -0.25f}}, {{harm_odd, 0.25f}, {evolve_tone, 0.3f}}}}});
  p.push_back({"BASS SPACE", "Riff Halo",
               {{mix, 50}, {anchor_freq, 110}, {duck_amount, 20}, {duck_release, 400}, {drift_depth, 15}, {drift_mix, 30}, {echo_mode, 1}, {echo_div, d8}, {echo_feedback, 35}, {echo_tone, 4000}, {echo_lowcut, 250}, {echo_level, 35}, {void_size, 65}, {void_decay, 5}, {void_damp, 45}, {void_lowcut, 140}, {void_level, 90}, {halo_climb, 30}, {halo_interval, 1}, {mut_on, 1}, {evolve_amount, 60}, {evolve_cycle, 3}, {evolve_formant, 50}, {xy_current, 3}, {xy_radius, 55}, {xy_rate, bar4}},
               {{halo_climb, 50}, {echo_feedback, 50}, {void_decay, 9}},
               {{{{halo_climb, 0.3f}, {echo_level, 0.25f}}, {{echo_feedback, 0.25f}, {echo_age, 0.3f}}, {{evolve_formant, 0.35f}, {void_mod, 0.3f}}, {{duck_amount, 0.3f}, {void_gravity, -0.35f}}}}});
  p.push_back({"DRONE", "Engine Hum",
               {{mix, 65}, {anchor_freq, 80}, {drift_depth, 25}, {drift_mix, 40}, {echo_on, 0}, {void_size, 85}, {void_decay, 25}, {void_damp, 55}, {void_lowcut, 90}, {void_level, 95}, {pulse_on, 1}, {pulse_bands, 3}, {band(2, pulse_b1_div), bar2}, {band(2, pulse_b1_depth), 40}, {band(2, pulse_b1_shape), 4}, {harm_odd, 25}, {harm_sub, 20}, {mut_on, 1}, {evolve_amount, 80}, {evolve_cycle, 4}, {evolve_formant, 55}, {evolve_tone, 70}, {xy_current, 3}, {xy_radius, 60}, {xy_rate, bar8}, {out_gain, -1}},
               {{harm_odd, 40}, {band(2, pulse_b1_depth), 60}, {void_decay, 40}},
               {{{{harm_odd, 0.3f}, {band(2, pulse_b1_depth), 0.3f}}, {{harm_sub, 0.3f}, {void_damp, 0.3f}}, {{evolve_tone, 0.25f}, {void_mod, 0.35f}}, {{halo_climb, 0.4f}, {void_gravity, -0.35f}}}}});
  p.push_back({"DRONE", "Distant Choir",
               {{mix, 70}, {anchor_freq, 80}, {drift_depth, 20}, {drift_mix, 35}, {echo_on, 0}, {void_size, 90}, {void_decay, 30}, {void_predelay, 150}, {void_damp, 40}, {void_lowcut, 100}, {void_level, 100}, {halo_climb, 30}, {halo_interval, 3}, {mut_on, 1}, {evolve_amount, 90}, {evolve_cycle, 4}, {evolve_formant, 95}, {xy_current, 2}, {xy_radius, 70}, {xy_rate, bar8}},
               {{halo_climb, 50}, {void_decay, 50}, {evolve_amount, 100}},
               {{{{halo_climb, 0.3f}, {void_predelay, 0.3f}}, {{evolve_tone, 0.35f}, {void_mod, 0.3f}}, {{harm_fifth, 0.35f}, {void_damp, 0.3f}}, {{void_gravity, -0.4f}, {evolve_size, 0.3f}}}}});
  p.push_back({"DRONE", "Tidal Hum",
               {{mix, 70}, {anchor_freq, 80}, {drift_depth, 25}, {drift_wow, 30}, {drift_mix, 40}, {echo_on, 0}, {void_size, 90}, {void_decay, 30}, {void_damp, 45}, {void_lowcut, 90}, {void_gravity, -60}, {void_level, 100}, {halo_climb, 25}, {halo_interval, 2}, {mut_on, 1}, {evolve_amount, 80}, {evolve_cycle, 4}, {evolve_size, 80}, {evolve_formant, 60}, {xy_current, 2}, {xy_radius, 85}, {xy_rate, bar8}},
               {{void_gravity, -85}, {halo_climb, 45}, {void_decay, 50}},
               {{{{void_gravity, -0.3f}, {drift_wow, 0.3f}}, {{halo_climb, 0.35f}, {evolve_tone, 0.3f}}, {{harm_sub, 0.3f}, {void_damp, 0.3f}}, {{evolve_formant, 0.35f}, {void_mod, 0.35f}}}}});
  p.push_back({"DRONE", "Mountain",
               {{mix, 70}, {anchor_freq, 80}, {drift_depth, 20}, {drift_mix, 35}, {echo_on, 0}, {void_size, 100}, {void_decay, 60}, {void_damp, 50}, {void_lowcut, 80}, {void_level, 95}, {harm_sub, 15}, {halo_climb, 35}, {halo_interval, 2}, {mut_on, 1}, {evolve_amount, 85}, {evolve_cycle, 4}, {evolve_formant, 60}, {evolve_size, 60}, {xy_current, 3}, {xy_radius, 60}, {xy_rate, bar8}, {out_gain, -1}},
               {{void_decay, 90}, {halo_climb, 55}, {harm_sub, 30}},
               {{{{halo_climb, 0.3f}, {harm_sub, 0.25f}}, {{void_mod, 0.35f}, {evolve_tone, 0.3f}}, {{void_damp, -0.3f}, {evolve_formant, 0.3f}}, {{halo_spread, 0.4f}, {void_gravity, -0.35f}}}}});
  return p;
}
} // namespace
const std::vector<PresetDef> &factoryPresets() {
  static const auto bank = build();
  return bank;
}
} // namespace ut
