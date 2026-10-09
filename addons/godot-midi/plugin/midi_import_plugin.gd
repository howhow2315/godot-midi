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
	#print("[GodotMIDI] Importing midi file: " + source_file)

	var save_file: String = save_path + "." + _get_save_extension()
	var midi_resource := MIDIResource.new()
	if midi_resource.load_file(source_file) != OK:
		printerr("[GodotMIDI] Failed to load midi file: " + source_file)
		return FAILED

	# uncompressed the .import file is 10x the size of the midi file and compressed its still 3x the size
	# we could most definently make this more efficient if we parse on demand and cache the decoded events in memory
	var save_error := ResourceSaver.save(midi_resource, save_file, ResourceSaver.FLAG_COMPRESS)
	if save_error != OK:
		printerr("[GodotMIDI] Save failed: ", save_error)
		return save_error

	# var loaded := ResourceLoader.load(
	# 	save_file,
	# 	"MIDIResource",
	# 	ResourceLoader.CACHE_MODE_IGNORE
	# ) as MIDIResource

	# if loaded == null:
	# 	printerr("[MIDI DEBUG] Failed to reload: ", save_file)
	# 	return FAILED

	return OK
