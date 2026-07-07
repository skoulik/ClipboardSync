# Plan: config dialog cleanup + start-at-login (2026-07-07)

Status: **implemented** (commit 119c3c2, 2026-07-07). Review found one bug beyond the plan:
the `QGroupBox` member must be declared before the widgets it reparents, or `~QObject`
double-destroys them on dialog close. The checkbox label shipped as "Automatically start
at login". macOS backend still compile- and runtime-unverified (Windows-only machine).

## Goals

1. Stop requiring the password on every visit to the config dialog — only when it actually
   needs re-hashing.
2. Add a start-at-login (auto-start) option, enabled from the config dialog.
3. Reorganize the dialog so the new app-level option doesn't mix with network settings.

## Confirmed current behavior

- `ConfigDialog`'s OK handler unconditionally rejects an empty password field
  (`ConfigDialog.cpp`), so the password must be retyped on every OK.
- The port number is the salt for the stored password hash
  (`confMgr.setPass(pass, port)`), so the stored hash is only valid for the port it was
  created with. This is the real coupling behind "password needed when the port changes".
- The tray menu holds runtime actions only (Config…, Disable, Quit); persistent
  preferences live in the dialog.

## Decisions

- **Password**: an empty password field on OK means "keep the stored hash". It is allowed
  iff a stored hash exists **and** the port spinbox still equals `confMgr.port()` (the
  setters haven't run yet at validation time, so that comparison is exactly "port
  unchanged"). Otherwise focus the field and refuse, as today. When a stored hash exists,
  give the field a `(unchanged)` placeholder as the hint that blank means "keep".
  `ConfigManager` needs no change — `setPass` is simply not called when the field is blank.
- **Auto-start ownership**: the OS is the source of truth (registry / launch agent), so it
  stays out of `ConfigManager` entirely — no `QSettings` entry, no `configChanged`
  emission (sockets don't care). A static-only helper `AutoStart` (mirroring
  `CryptoEngine`'s static-only style):
  - `static bool enabled()` / `static void setEnabled(bool)`.
  - **Windows**: value named after `QCoreApplication::applicationName()` under
    `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` via `QSettings` native format;
    value is the quoted native path of `applicationFilePath()`. `setEnabled(true)` on each
    OK also refreshes the path if the exe moved.
  - **macOS**: LaunchAgent plist at `~/Library/LaunchAgents/com.sksoft.clipboardsync.plist`
    (`RunAtLoad`, `ProgramArguments` = bundle binary path). Plain C++ (`QFile`), no ObjC —
    unlike `WakeNotifier`, no `.mm` split is needed; one `AutoStart.cpp` with `#ifdef`
    blocks and an empty stub for other platforms.
  - Deliberately **not** `SMAppService`: it requires macOS 13+, ObjC, and a properly
    registered bundle; the plist works everywhere and matches the project's minimalism.
- **Dialog layout** (chosen from three options: group box / flat form + separator /
  tray-menu action): wrap the six existing fields in a `QGroupBox("Network")`; the
  "Start at login" `QCheckBox` sits below it, above the button box. Semantically matches
  the code boundary (network fields are applied atomically via `beginSeqChange`/
  `endSeqChange` and feed the sockets; auto-start has a different lifecycle) and gives
  future app-level settings an obvious home.
- **Checkbox semantics**: initialized from `AutoStart::enabled()` at dialog construction,
  applied via `AutoStart::setEnabled(...)` in the OK handler — same commit-on-OK behavior
  as every other setting.
- **Platforms**: Windows + macOS (both at parity since the 2026-07-04 macOS port
  verification); empty stub elsewhere.

## Implementation steps

1. `AutoStart.h` / `AutoStart.cpp` — new static-only helper as above; add both files to
   the `qt_add_executable` source list in `CMakeLists.txt` (no platform conditionals
   needed — the `#ifdef`s live in the .cpp).
2. `ConfigDialog.h` — add `QGroupBox m_networkGroup { "Network", this }` and
   `QCheckBox m_autoStartCb { "Start at login", this }` members (aligned initializer
   style).
3. `ConfigDialog.cpp`:
   - Password acceptance rule + placeholder as decided above.
   - `setPass` called only when the field is non-empty (keep the port-as-salt comment).
   - `AutoStart::setEnabled(m_autoStartCb.isChecked())` before `accept()`.
   - Layout: the `QFormLayout` becomes the group box's layout; main `QVBoxLayout` is
     group box → checkbox → button box.
4. `CLAUDE.md` — extend the ConfigManager/ConfigDialog architecture bullet with the
   blank-password rule and the AutoStart helper (OS as source of truth, bypasses
   ConfigManager).

## Compatibility

No wire-format, KDF, or settings-schema change. An existing stored hash keeps working;
peers are unaffected.

## Verification

- Build both configs (`cmake --build --preset release` / `debug` inside the VS dev env).
- Manual, Windows: open dialog → OK with blank password (must accept, sync still works
  against a peer); change port → OK with blank password (must refuse and focus the
  field); enable Start at login → check the Run key value appears and points at the exe;
  disable → value removed; reboot-free check via `Get-ItemProperty` is enough.
- Manual, macOS: same dialog checks; enable → plist appears under
  `~/Library/LaunchAgents` with the right binary path; disable → plist removed.
- First-run case: with no stored hash (fresh `QSettings` scope), blank password must
  still be refused.
