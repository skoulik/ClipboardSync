# Plan: persistent disable state, retire blockSignals (2026-07-08)

Status: **implemented** (2026-07-08, in working tree, not yet committed). Implemented by
Developer_Opus; review found one crash bug beyond the plan: the tray `configChanged`
lambda captured the block-local `disable` QAction pointer via `[&]` (dangling stack
reference once the tray-setup block exits — the startup emission then crashed the Release
build with an access violation); fixed by capturing the pointer by value (`[&, disable]`).
Also moved the no-explicit-leave comment to the `m_socketRx.close()` line it explains.
Fixed build verified: launches and stays up. Post-review refinements (2026-07-08):
ClipboardManager's disabled guard restored via a `const ConfigManager&` (skipping the
wasted serialize/hash while disabled) and the icon pick de-duplicated into an `idleIcon`
lambda — see the revised Decisions. Manual verification (peer sync, netsh joins, restart
persistence) still pending.

## Goals

1. Persist the disabled state across restarts — with auto-start in place, an app that was
   disabled must come back disabled.
2. Make "disabled" a first-class config field: stored via `ConfigManager`, emitting
   `configChanged` like every other field.
3. Retire the `blockSignals()` convention. Disabling closes the sockets (which in
   multicast also leaves the group) and no-ops the send path; enabling rebuilds them.
4. The toggler stays in the tray menu for easy access (not in the config dialog).

## Confirmed current behavior

- The tray "Disable" action calls `blockSignals(disabled)` on `DatagramProcessor` and
  `ClipboardManager` (`main.cpp`); each checks `signalsBlocked()` in its handler —
  ClipboardManager returns early, DatagramProcessor drains and discards Rx datagrams.
- The state lives only in the checkable `QAction`; it is lost on restart.
- While disabled, the Rx socket stays bound and (in multicast) joined — traffic still
  arrives and is decrypt-skipped/discarded, and the multicast membership stays active.
- `main()` wires `clbMgr::dataChanged → dgProc::sendDatagram` and
  `dgProc::datagramReceived → clbMgr::setData` unconditionally at startup.
- `DatagramProcessor`'s `configChanged` handler already starts with `m_socketRx.close()`
  and rebuilds sockets from scratch, so "tear down / rebuild on state change" is the
  existing idiom.

## Decisions

- **ConfigManager**: new field `bool m_disabled { false }`, QSettings key `"disabled"`,
  getter `disabled()`, setter `setDisabled(bool)` through the existing `setter()`
  template — so it persists in the destructor and emits `configChanged`, exactly like the
  network fields. It is deliberately *not* in the dialog's `beginSeqChange`/`endSeqChange`
  batch; the tray action is its only writer.
- **Sockets (DatagramProcessor)**: the `configChanged` handler gets an early exit — after
  the existing `m_socketRx.close()`, if `m_confMgr.disabled()` also `m_socketTx.close()`
  and return. Closing the Rx socket implicitly drops the multicast membership, so
  join/leave falls out of the existing setup/teardown path with no extra multicast code.
  Re-enabling emits `configChanged` again and the untouched setup code re-binds/rejoins.
  The `signalsBlocked()` drain-and-discard block in the readyRead handler is removed —
  a closed socket receives nothing.
- **No explicit `leaveMulticastGroup()`** (considered, rejected): membership is
  per-socket OS state (`IP_ADD_MEMBERSHIP` is a socket option) and cannot outlive
  `close()`; the codebase has always relied on close-and-rejoin (every config change, and
  the macOS wake fix). A correct explicit leave would also need the *previously joined*
  group/iface — `m_confMgr` may already hold new values when the handler runs — i.e. new
  member state feeding a redundant call that fails with warnings when mis-timed. Instead,
  eliminate the future question in writing: a one-line comment at the disable early exit
  ("closing the Rx socket also drops the multicast membership") plus the CLAUDE.md
  mention below.
- **Tx guard (DatagramProcessor)**: `sendDatagram` gets `if(m_confMgr.disabled()) return
  false;` as its first line — before the `transmitStateChanged(true)` emit, so the tray
  icon doesn't flash while disabled. This one line is what actually stops sends: a closed
  `QUdpSocket` auto-binds on `writeDatagram`, so the closed Tx socket alone would not.
  The two cross-connections in `main()` stay one-time startup wiring, untouched
  (rejected alternative: disconnect/reconnect them on state change — more code for the
  same effect).
- **Tray action**: stays checkable "Disable" in the tray menu; its `toggled` handler
  shrinks to `confMgr.setDisabled(disabled)` — everything else reacts to `configChanged`.
  A tray-side `configChanged` lambda syncs the UI: `disable->setChecked(confMgr.disabled())`
  and sets the disabled/idle icon. Recursion is safe: `setChecked` on an unchanged value
  doesn't emit `toggled`, and `setDisabled` with an unchanged value doesn't emit
  `configChanged`. The startup `emit confMgr.configChanged()` then also initializes the
  checkbox and icon from the persisted state — no extra startup code.
- **Icon race fix (drive-by)**: the 100 ms `transmitStateChanged` singleShot resets the
  icon to *idle* even if the user disabled within that window; make it pick
  `confMgr.disabled() ? icnDisabled : icnIdle`.
- **ClipboardManager** (revised post-review): the `signalsBlocked()` early return is
  replaced, not just removed — letting the handler run while disabled would serialize and
  hash every clipboard change for nothing. It now takes `const ConfigManager&` at
  construction (the same pattern as DatagramProcessor) and its handler starts with
  `if(m_confMgr.disabled()) return;`. As with the old blockSignals behavior,
  `m_lastDataHash` goes stale while disabled — harmless, the next change after re-enable
  syncs.
- **Icon helper (main.cpp, post-review)**: the disabled/idle icon pick is used in three
  places (initial icon, `configChanged` sync, transmit-reset singleShot), so a local
  `auto idleIcon = [&]{ ... };` de-duplicates it. It is declared next to the `icons`
  array in `main` scope — deliberately not inside the tray block, so `[&]` captures of
  it cannot dangle (see the `disable`-capture crash in Status).
- **CLAUDE.md**: rewrite the tray-"Disable" paragraph (the blockSignals convention is
  gone) and extend the ConfigManager bullet with the `disabled` field and how
  DatagramProcessor reacts (sockets closed, sends no-opped).

## Implementation steps

1. `ConfigManager.h/.cpp` — `m_disabled` member, load/save, `disabled()`,
   `setDisabled(bool)`.
2. `DatagramProcessor.cpp` — early exit in the `configChanged` handler when disabled
   (close both sockets); disabled guard as the first line of `sendDatagram`; delete the
   `signalsBlocked()` block in the readyRead handler.
3. `ClipboardManager.cpp` — delete the `signalsBlocked()` early return.
4. `main.cpp` — replace the blockSignals lambda with `confMgr.setDisabled(disabled)`;
   add the tray `configChanged` lambda (checkbox + icon sync); apply the icon-race fix.
5. `CLAUDE.md` — update as decided above.

## Compatibility

No wire-format or KDF change; peers are unaffected. New QSettings key `"disabled"`
(default false) — older builds simply ignore it; existing installs start enabled, as
today.

## Out of scope

`ConfigManager` still persists only in its destructor (tracked in TODO.md), so a crash
loses a just-toggled disable state like it loses any other setting. The save-on-change
redesign stays a separate task.

## Verification

- Build both configs inside the VS dev env.
- Manual, Windows against a peer: disable → copy on peer → nothing lands locally, copy
  locally → nothing lands on peer; re-enable → both directions resume without restart.
- Multicast mode: while disabled, confirm the group membership is gone
  (`netsh interface ipv4 show joins` no longer lists the group on the selected
  interface); re-enable → listed again, sync works.
- Persistence: disable → quit → relaunch → starts with disabled icon, checked menu item,
  no sync; enable → works.
- Wake path: disable → sleep/wake (or emit configChanged via a config OK) → still
  disabled, sockets stay closed.
