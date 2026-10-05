#ifndef MIDI_RESOURCE_H
#define MIDI_RESOURCE_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "midi_resource.hpp"

using namespace godot;

/// @brief MIDIResource class, responsible for loading and parsing a midi file,
/// then storing it in a format that can be played back
class MIDIResource : public Resource {
  GDCLASS(MIDIResource, Resource);

protected:
  static void _bind_methods() {
    ClassDB::bind_method(D_METHOD("set_format", "format"),
                         &MIDIResource::set_format);
    ClassDB::bind_method(D_METHOD("get_format"), &MIDIResource::get_format);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "format"), "set_format",
                 "get_format");

    ClassDB::bind_method(D_METHOD("set_track_count", "track_count"),
                         &MIDIResource::set_track_count);
    ClassDB::bind_method(D_METHOD("get_track_count"),
                         &MIDIResource::get_track_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "track_count"), "set_track_count",
                 "get_track_count");

    ClassDB::bind_method(D_METHOD("set_division", "division"),
                         &MIDIResource::set_division);
    ClassDB::bind_method(D_METHOD("get_division"), &MIDIResource::get_division);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "division"), "set_division",
                 "get_division");

    ClassDB::bind_method(D_METHOD("set_division_type", "division_type"),
                         &MIDIResource::set_division_type);
    ClassDB::bind_method(D_METHOD("get_division_type"),
                         &MIDIResource::get_division_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "division_type"),
                 "set_division_type", "get_division_type");

    ClassDB::bind_method(D_METHOD("set_smpte_fps", "smpte_fps"),
                         &MIDIResource::set_smpte_fps);
    ClassDB::bind_method(D_METHOD("get_smpte_fps"),
                         &MIDIResource::get_smpte_fps);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "smpte_fps"), "set_smpte_fps",
                 "get_smpte_fps");

    ClassDB::bind_method(
        D_METHOD("set_smpte_ticks_per_frame", "smpte_ticks_per_frame"),
        &MIDIResource::set_smpte_ticks_per_frame);
    ClassDB::bind_method(D_METHOD("get_smpte_ticks_per_frame"),
                         &MIDIResource::get_smpte_ticks_per_frame);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "smpte_ticks_per_frame"),
                 "set_smpte_ticks_per_frame", "get_smpte_ticks_per_frame");

    ClassDB::bind_method(D_METHOD("set_tempo", "tempo"),
                         &MIDIResource::set_tempo);
    ClassDB::bind_method(D_METHOD("get_tempo"), &MIDIResource::get_tempo);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "tempo"), "set_tempo", "get_tempo");

    ClassDB::bind_method(D_METHOD("set_tracks", "tracks"),
                         &MIDIResource::set_tracks);
    ClassDB::bind_method(D_METHOD("get_tracks"), &MIDIResource::get_tracks);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "tracks"), "set_tracks",
                 "get_tracks");

    // save and load methods
    ClassDB::bind_method(D_METHOD("load_file", "path"),
                         &MIDIResource::load_file);
    ClassDB::bind_method(D_METHOD("save_file", "path", "resource"),
                         &MIDIResource::save_file);
  }

private:
  int format;
  int track_count;
  int division;
  int division_type;
  int smpte_fps;
  int smpte_ticks_per_frame;
  int tempo;
  Array tracks;

public:
  Error load_file(const String &p_path);
  Error save_file(const String &p_path, const Ref<Resource> &p_resource);

  // getters and setters

  /// @brief Sets the format of the midi file, see
  /// MIDIParser::MIDIHeaderChunk::MIDIFileFormat
  /// @param p_format
  inline void set_format(int p_format) { format = p_format; }

  /// @brief Gets the format of the midi file, see
  /// MIDIParser::MIDIHeaderChunk::MIDIFileFormat
  /// @return
  inline int get_format() const { return format; }

  /// @brief Sets the number of tracks in the midi file
  /// @param p_track_count
  inline void set_track_count(int p_track_count) {
    track_count = p_track_count;
  }

  /// @brief Gets the number of tracks in the midi file
  /// @return
  inline int get_track_count() const { return track_count; }

  /// @brief Sets the division of the midi file in ticks per quarter note
  /// @param p_division
  inline void set_division(int p_division) { division = p_division; }

  /// @brief Gets the division of the midi file in ticks per quarter note
  /// @return
  inline int get_division() const { return division; }

  /// @brief Sets the division type, see
  /// MIDIParser::MIDIHeaderChunk::MIDIDivisionType
  /// @param p_division_type
  inline void set_division_type(int p_division_type) {
    division_type = p_division_type;
  }

  /// @brief Gets the division type, see
  /// MIDIParser::MIDIHeaderChunk::MIDIDivisionType
  /// @return
  inline int get_division_type() const { return division_type; }

  /// @brief Sets the SMPTE frame rate (only meaningful when division_type is
  /// FramesPerSecond)
  /// @param p_smpte_fps
  inline void set_smpte_fps(int p_smpte_fps) { smpte_fps = p_smpte_fps; }

  /// @brief Gets the SMPTE frame rate (only meaningful when division_type is
  /// FramesPerSecond)
  /// @return
  inline int get_smpte_fps() const { return smpte_fps; }

  /// @brief Sets the SMPTE ticks per frame (only meaningful when division_type
  /// is FramesPerSecond)
  /// @param p_smpte_ticks_per_frame
  inline void set_smpte_ticks_per_frame(int p_smpte_ticks_per_frame) {
    smpte_ticks_per_frame = p_smpte_ticks_per_frame;
  }

  /// @brief Gets the SMPTE ticks per frame (only meaningful when division_type
  /// is FramesPerSecond)
  /// @return
  inline int get_smpte_ticks_per_frame() const { return smpte_ticks_per_frame; }

  /// @brief Sets the tempo in microseconds per quarter note
  /// @param p_tempo
  inline void set_tempo(int p_tempo) { tempo = p_tempo; }

  /// @brief Gets the tempo in microseconds per quarter note
  /// @return
  inline int get_tempo() const { return tempo; }

  /// @brief Sets the tracks of the midi file
  /// @param p_tracks
  inline void set_tracks(Array p_tracks) { tracks = p_tracks; }

  /// @brief Gets the tracks of the midi file
  /// @return
  inline Array get_tracks() const { return tracks; }
};

#endif // MIDI_RESOURCE_H