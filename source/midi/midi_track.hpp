#pragma once

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "midi_parser.hpp"
#include "midi_event.hpp"

using namespace godot;

/// @brief MIDITrack object represented decoded track data chunk 
class MIDITrack : public Resource {
  GDCLASS(MIDITrack, Resource);

protected:
  /// @brief override method for registering c++ functions in godot
  static void _bind_methods() {}

public:
  struct MIDITimeSignature {
      int32_t numerator;
      int32_t denominator;
      int32_t clocks_per_tick;
      int32_t num_32nd_notes_per_quarter;
  };

  struct MIDIKeySignature {
      int32_t sharps_flats;
      int32_t major_minor;
  };

  TypedArray<Ref<MIDIEvent>> events;

  MIDITimeSignature time_signature;
  MIDIKeySignature key_signature;
  
  MIDITrack();
  ~MIDITrack();

  bool parse(MIDIParser::RawMIDIChunk raw);
};