#!/usr/bin/env bash
# Build the app on the remote Mac from its repo clone.
set -eu
cd "$(dirname "$0")"
source ./config.sh

ssh -i "$REMOTE_KEY" -o BatchMode=yes "$REMOTE_USER@$REMOTE_HOST" \
    "cd $REMOTE_REPO && export PATH=$REMOTE_TOOLS:\$PATH CMAKE_PREFIX_PATH=$REMOTE_QT_PREFIX &&
     cmake --preset default && cmake --build --preset release"
