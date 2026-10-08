#pragma once

// We don't need windows.h in this plugin but many others do and it throws up on
// itself all the time So best to include it and make sure CI warns us when we
// use something Microsoft took for their own goals....
#ifdef WIN32
  #include <windows.h>
#endif

#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

/// @brief Represents a raw (binary) MIDI file chunk before it is interpreted.
class MIDIChunk {
  public:
    /// @brief Type of MIDI chunk found in a Standard MIDI File. [MTrk, MThd, and unknown]
    enum MIDIChunkType {
      Header,
      Track,
      Unknown
    };

    String id;
    uint32_t size = 0;
    MIDIChunkType type = MIDIChunkType::Unknown;

    PackedByteArray data;

    /// @brief Creates an empty MIDI chunk with an unknown type.
    MIDIChunk() = default;

    /// @brief Loads a MIDI chunk from raw bytes.
    /// @param bytes the bytes containing the MIDI chunk
    /// @return the remaining bytes after this chunk
    PackedByteArray load_from_bytes(PackedByteArray bytes);
};