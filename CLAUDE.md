# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Working conventions

- Follow the existing coding style (brace placement, aligned member initializers, compact lambda-heavy wiring, `m_` member prefix).
- Evolve the original design decisions rather than redesigning; the architecture below is deliberate.
- Keep the code minimalistic and easily graspable by a human reader — some design choices exist specifically to serve this goal. Prefer the smallest change that fits.
- Minimal comments: comment only truly tricky parts or unusual decisions. All other meta information belongs in CLAUDE.md or other meta-artifacts, not in the code.

## Project

ClipboardSync is a Qt Widgets system-tray app (C++17, Qt 6) that synchronizes clipboard contents between machines/user accounts over LAN via encrypted UDP. All source files live in the repository root; `CMakeLists.txt` is the source of truth for the build.

## Build

CMake with the tracked `CMakePresets.json` (Ninja Multi-Config, output under `build/`); `CMAKE_PREFIX_PATH` must point at the Qt installation prefix. On this machine, enter the VS dev environment first (see toolchain notes below):

```
cmake --preset default
cmake --build --preset release    # or: --preset debug
```

Everything under `build/` is gitignored — never commit it. Machine-specific settings belong in the `CMAKE_PREFIX_PATH` environment variable or an untracked `CMakeUserPresets.json`; tracked files (README, presets, CMakeLists.txt) must stay environment-agnostic — no hardcoded VS or Qt paths, no user-specific env var names.

`thirdparty/tiny-AES-c` is a git submodule (run `git submodule update --init` after a fresh clone). Its `aes.c` is compiled directly into the app; the cipher variant is selected by the `AES256=1 ECB=0 CTR=0` compile definitions in CMakeLists.txt.

There is no test suite and no linter configured.

### Local toolchain notes (verified 2026-07)

- Development targets **VS 2026 Community Insiders** (v18.x, prerelease channel; VS 2022 and 2019 are also installed). `vswhere.exe` (at its fixed location `%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\`) lists VS 2026 **only with `-prerelease`**.
- `cmake` and `ninja` are **not on PATH**; the VS-bundled ones become available after entering the dev environment — either the "x64 Native Tools Command Prompt for VS 2026" or `call "<VS>\Common7\Tools\VsDevCmd.bat" -arch=amd64`. Verified inside it: MSVC `cl` 19.51 (x64), CMake 4.2 (4.x — no pre-3.5 policy compat), Ninja 1.13. The Insiders `VsDevCmd.bat` prints a harmless `'vswhere.exe' is not recognized` warning.
- Qt 6.12 **static** build at `%QT2026%\amd64` (env var `QT2026`), configured with `static` + `static_runtime`; `debug_and_release` libs are present. Set `CMAKE_PREFIX_PATH=%QT2026%\amd64` before configuring. CMakeLists.txt is agnostic to the Qt build's CRT flavor: `qt_add_executable` itself applies `/MT` to the target when (and only when) the Qt install reports `static_runtime`.
- The Qt libs were compiled with clang-cl (mkspec `win32-clang-msvc`) and reference `clang_rt.builtins-x86_64.lib`; CMakeLists.txt locates it in the VS-bundled Clang toolchain via `VCToolsInstallDir` (a no-op for cl-built Qt), so the app itself builds with plain MSVC `cl`.

## Architecture

`main.cpp` constructs the event-driven components below and wires them together with Qt signals — the only other coupling is the `const ConfigManager&` that DatagramProcessor and ClipboardManager hold for reading the config; the static-only helpers (CryptoEngine, AutoStart) are called directly where needed:

- **ClipboardManager** — watches `QClipboard::dataChanged` (on macOS: a 1s poll timer instead, since that signal doesn't fire there) and serializes only `text/*` MIME formats into a `QByteArray` (QDataStream of format/data pairs). Emits `dataChanged(bytes)`; `setData(bytes)` writes received data back to the clipboard. Echo loops are prevented by comparing `qHash` of the serialized payload against the last seen hash.
- **DatagramProcessor** — UDP transport with separate Tx/Rx sockets. Splits payloads into MTU-sized fragments, each carrying a 12-byte big-endian header (`magic 0xB81E`, fragment count/index, random transfer id). Reassembles incoming fragments per `{sender address, transfer id}` key with a 10 s idle timeout. Supports Broadcast, Multicast (binds/joins per selected interface), and Unicast modes; sockets are (re)configured in response to `ConfigManager::configChanged`. Fragments are sent back-to-back; the Rx-starvation/async-send question is tracked in TODO.md.
- **CryptoEngine** — static-only wrapper over tiny-AES-c. The master key is PBKDF2-SHA256 of the password (125328 iterations); each datagram's key is derived by `bindContext(masterKey, header bytes)` (a cheap 13-iteration PBKDF2), so the header acts as associated data. Payloads are AES-256-CBC with PKCS#7 padding and a random IV *appended* after the ciphertext. Note the wire header is big-endian but `bindContext` hashes the raw in-memory struct (known TODO in DatagramProcessor.cpp re: endianness — changing this breaks compatibility between peers).
- **ConfigManager / ConfigDialog** — settings (interface, mode, address, port, MTU, password hash, disabled) load from `QSettings` at startup and save in the destructor. Setters emit `configChanged`; `beginSeqChange()`/`endSeqChange()` batch multiple setters into one emission. `main()` emits `configChanged` manually once at startup to trigger the initial socket setup. The `disabled` field is a first-class config field like the rest (persisted, emits `configChanged`), but its only writer is the tray "Disable" action — it is not part of the dialog or the `beginSeqChange` batch. When disabled, `DatagramProcessor`'s `configChanged` handler closes both sockets (dropping any multicast membership) and `sendDatagram` is no-opped; re-enabling emits `configChanged` again and the untouched setup code re-binds/rejoins. The dialog wraps the network fields in a `QGroupBox("Network")` — the group box member must stay declared before the widgets it reparents (ConfigDialog.h), or `~QObject` double-destroys them; a blank password field on OK keeps the stored hash, but only when a hash exists and the port (its salt) is unchanged — otherwise the field is refused, and a `(unchanged)` placeholder hints at the keep behavior.
- **AutoStart** — static-only helper (`enabled()`/`setEnabled()`, mirroring `CryptoEngine`'s style) driving the "Automatically start at login" checkbox in `ConfigDialog`. The OS is the source of truth, so it bypasses `ConfigManager` entirely (no `QSettings` entry, no `configChanged`): on Windows a value under `HKCU\...\CurrentVersion\Run` (via `QSettings` native format), on macOS a LaunchAgent plist under `~/Library/LaunchAgents`. Single `AutoStart.cpp` with `#ifdef` blocks (plain C++ on both, no `.mm` split) and an empty stub elsewhere.
- **WakeNotifier** — emits `systemWoke()` a few seconds after the system resumes from sleep; `main()` connects it to `ConfigManager::configChanged` so the sockets rebuild and multicast is rejoined (group delivery silently dies across sleep — see `debugging/2026-07-04-macos-sleep-multicast-drop.md`). Per-platform backends: `WakeNotifier.mm` (NSWorkspace wake notification; uses manual retain/release — the build has no ARC, do not enable `-fobjc-arc` for this file) and `WakeNotifier.cpp` (`WM_POWERBROADCAST` native event filter on Windows, empty stub elsewhere).

The tray "Disable" action is a checkable menu item whose `toggled` handler just calls `confMgr.setDisabled()`; everything else reacts to `configChanged`. A tray-side `configChanged` lambda syncs the check state and icon, so the persisted state initializes the UI through the startup emission. DatagramProcessor closes its sockets and no-ops sends while disabled (see the ConfigManager bullet); ClipboardManager's clipboard handler returns early, skipping the serialize/hash work.

Wire format per datagram: `[12-byte header][ciphertext][16-byte IV]`. Any change to the header, KDF parameters, or serialization format breaks interoperability with peers running older builds.
