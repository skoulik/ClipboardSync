#!/usr/bin/env bash
# Run the sync test in every operating mode; ends with multicast configured.
set -u
cd "$(dirname "$0")"
source ./config.sh

result=0
for mode in broadcast unicast multicast; do
    echo "=== $mode ==="
    ./set-mode.sh "$mode" || { result=1; continue; }
    sleep "$STARTUP_WAIT"
    ./sync-test.sh || result=1
done
exit $result
