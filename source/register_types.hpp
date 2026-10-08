#pragma once

#include <gdextension_interface.h>
#include <godot_cpp/classes/editor_import_plugin.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_godotmidi_types();
void uninitialize_godotmidi_types();