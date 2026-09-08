# TODO

- DatagramProcessor reports socket setup failures via modal `QMessageBox::critical`, which blocks
  the rest of the socket setup until dismissed. For a tray app that usually starts unattended,
  a tray notification plus a delayed retry would fail less silently.
- ClipboardManager's echo suppression assumes `serialize(deserialize(data))` is byte-identical to
  `data` (QMimeData preserving `text/*` format order, no normalization). Holds today; if a future
  Qt breaks it, every received datagram echoes back to the sender. Investigate/guard if that
  symptom ever appears.
- Fragmented sends no longer pump the event loop between datagrams (removed 2026-07; that only
  mattered when sent datagrams were immediately fed back to the same host, which
  `MulticastLoopbackOption = 0` avoids). Investigate whether Rx starvation can still occur in the
  other modes and document; the proper long-term answer is async send.
- WakeNotifier performs a single rejoin 5 s after wake; if the network takes longer to come up
  (VPN, slow Wi-Fi association) the rejoin misses and sync stays broken until the next wake or
  config change. Consider a bounded retry.

## Packaging, tier 1 — Windows and macOS installers

- No version exists anywhere in the project. Add `VERSION` to `project()` in CMakeLists.txt as the
  single source of truth, and have CI fail when a `v*` tag disagrees with it, so an installer can
  never misreport the version it claims.
- There is no application icon — only the three 32x32 tray PNGs, which upscale badly to the 256x256
  that an installer, an `.exe` resource and an `.icns` all want. Draw a new icon at native sizes,
  following the tray icon's design, and derive `.ico` and `.icns` from it.
- Add `.github/workflows/release.yml` mirroring IBKR-to-Xero's: a `v*` tag builds into a *draft*
  release, `workflow_dispatch` is a dry run that only attaches artifacts to the run. Nothing
  publishes automatically.
- CI builds against the official prebuilt (dynamic) Qt via `install-qt-action`, with
  `windeployqt`/`macdeployqt` bundling the runtime — building a static Qt per runner would cost
  hours. CMakeLists is already agnostic to the Qt flavor, so the local static single-exe build is
  unaffected.
- Windows installer: Inno Setup, single component, per-user by default (no UAC prompt) with the
  machine-wide choice offered. The uninstaller must remove the `HKCU\...\Run` value `AutoStart`
  writes, or a stale entry survives pointing at a deleted executable.
- Windows firewall: the app binds a listening UDP socket, so first run raises the Defender prompt —
  easy to miss for a tray app launched at login, and Rx stays silently dead if it is dismissed. Have
  the installer add an inbound UDP rule. It needs elevation, so it can only accompany the
  machine-wide install choice; a per-user install still depends on the prompt.
- macOS: a single universal DMG with drag-to-Applications. Official Qt macOS binaries are universal,
  so one runner covers Intel and Apple Silicon; the `.pkg` + `distribution.xml` machinery used in
  IBKR-to-Xero exists to place a CLI in `/usr/local/bin` and buys nothing for a single `.app`.
- Ship unsigned at first and document the SmartScreen and Gatekeeper workarounds in the README —
  certificates cost real money (~$99/yr Apple, plus a Windows OV cert). Signing and notarization are
  a self-contained later addition (workflow secrets and a few extra steps), not a reason to delay.

## Packaging, tier 2 — Linux

Ordered deliberately: the first item decides whether the rest is worth doing.

- Verify whether clipboard monitoring works under Wayland at all. Wayland only lets a *focused*
  surface read the clipboard, which a background tray app never is, and the macOS poll-timer
  workaround does not help — the reads are refused rather than merely unsignalled. Clipboard
  managers use the `wlr-data-control` protocol, which KDE implements and GNOME does not. If this
  holds, Linux support is X11-only, and that has to be stated up front rather than discovered.
- GNOME has no StatusNotifierItem host without the AppIndicator extension, so `QSystemTrayIcon`
  gives no UI at all on a stock Ubuntu or Fedora desktop — the app would start and show nothing.
  Establish what has to be required of the user.
- `AutoStart` is an empty stub on Linux. Add a branch writing
  `~/.config/autostart/clipboardsync.desktop`, mirroring the macOS LaunchAgent branch.
- `WakeNotifier` is an empty stub on Linux. Add a branch listening for logind's `PrepareForSleep`
  signal on `org.freedesktop.login1`, which pulls in `Qt6::DBus` as a new dependency.
- Linux installer: AppImage via `linuxdeploy` and its Qt plugin — one file, no root, no package
  manager, matching the download-and-run shape of the other two. Build on the oldest distro to be
  supported, since glibc sets the floor. Distro `.deb`/`.rpm` rejected: they link the distro's own
  Qt and so need a package per release, a matrix that never stops growing.
