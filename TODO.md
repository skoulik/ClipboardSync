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

## Packaging, tier 1 — Windows and macOS installers

In progress: `plans/2026-11-10-packaging-tier1.md`.

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
- `NetworkNotifier` is an empty stub on Linux. Add a branch listening for logind's
  `PrepareForSleep` signal on `org.freedesktop.login1`, which pulls in `Qt6::DBus` as a new
  dependency, and for `RTM_NEWADDR` on a netlink socket.
- Linux installer: AppImage via `linuxdeploy` and its Qt plugin — one file, no root, no package
  manager, matching the download-and-run shape of the other two. Build on the oldest distro to be
  supported, since glibc sets the floor. Distro `.deb`/`.rpm` rejected: they link the distro's own
  Qt and so need a package per release, a matrix that never stops growing.
