extends SceneTree
## Headless checks for the UNDERTOW extension. Run:
##   godot --headless --path demo -s res://test/run_tests.gd
## Feeds a known signal through a bus with UNDERTOW and a capture after it.

var failures := 0


func check(ok: bool, label: String) -> void:
	print(("PASS " if ok else "FAIL ") + label)
	if not ok:
		failures += 1


func _initialize() -> void:
	run.call_deferred()


func run() -> void:
	check(ClassDB.class_exists("AudioEffectUndertow"), "AudioEffectUndertow is registered")
	var fx = ClassDB.instantiate("AudioEffectUndertow")
	check(fx != null, "can create the effect")
	if fx == null:
		quit(1)
		return
	check(fx.get_game_preset_names().size() == 12, "12 game presets")
	check(fx.get_preset_names().size() == 104, "104 presets in all")
	check(fx.get_parameter_ids().size() == 132, "132 parameters")
	check(fx.find_preset("Cathedral") == 2, "find a preset by name")
	fx.preset = fx.find_preset("Pause")
	check(fx.freeze and is_equal_approx(fx.mix, 90.0), "loading Pause shows its mix and freeze in the inspector")
	fx.preset = 0
	check(not fx.freeze and is_equal_approx(fx.mix, 50.0), "loading Cave shows its values")
	check(fx.set_parameter("void_decay", 7.5), "set a parameter by id")
	check(is_equal_approx(fx.get_parameter("void_decay"), 7.5), "read it back")
	check(not fx.set_parameter("nope", 1.0), "unknown parameter refused")
	check(not fx.morph_to("Nope", 1.0), "unknown preset refused")

	# Audio: tone bursts in, capture after the effect.
	var bus := AudioServer.bus_count
	AudioServer.add_bus(bus)
	AudioServer.set_bus_name(bus, "Test")
	fx.preset = fx.find_preset("Cathedral")
	AudioServer.add_bus_effect(bus, fx)
	var cap := AudioEffectCapture.new()
	cap.buffer_length = 10.0
	AudioServer.add_bus_effect(bus, cap)
	var gen := AudioStreamGenerator.new()
	gen.mix_rate = AudioServer.get_mix_rate()
	gen.buffer_length = 0.5
	var player := AudioStreamPlayer.new()
	player.stream = gen
	player.bus = "Test"
	root.add_child(player)
	await process_frame
	player.play()
	await process_frame
	var pb: AudioStreamGeneratorPlayback = player.get_stream_playback()
	var rate := AudioServer.get_mix_rate()
	print("mix rate ", rate, ", driver ", AudioServer.get_driver_name() if AudioServer.has_method("get_driver_name") else "?")

	var sent := 0
	var got := PackedVector2Array()
	var start := Time.get_ticks_msec()
	var morph_sent := false
	while Time.get_ticks_msec() - start < 6000:
		while pb.get_frames_available() > 0:
			var t := float(sent) / rate
			var on := fmod(t, 1.0) < 0.15 and t < 3.0
			var v := 0.3 * sin(TAU * 330.0 * t) if on else 0.0
			pb.push_frame(Vector2(v, v))
			sent += 1
		if cap.get_frames_available() > 0:
			got.append_array(cap.get_buffer(cap.get_frames_available()))
		if not morph_sent and Time.get_ticks_msec() - start > 2500:
			morph_sent = fx.morph_to("Cave", 1.0)
		await process_frame
	print("frames in ", sent, ", captured ", got.size())
	check(got.size() > rate * 3, "audio ran through the bus (" + str(got.size()) + " frames)")
	var peak := 0.0
	var finite := true
	var tail := 0.0
	var n := got.size()
	for i in n:
		var f: Vector2 = got[i]
		if is_nan(f.x) or is_nan(f.y) or is_inf(f.x) or is_inf(f.y):
			finite = false
		peak = max(peak, abs(f.x), abs(f.y))
		if i > rate * 3.3 and i < rate * 3.9:
			tail = max(tail, abs(f.x))
	print("peak ", peak, ", reverb tail after the last burst ", tail)
	check(finite, "output finite")
	check(peak > 0.05 and peak <= 1.0, "output audible and under full scale")
	check(tail > 0.002, "the space rings on after the input stops")
	check(morph_sent, "morph_to accepted while playing")
	check(fx.preset == fx.find_preset("Cave") and not fx.is_morphing(), "morph finished on Cave")
	player.stop()
	player.queue_free()
	AudioServer.remove_bus(AudioServer.get_bus_index("Test"))
	fx = null
	pb = null
	await process_frame

	# The demo: walk through every room and check the space follows.
	var main = load("res://main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	var visited := 0
	for r in main.ROOMS:
		main.player_pos = r.rect.get_center()
		for i in 3:
			await process_frame
		if main.room == r.name and main.space.preset == main.space.find_preset(r.name):
			visited += 1
		else:
			print("  room ", r.name, ": demo says ", main.room, ", preset ", main.space.preset)
	check(visited == main.ROOMS.size(), "demo: walking into each room morphs to its space (%d/%d)" % [visited, main.ROOMS.size()])
	var ev := InputEventKey.new()
	ev.physical_keycode = KEY_P
	ev.pressed = true
	main._unhandled_input(ev)
	check(main.space.preset == main.space.find_preset("Pause"), "demo: P pauses (morphs to Pause)")
	main._unhandled_input(ev)
	check(main.space.preset == main.space.find_preset(main.room), "demo: P again returns to the room")
	main.queue_free()
	await process_frame
	print("RESULT ", failures, " failures")
	quit(1 if failures > 0 else 0)
