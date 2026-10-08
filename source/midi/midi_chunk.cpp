#include "midi_chunk.hpp"
#include "../utility.hpp"

/// @brief Loads a MIDI chunk from a stream of bytes
/// @param bytes the input stream of bytes
/// @return the original byte stream minus the read data
PackedByteArray MIDIChunk::load_from_bytes(PackedByteArray bytes) {
  int bytes_size = bytes.size();

  // guard against truncated/malformed files that don't even have enough
  // data left for a chunk header (8 bytes: 4 byte id + 4 byte size)
  if (bytes_size < 8) {
    if (bytes_size > 0) {
      UtilityFunctions::printerr("[GodotMIDI] Warning: not enough data remaining for a chunk header, ignoring trailing bytes");
    }
    id = "";
    size = 0;
    data = PackedByteArray();
    type = MIDIChunkType::Unknown;
    return PackedByteArray();
  }

  id = bytes.slice(0, 4).get_string_from_ascii();
  // we have to use this custom function because chunk size is stored in big endian
  size = Utility::decode_int32_be(bytes, 4);

  // clamp the declared chunk size to the data actually available, rather
  // than reading (or requesting a slice) past the end of the buffer if the
  // file is truncated or the size field is corrupt
  uint32_t available = static_cast<uint32_t>(bytes_size - 8);
  if (size > available) {
    UtilityFunctions::printerr(
        "[GodotMIDI] Warning: chunk '" + id + "' declares size " +
        String::num_int64(size) + " but only " +
        String::num_int64(available) + " bytes remain, truncating");
    size = available;
  }

  data = bytes.slice(8, 8 + size);
  // remove the chunk from the input stream
  PackedByteArray new_bytes = bytes.slice(8 + size);

  if (id == "MThd") {
    type = MIDIChunkType::Header;
  } else if (id == "MTrk") {
    type = MIDIChunkType::Track;
  } else {
    // unknown header type
    // this will be ignored per the midi specification
    type = MIDIChunkType::Unknown;
  }

  return new_bytes;
}