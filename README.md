# ClipboardSync

A small system-tray application that synchronizes clipboard contents between machines over the local network. Copy text on one computer, paste it on another. There is no server and no pairing: every running instance configured with the same network settings and password shares the clipboard.

Built with Qt 6 (C++17).

## Features

- Synchronizes all textual clipboard formats (`text/*` MIME types)
- Encrypted transport: AES-256-CBC with keys derived from a shared password via PBKDF2-SHA256
- UDP delivery in Broadcast, Multicast or Unicast mode
- Payloads larger than the configured datagram size are fragmented and reassembled

## Usage

Run ClipboardSync on every machine that should share the clipboard, then right-click the tray icon and open *Config…*:

- **Network Interface** — the interface facing your LAN; selecting it suggests a matching destination address
- **Operating Mode / Destination Address** — Broadcast (e.g. `192.168.1.255`), Multicast (e.g. `239.255.1.1`), or Unicast (the peer's address)
- **Port** — UDP port shared by all peers
- **Max datagram size** — keep at 1500 unless your network's MTU differs
- **Password** — the shared secret; encryption keys are derived from it

All peers must use the same mode, address, port and password (the port also salts the password hash, so a port change requires re-entering the password everywhere). The *Disable* menu entry pauses both sending and receiving.

## Build

Requires Qt 6 (Widgets + Network), CMake 3.23+ and Ninja. After a fresh clone:

```
git submodule update --init
cmake --preset default
cmake --build --preset release
```

Set the `CMAKE_PREFIX_PATH` environment variable to the Qt installation prefix (or put machine-specific settings in an untracked `CMakeUserPresets.json`). On Windows, run from a Visual Studio developer prompt so the compiler and the VS-bundled `cmake`/`ninja` are available.

The AES implementation is [tiny-AES-c](https://github.com/kokke/tiny-AES-c), compiled in directly from a git submodule.

## License

Public domain — see [LICENSE](LICENSE) (Unlicense). tiny-AES-c is public domain as well.
