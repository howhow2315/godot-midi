#include "midi_parser.hpp"
#include "../utility.hpp"

/// @brief Loads a MIDI chunk from a stream of bytes
/// @param bytes the input stream of bytes
/// @return the original byte stream minus the read data
PackedByteArray
MIDIParser::RawMIDIChunk::load_from_bytes(PackedByteArray bytes) {
  int bytes_size = bytes.size();

  // guard against truncated/malformed files that don't even have enough
  // data left for a chunk header (8 bytes: 4 byte id + 4 byte size)
  if (bytes_size < 8) {
    if (bytes_size > 0) {
      UtilityFunctions::printerr("[GodotMIDI] Warning: not enough data remaining for a chunk header, ignoring trailing bytes");
    }
    chunk_id = "";
    chunk_size = 0;
    chunk_data = PackedByteArray();
    chunk_type = MIDIChunkType::Unknown;
    return PackedByteArray();
  }

  chunk_id = bytes.slice(0, 4).get_string_from_ascii();
  // we have to use this custom function because chunk size is stored in big endian
  chunk_size = Utility::decode_int32_be(bytes, 4);

  // clamp the declared chunk size to the data actually available, rather
  // than reading (or requesting a slice) past the end of the buffer if the
  // file is truncated or the size field is corrupt
  uint32_t available = static_cast<uint32_t>(bytes_size - 8);
  if (chunk_size > available) {
    UtilityFunctions::printerr(
        "[GodotMIDI] Warning: chunk '" + chunk_id + "' declares size " +
        String::num_int64(chunk_size) + " but only " +
        String::num_int64(available) + " bytes remain, truncating");
    chunk_size = available;
  }

  chunk_data = bytes.slice(8, 8 + chunk_size);
  // remove the chunk from the input stream
  PackedByteArray new_bytes = bytes.slice(8 + chunk_size);

  if (chunk_id == "MThd") {
    chunk_type = MIDIChunkType::Header;
  } else if (chunk_id == "MTrk") {
    chunk_type = MIDIChunkType::Track;
  } else {
    // unknown header type
    // this will be ignored per the midi specification
    chunk_type = MIDIChunkType::Unknown;
  }

  return new_bytes;
}