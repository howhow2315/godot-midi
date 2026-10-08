#pragma once

#include "godot_cpp/classes/global_constants.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

/// @brief MIDIEvent
class MIDIEvent : public Resource {
  GDCLASS(MIDIEvent, Resource);

protected:
  /// @brief override method for registering c++ functions in godot
  static void _bind_methods() {
    ClassDB::bind_method(D_METHOD("get_channel"), &MIDIEvent::get_channel);

    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "channel", PROPERTY_HINT_NONE, "",
                     PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY),
        "", "get_channel");
  };
  int32_t bytes_used;

public:
  enum MIDIEventType { Unknown, Note, Meta, System };

  int32_t channel;
  double delta;

  MIDIEvent();
  MIDIEvent(int32_t channel, double delta);
  MIDIEvent(const MIDIEvent &other);
  ~MIDIEvent();

  virtual int32_t get_bytes_used() const;

  String to_string() const;

  int32_t get_channel() const { return channel; };

  virtual MIDIEventType get_type() const { return MIDIEventType::Unknown; }
};

class MIDIEventNote : public MIDIEvent {
protected:
  static void _bind_methods() {
    ClassDB::bind_method(D_METHOD("get_note"), &MIDIEvent::get_channel);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "note", PROPERTY_HINT_NONE, "",
                     PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY),
        "", "get_note");

    ClassDB::bind_method(D_METHOD("get_event_type"), &MIDIEvent::get_channel);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "event_type", PROPERTY_HINT_ENUM,
                     "MIDIEventNoteType",
                     PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY),
        "", "get_event_type");
  };

public:
  enum class MIDIEventNoteType {
    Unknown = 0xFF, // MIDI_MESSAGE_NONE ?

    NoteOff = 0x08,         // MIDI_MESSAGE_NOTE_OFF
    NoteOn = 0x09,          // MIDI_MESSAGE_NOTE_ON
    Aftertouch = 0x0A,      // MIDI_MESSAGE_AFTERTOUCH
    Controller = 0x0B,      // MIDI_MESSAGE_CONTROL_CHANGE
    ProgramChange = 0x0C,   // MIDI_MESSAGE_PROGRAM_CHANGE
    ChannelPressure = 0x0D, // MIDI_MESSAGE_CHANNEL_PRESSURE
    PitchBend = 0x0E        // MIDI_MESSAGE_PITCH_BEND
  };

  uint8_t note;
  uint8_t data;
  MIDIEventNoteType event_type;

  MIDIEventNote(int32_t channel, double delta_time, PackedByteArray data,
                MIDIEventNoteType event_type);

  MIDIEventNote(const MIDIEventNote &other) : MIDIEvent(other) {
    note = other.note;
    data = other.data;
    event_type = other.event_type;
  }

  int32_t get_note() const { return note; };
  MIDIEventNoteType get_event_type() const { return event_type; };

  MIDIEventType get_type() const override { return MIDIEventType::Note; };
};

class MIDIEventSystem : public MIDIEvent {
protected:
  static void _bind_methods() {
    ClassDB::bind_method(D_METHOD("get_event_type"), &MIDIEvent::get_channel);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "event_type", PROPERTY_HINT_NONE,
                     "MIDIEventSystemType",
                     PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY),
        "", "get_event_type");
  };

public:
  enum class MIDIEventSystemType {
    // https://en.wikipedia.org/wiki/MIDI_Machine_Control
    SystemExclusiveStart = 0xF0,  // MIDI_MESSAGE_SYSTEM_EXCLUSIVE ?
    SystemExclusiveEscape = 0xF7, // MIDI_MESSAGE_SYSTEM_EXCLUSIVE as well ?

    MTCQuarterFrame = 0xF1,     // MIDI_MESSAGE_QUARTER_FRAME
    SongPositionPointer = 0xF2, // MIDI_MESSAGE_SONG_POSITION_POINTER
    SongSelect = 0xF3,          // MIDI_MESSAGE_SONG_SELECT
    TuneRequest = 0xF6,         // MIDI_MESSAGE_TUNE_REQUEST
    TimingClock = 0xF8,         // MIDI_MESSAGE_TIMING_CLOCK
    Start = 0xFA,               // MIDI_MESSAGE_START
    Continue = 0xFB,            // MIDI_MESSAGE_CONTINUE
    Stop = 0xFC,                // MIDI_MESSAGE_STOP
    ActiveSensing = 0xFE,       // MIDI_MESSAGE_ACTIVE_SENSING
    Reset = 0xFF                // MIDI_MESSAGE_SYSTEM_RESET
  };

  MIDIEventSystemType event_type;

  MIDIEventSystem(double delta_time, PackedByteArray data);

  MIDIEventSystem(const MIDIEventSystem &other) : MIDIEvent(other) {
    event_type = other.event_type;
  }

  MIDIEventSystemType get_event_type() const { return event_type; };

  MIDIEventType get_type() const override { return MIDIEventType::System; };
};

// @GlobalScope.MIDIMessage doesn't include any meta events
class MIDIEventMeta : public MIDIEvent {
protected:
  static void _bind_methods() {
    ClassDB::bind_method(D_METHOD("get_event_type"), &MIDIEvent::get_channel);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "event_type", PROPERTY_HINT_NONE,
                     "MIDIEventMetaType",
                     PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY),
        "", "get_event_type");

    ClassDB::bind_method(D_METHOD("get_meta_data"), &MIDIEvent::get_channel);
    ADD_PROPERTY(
        PropertyInfo(Variant::NIL, "meta_data", PROPERTY_HINT_NONE, "",
                     PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY),
        "", "get_meta_data");
  };

public:
  enum class MIDIEventMetaType {
    SequenceNumber = 0x00,
    TextEvent = 0x01,
    CopyRightNotice = 0x02,
    SequenceOrTrackName = 0x03,
    InstrumentName = 0x04,
    Lyric = 0x05,
    Marker = 0x06,
    CuePoint = 0x07,
    ProgramName = 0x08,
    DeviceName = 0x09,
    ArtistName = 0x0A, // note, this isn't in the spec
    EndOfTrack = 0x2F,
    SetTempo = 0x51,
    SMPTEOffset = 0x54,
    TimeSignature = 0x58,
    KeySignature = 0x59
  };

  MIDIEventMetaType event_type;
  PackedByteArray data;
  int32_t event_data_length;

  // the actual processed data
  Variant meta_data;

  MIDIEventMeta(double delta_time, PackedByteArray data);
  MIDIEventMeta(const MIDIEventMeta &other) : MIDIEvent(other) {
    event_type = other.event_type;
    data = other.data;
    event_data_length = other.event_data_length;
    meta_data = other.meta_data;
  }

  MIDIEventMetaType get_event_type() const { return event_type; };
  Variant get_meta_data() const { return meta_data; };

  MIDIEventType get_type() const override { return MIDIEventType::Meta; };
};