#include "midi_player.hpp"
#include "../utility.hpp"

MIDIPlayer::MIDIPlayer() {
  this->current_time = 0.0;
  this->event_index = 0;
  this->speed_scale = 1.0;
  this->loop = false;
  this->state.store(PlayerState::Stopped);
  this->note_offset = 0.0;
  this->audio_output_latency =
      AudioServer::get_singleton()->get_output_latency();
  this->has_asp = false;
  this->longest_asp = nullptr;
  this->auto_stop = true;
  this->finish_requested.store(false);

  if (Engine::get_singleton()->is_editor_hint()) {
    set_process_mode(ProcessMode::PROCESS_MODE_DISABLED);
  } else {
    set_process_mode(ProcessMode::PROCESS_MODE_ALWAYS);
  }
}

MIDIPlayer::~MIDIPlayer() {
  this->state.store(PlayerState::Stopped);

  if (this->playback_thread.joinable()) {
    this->playback_thread.join();
  }
}

// Start playback.
// IMPORTANT: This function is only supposed to be called from the Godot/main
// thread.
void MIDIPlayer::play() {
  if (this->midi.is_null()) {
    UtilityFunctions::push_warning("[GodotMIDI] No midi resource set");
    return;
  }

  // If an old playback thread exists, stop it before creating another one.
  if (this->playback_thread.joinable()) {
    this->state.store(PlayerState::Stopped);
    this->playback_thread.join();
  }

  this->current_time = 0.0;
  this->event_index = 0;
  this->finish_requested.store(false);
  this->state.store(PlayerState::Playing);

  // UtilityFunctions::print("[GodotMIDI] Playing");

  // Start audio BEFORE starting the MIDI clock.
  // This avoids the playback thread seeing an AudioStreamPlayer at
  // position 0 before the stream has actually started.
  if (this->has_asp) {
    for (AudioStreamPlayer *asp : this->asps) {
      if (asp == nullptr) {
        continue;
      }

      asp->set_stream_paused(false);
      asp->play();
    }
  }

  this->playback_thread = std::thread(&MIDIPlayer::threaded_playback, this);
}

void MIDIPlayer::stop() {
  if (this->midi.is_null()) {
    return;
  }

  this->state.store(PlayerState::Stopped);

  // Wait for the playback thread to finish.
  if (this->playback_thread.joinable()) {
    this->playback_thread.join();
  }

  this->current_time = 0.0;
  this->event_index = 0;
  this->finish_requested.store(false);

  if (this->has_asp && this->auto_stop) {
    for (AudioStreamPlayer *asp : this->asps) {
      if (asp != nullptr) {
        asp->stop();
      }
    }
  }

  // UtilityFunctions::print("[GodotMIDI] Stopped");
}

void MIDIPlayer::pause() {
  if (this->state.load() != PlayerState::Playing) {
    return;
  }

  this->state.store(PlayerState::Paused);

  if (this->has_asp) {
    for (AudioStreamPlayer *asp : this->asps) {
      if (asp != nullptr) {
        asp->set_stream_paused(true);
      }
    }
  }

  // UtilityFunctions::print("[GodotMIDI] Paused");
}

void MIDIPlayer::resume() {
  if (this->state.load() != PlayerState::Paused) {
    return;
  }

  this->state.store(PlayerState::Playing);

  if (this->has_asp) {
    for (AudioStreamPlayer *asp : this->asps) {
      if (asp != nullptr) {
        asp->set_stream_paused(false);
      }
    }
  }

  // UtilityFunctions::print("[GodotMIDI] Resumed");
}

// Link AudioStreamPlayers.
// NOTICE: We intentionally DO NOT connect AudioStreamPlayer::finished.
// The MIDI timeline controls completion.
void MIDIPlayer::link_audio_stream_player(Array players) {
  this->asps.clear();
  this->has_asp = false;
  this->longest_asp = nullptr;

  double longest_time = 0.0;

  for (int i = 0; i < players.size(); i++) {
    AudioStreamPlayer *asp = Object::cast_to<AudioStreamPlayer>(players[i]);

    if (asp == nullptr) {
      continue;
    }

    Ref<AudioStream> stream = asp->get_stream();

    if (stream.is_null() || stream->get_length() <= 0.0) {
      UtilityFunctions::printerr("[GodotMIDI] Invalid AudioStream at index " +
                                 String::num_int64(i));
      continue;
    }

    const double length = stream->get_length();

    if (length > longest_time) {
      longest_time = length;
      this->longest_asp = asp;
    }

    this->asps.push_back(asp);
  }

  this->has_asp = !this->asps.empty();

  if (!this->has_asp) {
    UtilityFunctions::push_warning(
        "[GodotMIDI] No valid AudioStreamPlayers linked");
  }
}

void MIDIPlayer::_process(float delta) {
  (void)delta;

  if (this->get_tree() == nullptr) {
    return;
  }

  if (this->get_tree()->is_paused()) {
    if (this->state.load() == PlayerState::Playing) {
      this->pause();
    }
  } else {
    if (this->state.load() == PlayerState::Paused) {
      this->resume();
    }
  }
}

// Calculate the duration of one MIDI tick.
double MIDIPlayer::get_microseconds_per_tick(int32_t tempo) const {
  if (this->midi.is_null()) {
    return 0.0;
  }

  Ref<MIDIHeader> header = this->midi->get_header();

  if (header.is_null()) {
    return 0.0;
  }

  if (header->division_type == MIDIHeader::MIDIDivisionType::FramesPerSecond) {
    const double ticks_per_second =
        static_cast<double>(header->frames_per_second) *
        static_cast<double>(header->ticks_per_frame);

    if (ticks_per_second <= 0.0) {
      return 0.0;
    }

    return 1000000.0 / ticks_per_second;
  }

  if (header->division <= 0) {
    return 0.0;
  }

  return static_cast<double>(tempo) / static_cast<double>(header->division);
}

// Build one global MIDI timeline.
// Every event receives an absolute time in seconds.
void MIDIPlayer::build_playback_events() {
  this->playback_events.clear();

  if (this->midi.is_null()) {
    return;
  }

  Ref<MIDIHeader> header = this->midi->get_header();

  if (header.is_null()) {
    return;
  }

  // UtilityFunctions::print("[GodotMIDI] Division type: ",
  //                         static_cast<int>(header->division_type));

  // UtilityFunctions::print("[GodotMIDI] Division: ", header->division);

  // UtilityFunctions::print("[GodotMIDI] Default tempo: ", DEFAULT_MIDI_TEMPO);

  TypedArray<Ref<MIDITrack>> tracks = this->midi->get_tracks();

  struct RawEvent {
    Ref<MIDIEvent> event;
    double tick;
    int32_t track;
    size_t order;
  };

  std::vector<RawEvent> raw_events;
  size_t order = 0;

  for (int32_t track_index = 0; track_index < tracks.size(); track_index++) {
    Ref<MIDITrack> track = tracks[track_index];

    if (track.is_null()) {
      continue;
    }

    double tick = 0.0;

    for (int32_t i = 0; i < track->events.size(); i++) {
      Ref<MIDIEvent> event = track->events[i];

      if (event.is_null()) {
        continue;
      }

      tick += event->delta;

      RawEvent raw;
      raw.event = event;
      raw.tick = tick;
      raw.track = track_index;
      raw.order = order++;

      // if (raw.order < 20) {
      //   UtilityFunctions::print(
      //       "[GodotMIDI] order=", static_cast<int64_t>(raw.order),
      //       " track=", raw.track, " tick=", raw.tick,
      //       " event_delta=", raw.event->delta,
      //       " type=", static_cast<int>(raw.event->get_type()));
      // }

      raw_events.push_back(raw);
    }
  }

  // Stable sorting is important here.
  // Tempo events at the same tick must happen before events whose
  // timing depends on that tempo.
  std::stable_sort(
      raw_events.begin(), raw_events.end(),
      [](const RawEvent &a, const RawEvent &b) {
        if (a.tick != b.tick) {
          return a.tick < b.tick;
        }

        const bool a_tempo =
            a.event->get_type() == MIDIEvent::MIDIEventType::Meta &&
            Object::cast_to<MIDIEventMeta>(a.event.ptr()) != nullptr &&
            Object::cast_to<MIDIEventMeta>(a.event.ptr())->event_type ==
                MIDIEventMeta::MIDIEventMetaType::SetTempo;

        const bool b_tempo =
            b.event->get_type() == MIDIEvent::MIDIEventType::Meta &&
            Object::cast_to<MIDIEventMeta>(b.event.ptr()) != nullptr &&
            Object::cast_to<MIDIEventMeta>(b.event.ptr())->event_type ==
                MIDIEventMeta::MIDIEventMetaType::SetTempo;

        if (a_tempo != b_tempo) {
          return a_tempo;
        }

        return a.order < b.order;
      });

  double time_seconds = 0.0;
  double previous_tick = 0.0;

  int32_t tempo = DEFAULT_MIDI_TEMPO;

  for (const RawEvent &raw : raw_events) {
    const double delta_ticks = raw.tick - previous_tick;

    const double microseconds_per_tick = this->get_microseconds_per_tick(tempo);

    time_seconds += (delta_ticks * microseconds_per_tick) / 1000000.0;

    previous_tick = raw.tick;

    // if (raw.order < 10) {
    //   UtilityFunctions::print("[GodotMIDI] tick=", raw.tick,
    //                           " delta_ticks=", delta_ticks, " tempo=", tempo,
    //                           " us_per_tick=", microseconds_per_tick,
    //                           " time=", time_seconds);
    // }

    // Tempo changes affect all events AFTER this tick.
    if (raw.event->get_type() == MIDIEvent::MIDIEventType::Meta) {
      Ref<MIDIEventMeta> meta = Object::cast_to<MIDIEventMeta>(raw.event.ptr());

      // UtilityFunctions::print("[GodotMIDI] Tempo event: data=", meta->data,
      //                         " meta_data=", meta->meta_data,
      //                         " data_length=", meta->event_data_length);

      if (meta.is_valid() &&
          meta->event_type == MIDIEventMeta::MIDIEventMetaType::SetTempo) {
        tempo = static_cast<int32_t>(meta->meta_data);
      }

      if (meta.is_valid() &&
          meta->event_type == MIDIEventMeta::MIDIEventMetaType::SetTempo) {

        // UtilityFunctions::print("[GodotMIDI] SetTempo: ", meta->data);
        if (meta->data.size() >= 3) {
          tempo = Utility::decode_int24_be(meta->data, 0);
        }
      }
    }

    PlaybackEvent playback_event;
    playback_event.event = raw.event;
    playback_event.time = time_seconds;
    playback_event.track = raw.track;

    this->playback_events.push_back(playback_event);
  }

  // UtilityFunctions::print("[GodotMIDI] Built playback timeline: " +
  //                         String::num_int64(this->playback_events.size()) +
  //                         " events");

  // if (!this->playback_events.empty()) {
  //   UtilityFunctions::print("[GodotMIDI] MIDI duration: " +
  //                           String::num(this->playback_events.back().time) +
  //                           " seconds");
  // }
}

// Convert an internal MIDI event into Godot InputEventMIDI.
Ref<InputEventMIDI>
MIDIPlayer::event_to_input_event(const Ref<MIDIEvent> &event) const {
  if (event.is_null()) {
    return Ref<InputEventMIDI>();
  }

  if (event->get_type() != MIDIEvent::MIDIEventType::Note &&
      event->get_type() != MIDIEvent::MIDIEventType::System) {
    return Ref<InputEventMIDI>();
  }

  Ref<InputEventMIDI> input_event;
  input_event.instantiate();

  if (event->get_type() == MIDIEvent::MIDIEventType::Note) {
    Ref<MIDIEventNote> note = Object::cast_to<MIDIEventNote>(event.ptr());

    if (note.is_null()) {
      return Ref<InputEventMIDI>();
    }

    input_event->set_channel(note->channel);
    input_event->set_message(static_cast<MIDIMessage>(note->event_type));

    switch (note->event_type) {
    case MIDIEventNote::MIDIEventNoteType::NoteOff:
    case MIDIEventNote::MIDIEventNoteType::NoteOn:
      input_event->set_pitch(note->note);
      input_event->set_velocity(note->data);
      break;

    case MIDIEventNote::MIDIEventNoteType::Aftertouch:
      input_event->set_pitch(note->note);
      input_event->set_pressure(note->data);
      break;

    case MIDIEventNote::MIDIEventNoteType::Controller:
      input_event->set_controller_number(note->note);
      input_event->set_controller_value(note->data);
      break;

    case MIDIEventNote::MIDIEventNoteType::ProgramChange:
      input_event->set_instrument(note->note);
      break;

    case MIDIEventNote::MIDIEventNoteType::ChannelPressure:
      input_event->set_pressure(note->note);
      break;

    case MIDIEventNote::MIDIEventNoteType::PitchBend:
      input_event->set_pitch(note->note);
      input_event->set_velocity(note->data);
      break;

    default:
      return Ref<InputEventMIDI>();
    }

    return input_event;
  }

  Ref<MIDIEventSystem> system = Object::cast_to<MIDIEventSystem>(event.ptr());

  if (system.is_null()) {
    return Ref<InputEventMIDI>();
  }

  switch (system->event_type) {
  case MIDIEventSystem::MIDIEventSystemType::MTCQuarterFrame:
    input_event->set_message(MIDI_MESSAGE_QUARTER_FRAME);
    break;

  case MIDIEventSystem::MIDIEventSystemType::SongPositionPointer:
    input_event->set_message(MIDI_MESSAGE_SONG_POSITION_POINTER);
    break;

  case MIDIEventSystem::MIDIEventSystemType::SongSelect:
    input_event->set_message(MIDI_MESSAGE_SONG_SELECT);
    break;

  case MIDIEventSystem::MIDIEventSystemType::TuneRequest:
    input_event->set_message(MIDI_MESSAGE_TUNE_REQUEST);
    break;

  case MIDIEventSystem::MIDIEventSystemType::TimingClock:
    input_event->set_message(MIDI_MESSAGE_TIMING_CLOCK);
    break;

  case MIDIEventSystem::MIDIEventSystemType::Start:
    input_event->set_message(MIDI_MESSAGE_START);
    break;

  case MIDIEventSystem::MIDIEventSystemType::Continue:
    input_event->set_message(MIDI_MESSAGE_CONTINUE);
    break;

  case MIDIEventSystem::MIDIEventSystemType::Stop:
    input_event->set_message(MIDI_MESSAGE_STOP);
    break;

  case MIDIEventSystem::MIDIEventSystemType::ActiveSensing:
    input_event->set_message(MIDI_MESSAGE_ACTIVE_SENSING);
    break;

  case MIDIEventSystem::MIDIEventSystemType::Reset:
    input_event->set_message(MIDI_MESSAGE_SYSTEM_RESET);
    break;

  default:
    return Ref<InputEventMIDI>();
  }

  return input_event;
}

// Process one slice of playback time.
void MIDIPlayer::process_delta(double delta) {
  if (this->midi.is_null()) {
    this->state.store(PlayerState::Stopped);
    return;
  }

  if (delta < 0.0) {
    delta = 0.0;
  }

  // Never let an enormous frame advance the MIDI timeline by an unreasonable
  // amount.
  if (delta > 0.1) {
    delta = 0.1;
  }

  // current_time is already in real playback seconds.
  // note_offset is only for note querying, not for the actual playback clock.
  const double playback_time = this->current_time * this->speed_scale;

  while (this->event_index < this->playback_events.size()) {
    const PlaybackEvent &playback_event =
        this->playback_events[this->event_index];

    if (playback_event.time > playback_time) {
      break;
    }

    Ref<MIDIEvent> event = playback_event.event;

    if (event.is_null()) {
      this->event_index++;
      continue;
    }

    // Meta events.
    if (event->get_type() == MIDIEvent::MIDIEventType::Meta) {
      Ref<MIDIEventMeta> meta = Object::cast_to<MIDIEventMeta>(event.ptr());

      if (meta.is_valid()) {
        call_thread_safe("emit_signal", "meta", meta, playback_event.track);
      }
    }

    // MIDI note/system events.
    else {
      call_thread_safe("emit_signal", "event", event, playback_event.track);
    }

    this->event_index++;
  }

  this->current_time += delta;

  // IMPORTANT: We don't directly loop here.
  // We request a finish and let the main thread decide whether to stop or
  // restart.
  if (this->event_index >= this->playback_events.size() &&
      !this->playback_events.empty()) {
    this->request_finish();
  }
}

// Request completion from the playback thread.
// This function DOES NOT call play(), stop(), or join().
void MIDIPlayer::request_finish() {
  bool expected = false;

  if (!this->finish_requested.compare_exchange_strong(expected, true)) {
    return;
  }

  this->state.store(PlayerState::Stopped);

  // All Node operations happen on the main thread.
  call_thread_safe("_finish_on_main_thread");
}

// Main-thread completion handler.
// This is the ONLY place where looping happens.
void MIDIPlayer::_finish_on_main_thread() {
  // The playback thread has already set itself to Stopped.
  // Join it here, on the main thread.
  if (this->playback_thread.joinable()) {
    this->playback_thread.join();
  }

  call_thread_safe("emit_signal", "finished");

  if (!this->loop) {
    this->current_time = 0.0;
    this->event_index = 0;
    this->finish_requested.store(false);

    if (this->has_asp && this->auto_stop) {
      for (AudioStreamPlayer *asp : this->asps) {
        if (asp != nullptr) {
          asp->stop();
        }
      }
    }

    // UtilityFunctions::print("[GodotMIDI] Finished, stopping");

    return;
  }

  // Loop; play() will create exactly ONE new playback thread.
  // UtilityFunctions::print("[GodotMIDI] Finished, looping");

  this->finish_requested.store(false);
  this->current_time = 0.0;
  this->event_index = 0;
  this->play();
}

// Playback thread; This thread NEVER starts/stops/loops itself.
void MIDIPlayer::threaded_playback() {
  const auto get_now = []() -> int64_t {
    return Time::get_singleton()->get_ticks_usec();
  };

  // UtilityFunctions::print("[GodotMIDI] Playback thread started");

  int64_t previous_time = get_now();

  while (true) {
    PlayerState current_state = this->state.load();

    if (current_state == PlayerState::Stopped) {
      break;
    }

    if (current_state == PlayerState::Paused) {
      previous_time = get_now();

      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      continue;
    }

    const int64_t now = get_now();

    double delta = static_cast<double>(now - previous_time) / 1000000.0;

    previous_time = now;

    if (delta < 0.0) {
      delta = 0.0;
    }

    // When an AudioStreamPlayer is linked, use its position as the master
    // clock. However, only use it while it is actually playing.
    if (this->has_asp && this->longest_asp != nullptr &&
        this->longest_asp->is_playing()) {
      const double audio_time =
          this->longest_asp->get_playback_position() +
          AudioServer::get_singleton()->get_time_since_last_mix() -
          this->audio_output_latency;

      // Audio position can occasionally move backwards because
      // of mixing/latency corrections. Don't move MIDI backwards.
      if (audio_time >= this->current_time) {
        delta = audio_time - this->current_time;
      } else {
        delta = 0.0;
      }
    }

    this->process_delta(delta);

    if (this->state.load() == PlayerState::Stopped) {
      break;
    }

    // Don't busy-spin.
    if (delta < 0.001) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

// Build note query cache.
void MIDIPlayer::build_note_cache() {
  this->note_cache.clear();

  for (const PlaybackEvent &playback_event : this->playback_events) {
    if (playback_event.event.is_null()) {
      continue;
    }

    if (playback_event.event->get_type() != MIDIEvent::MIDIEventType::Note) {
      continue;
    }

    Ref<MIDIEventNote> note =
        Object::cast_to<MIDIEventNote>(playback_event.event.ptr());

    if (note.is_null()) {
      continue;
    }

    if (note->event_type != MIDIEventNote::MIDIEventNoteType::NoteOn &&
        note->event_type != MIDIEventNote::MIDIEventNoteType::NoteOff) {
      continue;
    }

    NoteCacheEntry entry;
    entry.event = note;
    entry.time = playback_event.time;
    entry.active =
        note->event_type == MIDIEventNote::MIDIEventNoteType::NoteOn &&
        note->data > 0;

    this->note_cache.push_back(entry);
  }
}

Array MIDIPlayer::get_notes_in_range(double start_time, double end_time) {
  Array result;

  if (start_time > end_time) {
    std::swap(start_time, end_time);
  }

  const double query_start = start_time - this->note_offset;

  const double query_end = end_time - this->note_offset;

  for (const NoteCacheEntry &entry : this->note_cache) {
    if (entry.time < query_start) {
      continue;
    }

    if (entry.time > query_end) {
      break;
    }

    Ref<InputEventMIDI> input_event = this->event_to_input_event(entry.event);

    if (input_event.is_valid()) {
      result.push_back(input_event);
    }
  }

  return result;
}

Array MIDIPlayer::get_notes_around(double time, double window_before,
                                   double window_after) {
  return this->get_notes_in_range(time - window_before, time + window_after);
}

// Seek.
void MIDIPlayer::set_current_time(double value) {
  if (this->midi.is_null()) {
    this->current_time = 0.0;
    this->event_index = 0;
    return;
  }

  this->current_time = std::max(0.0, value);

  const double playback_time = this->current_time * this->speed_scale;

  this->event_index = 0;

  while (this->event_index < this->playback_events.size() &&
         this->playback_events[this->event_index].time <= playback_time) {
    this->event_index++;
  }

  // If currently playing, also seek linked audio.
  if (this->has_asp) {
    for (AudioStreamPlayer *asp : this->asps) {
      if (asp != nullptr) {
        asp->seek(this->current_time);
      }
    }
  }
}

// Replace the MIDI resource.
void MIDIPlayer::set_midi(const Ref<MIDIResource> &value) {
  if (this->state.load() != PlayerState::Stopped) {
    this->state.store(PlayerState::Stopped);
  }

  if (this->playback_thread.joinable()) {
    this->playback_thread.join();
  }

  if (this->has_asp) {
    for (AudioStreamPlayer *asp : this->asps) {
      if (asp != nullptr) {
        asp->stop();
      }
    }
  }

  this->midi = value;
  this->current_time = 0.0;
  this->event_index = 0;
  this->playback_events.clear();
  this->note_cache.clear();
  this->finish_requested.store(false);

  if (this->midi.is_null()) {
    return;
  }

  this->build_playback_events();
  this->build_note_cache();
}