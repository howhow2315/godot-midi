class_name MIDIVisualizer
extends Node3D


# constants

# theres no issue with switching this out i just thought it looked nice
# a hue offset would be a good idea too
const GOLDEN_RATIO := (1.0 + sqrt(5.0)) / 2.0
const GOLDEN_RATIO_CONJUGATE := 1.0 / GOLDEN_RATIO
const GOLDEN_RATIO_CONJUGATE_COMPLEMENT := 1.0 - GOLDEN_RATIO_CONJUGATE
const HUE_RANGE := 1.0

const NOTE_SHADER_CODE := """
shader_type spatial;
render_mode unshaded;

uniform float cap_height : hint_range(0.001, 0.5) = 0.03;

varying vec3 local_pos;
varying float note_length;

void vertex() {
	local_pos = VERTEX;
	note_length = MODEL_MATRIX[1].y;
}

void fragment() {
	vec3 color = COLOR.rgb;
	
	// convert the fixed height into the boxs local-space
	float local_cap_height = cap_height / abs(note_length);
	
	if (local_pos.y > 0.5 - local_cap_height) {
		color = vec3(0.0);
	}
	
	ALBEDO = color;
}
"""

# properties

@export_group("Note Movement")
@export var note_speed: float = 1.0
@export var note_width: float = 0.1
@export var note_depth: float = 0.1
@export var max_notes: int = 4096

@export_group("Keyboard Layout")
@export var min_pitch: int = 21
@export var max_pitch: int = 108
@export var note_min_x: float = -15.0
@export var note_max_x: float = 15.0

@export_group("Note Range")
@export var note_min_y: float = -20.0
@export var note_max_y: float = 20.0

var note_shader: Shader
var multimesh: MultiMesh
var note_renderer: MultiMeshInstance3D

# active note data indexed by MultiMesh instance id
var notes: Array = [] # theres probably a more efficient way to do this but i really dont enough care for it
var free_slots: Array[int] = []

# MIDI pitch + track to active instance ID
var notes_on: Dictionary = {}


# virtual methods

func _ready() -> void:
	_setup_renderer()
	_initialize_note_slots()

func _process(delta: float) -> void:
	var speed := delta * note_speed

	for i in range(notes.size()):
		var note = notes[i]
		if note == null: continue
		
		note["y"] += speed
		
		if note["held"]:
			note["length"] += speed
			note["y"] -= speed * 0.5
		
		var half_length: float = note["length"] * 0.5
		var top_y: float = note["y"] - half_length
		var bottom_y: float = note["y"] + half_length
		
		# Release once the entire note has passed the bottom boundary.
		if top_y > note_max_y:
			_release_note(i)
			continue
		
		# clamp the bottom edge to the visible boundary
		var visible_length: float = max(0.0, min(bottom_y, note_max_y) - top_y)
		
		# the transform position must be the center of the visible portion
		var visible_center_y: float = top_y + visible_length * 0.5
		
		var note_transform := Transform3D(
			Basis.IDENTITY.scaled(Vector3(note_width, visible_length, note_depth)),
			Vector3(note["x"], visible_center_y, 0.0)
		)
		
		multimesh.set_instance_transform(i, note_transform)


# internal methods

func _setup_renderer() -> void:
	note_shader = Shader.new()
	note_shader.code = NOTE_SHADER_CODE
	
	var material := ShaderMaterial.new()
	material.shader = note_shader
	
	var mesh := BoxMesh.new()
	mesh.size = Vector3.ONE
	mesh.material = material
	
	multimesh = MultiMesh.new()
	multimesh.transform_format = MultiMesh.TRANSFORM_3D
	multimesh.use_colors = true
	multimesh.mesh = mesh
	multimesh.instance_count = max_notes
	multimesh.visible_instance_count = -1
	
	note_renderer = MultiMeshInstance3D.new()
	note_renderer.multimesh = multimesh
	note_renderer.custom_aabb = AABB(Vector3(-100, -100, -100), Vector3(200, 200, 200))
	add_child(note_renderer)

func _initialize_note_slots() -> void:
	notes.clear()
	notes.resize(max_notes)
	
	free_slots.clear()
	for i in range(max_notes - 1, -1, -1):
		free_slots.append(i)
		
		# hide unused instances
		multimesh.set_instance_transform(i, Transform3D(Basis.IDENTITY.scaled(Vector3.ZERO), Vector3.ZERO))

func _release_note(index: int) -> void:
	var note = notes[index]
	if note == null: return
	
	var pitch: int = note["pitch"]
	
	# Don't erase a newer note with the same pitch.
	if notes_on.get(pitch, -1) == index:
		notes_on.erase(pitch)
	
	notes[index] = null
	free_slots.append(index)
	
	# Hide this instance without affecting other instances.
	multimesh.set_instance_transform(index, Transform3D(Basis.IDENTITY.scaled(Vector3.ZERO), Vector3.ZERO))

func on_midi_event(input_event: InputEventMIDI, track: int) -> void:
	var pitch: int = input_event.pitch
	
	if input_event.message == MIDI_MESSAGE_NOTE_ON:
		if input_event.velocity == 0:
			_note_off(pitch, track)
			return
		
		if free_slots.is_empty(): return
		
		var index: int = free_slots.pop_back()
		
		# calculate a consistent color from instrument and track.
		var hue := fposmod(float(input_event.instrument) * GOLDEN_RATIO_CONJUGATE + float(track) * GOLDEN_RATIO_CONJUGATE_COMPLEMENT, HUE_RANGE)
		
		# Center the keyboard around X = 0.
		var x := pitch_to_x(pitch)
		
		var note := {
			"pitch": pitch,
			"track": track,
			"x": x,
			"y": 0.0,
			"length": 0.0001,
			"held": true
		}
		
		notes[index] = note
		var key := Vector2i(pitch, track)
		
		# in the event of 2 note ons and 1 note off, we want to
		# release the previous note before overwriting its entry in notes_on
		if notes_on.has(key):
			var old_index: int = notes_on[key]
			if notes[old_index] != null:
				notes[old_index]["held"] = false
		
		notes_on[key] = index
		
		multimesh.set_instance_color(index, Color.from_hsv(hue, 0.8, 1.0))
		
		# Initialize the instance immediately.
		multimesh.set_instance_transform(index, Transform3D(Basis.IDENTITY.scaled(Vector3(note_width, note["length"], note_depth)), Vector3(x, 0.0, 0.0)))
		
	elif input_event.message == MIDI_MESSAGE_NOTE_OFF:
		_note_off(pitch, track)

func _note_off(pitch: int, track: int) -> void:
	var key := Vector2i(pitch, track)
	if not notes_on.has(key): return
	
	var index: int = notes_on[key]
	if index >= 0 and index < notes.size():
		if notes[index] != null:
			notes[index]["held"] = false
	
	notes_on.erase(key)

# methods

# convert MIDI pitch to a centered x coordinate
func pitch_to_x(pitch: int) -> float:
	var pitch_center := (float(min_pitch) + float(max_pitch)) * 0.5
	var pitch_range := max_pitch - min_pitch
	if pitch_range <= 0: return 0.0
	
	var pitch_spacing := (note_max_x - note_min_x) / float(pitch_range)
	
	return (float(pitch) - pitch_center) * pitch_spacing
