# macOS post-sleep multicast drop — investigation report (2026-07-04)

Setup: ClipboardSync (Qt 6 build) running on Windows (192.168.1.51, WiFi) and macOS/arm64
(192.168.1.55, en0), Multicast mode, group 239.255.1.1:19211. After a ~18 min sleep/wake cycle
on the Mac: Mac→Win sync kept working, Win→Mac stopped. Investigated live without restarting
either instance.

## Evidence chain

1. Post-wake state: the Mac app was alive with both sockets intact, `netstat -g` still listed
   `239.255.1.1` on `en0`, and en0 kept its IP. Windows was equally healthy — bound, joined,
   transmitting. Both endpoints believed nothing was wrong.
2. A Python sniffer on the Mac performed a *fresh* group join at 13:36:19 — and five seconds later
   the Windows datagram arrived. More telling: the **app itself** simultaneously received and
   applied it (`wake-probe-A-1111` appeared on the Mac pasteboard). The app was never broken;
   delivery to the whole host was.
3. After the sniffer exited, sync kept working (`wake-probe-B-2222`) — one fresh IGMP join durably
   repaired the path.

## Root cause

While the Mac sleeps it can't answer IGMP membership queries, so the AP/switch IGMP-snooping table
prunes it from the group. On wake, macOS never re-announces membership (the kernel thinks it's
still joined, so there's no error anywhere), and on a typical home LAN there's no active querier to
trigger self-healing — the pruned state persists indefinitely. Sending is unaffected, hence the
exact Mac→Win-works / Win→Mac-dead asymmetry. This also explains why the Qt 5 build had the same
symptom — it's not a Qt issue at all.

## Fix directions considered

- Periodic membership refresh in DatagramProcessor: a ~60 s QTimer doing `leaveMulticastGroup` +
  `joinMulticastGroup` on the existing Rx socket (Multicast mode only). Heals any pruning cause
  within a minute; no platform-specific code.
- Rejoin on a system wake notification (platform-specific) — no timer, but wake fires before Wi-Fi
  re-associates, so the rejoin must be delayed or paired with a reachability signal.
- `QNetworkInformation` reachability alone is unreliable here: `pmset -g log` showed DarkWake
  cycles keeping TCP alive, so wake may not flap reachability at all.
