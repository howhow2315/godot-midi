#ifndef MIDI_PARSER_CLASS_H
#define MIDI_PARSER_CLASS_H

// We don't need windows.h in this plugin but many others do and it throws up on itself all the time
// So best to include it and make sure CI warns us when we use something Microsoft took for their own goals....
#ifdef WIN32
#include <windows.h>
#endif

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <memory>
#include <vector>
#include "utility.h"

using namespace godot;

/// @brief MIDIParser class, contains various classes and functions for parsing midi files
class MIDIParser : public RefCounted
{
    GDCLASS(MIDIParser, RefCounted);

protected:
    static void _bind_methods();

public:
    enum MIDIChunkType
    {
        Header,
        Track,
        Unknown
    };

    class RawMIDIChunk
    {
    public:
        String chunk_id;
        uint32_t chunk_size;
        PackedByteArray chunk_data;
        MIDIChunkType chunk_type;

        RawMIDIChunk()
        {
            chunk_id = "";
            chunk_size = 0;
            chunk_type = MIDIChunkType::Unknown;
        };

        PackedByteArray load_from_bytes(PackedByteArray bytes);
    };

    class MIDIHeaderChunk
    {
    public:
        enum MIDIFileFormat
        {
            SingleTrack = 0,
            MultipleSimultaneousTracks = 1,
            MultipleIndependentTracks = 2
        };

        enum MIDIDivisionType
        {
            TicksPerQuarterNote = 0,
            FramesPerSecond = 1
        };

        MIDIFileFormat file_format;
        int32_t num_tracks;
        MIDIDivisionType division_type;
        int32_t division;
        // only valid when division_type == FramesPerSecond
        // frames_per_second is the positive frame rate (24, 25, 29 for 29.97 drop-frame, or 30)
        // ticks_per_frame is the resolution within each frame
        int32_t frames_per_second;
        int32_t ticks_per_frame;
        int32_t tempo;
        bool end_of_track;
        bool only_notes;

        MIDIHeaderChunk();
        bool parse_chunk(RawMIDIChunk raw, MIDIHeaderChunk &header);
    };

    class MIDIChunk
    {
    public:
        virtual bool parse_chunk(RawMIDIChunk raw, MIDIHeaderChunk &header) = 0;
    };

    class MIDIEvent
    {
    public:
        enum EventType
        {
            Note,
            Meta,
            System
        };

        int32_t channel;
        double delta;

        virtual ~MIDIEvent() = default;
        MIDIEvent(int32_t channel, double delta);
        MIDIEvent(const MIDIEvent &other);

        virtual int32_t get_bytes_used() const;

        String to_string() const;

        virtual EventType get_type() const = 0;

    protected:
        int32_t bytes_used;
    };

    class MIDIEventNote : public MIDIEvent
    {
    public:
        enum NoteType
        {
            NoteOn = 0x09,
            NoteOff = 0x08,
            Aftertouch = 0x0A,
            Controller = 0x0B,
            ProgramChange = 0x0C,
            ChannelPressure = 0x0D,
            PitchBend = 0x0E,
            Unknown = 0xFF
        };

        uint8_t note;
        uint8_t data;
        NoteType event_type;

        MIDIEventNote(int32_t channel, double delta_time, PackedByteArray data, NoteType event_type);

        MIDIEventNote(const MIDIEventNote &other) : MIDIEvent(other)
        {
            note = other.note;
            data = other.data;
            event_type = other.event_type;
        }

        EventType get_type() const override
        {
            return MIDIEvent::EventType::Note;
        };
    };

    class MIDIEventSystem : public MIDIEvent
    {
    public:
        enum MIDISystemEventType
        {
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

        MIDISystemEventType event_type;

        MIDIEventSystem(double delta_time, PackedByteArray data);

        MIDIEventSystem(const MIDIEventSystem &other) : MIDIEvent(other)
        {
            event_type = other.event_type;
        }

        EventType get_type() const override
        {
            return MIDIEvent::EventType::System;
        };
    };

    class MIDIEventMeta : public MIDIEvent
    {
    public:
        enum MIDIMetaEventType
        {
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
            KeySignature = 0x59,
        };

        MIDIMetaEventType event_type;
        PackedByteArray data;
        int32_t event_data_length;

        // the actual processed data
        Variant meta_data;

        MIDIEventMeta(double delta_time, PackedByteArray data);
        MIDIEventMeta(const MIDIEventMeta &other) : MIDIEvent(other)
        {
            event_type = other.event_type;
            data = other.data;
            event_data_length = other.event_data_length;
            meta_data = other.meta_data;
        }

        EventType get_type() const override
        {
            return MIDIEvent::EventType::Meta;
        };
    };

    class MIDITrackChunk : MIDIChunk
    {
    public:
        struct MIDITimeSignature
        {
            int32_t numerator;
            int32_t denominator;
            int32_t clocks_per_tick;
            int32_t num_32nd_notes_per_quarter;
        };
        struct MIDIKeySignature
        {
            int32_t sharps_flats;
            int32_t major_minor;
        };

        enum MIDIEventType
        {
            Note,
            Meta,
            System
        };

        std::vector<MIDIEventNote> note_events;
        std::vector<MIDIEventMeta> meta_events;
        std::vector<MIDIEventSystem> system_events;
        std::vector<std::unique_ptr<MIDIEvent>> events;

        MIDITimeSignature time_signature;
        MIDIKeySignature key_signature;

        MIDITrackChunk()
        {
            note_events = std::vector<MIDIEventNote>();
            meta_events = std::vector<MIDIEventMeta>();
            system_events = std::vector<MIDIEventSystem>();
            events = std::vector<std::unique_ptr<MIDIEvent>>();

            time_signature = {
                4,
                4,
                24,
                8};
        }

        void IngestMetaEvent(MIDIEventMeta &meta_event, MIDIHeaderChunk &header);
        bool parse_chunk(RawMIDIChunk raw, MIDIHeaderChunk &header);
    };

    MIDIParser();
    ~MIDIParser();
};

#endif // MIDI_PARSER_CLASS_H