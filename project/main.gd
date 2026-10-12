extends Node

@onready var midi_player := $MIDIPlayer
@onready var midi_listener := $MIDIListener
@onready var midi_visualizer := $MIDIVisualizer
@onready var audio_player := $AudioStreamPlayer

func _ready() -> void:
	OS.close_midi_inputs()
	OS.open_midi_inputs()
	
	midi_listener.midi_event.connect(func(input_event: InputEventMIDI): midi_visualizer.on_midi_event(input_event, 1))
	
	midi_player.event.connect(on_midi_event)
	midi_player.link_audio_stream_player([audio_player])
	midi_player.play()

func on_midi_event(midi_event: MIDIEvent, track: int) -> void:
	var input_event: InputEventMIDI = midi_player.midi_event_to_input_event(midi_event)
	midi_visualizer.on_midi_event(input_event, track)
