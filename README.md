# PopTracker for Android

## ⚠️ Important disclaimer: AI-generated port

> **This Android port was created primarily through AI-generated code and AI-assisted development. It is an unofficial community fork and has not received a complete independent code, security or compatibility audit. Expect bugs, crashes, broken tracker packs and other unexpected behavior. Review the source where appropriate, keep backups of anything important and use the APK at your own risk.**
>
> **This disclaimer applies to the Android port and its Android-specific changes. The original [PopTracker project](https://github.com/black-sliver/PopTracker) is a separate upstream project; its maintainers did not create or endorse this port and are not responsible for its behavior or support.**

This is an Android-focused fork of [PopTracker](https://github.com/black-sliver/PopTracker), a universal, scriptable progress tracker commonly used with Archipelago randomizers.

The fork keeps PopTracker's native C++/SDL engine, Lua pack APIs, JSON layouts, images, saves, ZIP reader and Archipelago client. Existing tracker packs therefore run through the same engine instead of being converted to a separate Android format.

☕ Enjoying the port? [You can support me on Ko-fi](https://ko-fi.com/odrannnn). 💜

## What this fork adds

- An Android Studio/Gradle project targeting Android 5.0 (API 21) and newer.
- Direct import of tracker pack ZIP files through Android's document picker. Packs remain zipped and are stored in the app's private storage.
- A phone- and tablet-friendly toolbar with larger controls, including **Import Tracker ZIP**.
- Pinch-to-zoom and one-finger panning for maps, map selectors and item grids.
- A fixed map picker bar that remains accessible while the tracker workspace is zoomed.
- Touch controls: tap for left-click and hold for 500 ms for right-click.
- Android-safe layout around display cutouts, status bars and navigation bars.
- Native Android dialogs for confirmations and Archipelago connection details.
- Support for being launched by another Android app with a game name and Archipelago connection details.
- Lifecycle handling for switching away from the app and returning to it.

## Download and install

Download the APK from the [latest GitHub release](https://github.com/Odrannnn/PopTracker-Android/releases/latest).

The current APK is a universal development build containing ARM and x86 variants, so it is larger than a normal Play Store package. Android may ask you to allow installation from the browser or file manager used to open it. The APK uses a development signing key and is not distributed through Google Play.

After installation:

1. Open PopTracker.
2. Tap the ZIP import button in the toolbar.
3. Choose a trusted tracker pack ZIP without extracting it.
4. Load the installed pack from the pack picker.
5. Use the **AP** button to enter the Archipelago server, slot and password when the pack supports auto-tracking.

Tracker packs contain Lua code. Only install packs from sources you trust. ZIP imports are limited to 100 MiB.

## Launching from another Android app

Another app can explicitly launch `PopTrackerActivity` and supply these string extras:

| Extra | Purpose |
| --- | --- |
| `game` | Game name used to find and open a matching installed tracker pack. |
| `ap_host` | Archipelago server address, including a port when required. |
| `ap_slot` | Player slot name. Defaults to `Player` when omitted. |
| `ap_password` | Archipelago password. May be omitted or empty. |

Example in Kotlin:

```kotlin
val intent = Intent().apply {
    setClassName(
        "io.github.poptracker.android",
        "io.github.poptracker.android.PopTrackerActivity"
    )
    putExtra("game", "A Link to the Past")
    putExtra("ap_host", "archipelago.gg:38281")
    putExtra("ap_slot", "Player 1")
    putExtra("ap_password", "")
}
startActivity(intent)
```

The game match is case-insensitive and ignores punctuation and spaces. If exactly one installed pack UID matches, the newest installed version is opened before connecting. If the match is missing or ambiguous, PopTracker shows the pack picker instead of guessing. The same intent contract works when PopTracker is already running.

## Current limitations

- Manual state export is not implemented; automatic per-pack saves use app-private storage.
- Broadcast windows and pack-specific secondary settings windows are not implemented.
- Desktop self-updates are disabled because Android installs updates as APKs or app bundles.
- Tracker compatibility still depends on the individual pack and its use of PopTracker APIs.

## Building from source

Install Android Studio or an Android SDK containing SDK Platform 35, Build Tools 35, NDK `27.3.13750724`, CMake 3.22.1 and JDK 17. Then clone with submodules:

```sh
git clone --recurse-submodules https://github.com/Odrannnn/PopTracker-Android.git
cd PopTracker-Android/android
./gradlew assembleDebug
```

On Windows, use `.\gradlew.bat assembleDebug`. The APK is written under `android/app/build/outputs/apk/debug/`.

See [ANDROID.md](ANDROID.md) for architecture details, storage behavior and the device verification checklist. Upstream desktop build instructions remain in [BUILD.md](BUILD.md).

## Upstream and license

This fork is based on [black-sliver/PopTracker](https://github.com/black-sliver/PopTracker) and preserves its history. PopTracker is licensed under the [GNU General Public License v3.0](LICENSE).

For tracker packs and the wider PopTracker community, visit the [PopTracker Discord](https://discord.com/invite/gwThqMCPgK).
