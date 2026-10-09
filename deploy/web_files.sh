#!/bin/bash
# Runs on the server, as root: puts a packaged game client into the
# download site (deploy/publish_client.sh and deploy/deploy.sh send it).
#   bash web_files.sh stage IN          files/<sha256> into the site
#   bash web_files.sh publish IN NAME   makes IN/manifest.txt the live one
# IN is the folder the upload unpacked: manifest.txt, index.html,
# GameDemo.exe, the Caddyfile, files/ with only the game files the site did
# not have yet, and play/ when the browser game changed. Staging is safe at any time: game files are named by their hash and
# nothing points at a new one until the manifest does, so deploy.sh stages
# them before the server switches and publishes right after it.
#
# /opt/game-demo/web holds what game.sindansolutions.com serves
# (deploy/setup_downloads.sh sets that up once):
#   manifest.txt        the live version; replaced last, in one rename
#   files/<sha256>      every game file, named by its hash, never changed
#   manifests/<v>.txt   the manifests of earlier versions, for rollback
#   GameDemo.exe        the launcher, linked from index.html
#   play/               the browser game; play/build.txt names its build
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

# NOTE: a changed Caddyfile (deploy/web/Caddyfile) goes in place and the
# file server reloads it. It is written over, not replaced, because the
# container has the file itself mounted; one that does not validate is
# left out with a warning, and the site keeps the one it has
update_caddyfile() {
    local LIVE=/opt/game-demo/web-caddy/Caddyfile
    [ -f "$IN/Caddyfile" ] && [ -f "$LIVE" ] || return 0
    cmp -s "$IN/Caddyfile" "$LIVE" && return 0
    if ! docker run --rm -v "$IN/Caddyfile:/etc/caddy/Caddyfile:ro" caddy:2 \
        caddy validate --config /etc/caddy/Caddyfile >/dev/null 2>&1; then
        echo "[client] WARNING: the new Caddyfile does not validate; the site keeps the old one" >&2
        return 0
    fi
    cat "$IN/Caddyfile" > "$LIVE"
    docker exec game-demo-web caddy reload --config /etc/caddy/Caddyfile >/dev/null 2>&1 &&
        echo "[client] file server reloaded with the new Caddyfile"
}

# NOTE: the browser game, when this upload has one: the folder is swapped
# whole, so a page never mixes two builds
publish_play() {
    [ -d "$IN/play" ] || return 0
    rm -rf "$WEB/play.new" "$WEB/play.old"
    cp -r "$IN/play" "$WEB/play.new"
    chmod -R a+rX "$WEB/play.new"
    [ -d "$WEB/play" ] && mv "$WEB/play" "$WEB/play.old"
    mv "$WEB/play.new" "$WEB/play"
    rm -rf "$WEB/play.old"
    echo "[client] browser game live: build $(cat "$WEB/play/build.txt")"
}

publish() {
    NAME="$1"
    update_caddyfile
    publish_play
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
