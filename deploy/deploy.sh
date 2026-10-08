#!/bin/bash
# Ships the committed code to a Linux server and installs it there, then
# publishes the Windows game built from the same commit, which every
# running launcher switches to (deploy/web_files.sh).
#   deploy/deploy.sh user@host
# The game builds with Visual Studio, so the client half runs only from
# Git Bash on Windows; elsewhere only the server is deployed.
# The user needs ssh access as root, or sudo without a password. Only
# committed code is sent (git archive HEAD), so what runs is always a
# known commit; the release on the server is named after it.
#
# Made to be quick: three ssh connections in all, each about 1.5 s to
# open from Windows, where Git Bash's ssh cannot share one.
#   1. the server's source (code/ and deploy/ only, about 3 MB) goes up
#      and builds there, in the background, while the game client builds
#      here; the server keeps running the old release meanwhile. When the
#      server code did not change (install.sh hashes what it would
#      compile), nothing builds and the server is not restarted
#   2. the game files the download site does not have yet (usually 2 or 3
#      of 55, deploy/client_tar.sh) go up, and the server switches
#   3. after a join over the internet with a probe built from this same
#      commit, the client's manifest goes live
# Each step prints the seconds since the start.
set -euo pipefail

TARGET="${1:?usage: deploy/deploy.sh user@host}"
cd "$(dirname "$0")/.."
. deploy/client_tar.sh

START=$(date +%s%N)
say() {
    local T=$(( ($(date +%s%N) - START) / 100000000 ))
    printf '[deploy] %d.%ds %s\n' $((T / 10)) $((T % 10)) "$*"
}

if [ -n "$(git status --porcelain)" ]; then
    say "note: uncommitted changes are not deployed"
fi
REV="$(git rev-parse --short HEAD)"
NAME="$(date +%Y%m%d-%H%M%S)-$REV"
STAGE="/tmp/game-demo-$NAME"
# NOTE: root logins run as is; others through sudo, which must not ask
ROOT='S=; [ "$(id -u)" = 0 ] || S="sudo -n";'
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

CLIENT=0
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) CLIENT=1 ;;
    *) say "note: not on Windows, so the game client is not built or published" ;;
esac

say "sending $REV to $TARGET and building the server there"
git archive --format=tar HEAD code build_server.sh deploy | gzip -1 |
    ssh "$TARGET" "$ROOT"' set -e
        rm -rf '"'$STAGE'"' && mkdir -p '"'$STAGE'"' && tar -xz -C '"'$STAGE'"'
        $S bash '"'$STAGE'"'/deploy/install.sh build '"'$NAME'"'
        echo "port $(sed -n "s/^PORT=//p" /etc/game-demo/server.env)"
        if [ -d /opt/game-demo/web ]; then echo "web yes"; ls /opt/game-demo/web/files | sed "s/^/have /"; fi' \
    > "$WORK/server.log" 2>&1 &
SERVER_JOB=$!

# Built here while the server builds there. A game that does not compile
# stops the deploy before the server changes.
CLIENT_STATUS=0
if [ "$CLIENT" = 1 ]; then
    deploy/package_client.sh "$NAME" || CLIENT_STATUS=$?
    PROBE=build/client_src/build/probe.exe
    say "game client built"
elif command -v g++ >/dev/null; then
    g++ -std=c++11 -O2 -w -DAPP_SLOW=0 -DAPP_DEV=0 code/server/probe_main.cpp -o "$WORK/probe" &&
        PROBE="$WORK/probe"
fi

cleanup_stage() { ssh "$TARGET" "$ROOT"' $S rm -rf '"'$STAGE'" || true; }

SERVER_STATUS=0
wait "$SERVER_JOB" || SERVER_STATUS=$?
grep -vE '^(have|port|web|release) ' "$WORK/server.log" || true
if [ "$SERVER_STATUS" != 0 ] || [ "$CLIENT_STATUS" != 0 ]; then
    cleanup_stage
    say "build failed; nothing changed on $TARGET"
    exit 1
fi
RELEASE="$(sed -n 's/^release \([^ ]*\).*/\1/p' "$WORK/server.log")"
PORT="$(sed -n 's/^port //p' "$WORK/server.log")"
UNCHANGED=0
grep -q '^release .* unchanged$' "$WORK/server.log" && UNCHANGED=1
WEB=0
grep -q '^web yes$' "$WORK/server.log" && WEB=1
sed -n 's/^have //p' "$WORK/server.log" > "$WORK/have.txt"
say "server built"

SWITCH="true"
[ "$UNCHANGED" = 1 ] || SWITCH='$S bash '"'$STAGE/deploy/install.sh'"' switch '"'$RELEASE'"
if [ "$CLIENT" = 1 ] && [ "$WEB" = 1 ]; then
    say "uploading the game files the site lacks, and switching the server"
    client_tar "$WORK/have.txt" | ssh "$TARGET" "$ROOT"' set -e
        mkdir -p '"'$STAGE/client'"' && tar -xz -C '"'$STAGE/client'"'
        $S bash '"'$STAGE'"'/deploy/web_files.sh stage '"'$STAGE/client'"'
        '"$SWITCH" || { cleanup_stage; say "switch failed; the previous release is still live"; exit 1; }
elif [ "$UNCHANGED" = 0 ]; then
    say "switching the server"
    ssh "$TARGET" "$ROOT $SWITCH" < /dev/null ||
        { cleanup_stage; say "switch failed; the previous release is still live"; exit 1; }
fi
[ "$UNCHANGED" = 1 ] && say "the server code did not change; the server kept running"

# The server passed its check from inside the machine. This one goes over
# the internet, which also catches a cloud firewall blocking the port.
# ssh -G resolves an alias from ~/.ssh/config to the real address.
HOST="$(ssh -G "$TARGET" | sed -n 's/^hostname //p')"
if [ -n "${PROBE:-}" ] && [[ "$HOST" =~ ^[0-9.]+$ ]]; then
    PROBE_STATUS=0
    "$PROBE" "$HOST:$PORT" || PROBE_STATUS=$?
    case "$PROBE_STATUS" in
        0) ;;
        2) cleanup_stage; say "the server runs but does not answer from here: open UDP $PORT in the provider's firewall"; exit 1 ;;
        5) cleanup_stage; say "the server answers but runs another game build than this commit's"; exit 1 ;;
        *) cleanup_stage; say "the join check over the internet failed ($PROBE_STATUS)"; exit 1 ;;
    esac
else
    say "note: no outside check (no probe, or the host is not an IPv4 address)"
fi

# The server is live and answering; now move the players onto the game
# built from the same commit.
if [ "$CLIENT" = 1 ] && [ "$WEB" = 1 ]; then
    ssh "$TARGET" "$ROOT"' $S bash '"'$STAGE/deploy/web_files.sh'"' publish '"'$STAGE/client' '$NAME'"'; $S rm -rf '"'$STAGE'" < /dev/null
else
    [ "$CLIENT" = 1 ] && say "note: no download site on $TARGET yet (deploy/setup_downloads.sh); the client was not published"
    cleanup_stage
fi
say "done: $NAME"
