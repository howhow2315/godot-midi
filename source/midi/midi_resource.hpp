#pragma once

#include <godot_cpp/godot.hpp>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/input_event_midi.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/resource.hpp>

#include "godot_cpp/classes/global_constants.hpp"
#include "midi_header.hpp"
#include "midi_track.hpp"

using namespace godot;

/// @brief MIDIResource class, responsible for loading and parsing a midi file,
/// then storing it in a format that can be played back
class MIDIResource : public Resource {
  GDCLASS(MIDIResource, Resource);

protected:
  static void _bind_methods() {
    ClassDB::bind_method(D_METHOD("set_header", "header"),
                         &MIDIResource::set_header);
    ClassDB::bind_method(D_METHOD("get_header"), &MIDIResource::get_header);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "header", PROPERTY_HINT_NONE,
                     "MIDIHeader",
                     PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY), "set_header",
                 "get_header");

    ClassDB::bind_method(D_METHOD("set_tracks", "tracks"),
                         &MIDIResource::set_tracks);
    ClassDB::bind_method(D_METHOD("get_tracks"), &MIDIResource::get_tracks);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "tracks", godot::PROPERTY_HINT_ARRAY_TYPE,
                     "MIDITrack",
                     PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY), "set_tracks",
                 "get_tracks");

    // save and load methods
    ClassDB::bind_method(D_METHOD("load_file", "path"),
                         &MIDIResource::load_file);
    ClassDB::bind_method(D_METHOD("save_file", "path", "resource"),
                         &MIDIResource::save_file);
  };

public:
  Ref<MIDIHeader> header;
  TypedArray<Ref<MIDITrack>> tracks;

  Error load_file(const String &p_path);
  Error save_file(const String &p_path, const Ref<Resource> &p_resource);

  // getters and setters

  /// @brief Sets the header of the midi file
  /// @param p_header
  inline void set_header(Ref<MIDIHeader> p_header) { header = p_header; }

  /// @brief Gets the header of the midi file
  /// @return
  inline Ref<MIDIHeader> get_header() const { return header; }

  /// @brief Sets the tracks of the midi file
  /// @param p_tracks
  inline void set_tracks(TypedArray<Ref<MIDITrack>> p_tracks) {
    tracks = p_tracks;
  }

  /// @brief Gets the tracks of the midi file
  /// @return
  inline TypedArray<Ref<MIDITrack>> get_tracks() const { return tracks; }
};