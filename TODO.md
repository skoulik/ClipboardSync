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
- ConfigManager persists settings only in its destructor: a crash loses changes and a clean exit
  overwrites externally injected settings (the testbench must force-kill around this). Redesign to
  save when settings change instead.
- WakeNotifier performs a single rejoin 5 s after wake; if the network takes longer to come up
  (VPN, slow Wi-Fi association) the rejoin misses and sync stays broken until the next wake or
  config change. Consider a bounded retry.
