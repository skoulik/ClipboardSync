# Plan: ConfigManager save-on-change (2026-07-08)

Status: **implemented** (2026-07-08, in working tree, not yet committed). Implemented by
Developer_Opus; review found no issues. Local verification done: three bench-driven
kill/inject/relaunch cycles clean, and a persisted `disabled=true` (saved on change,
mid-session) survived force-kills with the relaunched app starting disabled (no sockets
bound). The `test-all-modes.sh` sync run is blocked: the bench needs the `claude` user to
own the Mac's GUI console (for `open`/pasteboard), but `sergeikulik` holds it — `open`
fails with RBSRequestErrorDomain "Launch failed". Dialog-change and clean-exit-no-clobber
checks folded into the user's manual verification.

## Goals

1. Persist settings when they change, not in the destructor — a crash must not lose
   changes (TODO.md item).
2. A clean exit must not overwrite settings injected externally while the app runs
   (the reason the testbench force-kills instead of quitting).

## Confirmed current behavior

- `ConfigManager` loads all fields in the constructor and writes all of them back in the
  destructor (`ConfigManager.cpp`); nothing is persisted in between.
- `configChanged` is emitted from two change points: the `setter()` template (when not
  inside a `beginSeqChange`/`endSeqChange` batch) and `endSeqChange()` (when the batch
  changed something). It is also emitted *without* any change: manually at startup
  (`main.cpp`) and by `WakeNotifier::systemWoke` via a direct signal-to-signal
  connection.
- The testbench (`set-mode.sh`) kills the app, injects registry/defaults values, then
  relaunches — injection never races a running instance, but only because of the
  force-kill.

## Decisions

- **Private `save()`**: the destructor body moves verbatim into a private
  `void save()` (local `QSettings`, which flushes when it goes out of scope). It writes
  all fields, like today — called only on user-driven changes, so write volume is not a
  concern.
- **Call sites**: exactly the two change points — in `setter()` right before the
  non-batched `configChanged` emission, and in `endSeqChange()` before its emission. So
  a dialog OK is one save, a tray disable toggle is one save.
- **Not a self-connection to `configChanged`** (considered, rejected): the signal also
  fires with no underlying change — every wake from sleep and the startup emission —
  which would produce spurious full rewrites and reintroduce the external-injection
  clobber (goal 2) on every wake. Saving belongs to the change points, not to the
  signal.
- **Destructor removed** entirely (declaration and definition — rule of zero). This is
  what delivers goal 2: exit writes nothing.
- **First-run note**: today the defaults get written on the first clean exit; after the
  change, nothing is written until the first actual setting change. Harmless for the
  app (defaults are compiled in), and both testbench machines already have the registry
  key/defaults domain. New machines using the testbench before ever touching the config
  would need one settings change first (or `set-mode.sh` simply creates the values, as
  `Set-ItemProperty`/`defaults write` do when the key exists/domain is creatable).
- **Testbench untouched**: kill → inject → relaunch still works unchanged; the
  force-kill just stops being load-bearing.

## Implementation steps

1. `ConfigManager.h` — remove `~ConfigManager()`, add private `void save();`.
2. `ConfigManager.cpp` — destructor body becomes `save()`; call it from `setter()`
   (non-batched path) and `endSeqChange()` (changed path), before the emissions.
3. `TODO.md` — remove the persist-in-destructor item.
4. `CLAUDE.md` — ConfigManager bullet: "save in the destructor" → saved on change
   (each `configChanged`-emitting change point), exits write nothing.

## Compatibility

No settings-schema, wire-format, or KDF change; same keys, same values, only the write
timing moves. Peers and existing installs are unaffected.

## Verification

- Build both configs.
- Change a setting via the dialog → check the registry values update immediately
  (`Get-ItemProperty 'HKCU:\Software\SKSoft\Clipboard Sync'`), while the app is still
  running.
- Toggle Disable → `disabled` value updates immediately.
- Kill the app (`Stop-Process -Force`) after a dialog change → relaunch → the change
  survived (crash-safety, goal 1).
- With the app running, inject a value externally (`Set-ItemProperty ... mode ...`),
  then quit the app cleanly via the tray → the injected value is still there (no
  clobber, goal 2).
- Run `testbench/test-all-modes.sh` against the Mac to confirm the bench still passes.
