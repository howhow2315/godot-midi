#!/usr/bin/env bash

set -euo pipefail


ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT_DIRECTORY="${ROOT}/project"

# rm bin
BINARY_DIRECTORY="${PROJECT_DIRECTORY}/addons/godot-midi/binaries"
rm -rf $BINARY_DIRECTORY

# rm caches 
rm -rf "$PROJECT_DIRECTORY/.godot/"
rm -rf "$ROOT/generated"
rm -rf "$ROOT/.cache"

BUILD_PROFILE_FILE="${ROOT}/build_profile.json"
if [ ! -e "$BUILD_PROFILE_FILE" ]; then
    scons compiledb=yes compile_commands.json
fi

# scons -c # clean out c++ build files 

# build for host platform
scons