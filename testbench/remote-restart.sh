#!/usr/bin/env bash
# Relaunch the remote app in the Mac GUI session and show its sockets.
set -eu
cd "$(dirname "$0")"
source ./config.sh

ssh -i "$REMOTE_KEY" -o BatchMode=yes "$REMOTE_USER@$REMOTE_HOST" \
    "pkill -x ClipboardSync 2>/dev/null; sleep 1;
     open $REMOTE_REPO/build/default/Release/ClipboardSync.app && sleep 3 &&
     lsof -nP -i udp -a -c ClipboardSync"
