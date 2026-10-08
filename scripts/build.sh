#!/usr/bin/env bash

set -euo pipefail


ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

rm -rf "$ROOT/addons/godot-midi/bin/" # rm built addon bin 
rm -rf "$ROOT/.godot/" # rm godot cache 

find "$ROOT/assets" -type f -name "*.import" -delete

# scons -c # clean out c++ build files 

scons