#!/bin/bash
# Installs the server from this source tree onto THIS machine. Run as root
# on the server; deploy/deploy.sh does that for you, in two steps so the
# build runs while the game client builds on the deploying machine:
#   sudo bash deploy/install.sh build NAME       builds the release, not live yet
#   sudo bash deploy/install.sh switch DIR       makes the release in DIR live
#   sudo bash deploy/install.sh [NAME]           both, by hand
#
# Each release keeps a key: a hash of exactly what the compiler would
# build (the preprocessed server and probe, the flags, the compiler and
# the service file), about 0.2 s to work out. A release with the same key
# is reused instead of built again, and when the live release has it,
# build prints "unchanged" and deploy.sh leaves the server running: a
# deploy that changed only the game client never restarts the server.
#
# Each release is a folder under /opt/game-demo/releases/, and
# /opt/game-demo/current points at the live one. After restarting, the
# probe must join the server within a few seconds; if it cannot, the
# previous release is put back and the switch fails. The last 3 releases
# are kept for manual rollback (see deploy/README.md).
set -euo pipefail

SRC="$(cd "$(dirname "$0")/.." && pwd)"
ROOT=/opt/game-demo
SERVICE=game-demo
CONFIG=/etc/game-demo/server.env

say() { echo "[install] $*"; }

[ "$(id -u)" = 0 ] || { echo "run as root (sudo)" >&2; exit 1; }

mkdir -p "$(dirname "$CONFIG")" "$ROOT/releases"
[ -f "$CONFIG" ] || echo "PORT=27015" > "$CONFIG"
PORT="$(sed -n 's/^PORT=//p' "$CONFIG")"

# Same flags as build_server.sh
SERVER_FLAGS="-std=c++11 -O2 -w -ffp-contract=off -DAPP_SLOW=0 -DAPP_DEV=0"
PROBE_FLAGS="-std=c++11 -O2 -Wall -Wno-unused-function -DAPP_SLOW=0 -DAPP_DEV=0"

# NOTE: run from SRC with relative paths, so the key does not depend on
# the folder the source was unpacked into
source_key() {
    (
        cd "$SRC"
        echo "$SERVER_FLAGS / $PROBE_FLAGS"
        g++ --version | head -1
        g++ -E -P $SERVER_FLAGS code/server/server_main.cpp
        g++ -E -P $PROBE_FLAGS code/server/probe_main.cpp
        cat deploy/game-demo.service
    ) | sha256sum | cut -c1-16
}

# NOTE: prints the release to use: an existing one with the same key, or a
# new one built here. Only the server and its probe go into a release, so
# only those two are built, side by side (build_server.sh builds the load
# test and replay tools too, which no release needs)
build() {
    NAME="$1"
    if ! command -v g++ >/dev/null; then
        say "installing g++"
        apt-get update -qq && apt-get install -y -qq g++ >/dev/null
    fi
    KEY="$(source_key)"
    LIVE="$(readlink -f "$ROOT/current" 2>/dev/null || true)"
    for DIR in "$ROOT"/releases/*; do
        if [ -f "$DIR/key" ] && [ "$(cat "$DIR/key")" = "$KEY" ]; then
            if [ "$DIR" = "$LIVE" ]; then
                say "unchanged: $(basename "$DIR") already runs this server code"
                echo "release $DIR unchanged"
            else
                say "reusing $(basename "$DIR"), built from the same server code"
                echo "release $DIR"
            fi
            return 0
        fi
    done

    say "building"
    mkdir -p "$SRC/build"
    g++ $SERVER_FLAGS "$SRC/code/server/server_main.cpp" -o "$SRC/build/server" &
    SERVER_JOB=$!
    g++ $PROBE_FLAGS "$SRC/code/server/probe_main.cpp" -o "$SRC/build/probe"
    wait "$SERVER_JOB"

    RELEASE="$ROOT/releases/$NAME"
    rm -rf "$RELEASE"
    mkdir -p "$RELEASE"
    install -m 755 "$SRC/build/server" "$SRC/build/probe" "$RELEASE/"
    install -m 644 "$SRC/deploy/game-demo.service" "$RELEASE/"
    echo "$KEY" > "$RELEASE/key"
    say "built $NAME"
    echo "release $RELEASE"
}

# Swap the symlink atomically so there is never a moment without one.
point_at() {
    ln -sfn "$1" "$ROOT/current.new"
    mv -T "$ROOT/current.new" "$ROOT/current"
}

healthy() {
    for _ in 1 2 3 4 5 6 7 8 9 10; do
        if "$ROOT/current/probe" "127.0.0.1:$PORT" >/dev/null; then return 0; fi
        sleep 0.5
    done
    return 1
}

switch() {
    RELEASE="$1"
    PREVIOUS="$(readlink "$ROOT/current" 2>/dev/null || true)"
    id gameserver >/dev/null 2>&1 ||
        useradd --system --no-create-home --shell /usr/sbin/nologin gameserver

    point_at "$RELEASE"
    SERVICE_FILE="$RELEASE/game-demo.service"
    [ -f "$SERVICE_FILE" ] || SERVICE_FILE="$SRC/deploy/game-demo.service"
    if ! cmp -s "$SERVICE_FILE" "/etc/systemd/system/$SERVICE.service"; then
        install -m 644 "$SERVICE_FILE" "/etc/systemd/system/$SERVICE.service"
        systemctl daemon-reload
    fi
    systemctl enable -q "$SERVICE"
    say "restarting $SERVICE on UDP port $PORT"
    systemctl restart "$SERVICE"

    if ! healthy; then
        say "new release failed its health check; last log lines:"
        journalctl -u "$SERVICE" -n 20 --no-pager || true
        if [ -n "$PREVIOUS" ] && [ -d "$PREVIOUS" ]; then
            say "rolling back to $(basename "$PREVIOUS")"
            point_at "$PREVIOUS"
            systemctl restart "$SERVICE"
            healthy && say "rollback is up" || say "rollback is ALSO down, needs a human"
        fi
        exit 1
    fi
    "$ROOT/current/probe" "127.0.0.1:$PORT"

    if command -v ufw >/dev/null && ufw status | grep -q "Status: active"; then
        ufw allow "$PORT/udp" >/dev/null && say "firewall: UDP $PORT open"
    fi

    # Keep the 3 newest releases; never delete the live one.
    ls -1dt "$ROOT"/releases/* | tail -n +4 | while read -r OLD; do
        [ "$OLD" = "$(readlink "$ROOT/current")" ] || rm -rf "$OLD"
    done
    say "live: $(basename "$RELEASE")"
}

case "${1:-}" in
    build) build "$2" ;;
    switch) switch "$2" ;;
    *)
        NAME="${1:-$(date +%Y%m%d-%H%M%S)}"
        OUT="$(build "$NAME")"
        echo "$OUT" | grep -v '^release '
        switch "$(echo "$OUT" | sed -n 's/^release \([^ ]*\).*/\1/p')"
        ;;
esac
