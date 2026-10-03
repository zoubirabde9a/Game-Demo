#!/bin/bash
# Installs the server from this source tree onto THIS machine and restarts
# it. Run as root on the server; deploy/deploy.sh does that for you.
#   sudo bash deploy/install.sh [release-name]
#
# Each install is a new folder under /opt/game-demo/releases/, and
# /opt/game-demo/current points at the live one. After restarting, the
# probe must join the server within a few seconds; if it cannot, the
# previous release is put back and the install fails. The last 3 releases
# are kept for manual rollback (see deploy/README.md).
set -euo pipefail

SRC="$(cd "$(dirname "$0")/.." && pwd)"
NAME="${1:-$(date +%Y%m%d-%H%M%S)}"
ROOT=/opt/game-demo
RELEASE="$ROOT/releases/$NAME"
SERVICE=game-demo
CONFIG=/etc/game-demo/server.env

say() { echo "[install] $*"; }

[ "$(id -u)" = 0 ] || { echo "run as root (sudo)" >&2; exit 1; }

if ! command -v g++ >/dev/null; then
    say "installing g++"
    apt-get update -qq && apt-get install -y -qq g++ >/dev/null
fi

say "building"
sh "$SRC/build_server.sh"

mkdir -p "$(dirname "$CONFIG")"
[ -f "$CONFIG" ] || echo "PORT=27015" > "$CONFIG"
PORT="$(sed -n 's/^PORT=//p' "$CONFIG")"

id gameserver >/dev/null 2>&1 ||
    useradd --system --no-create-home --shell /usr/sbin/nologin gameserver

rm -rf "$RELEASE"
mkdir -p "$RELEASE"
install -m 755 "$SRC/build/server" "$SRC/build/probe" "$RELEASE/"
PREVIOUS="$(readlink "$ROOT/current" 2>/dev/null || true)"

# Swap the symlink atomically so there is never a moment without one.
switch_to() {
    ln -sfn "$1" "$ROOT/current.new"
    mv -T "$ROOT/current.new" "$ROOT/current"
}

healthy() {
    for _ in 1 2 3 4 5; do
        if "$ROOT/current/probe" "127.0.0.1:$PORT"; then return 0; fi
        sleep 1
    done
    return 1
}

switch_to "$RELEASE"
install -m 644 "$SRC/deploy/game-demo.service" "/etc/systemd/system/$SERVICE.service"
systemctl daemon-reload
systemctl enable -q "$SERVICE"
say "restarting $SERVICE on UDP port $PORT"
systemctl restart "$SERVICE"

if ! healthy; then
    say "new release failed its health check; last log lines:"
    journalctl -u "$SERVICE" -n 20 --no-pager || true
    if [ -n "$PREVIOUS" ] && [ -d "$PREVIOUS" ]; then
        say "rolling back to $(basename "$PREVIOUS")"
        switch_to "$PREVIOUS"
        systemctl restart "$SERVICE"
        healthy && say "rollback is up" || say "rollback is ALSO down, needs a human"
    fi
    rm -rf "$RELEASE"
    exit 1
fi

if command -v ufw >/dev/null && ufw status | grep -q "Status: active"; then
    ufw allow "$PORT/udp" >/dev/null && say "firewall: UDP $PORT open"
fi

# Keep the 3 newest releases; never delete the live one.
ls -1dt "$ROOT"/releases/* | tail -n +4 | while read -r OLD; do
    [ "$OLD" = "$(readlink "$ROOT/current")" ] || rm -rf "$OLD"
done

say "live: $NAME"
