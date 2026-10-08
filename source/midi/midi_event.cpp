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
  event_type = (MIDIEventSystemType)data[0]; 

  // number of data bytes following the status byte varies per the MIDI spec:
  //  - system exclusive (0xF0) and the sysex escape/continuation (0xF7) are
  //    followed by a variable length quantity giving the length of the
  //    data that follows
  //  - system common messages have a fixed number of data bytes
  //  - system real-time messages (0xF8-0xFE) have no data bytes at all,
  //    and can legally appear in the middle of another message without
  //    disturbing it
  switch (data[0]) {
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
  // the second and third bytes are the event type and length
  event_type = data.size() > 1 ? (MIDIEventMetaType)data[1]: MIDIEventMetaType::TextEvent;
  event_data_length = data.size() > 2 ? Utility::decode_varint_be(data, 2, bytes_used) : 0;

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
  event_data_length = this->data.size();

  // increment bytes used
  bytes_used = event_data_length + bytes_used + 1;

  // Begin processing the various subtypes of meta events
  // text events
  if (this->event_type == MIDIEventMeta::MIDIEventMetaType::TextEvent ||
      this->event_type == MIDIEventMeta::MIDIEventMetaType::CopyRightNotice ||
      this->event_type == MIDIEventMeta::MIDIEventMetaType::SequenceOrTrackName ||
      this->event_type == MIDIEventMeta::MIDIEventMetaType::InstrumentName ||
      this->event_type == MIDIEventMeta::MIDIEventMetaType::Lyric ||
      this->event_type == MIDIEventMeta::MIDIEventMetaType::Marker ||
      this->event_type == MIDIEventMeta::MIDIEventMetaType::CuePoint ||
      this->event_type == MIDIEventMeta::MIDIEventMetaType::ProgramName ||
      this->event_type == MIDIEventMeta::MIDIEventMetaType::DeviceName ||
      this->event_type == MIDIEventMeta::MIDIEventMetaType::ArtistName) {
    
    // marker
    // variable length
    // first byte is always 0x06
    // second byte is the length of the text
    // the rest of the bytes are the text
    this->meta_data = this->data.slice(0, this->event_data_length).get_string_from_ascii();

    return;
  }

  switch (this->event_type) {
    case MIDIEventMeta::MIDIEventMetaType::SetTempo: {
      // set tempo
      // 3 bytes
      // the bytes are the tempo in microseconds per quarter note

      // set tempo of the track
      this->meta_data = Utility::decode_int24_be(this->data, 0);
      break;
    }
    case MIDIEventMeta::MIDIEventMetaType::TimeSignature: {
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
      time_signature["denominator"] = this->data.size() > 1 ? (int32_t)pow(2, this->data[1]) : 4;
      time_signature["clocks_per_tick"] = this->data.size() > 2 ? this->data[2] : 24;
      time_signature["num_32nd_notes_per_quarter"] = this->data.size() > 3 ? this->data[3] : 8;
      meta_data = time_signature;
      break;
    }
    case MIDIEventMeta::MIDIEventMetaType::KeySignature: {
      // key signature
      // 2 bytes
      // first byte is always 0x02
      // the rest of the bytes are the key signature
      // the first byte is the number of flats (-ve) or sharps (+ve)
      // the second byte is the major (0) or minor (1) key

      // set key signature of the track
      Dictionary key_signature;
      key_signature["sharps_flats"] = this->data.size() > 0 ? this->data[0] : 0;
      key_signature["major_minor"] = this->data.size() > 1 ? this->data[1] : 0;
      meta_data = key_signature;

      break;
    }
    case MIDIEventMeta::MIDIEventMetaType::EndOfTrack: {
      // end of track
      // no data

      // set end of track flag
      this->meta_data = true;
      break;
    }
    case MIDIEventMeta::MIDIEventMetaType::SequenceNumber:
    case MIDIEventMeta::MIDIEventMetaType::TextEvent:
    case MIDIEventMeta::MIDIEventMetaType::CopyRightNotice:
    case MIDIEventMeta::MIDIEventMetaType::SequenceOrTrackName:
    case MIDIEventMeta::MIDIEventMetaType::InstrumentName:
    case MIDIEventMeta::MIDIEventMetaType::Lyric:
    case MIDIEventMeta::MIDIEventMetaType::Marker:
    case MIDIEventMeta::MIDIEventMetaType::CuePoint:
    case MIDIEventMeta::MIDIEventMetaType::ProgramName:
    case MIDIEventMeta::MIDIEventMetaType::DeviceName:
    case MIDIEventMeta::MIDIEventMetaType::ArtistName:
    case MIDIEventMeta::MIDIEventMetaType::SMPTEOffset: {
      // unknown meta event
      break;
    }
  }
}