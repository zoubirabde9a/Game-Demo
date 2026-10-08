#!/bin/bash
# Runs on the server, as root: puts a packaged game client into the
# download site (deploy/publish_client.sh and deploy/deploy.sh send it).
#   bash web_files.sh stage IN          files/<sha256> into the site
#   bash web_files.sh publish IN NAME   makes IN/manifest.txt the live one
# IN is the folder the upload unpacked: manifest.txt, index.html,
# GameDemo.exe and files/ with only the game files the site did not have
# yet. Staging is safe at any time: game files are named by their hash and
# nothing points at a new one until the manifest does, so deploy.sh stages
# them before the server switches and publishes right after it.
#
# /opt/game-demo/web holds what game.sindansolutions.com serves
# (deploy/setup_downloads.sh sets that up once):
#   manifest.txt        the live version; replaced last, in one rename
#   files/<sha256>      every game file, named by its hash, never changed
#   manifests/<v>.txt   the manifests of earlier versions, for rollback
#   GameDemo.exe        the launcher, linked from index.html
# Files no longer named by one of the 5 newest manifests are deleted.
set -euo pipefail
WEB=/opt/game-demo/web
MODE="$1"
IN="$2"

stage() {
    mkdir -p "$WEB/files" "$WEB/manifests"
    if [ -d "$IN/files" ]; then
        for F in "$IN"/files/*; do
            [ -e "$F" ] || continue
            B="$(basename "$F")"
            [ -e "$WEB/files/$B" ] || mv "$F" "$WEB/files/$B"
        done
    fi
    chmod -R a+rX "$WEB/files"
    # Every file the manifest names must be there before it goes live.
    while read -r KIND HASH _; do
        case "$KIND" in file|launcher) [ -s "$WEB/files/$HASH" ] || { echo "[client] missing $HASH" >&2; exit 1; } ;; esac
    done < "$IN/manifest.txt"
}

publish() {
    NAME="$1"
    cp "$IN/index.html" "$WEB/index.html.new" && mv -f "$WEB/index.html.new" "$WEB/index.html"
    cp "$IN/GameDemo.exe" "$WEB/GameDemo.exe.new" && mv -f "$WEB/GameDemo.exe.new" "$WEB/GameDemo.exe"
    cp "$IN/manifest.txt" "$WEB/manifests/$NAME.txt"
    cp "$IN/manifest.txt" "$WEB/manifest.txt.new" && mv -f "$WEB/manifest.txt.new" "$WEB/manifest.txt"
    chmod a+r "$WEB/index.html" "$WEB/GameDemo.exe" "$WEB/manifest.txt" "$WEB/manifests/$NAME.txt"
    rm -rf "$IN"

    # Keep the 5 newest versions and the live one; drop the files only
    # older versions used.
    ls -1t "$WEB"/manifests/*.txt | tail -n +6 | xargs -r rm -f
    KEEP="$(cat "$WEB"/manifests/*.txt "$WEB/manifest.txt" | awk '$1=="file"||$1=="launcher"{print $2}' | sort -u)"
    ls -1 "$WEB/files" | sort | comm -23 - <(echo "$KEEP") | while read -r OLD; do
        rm -f "$WEB/files/$OLD"
    done
    echo "[client] live: $NAME"
}

case "$MODE" in
    stage) stage ;;
    publish) stage; publish "$3" ;;
    *) echo "usage: web_files.sh stage IN | publish IN NAME" >&2; exit 1 ;;
esac
