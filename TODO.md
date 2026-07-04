# TODO

- DatagramProcessor reports socket setup failures via modal `QMessageBox::critical`, which blocks
  the rest of the socket setup until dismissed. For a tray app that usually starts unattended,
  a tray notification plus a delayed retry would fail less silently.
- Multicast reception stops on macOS after sleep/wake (reproduced with Qt 6, diagnosed 2026-07:
  the kernel still lists the group membership and the socket stays healthy, but group traffic is
  no longer delivered to the host — the AP's IGMP snooping prunes the sleeping Mac and nothing
  rejoins on wake; sending is unaffected). One fresh `IP_ADD_MEMBERSHIP` from any process on the
  host restores delivery for all member sockets. Fix: periodically refresh the membership
  (leave + rejoin on the Rx socket, Multicast mode only), or rejoin on a wake/reachability signal.
