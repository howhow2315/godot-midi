#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "midi_event.hpp"
#include "midi_chunk.hpp"

using namespace godot;

/// @brief MIDITrack object representing decoded track data chunk
class MIDITrack : public Resource {
  GDCLASS(MIDITrack, Resource);

protected:
  /// @brief override method for registering c++ functions in godot
  static void _bind_methods() {}

public:
  struct MIDITimeSignature {
    int32_t numerator = 4;
    int32_t denominator = 4;
    int32_t clocks_per_tick = 24;
    int32_t num_32nd_notes_per_quarter = 8;
  };

  struct MIDIKeySignature {
    int32_t sharps_flats = 0;
    int32_t major_minor = 0;
  };

  TypedArray<Ref<MIDIEvent>> events;

  MIDITimeSignature time_signature;
  MIDIKeySignature key_signature;

  MIDITrack();
  ~MIDITrack() = default;

  bool parse(MIDIChunk raw);
};
