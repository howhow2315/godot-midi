#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "midi_chunk.hpp"
#include "midi_event.hpp"

using namespace godot;

/// @brief MIDITrack object representing decoded track data chunk.
class MIDITrack : public Resource {
  GDCLASS(MIDITrack, Resource);

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

  TypedArray<MIDIEvent> events;

  MIDITimeSignature time_signature;
  MIDIKeySignature key_signature;

protected:
  static void _bind_methods() {
    ClassDB::bind_method(D_METHOD("get_time_signature_numerator"),
                         &MIDITrack::get_time_signature_numerator);
    ClassDB::bind_method(D_METHOD("set_time_signature_numerator", "value"),
                         &MIDITrack::set_time_signature_numerator);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "time_signature_numerator"),
                 "set_time_signature_numerator",
                 "get_time_signature_numerator");

    ClassDB::bind_method(D_METHOD("get_time_signature_denominator"),
                         &MIDITrack::get_time_signature_denominator);
    ClassDB::bind_method(D_METHOD("set_time_signature_denominator", "value"),
                         &MIDITrack::set_time_signature_denominator);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "time_signature_denominator"),
                 "set_time_signature_denominator",
                 "get_time_signature_denominator");

    ClassDB::bind_method(D_METHOD("get_clocks_per_tick"),
                         &MIDITrack::get_clocks_per_tick);
    ClassDB::bind_method(D_METHOD("set_clocks_per_tick", "value"),
                         &MIDITrack::set_clocks_per_tick);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "clocks_per_tick"),
                 "set_clocks_per_tick", "get_clocks_per_tick");

    ClassDB::bind_method(D_METHOD("get_num_32nd_notes_per_quarter"),
                         &MIDITrack::get_num_32nd_notes_per_quarter);
    ClassDB::bind_method(D_METHOD("set_num_32nd_notes_per_quarter", "value"),
                         &MIDITrack::set_num_32nd_notes_per_quarter);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "num_32nd_notes_per_quarter"),
                 "set_num_32nd_notes_per_quarter",
                 "get_num_32nd_notes_per_quarter");

    ClassDB::bind_method(D_METHOD("get_sharps_flats"),
                         &MIDITrack::get_sharps_flats);
    ClassDB::bind_method(D_METHOD("set_sharps_flats", "value"),
                         &MIDITrack::set_sharps_flats);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "sharps_flats"), "set_sharps_flats",
                 "get_sharps_flats");

    ClassDB::bind_method(D_METHOD("get_major_minor"),
                         &MIDITrack::get_major_minor);
    ClassDB::bind_method(D_METHOD("set_major_minor", "value"),
                         &MIDITrack::set_major_minor);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "major_minor"), "set_major_minor",
                 "get_major_minor");

    ClassDB::bind_method(D_METHOD("set_events"), &MIDITrack::set_events);
    ClassDB::bind_method(D_METHOD("get_events"), &MIDITrack::get_events);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "events", PROPERTY_HINT_ARRAY_TYPE,
                     "MIDIEvent",
                     PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY),
        "set_events", "get_events");
  }

public:
  /// @brief Get all decoded MIDI events.

  void set_events(TypedArray<MIDIEvent> p_events) { events = p_events; }
  TypedArray<MIDIEvent> get_events() const { return events; }

  // Time signature
  int32_t get_time_signature_numerator() const {
    return time_signature.numerator;
  }
  void set_time_signature_numerator(int32_t value) {
    time_signature.numerator = value;
  }

  int32_t get_time_signature_denominator() const {
    return time_signature.denominator;
  }
  void set_time_signature_denominator(int32_t value) {
    time_signature.denominator = value;
  }

  int32_t get_clocks_per_tick() const { return time_signature.clocks_per_tick; }
  void set_clocks_per_tick(int32_t value) {
    time_signature.clocks_per_tick = value;
  }

  int32_t get_num_32nd_notes_per_quarter() const {
    return time_signature.num_32nd_notes_per_quarter;
  }
  void set_num_32nd_notes_per_quarter(int32_t value) {
    time_signature.num_32nd_notes_per_quarter = value;
  }

  // Key signature
  int32_t get_sharps_flats() const { return key_signature.sharps_flats; }
  void set_sharps_flats(int32_t value) { key_signature.sharps_flats = value; }

  int32_t get_major_minor() const { return key_signature.major_minor; }
  void set_major_minor(int32_t value) { key_signature.major_minor = value; }

  /// @brief Parse a raw MIDI track chunk.
  bool parse(MIDIChunk raw);
};