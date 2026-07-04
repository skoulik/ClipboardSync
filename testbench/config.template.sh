# Two-machine testbench configuration. Copy to config.sh (gitignored) and fill in.
# Local machine = Windows test driver (run the scripts from Git Bash);
# remote machine = macOS with SSH key auth and a logged-in GUI session.

REMOTE_HOST="<mac-hostname-or-ip>"
REMOTE_USER="<ssh-user>"
REMOTE_KEY="<path-to-ssh-private-key>"          # e.g. ~/.ssh/clipboardsync_mac
REMOTE_REPO="<path-to-repo-clone-on-remote>"    # e.g. ClipboardSync (relative to remote home)
REMOTE_QT_PREFIX="<qt-prefix-on-remote>"        # e.g. /usr/local/Trolltech/Qt-6.11/arm64
REMOTE_TOOLS="<dir-with-cmake-and-ninja>"       # e.g. /opt/homebrew/bin

LOCAL_APP="<path-to-local-exe>"                 # e.g. D:\code\ClipboardSync\build\default\Release\ClipboardSync.exe
LOCAL_IP="<local-lan-ip>"                       # e.g. 192.168.1.51
REMOTE_IP="<remote-lan-ip>"                     # e.g. 192.168.1.55
LOCAL_IFACE="<qt-iface-name-on-windows>"        # QNetworkInterface::name, e.g. wireless_32768
REMOTE_IFACE="<iface-name-on-mac>"              # e.g. en0
BCAST_ADDR="<subnet-broadcast-address>"         # e.g. 192.168.1.255

PORT="<udp-port>"                               # as configured in both apps, e.g. 19211
GROUP="<multicast-group>"                       # as configured in both apps, e.g. 239.255.1.1
SYNC_WAIT=4                                     # seconds allowed for a clipboard change to propagate
STARTUP_WAIT=5                                  # seconds allowed for a restarted instance to come up
