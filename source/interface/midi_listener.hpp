#pragma once

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_midi.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

/// @brief MIDIListener extremely simple helper object for listening
// to midi input from a single source through godots _input system
class MIDIListener : public Node {
	GDCLASS(MIDIListener, Node);

private:
	static MIDIListener *current;
	static int listener_count;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("make_current"), &MIDIListener::make_current);

		ClassDB::bind_method(D_METHOD("is_current"), &MIDIListener::is_current);

		ADD_SIGNAL(MethodInfo("midi_event", PropertyInfo(Variant::OBJECT, "event", PROPERTY_HINT_RESOURCE_TYPE, "InputEventMIDI")));
	};

public:
	MIDIListener() { set_process_input(true); }
	~MIDIListener() {}

	void _enter_tree() override;
	void _exit_tree() override;
	void _input(const Ref<InputEvent> &event) override;

	void make_current() { current = this; }
	bool is_current() const { return current == this; };
	MIDIListener *get_current() { return current; };
};
