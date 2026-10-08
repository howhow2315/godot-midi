#include "midi_event.hpp"
#include "../utility.hpp"

/// @brief constructor
/// @param channel
/// @param delta
MIDIEvent::MIDIEvent(int32_t channel, double delta) {
  this->channel = channel;
  this->delta = delta;
}

/// @brief copy constructor
/// @param other
MIDIEvent::MIDIEvent(const MIDIEvent &other) {
  this->channel = other.channel;
  this->delta = other.delta;
  this->bytes_used = other.bytes_used;
}

/// @brief gets the number of bytes used by the event
/// @return
int32_t MIDIEvent::get_bytes_used() const {
  return this->bytes_used;
}

/// @brief prints the contents of the event to a nice string
/// @return
String MIDIEvent::to_string() const {
  return String("MIDIEvent: channel=") + String::num_int64(channel) + String(", delta=") + String::num_int64(delta);
}

/// @brief Constructor for MIDI note events
/// @param channel the MIDI channel
/// @param delta the delta time in microseconds
/// @param data the data for the event
/// @param event_type the type of the event
MIDIEventNote::MIDIEventNote(int32_t channel, double delta, PackedByteArray data, MIDIEventNoteType event_type): MIDIEvent(channel, delta) {
  this->event_type = event_type;
  this->channel = channel;
  this->note = 0;
  this->data = 0;

  // data[0] is the status byte; guard against truncated payloads where
  // fewer bytes were actually available than this event type expects
  if (event_type == MIDIEventNoteType::NoteOn || event_type == MIDIEventNoteType::NoteOff ||
      event_type == MIDIEventNoteType::Aftertouch || event_type == MIDIEventNoteType::Controller || event_type == MIDIEventNoteType::PitchBend) {

    if (data.size() > 1) {
      this->note = data[1];
    }
    if (data.size() > 2) {
      this->data = data[2];
    }
    this->bytes_used = 2;

  } else if (event_type == MIDIEventNoteType::ProgramChange || event_type == MIDIEventNoteType::ChannelPressure) {
    if (data.size() > 1) {
      this->note = data[1];
    }
    bytes_used = 1;
    
  } else {
    // unknown/invalid event type
    // let's assume it's some weird controller event and set the bytes used to 2
    if (data.size() > 1) {
      this->note = data[1];
    }
    if (data.size() > 2) {
      this->data = data[2];
    }
    this->bytes_used = 2;
  }
}

/// @brief Constructor for MIDI system events
/// @param delta
/// @param data
MIDIEventSystem::MIDIEventSystem(double delta, PackedByteArray data): MIDIEvent(0, delta) {
  // FIXME: we should be careful about truncated data here as well for consistancy
  // specifically we're not checking that data actually contains anything
  event_type = static_cast<MIDIEventSystemType>(data[0]);
  
  // number of data bytes following the status byte varies per the MIDI spec:
  //  - system exclusive (0xF0) and the sysex escape/continuation (0xF7) are
  //    followed by a variable length quantity giving the length of the
  //    data that follows
  //  - system common messages have a fixed number of data bytes
  //  - system real-time messages (0xF8-0xFE) have no data bytes at all,
  //    and can legally appear in the middle of another message without
  //    disturbing it
  switch (event_type) {
    case MIDIEventSystemType::SystemExclusiveStart:
    case MIDIEventSystemType::SystemExclusiveEscape: {
      int32_t length_bytes = 0;
      int64_t sysex_length = Utility::decode_varint_be(data, 1, length_bytes);
      bytes_used = length_bytes + static_cast<int32_t>(sysex_length);
      break;
    }
    case MIDIEventSystemType::MTCQuarterFrame:
    case MIDIEventSystemType::SongSelect:
      bytes_used = 1;
      break;
    case MIDIEventSystemType::SongPositionPointer:
      bytes_used = 2;
      break;
    case MIDIEventSystemType::TuneRequest:
    case MIDIEventSystemType::TimingClock:
    case MIDIEventSystemType::Start:
    case MIDIEventSystemType::Continue:
    case MIDIEventSystemType::Stop:
    case MIDIEventSystemType::ActiveSensing:
    default:
      // real-time messages (and Reset/0xFF, which never reaches here since
      // it's intercepted as a meta event when parsing SMF tracks) have no
      // data bytes
      bytes_used = 0;
      break;
  }
}

/// @brief Constructor for MIDI meta events
/// @param delta
/// @param data
MIDIEventMeta::MIDIEventMeta(double delta, PackedByteArray data): MIDIEvent(0, delta) {
  // the first byte is always 0xFF
  // the second byte is the event type
  // the following bytes contain the variable-length data size
  event_type = data.size() > 1 ? static_cast<MIDIEventMetaType>(data[1]) : MIDIEventMetaType::TextEvent;

  // decode the length of the meta event data
  event_data_length =
      data.size() > 2
          ? Utility::decode_varint_be(data, 2, bytes_used)
          : 0;

  // if we have a non-zero length, then we have data; clamp to what's
  // actually available in case the source data was truncated
  int32_t data_start = bytes_used + 2;

  if (event_data_length > 0 && data_start < data.size()) {
    int32_t data_end = data_start + event_data_length;

    if (data_end > data.size()) {
      data_end = data.size();
    }

    this->data = data.slice(data_start, data_end);
  }

  // store the amount of data that was actually available
  event_data_length = this->data.size();

  // increment bytes used
  bytes_used = event_data_length + bytes_used + 1;

  // Text-based meta events are stored as strings.
  if (this->event_type == MIDIEventMetaType::TextEvent ||
      this->event_type == MIDIEventMetaType::CopyRightNotice ||
      this->event_type == MIDIEventMetaType::SequenceOrTrackName ||
      this->event_type == MIDIEventMetaType::InstrumentName ||
      this->event_type == MIDIEventMetaType::Lyric ||
      this->event_type == MIDIEventMetaType::Marker ||
      this->event_type == MIDIEventMetaType::CuePoint ||
      this->event_type == MIDIEventMetaType::ProgramName ||
      this->event_type == MIDIEventMetaType::DeviceName ||
      this->event_type == MIDIEventMetaType::ArtistName) {
   
    // marker
    // variable length
    // first byte is always 0x06
    // second byte is the length of the text
    // the rest of the bytes are the text
    this->meta_data = this->data.slice(0, this->event_data_length).get_string_from_ascii();

    return;
  }

  // Decode the remaining structured meta events into convenient Variants.
  // These values describe the event itself; playback state such as tempo
  // should be handled by MIDIPlayer rather than by the event object.
  switch (this->event_type) {
    case MIDIEventMetaType::SetTempo: {
      // set tempo
      // 3 bytes
      // the value is the tempo in microseconds per quarter note
      //
      // The MIDIPlayer is responsible for applying this value to playback
      // timing. MIDIEventMeta only stores the decoded value.
      if (this->data.size() >= 3) {
        meta_data = Utility::decode_int24_be(this->data, 0);
      } else {
        meta_data = 0;
      }

      break;
    }

    case MIDIEventMetaType::TimeSignature: {
      // time signature
      // 4 bytes
      // first byte is always 0x04
      // the rest of the bytes are the time signature
      // the first byte is the numerator
      // the second byte is the denominator (2^x)
      // the third byte is the clocks per metronome click
      // the fourth byte is the number of 32nd notes per quarter note

      // set time signature of the track
      Dictionary time_signature;

      time_signature["numerator"] = this->data.size() > 0 ? this->data[0] : 4;
      time_signature["denominator"] = this->data.size() > 1 ? static_cast<int32_t>(1 << this->data[1]) : 4;
      time_signature["clocks_per_tick"] = this->data.size() > 2 ? this->data[2] : 24;
      time_signature["num_32nd_notes_per_quarter"] = this->data.size() > 3 ? this->data[3] : 8;

      meta_data = time_signature;
      break;
    }

    case MIDIEventMetaType::KeySignature: {
      // key signature
      // 2 bytes
      // first byte is always 0x02
      // the rest of the bytes are the key signature
      // the first byte is the number of flats (-ve) or sharps (+ve)
      // the second byte is the major (0) or minor (1) key

      // set key signature of the track
      Dictionary key_signature;
      key_signature["sharps_flats"] = this->data.size() > 0 ? static_cast<int8_t>(this->data[0]) : 0;
      key_signature["major_minor"] = this->data.size() > 1 ? this->data[1] : 0;

      meta_data = key_signature;
      break;
    }

    case MIDIEventMetaType::EndOfTrack: {
      // end of track
      // no data
      this->meta_data = true;
      break;
    }

    case MIDIEventMetaType::SequenceNumber:
    case MIDIEventMetaType::TextEvent:
    case MIDIEventMetaType::CopyRightNotice:
    case MIDIEventMetaType::SequenceOrTrackName:
    case MIDIEventMetaType::InstrumentName:
    case MIDIEventMetaType::Lyric:
    case MIDIEventMetaType::Marker:
    case MIDIEventMetaType::CuePoint:
    case MIDIEventMetaType::ProgramName:
    case MIDIEventMetaType::DeviceName:
    case MIDIEventMetaType::ArtistName:
    case MIDIEventMetaType::SMPTEOffset:
    default: {
      // unknown or currently unhandled meta event
      //
      // The raw data is still available through `data`, so unsupported
      // meta events are not discarded.
      break;
    }
  }
}