#pragma once

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "midi_chunk.hpp"

using namespace godot;

/// @brief MIDIHeader object represented decoded header data chunk 
class MIDIHeader: public Resource {
  GDCLASS(MIDIHeader, Resource);

protected:
  /// @brief override method for registering c++ functions in godot
  static void _bind_methods() {};

public:
  enum MIDIFileFormat {
    SingleTrack = 0,
    MultipleSimultaneousTracks = 1,
    MultipleIndependentTracks = 2
  };

  enum MIDIDivisionType {
    TicksPerQuarterNote = 0,
    FramesPerSecond = 1
  };

  MIDIFileFormat file_format;
  int32_t track_count;

  MIDIDivisionType division_type;
  int32_t division;

  int32_t frames_per_second;
  int32_t ticks_per_frame;

  MIDIHeader();

  bool parse(MIDIChunk raw);
};
