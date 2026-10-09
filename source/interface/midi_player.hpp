#pragma once

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_midi.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "../midi/midi_event.hpp"
#include "../midi/midi_resource.hpp"
#include "../midi/midi_track.hpp"

using namespace godot;

#define DEFAULT_MIDI_TEMPO 500000

enum PlayerState {
    Playing,
    Paused,
    Stopped
};

class MIDIPlayer : public Node {
    GDCLASS(MIDIPlayer, Node);

protected:
    static void _bind_methods() {
        ClassDB::bind_method(
            D_METHOD("set_midi", "midi"),
            &MIDIPlayer::set_midi);

        ClassDB::bind_method(
            D_METHOD("get_midi"),
            &MIDIPlayer::get_midi);

        ADD_PROPERTY(
            PropertyInfo(
                Variant::OBJECT,
                "midi",
                PROPERTY_HINT_RESOURCE_TYPE,
                "MIDIResource"),
            "set_midi",
            "get_midi");

        ClassDB::bind_method(
            D_METHOD("play"),
            &MIDIPlayer::play);

        ClassDB::bind_method(
            D_METHOD("stop"),
            &MIDIPlayer::stop);

        ClassDB::bind_method(
            D_METHOD("pause"),
            &MIDIPlayer::pause);

        ClassDB::bind_method(
            D_METHOD("resume"),
            &MIDIPlayer::resume);

        ClassDB::bind_method(
            D_METHOD("get_state"),
            &MIDIPlayer::get_state);

        ClassDB::bind_method(
            D_METHOD("get_current_time"),
            &MIDIPlayer::get_current_time);

        ClassDB::bind_method(
            D_METHOD("set_current_time", "current_time"),
            &MIDIPlayer::set_current_time);

        ADD_PROPERTY(
            PropertyInfo(Variant::FLOAT, "current_time"),
            "set_current_time",
            "get_current_time");

        ClassDB::bind_method(
            D_METHOD("get_loop"),
            &MIDIPlayer::get_loop);

        ClassDB::bind_method(
            D_METHOD("set_loop", "loop"),
            &MIDIPlayer::set_loop);

        ADD_PROPERTY(
            PropertyInfo(Variant::BOOL, "loop"),
            "set_loop",
            "get_loop");

        ClassDB::bind_method(
            D_METHOD("get_speed_scale"),
            &MIDIPlayer::get_speed_scale);

        ClassDB::bind_method(
            D_METHOD("set_speed_scale", "speed_scale"),
            &MIDIPlayer::set_speed_scale);

        ADD_PROPERTY(
            PropertyInfo(Variant::FLOAT, "speed_scale"),
            "set_speed_scale",
            "get_speed_scale");

        ClassDB::bind_method(
            D_METHOD("get_auto_stop"),
            &MIDIPlayer::get_auto_stop);

        ClassDB::bind_method(
            D_METHOD("set_auto_stop", "auto_stop"),
            &MIDIPlayer::set_auto_stop);

        ADD_PROPERTY(
            PropertyInfo(Variant::BOOL, "auto_stop"),
            "set_auto_stop",
            "get_auto_stop");

        ClassDB::bind_method(
            D_METHOD("link_audio_stream_player", "audio_stream_player"),
            &MIDIPlayer::link_audio_stream_player);

        ClassDB::bind_method(
            D_METHOD("process_delta", "delta"),
            &MIDIPlayer::process_delta);

        ClassDB::bind_method(
            D_METHOD("get_note_offset"),
            &MIDIPlayer::get_note_offset);

        ClassDB::bind_method(
            D_METHOD("set_note_offset", "note_offset"),
            &MIDIPlayer::set_note_offset);

        ADD_PROPERTY(
            PropertyInfo(Variant::FLOAT, "note_offset"),
            "set_note_offset",
            "get_note_offset");

        ClassDB::bind_method(
            D_METHOD("get_notes_in_range", "start_time", "end_time"),
            &MIDIPlayer::get_notes_in_range);

        ClassDB::bind_method(
            D_METHOD(
                "get_notes_around",
                "time",
                "window_before",
                "window_after"),
            &MIDIPlayer::get_notes_around);

        /*
         * This is deliberately NOT connected to AudioStreamPlayer::finished.
         *
         * MIDI completion is controlled by the MIDI timeline.
         */
        ClassDB::bind_method(
            D_METHOD("_finish_on_main_thread"),
            &MIDIPlayer::_finish_on_main_thread);

        ADD_SIGNAL(MethodInfo("finished"));
        ADD_SIGNAL(MethodInfo("event"));
        ADD_SIGNAL(MethodInfo("meta"));

        ClassDB::bind_method(D_METHOD("event_to_input_event", "event"),&MIDIPlayer::event_to_input_event);
    }

private:
    struct PlaybackEvent {
        Ref<MIDIEvent> event;
        double time = 0.0;
        int32_t track = 0;
    };

    struct NoteCacheEntry {
        Ref<MIDIEventNote> event;
        double time = 0.0;
        bool active = false;
    };

    Ref<MIDIResource> midi;

    std::atomic<PlayerState> state;

    /*
     * This is the MIDI playback clock.
     *
     * It is only modified by the playback thread while playing.
     */
    double current_time;

    /*
     * Index of the next MIDI event to dispatch.
     */
    size_t event_index;

    bool loop;
    double speed_scale;

    std::vector<AudioStreamPlayer *> asps;

    AudioStreamPlayer *longest_asp;

    std::thread playback_thread;

    double audio_output_latency;

    bool has_asp;

    bool auto_stop;

    double note_offset;

    std::vector<PlaybackEvent> playback_events;

    std::vector<NoteCacheEntry> note_cache;

    /*
     * Prevents multiple finish requests from being queued.
     */
    std::atomic<bool> finish_requested;

    /*
     * Protects the playback thread lifecycle.
     */
    std::mutex playback_mutex;

    void threaded_playback();

    void request_finish();

    void _finish_on_main_thread();

    void build_playback_events();

    void build_note_cache();

    double get_microseconds_per_tick(
        int32_t tempo = DEFAULT_MIDI_TEMPO) const;

public:
    MIDIPlayer();
    ~MIDIPlayer();

    void play();
    void stop();

    void pause();
    void resume();

    void _process(float delta);

    void process_delta(double delta);

    void link_audio_stream_player(Array asps);

    bool get_auto_stop() const {
        return this->auto_stop;
    }

    void set_auto_stop(bool value) {
        this->auto_stop = value;
    }

    double get_speed_scale() const {
        return this->speed_scale;
    }

    void set_speed_scale(double value) {
        this->speed_scale = value > 0.0 ? value : 0.001;
    }

    bool get_loop() const {
        return this->loop;
    }

    void set_loop(bool value) {
        this->loop = value;
    }

    int get_state() const {
        return static_cast<int>(this->state.load());
    }

    double get_current_time() const {
        return this->current_time;
    }

    double get_note_offset() const {
        return this->note_offset;
    }

    void set_note_offset(double value) {
        this->note_offset = value;
    }

    Array get_notes_in_range(
        double start_time,
        double end_time);

    Array get_notes_around(
        double time,
        double window_before,
        double window_after);

    void set_current_time(double value);

    void set_midi(const Ref<MIDIResource> &value);

    Ref<MIDIResource> get_midi() const {
        return this->midi;
    }

    Ref<InputEventMIDI> event_to_input_event(const Ref<MIDIEvent> &event) const;
};