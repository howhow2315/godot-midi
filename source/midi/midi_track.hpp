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
    ClassDB::bind_method(D_METHOD("get_events"), &MIDITrack::get_events);

    ClassDB::bind_method(D_METHOD("get_time_signature"),
                         &MIDITrack::get_time_signature);

    ClassDB::bind_method(D_METHOD("get_key_signature"),
                         &MIDITrack::get_key_signature);

    ADD_PROPERTY(
        PropertyInfo(Variant::STRING, "time_signature", PROPERTY_HINT_NONE, "",
                     PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY),
        "", "get_time_signature");

    ADD_PROPERTY(
        PropertyInfo(Variant::STRING, "key_signature", PROPERTY_HINT_NONE, "",
                     PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY),
        "", "get_key_signature");

    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "events", PROPERTY_HINT_ARRAY_TYPE,
                     "MIDIEvent",
                     PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY),
        "", "get_events");
  }

public:
  /// @brief Get all decoded MIDI events.
  TypedArray<MIDIEvent> get_events() const { return events; }

  /// @brief Get the time signature in human-readable form.
  ///
  /// Example: "4/4"
  String get_time_signature() const {
    return String::num_int64(time_signature.numerator) + "/" +
           String::num_int64(time_signature.denominator);
  };

  /// @brief Get the key signature in human-readable form.
  ///
  /// Examples: "C", "G", "D", "F", "Bb", "Eb", "Ab", etc.
  String get_key_signature() const {
    static const char *major_keys[] = {"Cb", "Gb", "Db", "Ab", "Eb",
                                       "Bb", "F",  "C",  "G",  "D",
                                       "A",  "E",  "B",  "F#", "C#"};

    static const char *minor_keys[] = {"Ab", "Eb", "Bb", "F",  "C",
                                       "G",  "D",  "A",  "E",  "B",
                                       "F#", "C#", "G#", "D#", "A#"};

    // MIDI key signatures encode sharps/flats as:
    //
    // -7 = 7 flats
    //  0 = C
    // +7 = 7 sharps
    //
    // Clamp to the valid MIDI range.
    const int32_t index = CLAMP(key_signature.sharps_flats + 7, 0, 14);

    if (key_signature.major_minor == 0) {
      return String(major_keys[index]);
    };

    return String(minor_keys[index]) + "m";
  }

  /// @brief Parse a raw MIDI track chunk.
  bool parse(MIDIChunk raw);
};