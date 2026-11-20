# UNDERTOW for Godot

An ambient space effect for Godot 4.5 and later: drift, tape echo, a big
reverb with FREEZE, shimmer and slow evolving movement. Put it on an audio bus
and the space can follow the player: a cave, underwater, a cathedral, a pause
that freezes what was playing.

## Install

Copy this `addons/undertow` folder into your project's `addons/` folder and
restart the editor. There's nothing to enable: the effect appears in the
Audio panel's **Add Effect** menu as **AudioEffectUndertow**.

Included builds: macOS (Apple silicon and Intel), Windows x86_64, Linux
x86_64 and arm64.

## Use it

In the editor: add **AudioEffectUndertow** to a bus, pick a **Preset**, and
adjust **Mix**, **Freeze** and the five macros (Depth, Motion, Glow, Chaos,
Mutate).

From a script:

```gdscript
var space := AudioServer.get_bus_effect(AudioServer.get_bus_index("Music"), 0) as AudioEffectUndertow

space.morph_to("Cathedral", 2.0)   # glide to a preset over 2 seconds
space.morph_to("Pause", 0.6)       # freeze what's playing
space.freeze = false
space.mix = 70.0
space.set_parameter("void_decay", 12.0)   # any of the 132 parameters by id
```

Morphs are click-free: levels glide, a section that switches on fades up from
silence and one that switches off fades out first.

## Presets

The first twelve are made for games: Cave, Underwater, Cathedral, Tunnel,
Canyon, Dream, Memory, Pause, Low Health, Storm, Void, Orbit. Their echoes and
pulses are set in milliseconds and hertz, so they don't depend on a tempo.

After those come the 92 presets from the UNDERTOW plug-in. Some of them sync
to a tempo; set **Tempo** (or `space.tempo = 128.0`) to match your music.

`get_game_preset_names()`, `get_preset_names()` and `get_parameter_ids()` list
what's available.

## Notes

- Each bus the effect is on gets its own instance, so you can put it on
  several buses with different settings.
- It costs about 2 % of one CPU core per instance at 48 kHz on a recent Mac.
- The effect keeps processing when its bus goes quiet, so tails ring out.

## Licence

MIT (see LICENSE.md). Credit is appreciated, not required:
"UNDERTOW by Rob Cawley (robcawley.com)".

Source: https://github.com/robcawley/undertow
