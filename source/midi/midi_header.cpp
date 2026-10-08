#include "midi_header.hpp"
#include "../utility.hpp"

/// @brief default constructor for header
MIDIHeader::MIDIHeader() {
  file_format = MIDIFileFormat::SingleTrack;
  track_count = 0;
  division_type = MIDIDivisionType::TicksPerQuarterNote;
  division = 48;
  frames_per_second = 0;
  ticks_per_frame = 0;
}

/// @brief parses a chunk of raw bytes into a header chunk
/// @param chunk the raw chunk of bytes
/// @param header the header chunk to populate
/// @return true if the chunk was parsed successfully, false otherwise
bool MIDIHeader::parse(MIDIChunk chunk) {
  if (chunk.type != MIDIChunk::MIDIChunkType::Header) {
    return false;
  }

  // an MThd chunk must contain at least 6 bytes (format, ntrks, division);
  // anything shorter is malformed and can't be safely parsed
  if (chunk.data.size() < 6) {
    UtilityFunctions::printerr("[GodotMIDI] Error: MThd chunk is too short to contain a valid header");
    return false;
  }

  // data section of a header contains 3 16-bit words
  // first word is format
  // 0 - single track, 1 - multiple tracks, 2 - multiple songs
  file_format = (MIDIFileFormat)Utility::decode_int16_be(chunk.data, 0);

  // second word is number of tracks
  track_count = Utility::decode_int16_be(chunk.data, 2);

  // third word is time division
  // ticks per quarter note or (negative SMPTE format + ticks per frame)
  // if bit 15 is 0, then it's ticks per quarter note
  // if bit 15 is 1, then it's SMPTE format

  // we use bit 15 to determine the division type
  // the bytes are in big endian, so we have to shift the byte to the right
  // and then mask it with 0x01 to get the bit
  division_type = (MIDIDivisionType)((chunk.data[4] >> 7) & 0x01);

  // if it's ticks per quarter note, then we just read the value
  if (division_type == MIDIDivisionType::TicksPerQuarterNote) {
    division = static_cast<int32_t>(Utility::decode_int16_be(chunk.data, 4));

  } else {
    // SMPTE format: byte 4 is a negative signed value giving the frame
    // rate (-24, -25, -29 for 29.97 drop-frame, or -30), byte 5 is the
    // resolution in ticks per frame
    int8_t smpte_format = static_cast<int8_t>(chunk.data[4]);
    frames_per_second = -static_cast<int32_t>(smpte_format);
    ticks_per_frame = static_cast<int32_t>(chunk.data[5]);

    // tempo-based (ticks per quarter note) math doesn't apply in SMPTE
    // mode since tick duration is fixed by the frame rate, not tempo;
    // consumers should check division_type before using `division`
    division = frames_per_second * ticks_per_frame;
  }

  return true;
}