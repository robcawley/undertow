#include "Controls.h"
#include <map>

namespace ui
{
juce::String describe (const juce::String& id)
{
    static const std::map<juce::String, juce::String> text {
        { "xy_return",      "After you let go, the puck glides back to the centre over one bar. Automatable." },
        { "mut_on",         "MUTATE: harmonics before the space, pitch-climbing HALO inside it, and a slow EVOLVE that keeps the space moving." },
        { "harm_octave",    "Adds an octave above, from a full-wave fold of the signal. Bright and glassy." },
        { "harm_fifth",     "Adds a pitch-shifted fifth above. Organ-like when mixed with OCTAVE." },
        { "harm_odd",       "Adds odd harmonics from a soft wavefolder. Hollow, reedy, a little rusty." },
        { "harm_sub",       "Adds a square sub an octave below. Strongest on simple, steady parts." },
        { "halo_climb",     "Feeds the reverb back into itself pitch-shifted, so every pass climbs (or sinks) another interval. Long DECAY makes it shimmer for ever." },
        { "halo_interval",  "The interval HALO adds on every pass through the reverb." },
        { "halo_spread",    "Detunes HALO left against right, so the climbing voices beat and widen." },
        { "evolve_amount",  "How far EVOLVE moves the space. It wanders in time with the song, and repeats exactly every time you play the same bar." },
        { "evolve_cycle",   "How long one EVOLVE wander takes, in bars." },
        { "evolve_size",    "How much EVOLVE breathes the reverb size in and out." },
        { "evolve_tone",    "How much EVOLVE darkens and brightens the tail." },
        { "evolve_formant", "Vowel-like resonances that slide through the tail as it evolves." },
        { "evolve_follow",  "EVOLVE follows the XY pad instead of the song: X is size, Y is tone, the angle picks the vowel." },
        { "macro_mutate",   "One knob for MUTATE: adds octave, odd harmonics, HALO and EVOLVE together. Turning it up switches MUTATE on even if its switch is off." },
        { "mix",            "Dry against space. Both stay full up to 50 %; at 100 % you hear only the space. With ANCHOR on the lows stay dry whatever this is set to." },
        { "in_gain",        "Level into UNDERTOW." },
        { "out_gain",       "Level out of UNDERTOW." },
        { "anchor_on",      "Keeps everything below the anchor frequency dry and out of the effects, so a bass stays solid." },
        { "anchor_freq",    "Where the dry lows end and the space begins." },
        { "anchor_mono",    "Sums the anchored lows to mono." },
        { "duck_amount",    "Pushes the space down while you play, and lets it rise in the gaps." },
        { "duck_release",   "How quickly the space comes back after you stop playing." },
        { "bloom",          "Morphs every knob from Scene A to Scene B. Automate it for a whole arc in one lane." },
        { "drift_on",       "Slow chorus, vibrato and tape wobble before the space." },
        { "drift_sync",     "Locks the drift rate to the song tempo." },
        { "drift_rate",     "Speed of the drift." },
        { "drift_div",      "Drift cycle length in note values." },
        { "drift_depth",    "How far the pitch and stereo image wander." },
        { "drift_wow",      "Slow, uneven tape wow." },
        { "drift_flutter",  "Faster, smaller tape flutter." },
        { "drift_width",    "Stereo spread of the movement." },
        { "drift_mix",      "How much of the drifting signal goes on into the space." },
        { "echo_on",        "Tape-style delay." },
        { "echo_mode",      "Tape repeats in the middle; ping-pong bounces left and right; reverse plays each repeat backwards." },
        { "echo_sync",      "Locks the delay time to the song tempo." },
        { "echo_div",       "Delay time in note values." },
        { "echo_time",      "Delay time. Changes glide like tape rather than clicking." },
        { "echo_feedback",  "How many repeats. Near the top the echoes hang for a long time." },
        { "echo_tone",      "Darkens each repeat." },
        { "echo_lowcut",    "Thins the low end out of the repeats." },
        { "echo_drive",     "Tape saturation inside the feedback." },
        { "echo_age",       "Each repeat wobbles and darkens a little more." },
        { "echo_level",     "How loud the echoes are, and how much of them feeds the space." },
        { "void_on",        "The big reverb." },
        { "void_size",      "From a room to a cathedral." },
        { "void_decay",     "How long the tail lasts, up to a near-endless wash." },
        { "void_predelay",  "A gap before the space answers." },
        { "void_damp",      "Darkens the tail as it decays." },
        { "void_lowcut",    "Keeps the low end out of the reverb." },
        { "void_diffusion", "From distinct reflections to a smooth wash." },
        { "void_mod",       "Slow movement inside the tail, so long decays don't ring." },
        { "void_width",     "Stereo width of the tail." },
        { "void_gravity",   "Positive: a heavy early drop. Negative: the tail swells before it fades." },
        { "void_freeze",    "Holds the current tail as an endless drone. Right-click to choose click or hold." },
        { "void_level",     "How loud the space is." },
        { "pulse_on",       "Makes chosen frequency bands throb in time." },
        { "pulse_position", "Before: only what feeds the reverb pulses, so it smears into the tail. After: everything above the anchor pulses, dry and space. Wet only: the space pulses, the dry never does." },
        { "pulse_bands",    "How many frequency bands to split into." },
        { "pulse_smooth",   "Softens the edges of each pulse and step." },
        { "pulse_sub",      "Lets band 1's pulse reach the anchored lows too." },
        { "pulse_focus_on", "Narrow resonant peaks on one note and its harmonics, pulsing in time." },
        { "pulse_focus_note","The note the peaks lock to." },
        { "pulse_focus_harm","How many harmonics of that note ring." },
        { "pulse_focus_q",  "How narrow the peaks are." },
        { "pulse_focus_depth","How strongly the peaks pulse." },
        { "pulse_focus_div","Pulse rate of the peaks in note values." },
        { "xy_x",           "Puck position, left to right." },
        { "xy_y",           "Puck position, bottom to top." },
        { "xy_amount",      "How much the corner variations apply." },
        { "xy_flip_a",      "Corner A subtracts its variation instead of adding it." },
        { "xy_flip_b",      "Corner B subtracts its variation instead of adding it." },
        { "xy_flip_c",      "Corner C subtracts its variation instead of adding it." },
        { "xy_flip_d",      "Corner D subtracts its variation instead of adding it." },
        { "xy_current",     "Moves the puck by itself: orbit around it, rock back and forth, or wander." },
        { "xy_radius",      "How far the current moves the puck." },
        { "xy_rate",        "How long one cycle of the current takes." },
        { "xy_angle",       "Direction of the TIDE movement." },
        { "xy_motion_bars", "Length of the recorded puck path." },
        { "xy_motion_play", "Plays the recorded puck path in time with the song." },
        { "macro_depth",    "Bigger, longer space one way; smaller and drier the other." },
        { "macro_motion",   "More drift, wobble and pulse one way; stiller the other." },
        { "macro_glow",     "Brighter one way; darker the other." },
        { "macro_chaos",    "Adds wander to the puck, tape age and scattered pulse phases." },
    };
    if (const auto it = text.find (id); it != text.end()) return it->second;
    if (id.startsWith ("pulse_b"))
    {
        const auto band = id.substring (7, 8);
        if (id.endsWith ("_depth"))  return "How strongly band " + band + " throbs.";
        if (id.endsWith ("_div"))    return "Band " + band + " pulse rate in note values.";
        if (id.endsWith ("_rate"))   return "Band " + band + " pulse rate.";
        if (id.endsWith ("_sync"))   return "Locks band " + band + " to the song tempo.";
        if (id.endsWith ("_shape"))  return "Shape of band " + band + "'s pulse.";
        if (id.endsWith ("_phase"))  return "Offsets band " + band + "'s pulse against the others.";
        if (id.endsWith ("_source")) return "Band " + band + " follows its LFO or its 16-step pattern.";
    }
    if (id.startsWith ("pulse_xover")) return "Crossover between two pulse bands. Drag the line on the spectrum.";
    return {};
}
} // namespace ui
