#include "midi_listener.hpp"

MIDIListener *MIDIListener::current = nullptr;
int MIDIListener::listener_count = 0;

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
