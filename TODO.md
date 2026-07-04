# TODO

- DatagramProcessor reports socket setup failures via modal `QMessageBox::critical`, which blocks
  the rest of the socket setup until dismissed. For a tray app that usually starts unattended,
  a tray notification plus a delayed retry would fail less silently.
