extends Node2D
## UNDERTOW demo: walk between rooms and the music's space follows you.
##
## Arrow keys or WASD to move (or click where you want to go).
## P pauses: the space freezes what was playing, until you press P again.
##
## How it's wired, in three lines you can copy into your own game:
##   var space := AudioEffectUndertow.new()
##   AudioServer.add_bus_effect(bus_index, space)
##   space.morph_to("Cathedral", 2.0)

const ROOMS := [
	{"name": "Cave", "rect": Rect2(40, 90, 380, 270), "color": Color("3b3226")},
	{"name": "Underwater", "rect": Rect2(450, 90, 380, 270), "color": Color("163a4a")},
	{"name": "Cathedral", "rect": Rect2(860, 90, 380, 270), "color": Color("3d3550")},
	{"name": "Canyon", "rect": Rect2(40, 390, 380, 270), "color": Color("553521")},
	{"name": "Dream", "rect": Rect2(450, 390, 380, 270), "color": Color("4a2f4f")},
	{"name": "Void", "rect": Rect2(860, 390, 380, 270), "color": Color("10131c")},
]
const MORPH_SECONDS := 2.0
const SPEED := 340.0

var space: AudioEffectUndertow
var player_pos := Vector2(230, 225)
var target = null
var room := ""
var paused_from := ""
var hud: Label


func _ready() -> void:
	# A bus for the music with UNDERTOW on it.
	var bus := AudioServer.bus_count
	AudioServer.add_bus(bus)
	AudioServer.set_bus_name(bus, "Space")
	AudioServer.set_bus_send(bus, "Master")
	space = AudioEffectUndertow.new()
	AudioServer.add_bus_effect(bus, space)

	var music := AudioStreamPlayer.new()
	var stream: AudioStreamOggVorbis = load("res://music/loop_90bpm.ogg")
	stream.loop = true
	music.stream = stream
	music.bus = "Space"
	add_child(music)
	music.play()

	hud = Label.new()
	hud.position = Vector2(40, 18)
	hud.add_theme_font_size_override("font_size", 22)
	add_child(hud)

	_enter_room(_room_at(player_pos), 0.0)


func _process(delta: float) -> void:
	var dir := Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	for k in [[KEY_A, Vector2.LEFT], [KEY_D, Vector2.RIGHT], [KEY_W, Vector2.UP], [KEY_S, Vector2.DOWN]]:
		if Input.is_physical_key_pressed(k[0]):
			dir += k[1]
	if dir != Vector2.ZERO:
		target = null
		player_pos += dir.normalized() * SPEED * delta
	elif target != null:
		player_pos = player_pos.move_toward(target, SPEED * delta)
		if player_pos.distance_to(target) < 1.0:
			target = null
	player_pos = player_pos.clamp(Vector2(20, 80), Vector2(1260, 700))

	var here := _room_at(player_pos)
	if here != "" and here != room and paused_from == "":
		_enter_room(here, MORPH_SECONDS)

	var state := "morphing…" if space.is_morphing() else "settled"
	hud.text = "%s   (%s)      Move: arrows / WASD / click      P: pause" % [paused_from if paused_from != "" else room, state]
	if paused_from != "":
		hud.text = "PAUSED: the space holds what was playing.   P to carry on"
	queue_redraw()


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		target = event.position
	if event is InputEventKey and event.pressed and not event.echo and event.physical_keycode == KEY_P:
		if paused_from == "":
			paused_from = room
			space.morph_to("Pause", 0.6)
		else:
			space.morph_to(paused_from, 1.0)
			paused_from = ""


func _enter_room(name: String, seconds: float) -> void:
	room = name
	if seconds <= 0.0:
		space.preset = space.find_preset(name)
	else:
		space.morph_to(name, seconds)


func _room_at(p: Vector2) -> String:
	for r in ROOMS:
		if r.rect.has_point(p):
			return r.name
	return ""


func _draw() -> void:
	draw_rect(Rect2(0, 0, 1280, 720), Color("0b0d12"))
	var font := ThemeDB.fallback_font
	for r in ROOMS:
		var lit: bool = r.name == room
		draw_rect(r.rect, r.color.lightened(0.15 if lit else 0.0))
		draw_rect(r.rect, Color(1, 1, 1, 0.55 if lit else 0.12), false, 2.0)
		draw_string(font, r.rect.position + Vector2(18, 40), r.name.to_upper(), HORIZONTAL_ALIGNMENT_LEFT, -1, 28, Color(1, 1, 1, 0.9 if lit else 0.45))
	draw_circle(player_pos, 13.0, Color("f2c14e"))
	draw_circle(player_pos, 13.0, Color.BLACK, false, 2.0)
