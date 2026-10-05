#pragma once

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node.hpp>

namespace godot {

class MIDIListener : public Node {
  GDCLASS(MIDIListener, Node);

private:
  static MIDIListener *current;
  static int listener_count;

protected:
  static void _bind_methods();

public:
  MIDIListener();
  ~MIDIListener();

  void _enter_tree();
  void _exit_tree();
  void _input(const Ref<InputEvent> &event);

  void make_current();
  bool is_current() const;

  static MIDIListener *get_current();
};

} // namespace godot
