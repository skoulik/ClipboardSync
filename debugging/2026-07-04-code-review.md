# Code review — full codebase after the Qt 6/CMake port (2026-07-04)

Scope: all application sources, WakeNotifier, CMake files, testbench scripts, at commit
`38527bc`. Reviewed against the repo's stated philosophy (minimal, human-graspable, evolve
don't redesign); suggestions are framed within it. The two external facts relied on — OWASP's
PBKDF2 guidance and tiny-AES-c's maintenance status — were verified online 2026-07-04; sources
inline.

## Executive summary

The codebase is in good shape for its purpose and the port is faithful: no port-introduced
regressions were found. The design decisions (signal-only coupling, blockSignals() as the
disable mechanism, header-bound per-datagram keys) hold up. The most defensible real defect
is in PKCS#7 padding validation (`pad == 0` is accepted), which lets ~1/256 of random/forged
datagrams through to the clipboard-set path. A handful of latent risks are inherited from the
Qt 5 era and are either unreachable through the UI (MTU underflow) or benign in practice
(dangling reference to a capture-less lambda, qHash truncation). The crypto is honest about
being LAN-grade; its two real weaknesses under that threat model are replayability and
first-block malleability, both fixable only by breaking wire compatibility.

## Bugs

### B1. PKCS#7 validation accepts `pad == 0` — CryptoEngine.cpp:49-52 (pre-port)
`uint8_t pad = data.back();` followed by `data.endsWith(QByteArray(pad, pad))`. For
`pad == 0`, `QByteArray(0, 0)` is empty and `endsWith(empty)` is true, so `chop(0)` runs and
`decrypt` returns success. Valid PKCS#7 pad is 1..16; 0 must be rejected. Consequence: a
forged or corrupted datagram whose garbage plaintext ends in 0x00 (~1/256 of random inputs)
passes the only integrity check and reaches `ClipboardManager::setData` →
`deserialize` — usually yielding an empty QMimeData that **clears the user's clipboard**, or,
with more luck for the attacker, garbage text formats. An on-LAN attacker without the password
can spam random datagrams (only the cleartext magic is checked before decrypt) and degrade the
clipboard. Fix is one condition (`pad == 0 || pad > AES_BLOCKLEN → reject`), local-only, no
wire impact.

### B2. Ineffective `maxPayload <= 0` guard — DatagramProcessor.cpp:161-163 (pre-port)
`const quint32 maxPayload = m_confMgr.mtu() - 20 - 8 - sizeof(Header) - 16 - 16;` — the
arithmetic promotes through `size_t` (because `sizeof`), so an `mtu() < 72` wraps to a huge
unsigned value, and `if(maxPayload <= 0)` on an unsigned type is only true for exactly 0 —
the guard cannot catch the wrap it was written for. Unreachable through ConfigDialog (spin
box minimum 128), but reachable via direct registry/defaults injection (which the testbench
now does). If triggered: one oversized `writeDatagram` that either IP-fragments or fails.
Fix: compute in a signed type, or guard `mtu() <= 72` explicitly before the subtraction.
Local-only, no wire impact. Verify by injecting `mtu=64` and watching for a single giant
datagram instead of the intended early return.

## Latent risks

### L1. Dangling reference to a local lambda in two connections — ConfigDialog.cpp:71,112 (pre-port)
`getIfaceAddr` is a local `auto` lambda in the constructor; both `connect(&m_modeCb, ...)` and
`connect(&m_ifaceMenu, ...)` capture it by reference (`[this, &getIfaceAddr]`). The connections
outlive the constructor, so every later invocation dereferences a reference to a destroyed
object — formally UB. It works in practice **only** because `getIfaceAddr` is capture-less
(stateless): calling `operator()` touches no member storage, so the dangling reference is never
actually read. Robust fix: capture it by value (it is stateless, so the copy is free), or make
it a `static` function. Worth doing because the "it's fine" reasoning silently breaks the moment
anyone adds a capture to that lambda.

### L2. qHash truncation to `uint` — ClipboardManager.h:16, .cpp:40,55,70 (port-exposed)
`m_lastDataHash` is `uint` (32-bit) but Qt 6 `qHash(QByteArray)` returns `size_t` (64-bit on
these targets). The assignment narrows. Harmless today: all producers and the comparator narrow
identically and all comparisons happen within one process, so echo suppression still works.
Only a correctness *smell*, not a bug. If touched, change the member to `size_t`. (In Qt 5
qHash already returned `uint`, so the port is what introduced the width mismatch.)

### L3. QDataStream default version across peers — ClipboardManager.cpp:10,21 (port-exposed)
`serialize`/`deserialize` construct `QDataStream` without `setVersion()`, so they use the
compiled-in default, which differs between the 6.11 Mac peer and the 6.12 Windows peer. The
wire encoding of `QString`/`QByteArray` has not changed across Qt 6.x, so this is currently
harmless, but it is an unpinned cross-version dependency in the one place that crosses the wire
(the clipboard payload; the datagram header is hand-rolled big-endian and unaffected). Cheap
insurance: `ds.setVersion(QDataStream::Qt_6_0)` in both functions. Note: this is only latent —
it does not currently misbehave. This is arguably the highest-value hardening item because it
touches interoperability, though it does not *break* existing compatibility (both ends already
emit identical bytes).

### L4. Echo suppression assumes serialize∘deserialize is byte-identical — ClipboardManager.cpp:70 vs 54 (pre-port)
The clipboard handler hashes `serialize(clipboardMime)`; `setData` hashes the raw received
`data`. Suppression of the set→dataChanged echo relies on `serialize(deserialize(data)) == data`
byte-for-byte, so that after `setMimeData` the re-serialized clipboard hashes equal to the
pre-stored value. This holds as long as `QMimeData` preserves format insertion order and does no
normalization for `text/*` — true in practice, but an implicit invariant with no guardrail. If a
future Qt reorders formats, every received datagram would echo back to the sender. Document the
invariant; no change needed now.

### L5. Reentrant `sendDatagram` via `processEvents()` — DatagramProcessor.cpp:192 (pre-port)
`QApplication::processEvents()` between fragments can dispatch a queued `QClipboard::dataChanged`,
whose slot is directly connected to `sendDatagram` (main.cpp:68, same thread) — so a rapid
clipboard change mid-send re-enters `sendDatagram`. State survives it (`m_transferIdCounter++`
hands the nested call a distinct id; sockets are synchronous), but the nested send flips
`transmitStateChanged(false)`/icon while the outer send is still running, and pathological
clipboard churn could grow the stack. The existing "use async send instead" TODO already covers
the real fix; note here that the reentrancy is the concrete reason to do it.

## Design observations

- **Signal-only coupling (main.cpp)** — holds up well; the WakeNotifier addition slotted in as a
  single `connect` reusing the existing `configChanged` socket-rebuild path, which is exactly the
  extensibility this design promises. Keep.
- **`blockSignals()` as the Disable mechanism** — verified consistent: ClipboardManager's handler
  and DatagramProcessor's `readyRead` both check `signalsBlocked()` (drain-and-discard on Rx), and
  the send path is gated because `dataChanged` is blocked at the source. One wrinkle: WakeNotifier
  → `configChanged` still reconfigures sockets while disabled. Harmless (Rx is drained anyway), but
  it means "Disable" doesn't stop socket churn on wake. Not worth changing.
- **Settings persisted in ConfigManager's destructor** — a graceful quit overwrites whatever is in
  QSettings, which is why the testbench must force-kill before injecting a mode. Fragile pattern
  (crash = no save; external edits lost on clean exit), but deliberate and fine for the app.
- **No replay protection (crypto)** — a captured datagram replays successfully: the receiver
  re-derives the key from the header and sets the clipboard again. Under the LAN threat model this
  is low-severity (worst case: stale clipboard content reappears). Fixing it needs a nonce/counter
  the receiver tracks — a wire change. Note, don't necessarily fix.
- **`setData`'s `emitDataChanged` parameter is dead — ClipboardManager.cpp:68,73** — the only caller
  (main.cpp:69) never passes it, so it is always `false` and the `if(!emitDataChanged)` branch
  always runs. The parameter and the header's `bool emitDataChanged = false` default are unused
  surface. Safe to delete both; pure simplification, no behavior change. (Pre-port.)

## Crypto assessment (LAN threat model)

Honest and appropriately scoped. IV uses `QRandomGenerator::system()` (CSPRNG) — correct. Master
key PBKDF2-HMAC-SHA256 at 125328 iterations; per-datagram key via 13-iteration PBKDF2 over the
master key salted by the header bytes (header = associated data). IV appended after ciphertext is
unusual but fully functional (`decrypt` reads `right(AES_BLOCKLEN)` then chops).

- **Iteration count** — 125328 is below current OWASP guidance for PBKDF2-HMAC-SHA256 (600000 per
  the [OWASP Password Storage Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html);
  older guidance said 310000). For this design the real protection is password entropy and the fact
  that an offline attack first requires capturing datagrams; still, bumping the count is cheap.
  **Breaks wire compatibility** (changes the master key → all peers must upgrade together).
- **No authentication** — integrity rests solely on PKCS#7 validity (see B1) plus the key being
  password-derived. No HMAC/AEAD. An attacker without the password cannot forge meaningful
  plaintext (bit-flips yield random plaintext that fails padding ~255/256 of the time), so this is
  acceptable for LAN. A "proper" fix is AES-GCM or encrypt-then-MAC — a wire change; not
  recommended given the threat model, but the tradeoff should be a conscious one.
- **Port-as-salt** — not a per-user salt but a domain separator: peers must derive the same key, so
  a shared, non-secret salt is inherent to the design. Consequence: no defense against a precomputed
  dictionary attack targeting a known port+password. Low concern; leave as is.
- **tiny-AES-c** — public-domain, simple, effectively feature-complete rather than actively
  maintained (verified 2026-07: [repo](https://github.com/kokke/tiny-AES-c) dormant, last push over
  a year ago); table-based, not constant-time / not side-channel resistant. Fine for LAN clipboard
  sync; would be disqualifying for a network-facing service. No action.

## Nitpicks

- **N1. `Q_DECLARE_METATYPE(QHostAddress)` — ConfigDialog.cpp:10 (port-redundant).** `QHostAddress`
  is already a declared metatype in Qt 6 (`qhostaddress.h`). The re-declaration is redundant; the
  macro is defined to tolerate repetition, so it is harmless. Removable.
- **N2. No-tray fallback keeps running headless — main.cpp:63-66.** If the system tray is
  unavailable, the app shows a critical box and then runs with no UI and no way to quit (orphan
  process). Edge case on desktop; consider quitting after the message box.
- **N3. WakeNotifier `void* m_impl` (WakeNotifier.h:13).** Minimal PIMPL to keep `windows.h`/AppKit
  out of the header — a reasonable, if slightly blunt, choice; fine as is. The `.mm` relies on
  manual `retain`/`release` (MRC), which is correct only because ARC is not enabled in the build.
  If anyone adds `-fobjc-arc`, that file breaks. Worth a one-line note in CLAUDE.md rather than a
  code change.
- **N4. Fixed 5 s wake delay, no retry (WakeNotifier.h:12).** If the network takes longer than 5 s
  to come back after wake (VPN, slow Wi-Fi association), the single rejoin can still miss and sync
  stays broken until the next wake or config change. Acceptable minimal behavior; a bounded retry
  would be the natural evolution if it proves flaky in the soak test.
- **N5. Testbench: mode switch depends on force-kill ordering.** `set-mode.sh` relies on
  `Stop-Process -Force` / `pkill` completing before relaunch so ConfigManager's destructor doesn't
  re-persist stale settings. The `sleep 1` after kill is a race in principle; fine in practice.

## Prioritized action items

Small, self-contained; **[WIRE]** marks anything that breaks interop with older peers.

1. **B1 — reject `pad == 0` (and `pad > AES_BLOCKLEN`) in `CryptoEngine::decrypt`.** One condition;
   highest-value correctness fix; no wire impact. Local test: feed a datagram that decrypts to a
   0x00-terminated block and confirm rejection.
2. **B2 — make the `maxPayload` MTU guard effective** (signed compute or explicit `mtu() <= 72`
   check). No wire impact. Testbench can now inject a tiny MTU to verify.
3. **L1 — capture `getIfaceAddr` by value (or make it `static`)** in the two ConfigDialog
   connections. Removes the UB. No wire impact.
4. **L3 — pin `QDataStream` version** (`Qt_6_0`) in `serialize`/`deserialize`. Cheap
   cross-version insurance; does not break current compatibility. No wire impact.
5. **Simplify — drop the dead `emitDataChanged` parameter** from `ClipboardManager::setData`. Pure
   cleanup. No wire impact.
6. **N1 — remove the redundant `Q_DECLARE_METATYPE(QHostAddress)`.** No wire impact.
7. **L2 — widen `m_lastDataHash` to `size_t`** (optional tidiness). No wire impact.
8. **Crypto (optional, decide consciously) — raise PBKDF2 iterations toward ~600000.** **[WIRE]** —
   requires all peers to upgrade together. Only if you're willing to break compat on the next
   deploy; otherwise leave and document.

Items 1–7 are non-breaking and can ship together. Item 8 is the only wire-compatibility hazard and
should be bundled with any *other* future wire change (auth, replay protection) rather than spent
alone.

## What is fine and should NOT be changed

- The signal-only wiring in `main.cpp` and the `blockSignals()` disable mechanism — deliberate,
  consistent, and the basis for how new components attach. Don't refactor into observers/managers.
- `CryptoEngine` being static-only, IV-after-ciphertext, header-as-associated-data key binding —
  all functional and intentional; changing any of them is a wire break for no benefit.
- The AES define scoping in CMake (the `set_source_files_properties` split) — it is a real macOS
  header-collision workaround, not gratuitous complexity. The one-line comment must stay.
- The CMake `clang_rt` block and the CRT-agnostic setup — environment glue that is inert off this
  machine; leave it.
- `Qt::Uninitialized` fragment allocation + `replace()`, and `QByteArray::fromRawData` over the
  header — valid, deliberate micro-optimizations under Qt 6.
- WakeNotifier's overall shape (per-platform TU behind a single signal, `void*` impl) — matches the
  minimalist idiom; only the small notes in N3/N4 apply.
- The Header endianness TODO — leave as documented. Both current peers are little-endian (arm64
  Mac, x64 Windows), so hashing the raw in-memory struct in `bindContext` agrees between them; the
  BE/LE mismatch only matters for a hypothetical big-endian peer, which would additionally have to
  match the raw-struct hashing, not just the big-endian wire fields. Do not "fix" it speculatively.