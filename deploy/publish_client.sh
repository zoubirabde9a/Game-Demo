#!/bin/bash
# Uploads the client that deploy/package_client.sh built and makes it the
# one every launcher installs, without touching the server:
#   deploy/publish_client.sh user@host
# deploy.sh does the same as part of every deploy. Launchers that are
# running notice the new manifest within about 10 seconds, download the
# files that changed, and restart the game on them. Only the game files
# the site does not have yet are sent (deploy/client_tar.sh); the site's
# layout is in deploy/web_files.sh, which runs on the server.
set -euo pipefail

TARGET="${1:?usage: deploy/publish_client.sh user@host}"
cd "$(dirname "$0")/.."
. deploy/client_tar.sh
OUT=build/client_package
[ -f "$OUT/manifest.txt" ] || { echo "[client] nothing packaged; run deploy/package_client.sh first" >&2; exit 1; }
NAME="$(sed -n 's/^version //p' "$OUT/manifest.txt")"
IN="/tmp/game-demo-client-$NAME"
HAVE="$(mktemp)"
trap 'rm -f "$HAVE"' EXIT

ssh "$TARGET" "ls /opt/game-demo/web/files 2>/dev/null || true" > "$HAVE"
echo "[client] uploading $NAME to $TARGET"
client_tar "$HAVE" | ssh "$TARGET" 'S=; [ "$(id -u)" = 0 ] || S="sudo -n"; set -e
    rm -rf '"'$IN'"' && mkdir -p '"'$IN'"' && tar -xz -C '"'$IN'"'
    $S bash '"'$IN/web_files.sh'"' publish '"'$IN' '$NAME'"
