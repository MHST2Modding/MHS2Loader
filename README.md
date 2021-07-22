# MHS2 Loader

A .dll plugin/mod loader for Monster Hunter Stories 2. This is an early alpha version and will be subject to non-compatible changes in the future.

Disables various anti-tamper checks in the game in order to load mods / aid it mod development:
* Anti-Debug
* CRC of the `.text` section in memory
* DLL validity checks (for proxy dlls)
* ... more ...

## Installation
1. Download the latest release for your game version.
2. Copy `dinput8.dll` and `LoaderCore.dll` to your game folder (next to `game.exe`)
3. Place .dll mods into the `mods` folder.