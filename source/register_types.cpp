#include "register_types.hpp"

#include <godot_cpp/classes/resource_loader.hpp>

#include "interface/midi_listener.hpp"
#include "interface/midi_player.hpp"
#include "midi/midi_event.hpp"
#include "midi/midi_header.hpp"
#include "midi/midi_loader.hpp"
#include "midi/midi_resource.hpp"
#include "midi/midi_track.hpp"

static Ref<MIDILoader> midi_loader;

void initialize_midi_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_CLASS(MIDIHeader);
	GDREGISTER_CLASS(MIDITrack);
	GDREGISTER_CLASS(MIDIEvent);
	GDREGISTER_CLASS(MIDIEventNote);
	GDREGISTER_CLASS(MIDIEventSystem);
	GDREGISTER_CLASS(MIDIEventMeta);
	GDREGISTER_CLASS(MIDIResource);
	GDREGISTER_CLASS(MIDILoader);
	GDREGISTER_CLASS(MIDIPlayer);
	GDREGISTER_CLASS(MIDIListener);

	midi_loader.instantiate();
	ResourceLoader::get_singleton()->add_resource_format_loader(midi_loader);
}

void uninitialize_midi_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (midi_loader.is_valid()) {
		ResourceLoader::get_singleton()->remove_resource_format_loader(midi_loader);
		midi_loader.unref();
	}
}

extern "C" {
GDExtensionBool GDE_EXPORT
midi_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_midi_module);
	init_obj.register_terminator(uninitialize_midi_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
