#include "../utility.hpp"
#include "midi_track.hpp"

using namespace godot;

/// @brief The main chunk parser, takes bytes from the input stream and parses them into MIDI chunks
/// @param raw the raw chunk of bytes
/// @return
bool MIDITrack::parse(MIDIChunk raw) {
  if (raw.type != MIDIChunk::MIDIChunkType::Track) {
    return false;
  }

  // read events until we reach the end of the chunk
  // we have to use a while loop because we don't know how many events there are
  // we can only know when we reach the end of the chunk
  int32_t offset = 0;

  // per the MIDI spec, consecutive channel voice/mode messages may omit
  // a repeated status byte and reuse the previous one ("running status")
  uint8_t running_status = 0;

  while (offset < raw.size) {
    int32_t bytes_used = 0;

    // first variable length quantity is delta time
    int32_t delta_time = Utility::decode_varint_be(raw.data, offset, bytes_used);
    offset += bytes_used;

    if (offset >= raw.size) {
      // truncated track: nothing left to read after the delta time
      UtilityFunctions::printerr("[GodotMIDI] Warning: track data ended unexpectedly after a delta time, stopping");
      break;
    }

    uint8_t event_status;
    uint8_t next_byte = raw.data[offset];

    // NOTE: MIDI realtime messages (F8-FF) can occur interleaved with channel/system messages
    
    // explicit status
    if (next_byte & 0x80) {
      // byte present
      event_status = next_byte;
      offset += 1;

      // channel voice/mode message, updates running status
      if (event_status < 0xF0) {
        running_status = event_status;

      // system common messages cancel running status
      } else if (event_status <= 0xF7) {
        running_status = 0;
      }
      // system realtime messages (0xF8-0xFE) and meta events (0xFF)
      // don't affect running status

    // running status
    } else {
      // this byte is actually the first data byte of 
      // a channel voice/mode message, reusing the last status byte
      if (running_status == 0) {
        UtilityFunctions::printerr("[GodotMIDI] Malformed MIDI track: data byte encountered with no active running status");
        break;
      }

      event_status = running_status;
    }

    // offset points at the first payload byte in both cases
    // Work out exactly how many payload bytes this event needs *without*
    // copying anything yet, so that we only ever copy the bytes a given
    // event actually needs; slicing to the end of the chunk for every
    // single event would make parsing large tracks quadratic.
    int32_t payload_needed = 0;

    // meta event: <type byte> <length varint> <data...>
    if (event_status == 0xFF) {
      int32_t length_bytes = 0;

      int64_t data_len = Utility::decode_varint_be(raw.data, offset + 1, length_bytes);

      payload_needed = 1 + length_bytes + static_cast<int32_t>(data_len);

    // system exclusive / escape: <length varint> <data...>
    } else if (
        event_status == static_cast<uint8_t>(MIDIEventSystem::MIDIEventSystemType::SystemExclusiveStart) ||
        event_status == static_cast<uint8_t>(MIDIEventSystem::MIDIEventSystemType::SystemExclusiveEscape)) {

      int32_t length_bytes = 0;

      int64_t data_len = Utility::decode_varint_be(raw.data, offset, length_bytes);

      payload_needed = length_bytes + static_cast<int32_t>(data_len);

    // channel voice/mode message
    } else if (event_status < 0xF0) {
      MIDIEventNote::MIDIEventNoteType event_type = static_cast<MIDIEventNote::MIDIEventNoteType>((event_status >> 4) & 0x0F);

      // program change and channel pressure only have one data byte
      switch (event_type) {
        case MIDIEventNote::MIDIEventNoteType::ProgramChange:
        case MIDIEventNote::MIDIEventNoteType::ChannelPressure:
          payload_needed = 1;
          break;

        default:
          payload_needed = 2;
          break;
      }

    // remaining system common/real-time messages
    } else {
      MIDIEventSystem::MIDIEventSystemType event_type = static_cast<MIDIEventSystem::MIDIEventSystemType>(event_status);

      switch (event_type) {
        case MIDIEventSystem::MIDIEventSystemType::MTCQuarterFrame:
        case MIDIEventSystem::MIDIEventSystemType::SongSelect:
          payload_needed = 1;
          break;

        case MIDIEventSystem::MIDIEventSystemType::SongPositionPointer:
          payload_needed = 2;
          break;

        default:
          // F6, F8, FA, FB, FC, FE and other messages have no payload
          payload_needed = 0;
          break;
      }
    }

    // clamp to what's actually available in case of a truncated file
    int32_t remaining_in_chunk = static_cast<int32_t>(raw.size) - offset;

    if (payload_needed > remaining_in_chunk) {
      payload_needed = remaining_in_chunk;
    }

    if (payload_needed < 0) {
      payload_needed = 0;
    }

    // build a small, self-contained buffer with the (real or reused)
    // status byte followed by exactly the payload bytes needed for this
    // event; bounded copies keep parsing linear in the size of the track
    PackedByteArray event_data;

    event_data.append(event_status);

    if (payload_needed > 0) {
      event_data.append_array(raw.data.slice(offset, offset + payload_needed));
    }

    // the event type is the first 4 bits of the byte
    // the channel is the last 4 bits
    int32_t channel = event_status & 0x0F;

    // special case for meta events
    if (event_status == 0xFF) {
      // meta event
      Ref<MIDIEventMeta> event = Ref<MIDIEventMeta>(memnew(MIDIEventMeta(delta_time, event_data)));
      offset += event->get_bytes_used();
      events.push_back(event);

      // EndOfTrack is a track-level marker.
      // There should normally be nothing meaningful after it.
      if (event->event_type == MIDIEventMeta::MIDIEventMetaType::EndOfTrack) {
        return true;
      }

      continue;
    }

    // channel voice/mode messages
    if (event_status < 0xF0) {
      MIDIEventNote::MIDIEventNoteType event_type = static_cast<MIDIEventNote::MIDIEventNoteType>((event_status >> 4) & 0x0F);

      // the event type determines which MIDIEventNote to construct
      switch (event_type) {
        case MIDIEventNote::MIDIEventNoteType::NoteOff: {
          Ref<MIDIEventNote> event = Ref<MIDIEventNote>(memnew(MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::NoteOff)));

          offset += event->get_bytes_used();
          events.push_back(event);
          break;
        }

        case MIDIEventNote::MIDIEventNoteType::NoteOn: {
          Ref<MIDIEventNote> event = Ref<MIDIEventNote>(memnew(MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::NoteOn)));

          offset += event->get_bytes_used();
          events.push_back(event);
          break;
        }

        case MIDIEventNote::MIDIEventNoteType::Aftertouch: {
          Ref<MIDIEventNote> event = Ref<MIDIEventNote>(memnew(MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::Aftertouch)));

          offset += event->get_bytes_used();
          events.push_back(event);
          break;
        }

        case MIDIEventNote::MIDIEventNoteType::Controller: {
          Ref<MIDIEventNote> event = Ref<MIDIEventNote>(memnew(MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::Controller)));

          offset += event->get_bytes_used();
          events.push_back(event);
          break;
        }

        case MIDIEventNote::MIDIEventNoteType::ProgramChange: {
          Ref<MIDIEventNote> event = Ref<MIDIEventNote>(memnew(MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::ProgramChange)));

          offset += event->get_bytes_used();
          events.push_back(event);
          break;
        }

        case MIDIEventNote::MIDIEventNoteType::ChannelPressure: {
          Ref<MIDIEventNote> event = Ref<MIDIEventNote>(memnew(MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::ChannelPressure)));

          offset += event->get_bytes_used();
          events.push_back(event);
          break;
        }

        case MIDIEventNote::MIDIEventNoteType::PitchBend: {
          Ref<MIDIEventNote> event = Ref<MIDIEventNote>(memnew(MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::PitchBend)));

          offset += event->get_bytes_used();
          events.push_back(event);
          break;
        }

        default: {
          // unknown event type
          UtilityFunctions::print(String("Unknown event type: ") + String::num_int64(static_cast<int32_t>(event_type)));

          // print data as hex
          UtilityFunctions::print(event_data.slice(0, 10).hex_encode() + String("..."));

          // print delta time (parsed)
          UtilityFunctions::print(String("Delta time: ") + String::num_int64(delta_time));

          // Make sure an unknown event cannot leave the parser stuck
          // at the same offset forever.
          offset += payload_needed;
          break;
        }
      }

      continue;
    }

    // remaining system common/real-time messages
    {
      MIDIEventSystem::MIDIEventSystemType event_type = static_cast<MIDIEventSystem::MIDIEventSystemType>(event_status);

      switch (event_type) {
        case MIDIEventSystem::MIDIEventSystemType::SystemExclusiveStart:
        case MIDIEventSystem::MIDIEventSystemType::SystemExclusiveEscape:
        case MIDIEventSystem::MIDIEventSystemType::MTCQuarterFrame:
        case MIDIEventSystem::MIDIEventSystemType::SongPositionPointer:
        case MIDIEventSystem::MIDIEventSystemType::SongSelect:
        case MIDIEventSystem::MIDIEventSystemType::TuneRequest:
        case MIDIEventSystem::MIDIEventSystemType::TimingClock:
        case MIDIEventSystem::MIDIEventSystemType::Start:
        case MIDIEventSystem::MIDIEventSystemType::Continue:
        case MIDIEventSystem::MIDIEventSystemType::Stop:
        case MIDIEventSystem::MIDIEventSystemType::ActiveSensing:
        case MIDIEventSystem::MIDIEventSystemType::Reset: {
          // system event
          Ref<MIDIEventSystem> event = Ref<MIDIEventSystem>(memnew(MIDIEventSystem(delta_time, event_data)));
          offset += event->get_bytes_used();
          events.push_back(event);
          break;
        }

        default: {
          // unknown event type
          UtilityFunctions::print(String("Unknown event type: ") + String::num_int64(static_cast<int32_t>(event_type)));

          // print data as hex
          UtilityFunctions::print(event_data.slice(0, 10).hex_encode() + String("..."));

          // print delta time (parsed)
          UtilityFunctions::print(String("Delta time: ") + String::num_int64(delta_time));

          // Make sure an unknown event cannot leave the parser stuck
          // at the same offset forever.
          offset += payload_needed;
          break;
        }
      }
    }
  }

  return true;
}