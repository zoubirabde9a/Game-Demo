#!/bin/bash
# Uploads the client that deploy/package_client.sh built and makes it the
# one every launcher installs:
#   deploy/publish_client.sh user@host
# deploy.sh runs it after the new server passed its checks. Launchers
# that are running notice the new manifest within about 10 seconds,
# download the files that changed, and restart the game on them.
#
# On the server, /opt/game-demo/web holds what game.sindansolutions.com
# serves (deploy/setup_downloads.sh sets that up once):
#   manifest.txt        the live version; replaced last, in one rename
#   files/<sha256>      every game file, named by its hash, never changed
#   manifests/<v>.txt   the manifests of earlier versions, for rollback
#   GameDemo.exe        the launcher, linked from index.html
# Files no longer named by one of the 5 newest manifests are deleted.
set -euo pipefail

TARGET="${1:?usage: deploy/publish_client.sh user@host}"
cd "$(dirname "$0")/.."
OUT=build/client_package
[ -f "$OUT/manifest.txt" ] || { echo "[client] nothing packaged; run deploy/package_client.sh first" >&2; exit 1; }
NAME="$(sed -n 's/^version //p' "$OUT/manifest.txt")"

SUDO=""
[ "$(ssh "$TARGET" id -u)" = 0 ] || SUDO="sudo"
INCOMING="/tmp/game-demo-client-$NAME"

echo "[client] uploading $NAME to $TARGET"
tar -c -C "$OUT" . | ssh "$TARGET" "rm -rf '$INCOMING' && mkdir -p '$INCOMING' && tar -x -C '$INCOMING'"

ssh "$TARGET" "$SUDO bash -s -- '$INCOMING' '$NAME'" <<'EOF'
set -euo pipefail
IN="$1"
NAME="$2"
WEB=/opt/game-demo/web
mkdir -p "$WEB/files" "$WEB/manifests"

for F in "$IN"/files/*; do
    B="$(basename "$F")"
    [ -e "$WEB/files/$B" ] || mv "$F" "$WEB/files/$B"
done
# Every file the manifest names must be there before it goes live.
while read -r KIND HASH _; do
    case "$KIND" in file|launcher) [ -s "$WEB/files/$HASH" ] || { echo "[client] missing $HASH" >&2; exit 1; } ;; esac
done < "$IN/manifest.txt"

cp "$IN/index.html" "$WEB/index.html.new" && mv -f "$WEB/index.html.new" "$WEB/index.html"
cp "$IN/GameDemo.exe" "$WEB/GameDemo.exe.new" && mv -f "$WEB/GameDemo.exe.new" "$WEB/GameDemo.exe"
cp "$IN/manifest.txt" "$WEB/manifests/$NAME.txt"
cp "$IN/manifest.txt" "$WEB/manifest.txt.new" && mv -f "$WEB/manifest.txt.new" "$WEB/manifest.txt"
chmod -R a+rX "$WEB"
rm -rf "$IN"

# Keep the 5 newest versions and the live one; drop the files only older
# versions used.
ls -1t "$WEB"/manifests/*.txt | tail -n +6 | xargs -r rm -f
KEEP="$(cat "$WEB"/manifests/*.txt "$WEB/manifest.txt" | awk '$1=="file"||$1=="launcher"{print $2}' | sort -u)"
for F in "$WEB"/files/*; do
    echo "$KEEP" | grep -qx "$(basename "$F")" || rm -f "$F"
done
echo "[client] live: $NAME"
EOF
