#pragma once

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node.hpp>

namespace godot {

/// @brief MIDIListener extremely simple helper object for listening 
// to midi input from a single source through godots _input system
class MIDIListener: public Node {
  GDCLASS(MIDIListener, Node);

private:
  static MIDIListener *current;
  static int listener_count;

protected:
  static void _bind_methods();

public:
  MIDIListener();
  ~MIDIListener();

  void _enter_tree() override;
  void _exit_tree() override;
  void _input(const Ref<InputEvent> &event) override;

  void make_current();
  bool is_current() const;

  static MIDIListener *get_current();
};

} // namespace godot
