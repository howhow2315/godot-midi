#pragma once

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "midi_chunk.hpp"

using namespace godot;

/// @brief MIDIHeader object represented decoded header data chunk
class MIDIHeader : public Resource {
  GDCLASS(MIDIHeader, Resource);

protected:
  static void _bind_methods() {
    ClassDB::bind_method(D_METHOD("get_file_format"),
                         &MIDIHeader::get_file_format);
    ClassDB::bind_method(D_METHOD("set_file_format", "value"),
                         &MIDIHeader::set_file_format);

    ClassDB::bind_method(D_METHOD("get_track_count"),
                         &MIDIHeader::get_track_count);
    ClassDB::bind_method(D_METHOD("set_track_count", "value"),
                         &MIDIHeader::set_track_count);

    ClassDB::bind_method(D_METHOD("get_division_type"),
                         &MIDIHeader::get_division_type);
    ClassDB::bind_method(D_METHOD("set_division_type", "value"),
                         &MIDIHeader::set_division_type);

    ClassDB::bind_method(D_METHOD("get_division"), &MIDIHeader::get_division);
    ClassDB::bind_method(D_METHOD("set_division", "value"),
                         &MIDIHeader::set_division);

    ClassDB::bind_method(D_METHOD("get_frames_per_second"),
                         &MIDIHeader::get_frames_per_second);
    ClassDB::bind_method(D_METHOD("set_frames_per_second", "value"),
                         &MIDIHeader::set_frames_per_second);

    ClassDB::bind_method(D_METHOD("get_ticks_per_frame"),
                         &MIDIHeader::get_ticks_per_frame);
    ClassDB::bind_method(D_METHOD("set_ticks_per_frame", "value"),
                         &MIDIHeader::set_ticks_per_frame);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "file_format"), "set_file_format",
                 "get_file_format");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "track_count"), "set_track_count",
                 "get_track_count");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "division_type"),
                 "set_division_type", "get_division_type");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "division"), "set_division",
                 "get_division");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "frames_per_second"),
                 "set_frames_per_second", "get_frames_per_second");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ticks_per_frame"),
                 "set_ticks_per_frame", "get_ticks_per_frame");
  }

public:
  enum MIDIFileFormat {
    SingleTrack = 0,
    MultipleSimultaneousTracks = 1,
    MultipleIndependentTracks = 2
  };

  enum MIDIDivisionType { TicksPerQuarterNote = 0, FramesPerSecond = 1 };

  MIDIFileFormat file_format;
  int32_t track_count;

  MIDIDivisionType division_type;
  int32_t division;

  int32_t frames_per_second;
  int32_t ticks_per_frame;

  MIDIHeader();

  bool parse(MIDIChunk raw);

  int32_t get_file_format() const { return static_cast<int32_t>(file_format); }

  void set_file_format(int32_t value) {
    file_format = static_cast<MIDIFileFormat>(value);
  }

  int32_t get_track_count() const { return track_count; }
  void set_track_count(int32_t value) { track_count = value; }

  int32_t get_division_type() const {
    return static_cast<int32_t>(division_type);
  }

  void set_division_type(int32_t value) {
    division_type = static_cast<MIDIDivisionType>(value);
  }

  int32_t get_division() const { return division; }
  void set_division(int32_t value) { division = value; }

  int32_t get_frames_per_second() const { return frames_per_second; }

  void set_frames_per_second(int32_t value) { frames_per_second = value; }

  int32_t get_ticks_per_frame() const { return ticks_per_frame; }

  void set_ticks_per_frame(int32_t value) { ticks_per_frame = value; }
};
