#include "register_types.hpp"

#include "midi/midi_header.hpp"
#include "midi/midi_track.hpp"
#include "midi/midi_event.hpp"
#include "midi/midi_resource.hpp"
#include "interface/midi_player.hpp"

void initialize_godotmidi_types(ModuleInitializationLevel p_level) {
  if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
    return;
  }
  ClassDB::register_class<MIDIHeader>();
  ClassDB::register_class<MIDITrack>();
  ClassDB::register_class<MIDIEvent>();
  ClassDB::register_class<MIDIResource>();
  ClassDB::register_class<MIDIPlayer>();
}

void uninitialize_godotmidi_types(ModuleInitializationLevel p_level) {
  if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
    return;
  }
}

extern "C" {

// Initialization.

GDExtensionBool GDE_EXPORT
godotmidi_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                       const GDExtensionClassLibraryPtr p_library,
                       GDExtensionInitialization *r_initialization) {

  godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library,
                                                 r_initialization);

  init_obj.register_initializer(initialize_godotmidi_types);
  init_obj.register_terminator(uninitialize_godotmidi_types);
  init_obj.set_minimum_library_initialization_level(
      MODULE_INITIALIZATION_LEVEL_SCENE);

  return init_obj.init();
}
}
