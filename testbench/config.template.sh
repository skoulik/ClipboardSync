# Two-machine testbench configuration. Copy to config.sh (gitignored) and fill in.
# Local machine = Windows test driver (run the scripts from Git Bash);
# remote machine = macOS with SSH key auth and a logged-in GUI session.

REMOTE_HOST="<mac-hostname-or-ip>"
REMOTE_USER="<ssh-user>"
REMOTE_KEY="<path-to-ssh-private-key>"          # e.g. ~/.ssh/clipboardsync_mac
REMOTE_REPO="<path-to-repo-clone-on-remote>"    # e.g. ClipboardSync (relative to remote home)
REMOTE_QT_PREFIX="<qt-prefix-on-remote>"        # e.g. /usr/local/Trolltech/Qt-6.11/arm64
REMOTE_TOOLS="<dir-with-cmake-and-ninja>"       # e.g. /opt/homebrew/bin

PORT="<udp-port>"                               # as configured in both apps, e.g. 19211
GROUP="<multicast-group>"                       # as configured in both apps, e.g. 239.255.1.1
SYNC_WAIT=4                                     # seconds allowed for a clipboard change to propagate
