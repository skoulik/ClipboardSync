# Two-machine testbench

Scripts for validating clipboard sync between two live instances: a Windows machine acting as
the test driver and a macOS machine driven over SSH. Distilled from the manual verification of
the Qt 6 port (see `debugging/`).

## Setup

- Copy `config.template.sh` to `config.sh` (gitignored) and fill in the machine details.
- SSH key authentication to the Mac account must work non-interactively.
- The Mac account needs a logged-in GUI session — the tray app and the pasteboard don't work
  without one. `pbcopy`/`pbpaste` over SSH share that session's pasteboard.
- Both instances must be configured identically (mode/address/port/password) via the Config
  dialog and be running.
- macOS TCC: on first launch the app prompts for Local Network and Pasteboard access on the
  Mac's screen. Every rebuild changes the ad-hoc code signature, so the Local Network prompt
  reappears on the next launch — until it is granted (and the app restarted), inbound multicast
  fails silently while outbound keeps working.

## Scripts

Run from Git Bash on the Windows machine:

- `sync-test.sh` — bidirectional sync check with random tokens; exits non-zero on failure.
  Overwrites the clipboard on both machines.
- `status.sh` — processes, UDP sockets and multicast group membership on both machines.
- `remote-build.sh` — configure + build the Release app on the Mac's repo clone.
- `remote-restart.sh` — relaunch the app in the Mac's GUI session and show its sockets.

Sleep/wake regression check (the scenario WakeNotifier fixes): put the Mac to sleep with
`pmset sleepnow` over SSH, wake it manually after ~10 minutes, then run `sync-test.sh`.
