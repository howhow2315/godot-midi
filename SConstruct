from glob import glob

ADDON_DIRECTORY = "addons/godot-fluidsynth"
FLUIDSYNTH_SOURCE = "library/fluidsynth"
FLUIDSYNTH_INSTALL = "build/fluidsynth"

GODOT_API_VERSION = "4.7"

env = SConscript(
    "library/godot-cpp/SConstruct",
    {
        "api_version": GODOT_API_VERSION,
        # "symbols_visibility": "visible"
    }
)
env.Tool("fluidsynth", toolpath=["tools"])

platform = env["platform"]
arch = env["arch"]

if platform not in ("linux", "windows", "macos"):
    raise ValueError(f"Unsupported platform: {platform}")

bin_directory = f"{ADDON_DIRECTORY}/bin/{platform}.{arch}"

FLUIDSYNTH_ARTIFACTS = {
    "linux": [
        "lib/libfluidsynth.so",
        "lib/libfluidsynth.so.3",
        "lib/libfluidsynth.so.3.6.1",
    ],
    "windows": [
        "bin/libfluidsynth-3.dll",
        "lib/libfluidsynth-3.lib",
    ],
    "macos": [
        "lib/libfluidsynth.dylib",
        "lib/libfluidsynth.3.dylib",
    ],
}

FLUIDSYNTH_LINK_LIBRARY = {
    "linux": "lib/libfluidsynth.so",
    "windows": "lib/libfluidsynth-3.lib",
    "macos": "lib/libfluidsynth.dylib",
}


fluidsynth_targets = [f"{FLUIDSYNTH_INSTALL}/{artifact}" for artifact in FLUIDSYNTH_ARTIFACTS[platform]]
fluidsynth_build = env.fluidsynth(fluidsynth_targets, FLUIDSYNTH_SOURCE)

env.Append(
    CPPPATH=[
        "source",
        f"{FLUIDSYNTH_INSTALL}/include",
    ],
    LIBPATH=[
        f"{FLUIDSYNTH_INSTALL}/lib",
        "library/godot-cpp/bin",
    ],
)

sources = glob("source/*.cpp")
objects = env.SharedObject(sources)
env.Depends(objects, fluidsynth_build)

suffix = env["suffix"]

godot_cpp = File(f"library/godot-cpp/bin/libgodot-cpp{suffix}{env['LIBSUFFIX']}")
fluidsynth = File(f"{FLUIDSYNTH_INSTALL}/{FLUIDSYNTH_LINK_LIBRARY[platform]}")

lib_filename = (
    f"{env.subst('$SHLIBPREFIX')}"
    f"fluidsynth"
    f"{suffix}"
    f"{env.subst('$SHLIBSUFFIX')}"
)

library = env.SharedLibrary(
    f"{bin_directory}/{lib_filename}",
    source=objects,
    LIBS=[godot_cpp, fluidsynth],
)

env.Depends(library, fluidsynth_build)

default_targets = [library]

if platform == "linux":
    runtime = env.Command(
        f"{bin_directory}/libfluidsynth.so.3.6.1",
        f"{FLUIDSYNTH_INSTALL}/lib/libfluidsynth.so.3.6.1",
        Copy("$TARGET", "$SOURCE"),
    )
    env.Depends(runtime, fluidsynth_build)

    soname = env.Command(
        f"{bin_directory}/libfluidsynth.so.3",
        runtime,
        Action("ln -sf libfluidsynth.so.3.6.1 $TARGET"),
    )
    env.Depends(soname, runtime)
    env.Depends(library, soname)

    default_targets.extend([runtime, soname])

elif platform == "windows":
    runtime = env.Command(
        f"{bin_directory}/libfluidsynth-3.dll",
        f"{FLUIDSYNTH_INSTALL}/bin/libfluidsynth-3.dll",
        Copy("$TARGET", "$SOURCE"),
    )
    env.Depends(runtime, fluidsynth_build)
    env.Depends(library, runtime)

    default_targets.append(runtime)

elif platform == "macos":
    runtime = env.Command(
        f"{bin_directory}/libfluidsynth.3.dylib",
        f"{FLUIDSYNTH_INSTALL}/lib/libfluidsynth.3.dylib",
        Copy("$TARGET", "$SOURCE"),
    )
    env.Depends(runtime, fluidsynth_build)

    dylib = env.Command(
        f"{bin_directory}/libfluidsynth.dylib",
        runtime,
        Action("ln -sf libfluidsynth.3.dylib $TARGET"),
    )
    env.Depends(dylib, runtime)
    env.Depends(library, dylib)

    default_targets.extend([runtime, dylib])


addons_source = ["license.md", "readme.md"]
addons_files = env.Command(
    [f"{ADDON_DIRECTORY}/{file}" for file in addons_source],
    addons_source,
    Copy(ADDON_DIRECTORY, addons_source),
)

default_targets.append(addons_files)
Default(*default_targets)
Alias("addons_files", addons_files)
