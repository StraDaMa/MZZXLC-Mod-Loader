# MZZXLC Mod Loader

MZZXLC Mod Loader is a basic mod loader for the PC version of *Mega Man Zero/ZX Legacy Collection*. This loader is meant to allow existing mods for **Fluffy Manager 5000** to be used as well as code-modifying mods.

Many concepts have been "borrowed" from [chaudloader](https://github.com/RockmanEXEZone/chaudloader).

## For users
1. Navigate to the game's folder by right-clicking on *Mega Man Zero/ZX Legacy Collection* in Steam and clicking on `Manage -> Browse Local Files`
1. Copy `MZZXLC_mod_loader.asi` and `d3d9.dll` to the game's folder.
1. Run `MZZXLC.exe`, this will create a `mods` folder.
1. Copy mods into the `mods` folder and start the game.

This loader is meant to be compatible with existing asset-swapping mods for **Fluffy Manager 5000**. Mods meant for this mod manager can be copied to the `mods` folder and will show up in the mod list.

## For modders

Mods consists of the following files in a folder inside the `mods` folder:

- `info.toml`: Metadata about your mod. It should look something like this:

    ```toml
    title = "my cool mod"
    version = "0.0.1"
    authors = ["my cool name"]
    requires_loader_version = "0.0.0"  # or any semver string, can be unset if not required
    ```
- If the mod directory contains a folder named `nativePCx64`, files in the folder will replace files in the game's `nativePCx64`.
- If the mod directory contains a `.dll` file with the same name as the mod directory, it will be loaded. If the dll exports a function `mod_open`, it will be called when the mod is loaded.

    The `mod_open` function should have the following signature:
    ```c
    __declspec(dllexport) void mod_open() {
    // Do all your logic here.
    }
    ```


## For developers

Currently, this repo is really not set up for other people to easily compile.

Compiling the source requires:

- Detours
- Boost
- tomlplusplus

After obtaining the other prerequisites, open `MZZXLC_mod_loader.sln` in Visual Studio.

Update the include and library directories in `Project -> MZZXLC_mod_loader -> VC++ Directories` for the requirements and run `Build -> Build Solution`.