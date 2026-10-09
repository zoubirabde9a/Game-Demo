#!/bin/bash
# Sourced by deploy/deploy.sh and deploy/publish_client.sh: writes to
# stdout a gzipped tar of the packaged client (build/client_package),
# holding only the game files the site does not have yet, and
# deploy/web_files.sh, which puts them in place on the server. HAVE is a
# file listing the hashes already in the site's files/, one per line, and
# "play:<build>" for the browser game the site serves. A deploy usually
# changes two or three of the 55 files, so this sends about 2 MB instead
# of 12. The browser game (play/, about 14 MB) goes only when its build
# differs from the site's.
client_tar() {
    local HAVE="$1"
    local OUT=build/client_package
    local LIST PLAY=""
    LIST="$(awk '$1=="file"||$1=="launcher"{print $2}' "$OUT/manifest.txt" | sort -u |
            comm -23 - <(grep -v '^play:' "$HAVE" | sort -u) | sed 's|^|files/|')"
    if [ -f "$OUT/play/build.txt" ] && ! grep -qx "play:$(cat "$OUT/play/build.txt")" "$HAVE"; then
        PLAY=play
    fi
    # shellcheck disable=SC2086
    tar -c -C "$OUT" manifest.txt index.html GameDemo.exe Caddyfile $PLAY $LIST -C "$PWD/deploy" web_files.sh |
        gzip -1
}
