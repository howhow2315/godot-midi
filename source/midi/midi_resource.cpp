#include "midi_resource.hpp"

Error MIDIResource::load_file(const String &p_path) {
  UtilityFunctions::print(String("[GodotMIDI] Reading midi file data: ") + p_path);

  // get midi file data
  Ref<FileAccess> midi_file = FileAccess::open(p_path, FileAccess::READ);
  if (midi_file == NULL) {
    UtilityFunctions::print(String("[GodotMIDI] Error: Could not open file: ") + p_path);
    return FAILED;
  }

  PackedByteArray midi_data = midi_file->get_buffer(midi_file->get_length());
  // file will be auto-closed when midi_file goes out of scope

  // read header chunk
  MIDIChunk header_chunk;
  midi_data = header_chunk.load_from_bytes(midi_data);

  // parse header chunk
  Ref<MIDIHeader> parsed_header;
  parsed_header.instantiate();

  if (!parsed_header->parse(header_chunk)) {
    UtilityFunctions::print("[GodotMIDI] Error: Could not parse header chunk.");
    return FAILED;
  }

  // store the parsed header directly rather than copying its fields into
  // MIDIResource; MIDIResource owns the complete MIDIHeader resource.
  header = parsed_header;

  // clear any tracks from a previous load
  tracks.clear();

  for (int trk_idx = 0; trk_idx < header->track_count; ++trk_idx) {
    // read track chunk, skipping over any unrecognized chunks that may
    // appear between/before track chunks, as required by the MIDI spec
    MIDIChunk track_chunk;
    midi_data = track_chunk.load_from_bytes(midi_data);

    while (track_chunk.type == MIDIChunk::MIDIChunkType::Unknown) {
      if (midi_data.size() == 0) {
        UtilityFunctions::print(
            "[GodotMIDI] Error: Ran out of data while skipping unknown chunks before track: " +
            String::num_int64(trk_idx));
        return FAILED;
      }

      track_chunk = MIDIChunk();
      midi_data = track_chunk.load_from_bytes(midi_data);
    }

    // parse track chunk
    Ref<MIDITrack> track;
    track.instantiate();

    if (!track->parse(track_chunk)) {
      UtilityFunctions::print(
          "[GodotMIDI] Error: Could not parse track chunk: " +
          String::num_int64(trk_idx));
      return FAILED;
    }

    // add the parsed track directly to the resource
    tracks.push_back(track);
  }

  return OK;
}

Error MIDIResource::save_file(const String &p_path, const Ref<Resource> &p_resource) {
  return OK;
}