#include "midi_loader.hpp"

PackedStringArray MIDILoader::_get_recognized_extensions() const {
	PackedStringArray extensions;
	extensions.push_back("mid");
	extensions.push_back("midi");
	return extensions;
}

bool MIDILoader::_handles_type(const StringName &type) const {
	return type == StringName("MIDIResource");
}

String MIDILoader::_get_resource_type(const String &path) const {
	const String extension = path.get_extension().to_lower();

	if (extension == "mid" || extension == "midi") {
		return "MIDIResource";
	}

	return String();
}

Variant MIDILoader::_load(
		const String &path,
		const String &original_path,
		bool use_sub_threads,
		int32_t cache_mode) const {
	Ref<MIDIResource> midi;
	midi.instantiate();

	const Error err = midi->load_file(path);

	if (err != OK) {
		UtilityFunctions::push_error(
				"[GodotMIDI] Failed to load MIDI file: " + path);
		return Variant();
	}

	return midi;
}