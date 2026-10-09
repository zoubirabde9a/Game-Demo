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
# are kept for manual rollback (see deploy/README.md). A healthy release
# also (re)starts the web gateway for the browser game (start_gateway).
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
# The web gateway runs in a container (start_gateway below), so it carries
# its own C++ runtime instead of needing the container's
GATEWAY_FLAGS="$PROBE_FLAGS -static-libstdc++ -static-libgcc"
GATEWAY=game-demo-gateway
GATEWAY_CONFIG=/etc/game-demo/gateway.env

# NOTE: run from SRC with relative paths, so the key does not depend on
# the folder the source was unpacked into
source_key() {
    (
        cd "$SRC"
        echo "$SERVER_FLAGS / $PROBE_FLAGS / $GATEWAY_FLAGS"
        g++ --version | head -1
        g++ -E -P $SERVER_FLAGS code/server/server_main.cpp
        g++ -E -P $PROBE_FLAGS code/server/probe_main.cpp
        g++ -E -P $GATEWAY_FLAGS code/server/web_gateway_main.cpp
        cat deploy/game-demo.service
    ) | sha256sum | cut -c1-16
}

# NOTE: prints the release to use: an existing one with the same key, or a
# new one built here. Only the server, its probe and the web gateway go
# into a release, so only those are built, side by side (build_server.sh
# builds the load test and replay tools too, which no release needs)
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
    g++ $GATEWAY_FLAGS "$SRC/code/server/web_gateway_main.cpp" -o "$SRC/build/web_gateway"
    wait "$SERVER_JOB"

    RELEASE="$ROOT/releases/$NAME"
    rm -rf "$RELEASE"
    mkdir -p "$RELEASE"
    install -m 755 "$SRC/build/server" "$SRC/build/probe" "$SRC/build/web_gateway" "$RELEASE/"
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

# NOTE: the web gateway lets the browser game (game.sindansolutions.com/play/)
# reach the server. It runs in a container on the download site's Docker
# network, next to game-demo-web, whose Caddy passes /net/ to it, so it
# opens no port of its own. It may only reach the server's public address,
# kept in $GATEWAY_CONFIG (ALLOW=ip:port; edit it and redeploy to change),
# which it reaches through the Docker network's address for this machine.
# Without Docker or the download site (deploy/setup_downloads.sh) there is
# no browser game to serve and this does nothing. A gateway that does not
# answer is reported but does not undo the server's release: the desktop
# game still plays.
start_gateway() {
    RELEASE="$1"
    if ! command -v docker >/dev/null || ! docker inspect game-demo-web >/dev/null 2>&1; then
        say "no download site here, so no web gateway (deploy/setup_downloads.sh)"
        return 0
    fi
    if [ ! -f "$GATEWAY_CONFIG" ]; then
        IP="$(curl -4 -s --max-time 10 https://api.ipify.org || hostname -I | cut -d' ' -f1)"
        echo "ALLOW=$IP:$PORT" > "$GATEWAY_CONFIG"
    fi
    ALLOW="$(sed -n 's/^ALLOW=//p' "$GATEWAY_CONFIG")"
    NETWORK="$(docker inspect game-demo-web -f '{{range $Name, $_ := .NetworkSettings.Networks}}{{$Name}} {{end}}' | cut -d' ' -f1)"
    # NOTE: from inside a container the machine's public address does not
    # loop back to the server; the network's gateway address is the host
    # itself. Browsers still ask for the public one (client/server_list.cpp)
    HOST="$(docker network inspect "$NETWORK" -f '{{(index .IPAM.Config 0).Gateway}}')"
    ROUTE="$ALLOW"
    case "$ALLOW" in *=*) ;; *) [ -n "$HOST" ] && ROUTE="$ALLOW=$HOST:${ALLOW##*:}" ;; esac
    docker rm -f "$GATEWAY" >/dev/null 2>&1 || true
    if ! docker run -d --name "$GATEWAY" --restart unless-stopped --network "$NETWORK" \
        --memory 64m --read-only -v "$RELEASE/web_gateway:/web_gateway:ro" \
        debian:stable-slim /web_gateway --allow "$ROUTE" >/dev/null; then
        say "WARNING: the web gateway container did not start; the browser game cannot join"
        return 0
    fi
    # NOTE: a plain request to /net/ gets 426 from the gateway; wget calls
    # that a failure, so its output is read instead of its exit code
    for _ in 1 2 3 4 5 6 7 8 9 10; do
        ANSWER="$(docker exec game-demo-web wget -q -S -O /dev/null "http://$GATEWAY:27080/net/" 2>&1 || true)"
        case "$ANSWER" in
            *" 426"*)
                # NOTE: and a player can join from where the gateway sits,
                # at the address it sends to
                PID="$(docker inspect -f '{{.State.Pid}}' "$GATEWAY")"
                if nsenter -t "$PID" -n "$RELEASE/probe" "${ROUTE##*=}" >/dev/null 2>&1; then
                    say "web gateway up: browsers reach $ROUTE"
                else
                    say "WARNING: the web gateway runs but cannot reach ${ROUTE##*=}; the browser game cannot join"
                fi
                return 0 ;;
        esac
        sleep 0.5
    done
    say "WARNING: the web gateway does not answer; the browser game cannot join. docker logs $GATEWAY"
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
    start_gateway "$RELEASE"

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
