# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Working conventions

- Follow the existing coding style (brace placement, aligned member initializers, compact lambda-heavy wiring, `m_` member prefix).
- Evolve the original design decisions rather than redesigning; the architecture below is deliberate.
- Keep the code minimalistic and easily graspable by a human reader — some design choices exist specifically to serve this goal. Prefer the smallest change that fits.
- Minimal comments: comment only truly tricky parts or unusual decisions. All other meta information belongs in CLAUDE.md or other meta-artifacts, not in the code.

## Project

ClipboardSync is a Qt Widgets system-tray app (C++17) that synchronizes clipboard contents between machines/user accounts over LAN via encrypted UDP. All source files live in the repository root; the qmake project file `ClipboardSync.pro` is the source of truth for the build.

## Build

The tracked build entry point is `build.cmd`, which generates a Visual Studio project from the .pro file:

```
build.cmd    # runs vcvarsall amd64, then: qmake -tp vc -spec win32-clang-msvc (ClangCL toolset)
```

It requires two environment variables: `VS160COMNTOOLS` (Visual Studio 2019) and `QT2020_64` (Qt installation root). Build the result with `msbuild ClipboardSync.sln` or from the VS IDE. A plain `qmake ClipboardSync.pro` + make/nmake also works.

Generated artifacts (`ClipboardSync.sln`, `*.vcxproj*`, `Makefile`, `debug/`, `release/`, `.qmake.stash`, `.vs/`, `ClipboardSync.si4project/`) are untracked — never commit them, and never edit the .vcxproj directly; change `ClipboardSync.pro` and regenerate.

`thirdparty/tiny-AES-c` is a git submodule (run `git submodule update --init` after a fresh clone). Its `aes.c` is compiled directly into the app; the cipher variant is selected by `DEFINES += AES256=1 ECB=0 CTR=0` in the .pro file.

There is no test suite and no linter configured.

## Architecture

`main.cpp` constructs four components and wires them together with Qt signals — there is no other coupling between them:

- **ClipboardManager** — watches `QClipboard::dataChanged` (on macOS: a 1s poll timer instead, since that signal doesn't fire there) and serializes only `text/*` MIME formats into a `QByteArray` (QDataStream of format/data pairs). Emits `dataChanged(bytes)`; `setData(bytes)` writes received data back to the clipboard. Echo loops are prevented by comparing `qHash` of the serialized payload against the last seen hash.
- **DatagramProcessor** — UDP transport with separate Tx/Rx sockets. Splits payloads into MTU-sized fragments, each carrying a 12-byte big-endian header (`magic 0xB81E`, fragment count/index, random transfer id). Reassembles incoming fragments per `{sender address, transfer id}` key with a 10 s idle timeout. Supports Broadcast, Multicast (binds/joins per selected interface), and Unicast modes; sockets are (re)configured in response to `ConfigManager::configChanged`. Tx pumps `QApplication::processEvents()` between fragments so the Rx queue isn't starved (known TODO: async send).
- **CryptoEngine** — static-only wrapper over tiny-AES-c. The master key is PBKDF2-SHA256 of the password (125328 iterations); each datagram's key is derived by `bindContext(masterKey, header bytes)` (a cheap 13-iteration PBKDF2), so the header acts as associated data. Payloads are AES-256-CBC with PKCS#7 padding and a random IV *appended* after the ciphertext. Note the wire header is big-endian but `bindContext` hashes the raw in-memory struct (known TODO in DatagramProcessor.cpp re: endianness — changing this breaks compatibility between peers).
- **ConfigManager / ConfigDialog** — settings (interface, mode, address, port, MTU, password hash) load from `QSettings` at startup and save in the destructor. Setters emit `configChanged`; `beginSeqChange()`/`endSeqChange()` batch multiple setters into one emission. `main()` emits `configChanged` manually once at startup to trigger the initial socket setup.

The tray "Disable" action works by calling `blockSignals()` on ClipboardManager and DatagramProcessor; both check `signalsBlocked()` inside their handlers to skip work (Rx datagrams are drained and discarded), so any new event-driven code in these classes should respect the same convention.

Wire format per datagram: `[12-byte header][ciphertext][16-byte IV]`. Any change to the header, KDF parameters, or serialization format breaks interoperability with peers running older builds.
