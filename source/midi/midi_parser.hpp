#pragma once

// We don't need windows.h in this plugin but many others do and it throws up on
// itself all the time So best to include it and make sure CI warns us when we
// use something Microsoft took for their own goals....
#ifdef WIN32
#include <windows.h>
#endif

#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/ref.hpp>

#include "midi_event.hpp"

using namespace godot;

/// @brief MIDIParser class, contains various classes and functions for parsing midi files
class MIDIParser : public RefCounted {
  GDCLASS(MIDIParser, RefCounted);

protected:
  /// @brief override method for registering c++ functions in godot
  static void _bind_methods() {};

public:
  // raw bytes vs interpreted MIDI data

  // MTrk, MThd, and unknown
  enum MIDIChunkType { Header, Track, Unknown };

  // generic binary representation of a chunk
  class RawMIDIChunk {
  public:
    String chunk_id;
    uint32_t chunk_size;
    PackedByteArray chunk_data;
    MIDIChunkType chunk_type;

    RawMIDIChunk() {
      chunk_id = "";
      chunk_size = 0;
      chunk_type = MIDIChunkType::Unknown;
    };

    PackedByteArray load_from_bytes(PackedByteArray bytes);
  };

  // decoded data objects 
  class MIDIHeader {
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
      int32_t num_tracks;
      MIDIDivisionType division_type;
      int32_t division;

      int32_t frames_per_second;
      int32_t ticks_per_frame;

      int32_t tempo;
      bool end_of_track;
      bool only_notes;

      MIDIHeader();

      bool parse(RawMIDIChunk raw);
  };

  class MIDITrack {
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

      // std::vector<std::unique_ptr<MIDIEvent>> events; // we should move these to proper references of the events rather than pointers
      TypedArray<Ref<MIDIEvent>> events;

      MIDITimeSignature time_signature;
      MIDIKeySignature key_signature;

      MIDITrack();

      bool parse(RawMIDIChunk raw, MIDIHeader &header);
  };

  MIDIParser();
  ~MIDIParser();
};