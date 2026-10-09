#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/// @brief Base class for decoded MIDI events.
class MIDIEvent : public Resource {
    GDCLASS(MIDIEvent, Resource);

protected:
    static void _bind_methods() {
        ClassDB::bind_method(
            D_METHOD("get_channel"),
            &MIDIEvent::get_channel
        );
        ClassDB::bind_method(
            D_METHOD("set_channel", "channel"),
            &MIDIEvent::set_channel
        );

        ClassDB::bind_method(
            D_METHOD("get_delta"),
            &MIDIEvent::get_delta
        );
        ClassDB::bind_method(
            D_METHOD("set_delta", "delta"),
            &MIDIEvent::set_delta
        );

        ADD_PROPERTY(
            PropertyInfo(Variant::INT, "channel"),
            "set_channel",
            "get_channel"
        );
        ADD_PROPERTY(
            PropertyInfo(Variant::FLOAT, "delta"),
            "set_delta",
            "get_delta"
        );
    }

    int32_t bytes_used = 0;

public:
    enum MIDIEventType {
        Unknown,
        Note,
        Meta,
        System
    };

    int32_t channel = 0;
    double delta = 0.0;

    MIDIEvent();
    MIDIEvent(int32_t p_channel, double p_delta);
    MIDIEvent(const MIDIEvent &other);
    ~MIDIEvent();

    void set_channel(int32_t p_channel) {
        channel = p_channel;
    }

    int32_t get_channel() const {
        return channel;
    }

    void set_delta(double p_delta) {
        delta = p_delta;
    }

    double get_delta() const {
        return delta;
    }

    virtual int32_t get_bytes_used() const;

    String to_string() const;

    virtual MIDIEventType get_type() const {
        return MIDIEventType::Unknown;
    }
};

/// @brief MIDI channel voice event.
class MIDIEventNote : public MIDIEvent {
    GDCLASS(MIDIEventNote, MIDIEvent);

protected:
    static void _bind_methods() {
        ClassDB::bind_method(
            D_METHOD("get_note"),
            &MIDIEventNote::get_note
        );
        ClassDB::bind_method(
            D_METHOD("set_note", "note"),
            &MIDIEventNote::set_note
        );

        ClassDB::bind_method(
            D_METHOD("get_data"),
            &MIDIEventNote::get_data
        );
        ClassDB::bind_method(
            D_METHOD("set_data", "data"),
            &MIDIEventNote::set_data
        );

        ClassDB::bind_method(
            D_METHOD("get_event_type"),
            &MIDIEventNote::get_event_type
        );
        ClassDB::bind_method(
            D_METHOD("set_event_type", "event_type"),
            &MIDIEventNote::set_event_type
        );

        ADD_PROPERTY(
            PropertyInfo(Variant::INT, "note"),
            "set_note",
            "get_note"
        );
        ADD_PROPERTY(
            PropertyInfo(Variant::INT, "data"),
            "set_data",
            "get_data"
        );
        ADD_PROPERTY(
            PropertyInfo(Variant::INT, "event_type"),
            "set_event_type",
            "get_event_type"
        );
    }

public:
    MIDIEventNote():
      MIDIEvent(),
      note(0),
      data(0),
      event_type(MIDIEventNoteType::Unknown) {}

    enum class MIDIEventNoteType {
        Unknown = 0xFF,
        NoteOff = 0x08,
        NoteOn = 0x09,
        Aftertouch = 0x0A,
        Controller = 0x0B,
        ProgramChange = 0x0C,
        ChannelPressure = 0x0D,
        PitchBend = 0x0E
    };

    uint8_t note = 0;
    uint8_t data = 0;
    MIDIEventNoteType event_type = MIDIEventNoteType::Unknown;

    MIDIEventNote(
        int32_t p_channel,
        double p_delta_time,
        PackedByteArray p_data,
        MIDIEventNoteType p_event_type
    );

    MIDIEventNote(const MIDIEventNote &other)
        : MIDIEvent(other) {
        note = other.note;
        data = other.data;
        event_type = other.event_type;
    }

    void set_note(int32_t p_note) {
        note = static_cast<uint8_t>(p_note);
    }

    int32_t get_note() const {
        return note;
    }

    void set_data(int32_t p_data) {
        data = static_cast<uint8_t>(p_data);
    }

    int32_t get_data() const {
        return data;
    }

    void set_event_type(int32_t p_event_type) {
      event_type = static_cast<MIDIEventNoteType>(p_event_type);
    }

    int32_t get_event_type() const {
      return static_cast<int32_t>(event_type);
    }

    MIDIEventType get_type() const override {
        return MIDIEventType::Note;
    }
};

/// @brief MIDI system event.
class MIDIEventSystem : public MIDIEvent {
    GDCLASS(MIDIEventSystem, MIDIEvent);

protected:
    static void _bind_methods() {
        ClassDB::bind_method(
            D_METHOD("get_event_type"),
            &MIDIEventSystem::get_event_type
        );
        ClassDB::bind_method(
            D_METHOD("set_event_type", "event_type"),
            &MIDIEventSystem::set_event_type
        );

        ADD_PROPERTY(
            PropertyInfo(Variant::INT, "event_type"),
            "set_event_type",
            "get_event_type"
        );
    }

public:
    MIDIEventSystem():
      MIDIEvent(),
      event_type(MIDIEventSystemType::TimingClock) {}

    enum class MIDIEventSystemType {
        SystemExclusiveStart = 0xF0,
        MTCQuarterFrame = 0xF1,
        SongPositionPointer = 0xF2,
        SongSelect = 0xF3,
        TuneRequest = 0xF6,
        SystemExclusiveEscape = 0xF7,
        TimingClock = 0xF8,
        Start = 0xFA,
        Continue = 0xFB,
        Stop = 0xFC,
        ActiveSensing = 0xFE,
        Reset = 0xFF
    };

    MIDIEventSystemType event_type = MIDIEventSystemType::TimingClock;

    MIDIEventSystem(double p_delta_time, PackedByteArray p_data);

    MIDIEventSystem(const MIDIEventSystem &other)
        : MIDIEvent(other) {
        event_type = other.event_type;
    }

    void set_event_type(int32_t p_event_type) {
        event_type = static_cast<MIDIEventSystemType>(p_event_type);
    }

    int32_t get_event_type() const {
      return static_cast<int32_t>(event_type);
    }

    MIDIEventType get_type() const override {
        return MIDIEventType::System;
    }
};

/// @brief MIDI meta event.
class MIDIEventMeta : public MIDIEvent {
    GDCLASS(MIDIEventMeta, MIDIEvent);

protected:
    static void _bind_methods() {
        ClassDB::bind_method(
            D_METHOD("get_event_type"),
            &MIDIEventMeta::get_event_type
        );
        ClassDB::bind_method(
            D_METHOD("set_event_type", "event_type"),
            &MIDIEventMeta::set_event_type
        );

        ClassDB::bind_method(
            D_METHOD("get_data"),
            &MIDIEventMeta::get_data
        );
        ClassDB::bind_method(
            D_METHOD("set_data", "data"),
            &MIDIEventMeta::set_data
        );

        ClassDB::bind_method(
            D_METHOD("get_event_data_length"),
            &MIDIEventMeta::get_event_data_length
        );
        ClassDB::bind_method(
            D_METHOD("set_event_data_length", "length"),
            &MIDIEventMeta::set_event_data_length
        );



        ADD_PROPERTY(
            PropertyInfo(Variant::INT, "event_type"),
            "set_event_type",
            "get_event_type"
        );
        ADD_PROPERTY(
            PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data"),
            "set_data",
            "get_data"
        );
        ADD_PROPERTY(
            PropertyInfo(Variant::INT, "event_data_length"),
            "set_event_data_length",
            "get_event_data_length"
        );

        // ClassDB::bind_method(
        //     D_METHOD("get_meta_data"),
        //     &MIDIEventMeta::get_meta_data
        // );
        // ClassDB::bind_method(
        //     D_METHOD("set_meta_data", "meta_data"),
        //     &MIDIEventMeta::set_meta_data
        // );
        // ADD_PROPERTY(
        //     PropertyInfo(Variant::NIL, "meta_data"),
        //     "set_meta_data",
        //     "get_meta_data"
        // );
    }

public:
    MIDIEventMeta():
      MIDIEvent(),
      event_type(MIDIEventMetaType::EndOfTrack),
      event_data_length(0) {}
        
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
        ArtistName = 0x0A,
        EndOfTrack = 0x2F,
        SetTempo = 0x51,
        SMPTEOffset = 0x54,
        TimeSignature = 0x58,
        KeySignature = 0x59
    };

    MIDIEventMetaType event_type = MIDIEventMetaType::EndOfTrack;
    PackedByteArray data;
    int32_t event_data_length = 0;
    Variant meta_data;

    MIDIEventMeta(double p_delta_time, PackedByteArray p_data);

    MIDIEventMeta(const MIDIEventMeta &other)
        : MIDIEvent(other) {
        event_type = other.event_type;
        data = other.data;
        event_data_length = other.event_data_length;
        meta_data = other.meta_data;
    }

    void set_event_type(int32_t p_event_type) {
        event_type = static_cast<MIDIEventMetaType>(p_event_type);
    }

    int32_t get_event_type() const {
        return static_cast<int32_t>(event_type);
    }

    void set_data(const PackedByteArray &p_data) {
        data = p_data;
    }

    PackedByteArray get_data() const {
        return data;
    }

    void set_event_data_length(int32_t p_length) {
        event_data_length = p_length;
    }

    int32_t get_event_data_length() const {
        return event_data_length;
    }

    // void set_meta_data(const Variant &p_meta_data) {
    //     meta_data = p_meta_data;
    // }

    // Variant get_meta_data() const {
    //     return meta_data;
    // }

    MIDIEventType get_type() const override {
        return MIDIEventType::Meta;
    }
};