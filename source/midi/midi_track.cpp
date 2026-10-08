#include "../utility.hpp"
#include "midi_track.hpp"
#include <memory>

using namespace godot;

/// @brief The main chunk parser, takes bytes from the input stream and parses them into MIDI chunks
/// @param raw the raw chunk of bytes
// /// @param header the header chunk (will be modified for tempo changes, etc.)
/// @return
bool MIDITrack::parse(MIDIParser::RawMIDIChunk raw) {
  if (raw.chunk_type != MIDIParser::MIDIChunkType::Track) {
    return false;
  }

  // read events until we reach the end of the chunk
  // we have to use a while loop because we don't know how many events there are
  // we can only know when we reach the end of the chunk
  int32_t offset = 0;

  // per the MIDI spec, consecutive channel voice/mode messages may omit
  // a repeated status byte and reuse the previous one ("running status")
  uint8_t running_status = 0;
  while (offset < raw.chunk_size) {
    
    // if (header.end_of_track) { break; }
    
    int32_t bytes_used = 0;

    // first variable length quantity is delta time
    int32_t delta_time = Utility::decode_varint_be(raw.chunk_data, offset, bytes_used);
    offset += bytes_used;

    if (offset >= raw.chunk_size) {
      // truncated track: nothing left to read after the delta time
      UtilityFunctions::printerr("[GodotMIDI] Warning: track data ended unexpectedly after a delta time, stopping");
      break;
    }

    int32_t event_type;
    bool explicit_status;

    uint8_t next_byte = raw.chunk_data[offset];
    if (next_byte & 0x80) {
      // explicit status byte present
      explicit_status = true;
      event_type = next_byte;
      offset += 1;

      if (event_type < 0xF0) {
        // channel voice/mode message, updates running status
        running_status = static_cast<uint8_t>(event_type);
      } else if (event_type <= 0xF7) {
        // system common messages cancel running status
        running_status = 0;
      }
      // system realtime messages (0xF8-0xFE) and meta events (0xFF)
      // don't affect running status

    } else {
      // running status: this byte is actually the first data byte of
      // a channel voice/mode message, reusing the last status byte
      if (running_status == 0) {
        UtilityFunctions::printerr("[GodotMIDI] Malformed MIDI track: data byte encountered with no active running status");
        break;
      }

      explicit_status = false;
      event_type = running_status;
    }

    // offset points at the first payload byte in both cases
    // Work out exactly how many payload bytes this event needs *without*
    // copying anything yet, so that we only ever copy the bytes a given
    // event actually needs; slicing to the end of the chunk for every
    // single event would make parsing large tracks quadratic.
    int32_t payload_needed = 0;

    if (event_type == 0xFF) {
      // meta event: <type byte> <length varint> <data...>
      int32_t length_bytes = 0;
      int64_t data_len = Utility::decode_varint_be(raw.chunk_data, offset + 1, length_bytes);
      payload_needed = 1 + length_bytes + static_cast<int32_t>(data_len);

    } else if (event_type == 0xF0 || event_type == 0xF7) {
      // system exclusive / escape: <length varint> <data...>
      int32_t length_bytes = 0;
      int64_t data_len = Utility::decode_varint_be(raw.chunk_data, offset, length_bytes);
      payload_needed = length_bytes + static_cast<int32_t>(data_len);

    } else if (event_type < 0xF0) {
      // channel voice/mode message
      int32_t code = (event_type >> 4) & 0x0F;
      payload_needed = (code == 0x0C || code == 0x0D) ? 1 : 2;

    } else {
      // remaining system common/real-time messages
      switch (event_type) {
        case 0xF1:
        case 0xF3:
          payload_needed = 1;
          break;
        case 0xF2:
          payload_needed = 2;
          break;
        default: // F6, F8, FA, FB, FC, FE
          payload_needed = 0;
          break;
      }
    }

    // clamp to what's actually available in case of a truncated file
    int32_t remaining_in_chunk = static_cast<int32_t>(raw.chunk_size) - offset;
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
    event_data.append(static_cast<uint8_t>(event_type));
    if (payload_needed > 0) {
      event_data.append_array(raw.chunk_data.slice(offset, offset + payload_needed));
    }

    // the event type is the first 4 bits of the byte
    // the channel is the last 4 bits
    int32_t event_code = event_type >> 4;
    int32_t channel = event_type & 0x0F;

    // special case for meta events
    if (event_type == 0xFF) {
      event_code = 0xFF;
    }

    // the event code determines the type of the event
    std::unique_ptr<MIDIEvent> ptr;
    switch (event_code) {
      case 0x08: // note off
      {
        MIDIEventNote note_off_event = MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::NoteOff);
        offset += note_off_event.get_bytes_used();
        note_events.push_back(note_off_event);
        ptr = std::make_unique<MIDIEventNote>(note_off_event);
        events.push_back(std::move(ptr));
        break;
      }
      case 0x09: // note on
      {
        MIDIEventNote note_on_event = MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::NoteOn);
        offset += note_on_event.get_bytes_used();
        note_events.push_back(note_on_event);
        ptr = std::make_unique<MIDIEventNote>(note_on_event);
        events.push_back(std::move(ptr));
        break;
      }
      case 0x0A: // note aftertouch
      {
        MIDIEventNote note_aftertouch_event = MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::Aftertouch);
        offset += note_aftertouch_event.get_bytes_used();
        note_events.push_back(note_aftertouch_event);
        ptr = std::make_unique<MIDIEventNote>(note_aftertouch_event);
        events.push_back(std::move(ptr));
        break;
      }
      case 0x0B: // controller
      {
        MIDIEventNote note_controller_event = MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::Controller);
        offset += note_controller_event.get_bytes_used();
        note_events.push_back(note_controller_event);
        ptr = std::make_unique<MIDIEventNote>(note_controller_event);
        events.push_back(std::move(ptr));
        break;
      }
      case 0x0C: // program change
      {
        MIDIEventNote note_program_change = MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::ProgramChange);
        offset += note_program_change.get_bytes_used();
        note_events.push_back(note_program_change);
        ptr = std::make_unique<MIDIEventNote>(note_program_change);
        events.push_back(std::move(ptr));
        break;
      }
      case 0x0D: // channel pressure
      {
        MIDIEventNote note_channel_pressure = MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::ChannelPressure);
        offset += note_channel_pressure.get_bytes_used();
        note_events.push_back(note_channel_pressure);
        ptr = std::make_unique<MIDIEventNote>(note_channel_pressure);
        events.push_back(std::move(ptr));
        break;
      }
      case 0x0E: // pitch bend
      {
        MIDIEventNote note_pitch_blend = MIDIEventNote(channel, delta_time, event_data, MIDIEventNote::MIDIEventNoteType::PitchBend);
        offset += note_pitch_blend.get_bytes_used();
        note_events.push_back(note_pitch_blend);
        ptr = std::make_unique<MIDIEventNote>(note_pitch_blend);
        events.push_back(std::move(ptr));
        break;
      }
      case 0x0F: // system event
      {
        MIDIEventSystem system_event = MIDIEventSystem(delta_time, event_data);
        offset += system_event.get_bytes_used();
        system_events.push_back(system_event);
        ptr = std::make_unique<MIDIEventSystem>(system_event);
        events.push_back(std::move(ptr));
        break;
      }
      case 0xFF: // meta event
      {
        MIDIEventMeta meta_event = MIDIEventMeta(delta_time, event_data);
        offset += meta_event.get_bytes_used();
        meta_events.push_back(meta_event);

        // if (meta_event.event_type == MIDIEventMeta::MIDIEventMetaType::EndOfTrack) { header.end_of_track = true; }

        ptr = std::make_unique<MIDIEventMeta>(meta_event);
        events.push_back(std::move(ptr));
        break;
      }
      default: {
        // unknown event type
        UtilityFunctions::print(String("Unknown event type: ") + String::num_int64(event_code));
        // print data as hex
        UtilityFunctions::print(event_data.slice(0, 10).hex_encode() + String("..."));
        //  print delta time (parsed)
        UtilityFunctions::print(String("Delta time: ") + String::num_int64(delta_time));
      }
    }
  }

  return true;
}
