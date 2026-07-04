#!/usr/bin/env bash
# State of both instances: process, UDP sockets, multicast membership.
set -u
cd "$(dirname "$0")"
source ./config.sh

echo "=== local (Windows) ==="
powershell.exe -NoProfile -Command "Get-Process ClipboardSync -ErrorAction SilentlyContinue | Select-Object Id, StartTime | Format-Table -AutoSize"
netstat -ano -p udp | grep ":$PORT " || echo "no socket on port $PORT"
netsh interface ipv4 show joins | grep "$GROUP" || echo "no membership in $GROUP"

echo "=== remote (macOS) ==="
ssh -i "$REMOTE_KEY" -o BatchMode=yes "$REMOTE_USER@$REMOTE_HOST" \
    "pgrep -x ClipboardSync || echo 'not running';
     lsof -nP -i udp -a -c ClipboardSync;
     netstat -g -n | grep '$GROUP' || echo 'no membership in $GROUP'"
