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
  MIDIParser::RawMIDIChunk header_chunk;
  midi_data = header_chunk.load_from_bytes(midi_data);

  // parse header chunk
  MIDIParser::MIDIHeader header;
  if (!header.parse(header_chunk)) {
    UtilityFunctions::print("[GodotMIDI] Error: Could not parse header chunk.");
    return FAILED;
  }

  header.only_notes = false;

  // load header into resource
  this->format = header.file_format;
  this->track_count = header.num_tracks;
  this->division = header.division;
  this->division_type = header.division_type;
  this->smpte_fps = header.frames_per_second;
  this->smpte_ticks_per_frame = header.ticks_per_frame;
  this->tempo = header.tempo;

  for (int trk_idx = 0; trk_idx < track_count; ++trk_idx) {
    // read track chunk, skipping over any unrecognized chunks that may
    // appear between/before track chunks, as required by the MIDI spec
    MIDIParser::RawMIDIChunk trackChunk;
    midi_data = trackChunk.load_from_bytes(midi_data);

    while (trackChunk.chunk_type == MIDIParser::MIDIChunkType::Unknown) {
      if (midi_data.size() == 0) {
        UtilityFunctions::print("[GodotMIDI] Error: Ran out of data while skipping unknown chunks before track: " + String::num_int64(trk_idx));
        return FAILED;
      }

      trackChunk = MIDIParser::RawMIDIChunk();
      midi_data = trackChunk.load_from_bytes(midi_data);
    }

    // parse track chunk
    MIDIParser::MIDITrack track;
    if (!track.parse(trackChunk, header)) {
      UtilityFunctions::print("[GodotMIDI] Error: Could not parse track chunk: " + String::num_int64(trk_idx));
      return FAILED;
    }

    // add track
    Dictionary track_dict;
    track_dict["name"] = String("Track ") + String::num_int64(trk_idx);
    track_dict["events"] = Array();

    this->tracks.push_back(track_dict);

    // loop through events
    for (int i = 0; i < track.events.size(); i++) {
      // get event pointer
      std::unique_ptr<MIDIEvent> p_event = std::move(track.events[i]);

      double delta = 0.0;

      // note events
      if (p_event->get_type() == MIDIEvent::EventType::Note) {
        MIDIEventNote note_event = *dynamic_cast<MIDIEventNote *>(p_event.get());
        delta = (double)note_event.delta;

        // load note event into current track
        Dictionary event_dict;
        event_dict["type"] = "note";
        event_dict["track"] = trk_idx;
        event_dict["subtype"] = note_event.event_type;
        event_dict["delta"] = delta;
        event_dict["note"] = note_event.note;
        event_dict["data"] = note_event.data;
        event_dict["channel"] = note_event.channel;

        // add event to track
        Array event_array = this->tracks[trk_idx].get("events");
        event_array.push_back(event_dict);
      
      // system events
      } else if (p_event->get_type() == MIDIEvent::EventType::System) {
        MIDIEventSystem system_event = *dynamic_cast<MIDIEventSystem *>(p_event.get());
        delta = (double)system_event.delta;

        // load system event into current track
        Dictionary event_dict;
        event_dict["type"] = "system";
        event_dict["track"] = trk_idx;
        event_dict["subtype"] = system_event.event_type;
        event_dict["delta"] = delta;
        event_dict["channel"] = system_event.channel;

        // add event to track
        Array event_array = this->tracks[trk_idx].get("events");
        event_array.push_back(event_dict);


      // meta events
      } else if (p_event->get_type() == MIDIEvent::EventType::Meta) {
        MIDIEventMeta meta_event = *dynamic_cast<MIDIEventMeta *>(p_event.get());
        delta = (double)meta_event.delta;

        // load meta event into current track
        Dictionary event_dict;
        event_dict["type"] = "meta";
        event_dict["track"] = trk_idx;
        // cast to int
        event_dict["subtype"] = static_cast<int64_t>(meta_event.event_type);
        event_dict["delta"] = delta;
        // since raw data is almost never useful for meta events, we store it as
        // a variant and put it in the data field
        event_dict["data"] = static_cast<Variant>(meta_event.meta_data);
        event_dict["channel"] = meta_event.channel;

        // add event to track
        Array event_array = this->tracks[trk_idx].get("events");
        event_array.push_back(event_dict);

        // if we have a track name event, update the track name
        if (meta_event.event_type == MIDIEventMeta::MIDIEventMetaType::SequenceOrTrackName) {
          this->tracks[trk_idx].set("name", meta_event.meta_data);
        }
      }

    }
  }

  return OK;
}

Error MIDIResource::save_file(const String &p_path, const Ref<Resource> &p_resource) {
  return OK;
}

Ref<InputEventMIDI> MIDIResource::event_to_input_event(const Dictionary &event) {
  Ref<InputEventMIDI> midi_event; midi_event.instantiate();

  const String type = event.get("type", "");

  // InputEventMIDI uses the message: "MIDIMessage" property which doesn't include any meta events 
  // as well as reading all SystemExclusive messages as exclusively "MIDI_MESSAGE_SYSTEM_EXCLUSIVE"
  if (type == "meta") {
    return midi_event;
  }

  midi_event->set_message(static_cast<MIDIMessage>(static_cast<int64_t>(event.get("subtype", 0))));

  midi_event->set_channel(static_cast<int32_t>(event.get("channel", 0)));

  if (type == "note") {
    const int32_t note = static_cast<int32_t>(event.get("note", 0));
    const int32_t data = static_cast<int32_t>(event.get("data", 0));

    switch (midi_event->get_message()) {
    case MIDI_MESSAGE_NOTE_ON:
    case MIDI_MESSAGE_NOTE_OFF:
      midi_event->set_pitch(note);
      midi_event->set_velocity(data);
      break;

    case MIDI_MESSAGE_AFTERTOUCH:
      midi_event->set_pitch(note);
      midi_event->set_pressure(data);
      break;

    case MIDI_MESSAGE_CONTROL_CHANGE:
      midi_event->set_controller_number(note);
      midi_event->set_controller_value(data);
      break;

    case MIDI_MESSAGE_PROGRAM_CHANGE:
      midi_event->set_instrument(note);
      break;

    case MIDI_MESSAGE_CHANNEL_PRESSURE:
      midi_event->set_pressure(note);
      break;

    case MIDI_MESSAGE_PITCH_BEND:
      midi_event->set_controller_value(note | (data << 7));
      break;

    default:
      break;
    }
  }

  return midi_event;
}
