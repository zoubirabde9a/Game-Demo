#!/bin/bash
# Ships the committed code to a Linux server and installs it there.
#   deploy/deploy.sh user@host
# The user needs ssh access and sudo on the host. Only committed code is
# sent (git archive HEAD), so what runs is always a known commit; the
# release on the server is named after it. Works from Linux, macOS and
# Git Bash on Windows.
set -euo pipefail

TARGET="${1:?usage: deploy/deploy.sh user@host}"
cd "$(dirname "$0")/.."

if [ -n "$(git status --porcelain)" ]; then
    echo "[deploy] note: uncommitted changes are not deployed"
fi
REV="$(git rev-parse --short HEAD)"
NAME="$(date +%Y%m%d-%H%M%S)-$REV"
DIR="/tmp/game-demo-$NAME"

echo "[deploy] sending $REV to $TARGET"
git archive --format=tar HEAD | ssh "$TARGET" "mkdir -p '$DIR' && tar -x -C '$DIR'"

echo "[deploy] installing"
STATUS=0
# A terminal only when a person is watching (sudo may ask a password then);
# root logins skip sudo, which minimal images may not have.
TTY=""
[ -t 0 ] && TTY="-t"
ssh $TTY "$TARGET" "if [ \$(id -u) = 0 ]; then bash '$DIR/deploy/install.sh' '$NAME'; else sudo bash '$DIR/deploy/install.sh' '$NAME'; fi" || STATUS=$?
ssh "$TARGET" "rm -rf '$DIR'" || true
[ "$STATUS" = 0 ] || { echo "[deploy] install failed; the previous release is still live"; exit "$STATUS"; }

# The server passed its check from inside the machine. This one goes over
# the internet, which also catches a cloud firewall blocking the port.
# ssh -G resolves an alias from ~/.ssh/config to the real address.
HOST="$(ssh -G "$TARGET" | sed -n 's/^hostname //p')"
PORT="$(ssh "$TARGET" "sed -n 's/^PORT=//p' /etc/game-demo/server.env")"
CHECKED=0
for PROBE in build/probe build/probe.exe; do
    if [ -x "$PROBE" ]; then
        if [[ "$HOST" =~ ^[0-9.]+$ ]]; then
            "$PROBE" "$HOST:$PORT" || { echo "[deploy] the server runs but is not reachable from here: open UDP $PORT in the provider's firewall"; exit 1; }
            CHECKED=1
        fi
        break
    fi
done
[ "$CHECKED" = 1 ] || echo "[deploy] note: no outside check (build the probe with build_server, or the host is not an IPv4 address)"
echo "[deploy] done: $NAME"
