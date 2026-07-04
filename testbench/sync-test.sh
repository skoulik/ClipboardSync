#!/usr/bin/env bash
# Bidirectional clipboard sync test between the local (Windows) and remote (macOS) instances.
set -u
cd "$(dirname "$0")"
source ./config.sh

remote() { ssh -i "$REMOTE_KEY" -o BatchMode=yes "$REMOTE_USER@$REMOTE_HOST" "$@"; }
fail=0

token="tb-l2r-$RANDOM$RANDOM"
powershell.exe -NoProfile -Command "Set-Clipboard '$token'"
sleep "$SYNC_WAIT"
got=$(remote pbpaste)
if [ "$got" = "$token" ]; then echo "PASS  local -> remote"
else echo "FAIL  local -> remote: expected '$token', got '$got'"; fail=1; fi

token="tb-r2l-$RANDOM$RANDOM"
remote "printf %s '$token' | pbcopy"
sleep "$SYNC_WAIT"
got=$(powershell.exe -NoProfile -Command Get-Clipboard | tr -d '\r\n')
if [ "$got" = "$token" ]; then echo "PASS  remote -> local"
else echo "FAIL  remote -> local: expected '$token', got '$got'"; fail=1; fi

exit $fail
