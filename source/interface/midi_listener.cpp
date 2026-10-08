#include "midi_listener.hpp"

#include <godot_cpp/classes/input_event_midi.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

MIDIListener *MIDIListener::current = nullptr;
int MIDIListener::listener_count = 0;

void MIDIListener::_bind_methods() {
  ClassDB::bind_method(D_METHOD("make_current"), &MIDIListener::make_current);

  ClassDB::bind_method(D_METHOD("is_current"), &MIDIListener::is_current);

  ADD_SIGNAL(MethodInfo("midi_event", PropertyInfo(Variant::OBJECT, "event",
                                                   PROPERTY_HINT_RESOURCE_TYPE,
                                                   "InputEventMIDI")));
}

MIDIListener::MIDIListener() { set_process_input(true); }

MIDIListener::~MIDIListener() {}

void MIDIListener::_enter_tree() {
  if (listener_count == 0) {
    // OS::get_singleton()->open_midi_inputs();
    make_current();
  }

  listener_count++;
}

void MIDIListener::_exit_tree() {
  if (current == this) {
    current = nullptr;
  }

  listener_count--;

  if (listener_count <= 0) {
    listener_count = 0;

    // OS::get_singleton()->close_midi_inputs();
  }
}

void MIDIListener::_input(const Ref<InputEvent> &event) {
  if (current != this) {
    return;
  }

  Ref<InputEventMIDI> midi_event = Object::cast_to<InputEventMIDI>(event.ptr());

  if (midi_event.is_null()) {
    return;
  }

  emit_signal("midi_event", midi_event);
}

void MIDIListener::make_current() { current = this; }

bool MIDIListener::is_current() const { return current == this; }

MIDIListener *MIDIListener::get_current() { return current; }
