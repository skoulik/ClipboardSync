# TODO

- DatagramProcessor reports socket setup failures via modal `QMessageBox::critical`, which blocks
  the rest of the socket setup until dismissed. For a tray app that usually starts unattended,
  a tray notification plus a delayed retry would fail less silently.
- Multicast reception was observed to stop on macOS after sleep/wake (with the Qt 5 build; under
  soak test whether Qt 6 behaves the same). Suspected cause: the interface reset on wake drops the
  socket's group membership, and sockets are only (re)configured on `configChanged`. Likely fix:
  detect wake/interface change (e.g. `QNetworkInformation`) and rerun the socket setup to rejoin.
