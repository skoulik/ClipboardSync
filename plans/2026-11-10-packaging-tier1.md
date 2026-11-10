# Plan: packaging tier 1 — Windows and macOS installers (2026-11-10)

Status: **agreed** (2026-11-10) — Sergei's four calls folded in as Q1-Q4; nothing implemented.
Steps run in order, each reviewed before it is committed.

## Goal

A `v*` tag on `main` builds a Windows installer and a macOS DMG in GitHub Actions and attaches
both to a **draft** release. Sergei installs each, checks it, and publishes by hand. A manual
(`workflow_dispatch`) run is a dry run that only attaches the artifacts to the run.

## What changed since TODO.md was written (2026-09-08)

- **Apple membership renewed (2026-09-28), and CI notarisation is proven** — BrokerAI's
  `release.yml` signs and notarises with the current Developer ID pair through an App Store
  Connect API key. TODO.md's "ship unsigned at first" rested on the $99/yr, which is now paid
  anyway. A stable signature also fixes a real annoyance: macOS ties the Local Network and
  Pasteboard prompts to the signature, so every ad-hoc build re-prompts, and inbound multicast
  is dead until the prompt is granted *and* the app restarted.
- **Qt 6.12.0 prebuilt binaries are published** (`msvc2022_64`; macOS universal), the same
  version as the local static builds.
- **The repository is public**, so Actions minutes cost nothing, macOS runners included.
- **Every Windows runner image carries Inno Setup 6.7.1, none 7** (found for BrokerAI).

## Steps

### 0. Identity: SKSoft → skoulik

Sergei's call (2026-11-10), for consistency with the `com.skoulik.*` identifiers of his other
projects.

- `main.cpp`: organization name and domain `skoulik`. The settings move with them: the registry
  key becomes `HKCU\Software\skoulik\Clipboard Sync`, the macOS defaults domain
  `com.skoulik.Clipboard Sync`.
- `AutoStart.cpp`: LaunchAgent label and file `com.skoulik.clipboardsync`. The Windows `Run`
  value is named after the application name and does not change.
- Bundle identifier `com.skoulik.clipboardsync` (it is `com.yourcompany.ClipboardSync` today).
- `testbench/set-mode.sh`: both settings paths.
- **No migration code** (Q1, Sergei: A). Nothing has been released, so the only `SKSoft`
  settings are on his two machines: he re-enters them once, and cleans up with commands given
  at the time — the old registry key, and both old autostart entries, which hold the path of
  the dev build that ticked them:
  - macOS: the old LaunchAgent (`launchctl bootout`, then remove the plist). Its label differs,
    so the new build's checkbox shows unticked while it still launches the dev build at login.
  - Windows: the `Run` value keeps its name, so the installed app's checkbox shows ticked while
    it launches the dev build. Untick and re-tick in the installed app, or delete the value.

### 1. Version

`project(ClipboardSync VERSION x.y.z ...)` is the single source. It feeds the bundle's
`CFBundleShortVersionString`/`CFBundleVersion`, a Windows `VERSIONINFO` resource and the
installer's `AppVersion`. The workflow fails when a `v*` tag disagrees with it.

**The first release is 1.0.0** (Q3, Sergei): the app has been in daily use for years on both
platforms. **The major number changes only with the wire format**, so any two 1.x peers sync;
the README says so, and CLAUDE.md's wire-format paragraph names the bump.

### 2. Icon

**The tray design redrawn as vector art** (Q4, Sergei: A): the same yellow clipboard, dark
clip and green sync arrows, drawn at 1024 px and checked at 16, 32 and 48. Rendered both as a
bare clipboard and on a macOS-style rounded tile, at real sizes, for him to choose from. The
tray PNGs can later be regenerated from the same source; not in this plan. The `.ico`
and `.icns` are generated once and committed, so the build needs no image tooling. Windows gets
it through a `.rc` (beside the `VERSIONINFO`), macOS through `MACOSX_BUNDLE_ICON_FILE`.

### 3. CI build

- `jurplel/install-qt-action` with Qt 6.12.0 — the official dynamic build, since building a
  static Qt per runner would cost hours. The local static single-exe build is unaffected, since
  CMakeLists.txt is agnostic to the Qt flavor.
- macOS: `CMAKE_OSX_ARCHITECTURES="arm64;x86_64"` for one universal app, then `macdeployqt`.
- Windows: `windeployqt --no-translations`, and the MSVC runtime DLLs copied **app-local** from
  the VS redist folder. Not `vc_redist.x64.exe`: running it needs admin, which a per-user
  install does not have.

### 4. Windows installer (Inno Setup)

- Per-user by default, no UAC prompt; the machine-wide choice offered by Inno's own dialog
  (`PrivilegesRequiredOverridesAllowed=dialog`).
- Start-menu shortcut; "Launch Clipboard Sync" on the Finished page.
- The uninstaller removes the `HKCU\...\Run` value, or a stale entry survives pointing at a
  deleted exe. Settings are left in place (as most Windows apps do).
- Machine-wide install only: an inbound UDP firewall rule for the exe (`netsh advfirewall`),
  removed on uninstall. The app's listening socket raises Defender's prompt on first run — easy
  to miss for a tray app started at login, and Rx stays silently dead if it is dismissed. The
  rule needs elevation, so a per-user install still depends on the prompt; the README says so.
- The running app must be closed before files are replaced: Inno's Restart Manager support
  first, to be tested; a `taskkill` fallback if it does not see a tray app.
- Written for the runners' Inno Setup 6.7.1, so the workflow needs no download step; the local
  7.1 compiles it too (to be checked).
- Windows stays **unsigned** (as for BrokerAI): SmartScreen's "Windows protected your PC" →
  *More info* → *Run anyway*, documented in the README.

### 5. macOS DMG

- Sign the app with hardened runtime by SHA-1 (`Developer ID Application`, to 2027-12-09),
  build a DMG with `hdiutil` holding the app and an `Applications` link, sign the DMG, notarise
  it with `notarytool`, staple. **Signed and notarised from the first release** (Q2, Sergei:
  "why not"): he adds BrokerAI's five secrets to this repository (`MACOS_CERT_P12`,
  `MACOS_CERT_PASSWORD`, `APPLE_API_KEY_P8`, `APPLE_API_KEY_ID`, `APPLE_API_ISSUER_ID`).
  Notarising this app is untested until the first dry run.
- A DMG, not a `.pkg`: the `.pkg` + `distribution.xml` machinery in IBKR-to-Xero and BrokerAI
  exists to place files outside the app, and buys nothing for a single `.app`.
- Uninstalling is a drag to the Trash, which leaves the LaunchAgent if autostart was on; the
  README says to untick autostart first.

### 6. Release workflow

`.github/workflows/release.yml`, the shape of BrokerAI's: a `version` job, `windows` and
`macos` jobs uploading artifacts, and on a tag a `release` job creating the draft with a
`SHA256SUMS`.

### 7. README

Download and install for both platforms; the SmartScreen and firewall prompts on Windows; the
Local Network and Pasteboard prompts on macOS, and that the app must be restarted once Local
Network is granted. The Qt runtime now ships beside the app, so a note on Qt's LGPL-3.0 and its
license text goes into both packages.
