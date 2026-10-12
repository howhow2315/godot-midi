#!/bin/sh
printf '\033c\033]0;%s\a' godot-midi
base_path="$(dirname "$(realpath "$0")")"
"$base_path/godot-fluidsynth.x86_64" "$@"
