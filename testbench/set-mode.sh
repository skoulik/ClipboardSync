#!/usr/bin/env bash
# Switch both instances to the given operating mode (rewrites their stored settings,
# leaving port/password untouched) and force-restarts them.
set -eu
cd "$(dirname "$0")"
source ./config.sh

case "${1:-}" in
    broadcast) mode=0; laddr=$BCAST_ADDR; raddr=$BCAST_ADDR ;;
    multicast) mode=1; laddr=$GROUP;      raddr=$GROUP ;;
    unicast)   mode=2; laddr=$REMOTE_IP;  raddr=$LOCAL_IP ;;
    *) echo "usage: $0 broadcast|multicast|unicast"; exit 2 ;;
esac

powershell.exe -NoProfile -Command "
    Stop-Process -Name ClipboardSync -Force -Confirm:\$false -ErrorAction SilentlyContinue;
    \$k = 'HKCU:\Software\SKSoft\Clipboard Sync';
    Set-ItemProperty \$k mode $mode -Type DWord;
    Set-ItemProperty \$k addr '$laddr';
    Set-ItemProperty \$k ifaceName '$LOCAL_IFACE';
    Start-Sleep 1;
    Start-Process '$LOCAL_APP'"

ssh -i "$REMOTE_KEY" -o BatchMode=yes "$REMOTE_USER@$REMOTE_HOST" \
    "pkill -x ClipboardSync 2>/dev/null; sleep 1;
     D='com.sksoft.Clipboard Sync';
     defaults write \"\$D\" mode -int $mode;
     defaults write \"\$D\" addr -string $raddr;
     defaults write \"\$D\" ifaceName -string $REMOTE_IFACE;
     open $REMOTE_REPO/build/default/Release/ClipboardSync.app"

echo "both instances restarted in $1 mode"
