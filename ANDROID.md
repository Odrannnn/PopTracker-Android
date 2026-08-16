# PopTracker for Android

This branch wraps the existing PopTracker C++/SDL2 engine in an Android application. It deliberately keeps pack execution, Lua APIs, JSON layouts, images, saves, Archipelago networking and ZIP reads in the upstream engine instead of translating packs to a new format.

## Current milestone

- Builds the PopTracker 0.35.4 engine with SDL2, SDL2_image, SDL2_ttf, Lua and OpenSSL for Android.
- Supports `arm64-v8a`, `armeabi-v7a`, `x86_64` and `x86` from Android 5.0 (API 21).
- Copies the bundled PopTracker assets into app-private storage on first launch.
- Uses native Android dialogs for confirmations and Archipelago host/slot/password input.
- Opens JSON state files through Android's Storage Access Framework.
- Adds a **ZIP+** import action to PopTracker's native toolbar. The selected ZIP is copied into app-private storage and then sent through PopTracker's existing file-drop install/load path. It stays zipped; the `miniz` pack reader reads files directly from it.
- Maps a normal tap to left-click and a stationary 500 ms hold to right-click, preserving packs that use both item actions.
- Supports workspace pinch-to-zoom around the gesture midpoint and one-finger panning while zoomed, including item grids, map selectors and maps.
- Keeps Android storage permissions out of the manifest. The system document picker grants access only to the file selected by the user.

Manual state export, broadcast windows and pack-specific secondary settings windows are not implemented in this milestone. Automatic per-pack saves still use app-private storage. Desktop app self-updates are disabled because Android packages are updated as APKs/AABs.

## Build

Install Android Studio or an Android SDK containing:

- Android SDK Platform 35
- Android SDK Build Tools 35
- Android NDK `27.3.13750724`
- CMake 3.22.1
- JDK 17

Clone recursively so PopTracker and Android dependencies are present:

```sh
git clone --recurse-submodules https://github.com/Odrannnn/PopTracker-Android.git
cd PopTracker
```

If the repository was already cloned:

```sh
git submodule update --init --recursive
```

Set `ANDROID_HOME` or create `android/local.properties` with `sdk.dir=...`, then build:

```sh
cd android
./gradlew assembleDebug
```

On Windows:

```powershell
cd android
.\gradlew.bat assembleDebug
```

The APK is written under `android/app/build/outputs/apk/debug/`.

## ZIP compatibility and storage flow

```text
Android document picker
        |
        v
app cache/imports/<safe-name>.zip
        |
        | SDL_DROPFILE
        v
PopTracker's existing install handler
        |
        v
files/PopTracker/packs/<safe-name>.zip
        |
        v
Pack + miniz + Lua (unchanged upstream APIs)
```

Imports are capped at 100 MiB. Pack contents remain subject to PopTracker's normal validation and Lua sandbox. As upstream warns, tracker packs should still be treated as code and installed only from trusted sources.

## Device verification checklist

1. Launch on an arm64 phone or emulator and confirm the pack selection screen renders.
2. Import a known Archipelago `.zip` tracker pack and accept the install prompt.
3. Confirm PNG/JPEG/GIF assets and the pack's default layout render correctly.
4. Change item and location states, restart the app, and verify automatic state restore.
5. Connect to an Archipelago server with host, slot and password; verify received items and checked locations update.
6. Rotate the device, background/foreground the app, and reopen the tracker.
7. Repeat with at least one compact phone and one tablet-sized display.

## Upstream maintenance

The Android code is isolated under `android/`, plus small `__ANDROID__` branches in `src/core/fs.h`, `src/uilib/dlg.cpp` and `src/uilib/ui.*`. This keeps future PopTracker updates reviewable and minimizes pack-compatibility drift.
