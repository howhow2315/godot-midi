from glob import glob

ADDON_DIRECTORY = "addons/godot-midi"
GODOT_API_VERSION = "4.7"

env = SConscript(
    "library/godot-cpp/SConstruct",
    {"api_version": GODOT_API_VERSION}
)

platform = env["platform"]
arch = env["arch"]

if platform not in ("linux", "windows", "macos"):
    raise ValueError(f"Unsupported platform: {platform}")

bin_directory = f"{ADDON_DIRECTORY}/bin/{platform}.{arch}"

env.Append(
    CPPPATH=["source"],
    LIBPATH=["library/godot-cpp/bin"],
)

sources = glob("source/*.cpp") + glob("source/*/*.cpp")
print(f"compiling {sources}")
objects = env.SharedObject(sources)

suffix = env["suffix"]

godot_cpp = File(f"library/godot-cpp/bin/libgodot-cpp{suffix}{env['LIBSUFFIX']}")

lib_filename = (
    f"{env.subst('$SHLIBPREFIX')}"
    f"midi"
    f"{suffix}"
    f"{env.subst('$SHLIBSUFFIX')}"
)

library = env.SharedLibrary(
    f"{bin_directory}/{lib_filename}",
    source=objects,
    LIBS=[godot_cpp],
)

addons_source = ["license.md", "readme.md"]
addons_files = env.Command(
    [f"{ADDON_DIRECTORY}/{file}" for file in addons_source],
    addons_source,
    Copy(ADDON_DIRECTORY, addons_source),
)

Default(library, addons_files)
Alias("addons_files", addons_files)
