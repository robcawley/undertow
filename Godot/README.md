# UNDERTOW for Godot

The `demo/` folder is a complete Godot 4.5+ project: the addon is in
`demo/addons/undertow/` (see its README for how to use it), and the demo
scene lets you walk between rooms while the music's space follows you.

## Build

Needs Python with SCons (`pip install scons`) and a godot-cpp checkout on the
4.5 branch.

    git clone -b 4.5 https://github.com/godotengine/godot-cpp.git godot-cpp
    scons platform=macos arch=universal target=template_release
    # or platform=windows arch=x86_64, platform=linux arch=x86_64 / arm64

The library lands in `demo/addons/undertow/bin/`. The sound engine is
`../Source/Core` (MIT); nothing here depends on JUCE.

## Check

    godot --headless --editor --path demo --quit-after 900   # first import
    godot --headless --path demo -s res://test/run_tests.gd

The checks run audio through a bus with UNDERTOW on it, morph between
presets, and walk the demo's player through every room. GitHub Actions
builds all platforms and runs them on Linux and Windows
(`.github/workflows/godot.yml`).
