@tool
extends EditorImportPlugin

enum Presets { DEFAULT }

func _get_importer_name():
	return "com.nlaha.godotmidi"

func _get_visible_name():
	return "Godot MIDI"

func _get_recognized_extensions() -> PackedStringArray:
	return PackedStringArray(["mid", "midi"])

func _get_save_extension():
	return "res"

func _get_resource_type():
	return "MIDIResource"

func _get_preset_count():
	return Presets.size()
	
func _get_priority():
	return 1.0
	
func _get_option_visibility(option, name, options):
	return true
	
func _get_import_options(name, preset):
	match preset:
		Presets.DEFAULT:
			return []
		_:
			return []

func _get_preset_name(preset):
	match preset:
		Presets.DEFAULT:
			return "Default"
		_:
			return "Unknown"

func _get_import_order():
	return 0

func _import(source_file, save_path, options, r_platform_variants, r_gen_files):

	print("[GodotMIDI] Importing midi file: " + source_file)

	var save_file = save_path + "." + _get_save_extension()
	var midi_resource = MIDIResource.new()
	if midi_resource.load_file(source_file) != OK:
		printerr("[GodotMIDI] Failed to load midi file: " + source_file)
		return FAILED

	return ResourceSaver.save(midi_resource, save_file)
