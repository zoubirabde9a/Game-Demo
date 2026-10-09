#!/bin/bash
# Builds the Windows game and launcher from the committed code and lays
# them out for publishing:
#   deploy/package_client.sh [version-name]
# Leaves build/client_package/: manifest.txt, files/<sha256> (one per
# game file), GameDemo.exe, index.html, the site's Caddyfile and play/, the
# browser game (when Emscripten is installed). deploy/publish_client.sh
# uploads that folder. Runs from Git Bash on Windows, because the game is
# built with Visual Studio (build.bat release).
#
# Only committed code is built (git archive HEAD into build/client_src),
# like deploy.sh does for the server, so the game and the server it joins
# come from the same commit and agree on the content id.
set -euo pipefail
cd "$(dirname "$0")/.."

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) ;;
    *) echo "[client] the game builds with Visual Studio; run this from Git Bash on Windows" >&2; exit 1 ;;
esac

REV="$(git rev-parse --short HEAD)"
NAME="${1:-$(date +%Y%m%d-%H%M%S)-$REV}"
SRC=build/client_src
OUT=build/client_package

echo "[client] building $REV (release) for version $NAME"
rm -rf "$SRC" "$OUT"
mkdir -p "$SRC" "$OUT/files"
git archive --format=tar HEAD | tar -x -C "$SRC"
# The join probe from the same tree, beside the game: deploy.sh joins the
# new server with it over the internet (deploy/build_probe.bat)
cmd.exe //c "$(cygpath -w "$SRC/deploy/build_probe.bat")" > "$SRC/probe.log" 2>&1 &
PROBE_JOB=$!
# The browser game (web/build.bat), from the same tree, so it joins the
# same server. It needs Emscripten (misc/shell_emscripten.bat says how to
# install it); without it the site keeps the browser game it has.
WEB_STATUS=0
cmd.exe //c "$(cygpath -w "$SRC/web/build.bat")" > "$SRC/web.log" 2>&1 &
WEB_JOB=$!
cmd.exe //c "$(cygpath -w "$SRC/build.bat")" release > "$SRC/build.log" 2>&1 || {
    grep -iE "error|warning C" "$SRC/build.log" | head -20 >&2
    echo "[client] build failed; full log in $SRC/build.log" >&2
    exit 1
}

# What the game needs at run time: it loads asset_1.zas, shaders/, fonts/,
# the class skins in heroes/ and heroes_v2/ and the recorded sound effects in
# sfx/ from the folder it runs in.
GAME="$SRC/build"
FILES=(win32_app.exe app.dll asset_1.zas)
while IFS= read -r F; do FILES+=("$F"); done < <(cd "$GAME" && find shaders fonts sfx heroes heroes_v2 -type f | sort)

hash_of() { sha256sum "$1" | cut -d' ' -f1; }
size_of() { wc -c < "$1" | tr -d ' '; }

LAUNCHER_HASH="$(hash_of "$GAME/launcher.exe")"
{
    echo "game-demo-manifest 1"
    echo "version $NAME"
    echo "launcher $LAUNCHER_HASH $(size_of "$GAME/launcher.exe")"
    for F in "${FILES[@]}"; do
        H="$(hash_of "$GAME/$F")"
        cp "$GAME/$F" "$OUT/files/$H"
        echo "file $H $(size_of "$GAME/$F") $F"
    done
    echo "end"
} > "$OUT/manifest.txt"
cp "$GAME/launcher.exe" "$OUT/files/$LAUNCHER_HASH"
cp "$GAME/launcher.exe" "$OUT/GameDemo.exe"
cp "$SRC/deploy/web/index.html" "$OUT/index.html"
cp "$SRC/deploy/web/Caddyfile" "$OUT/Caddyfile"

wait "$PROBE_JOB" || { cat "$SRC/probe.log" >&2; echo "[client] the probe did not build" >&2; exit 1; }
wait "$WEB_JOB" || WEB_STATUS=$?
if [ "$WEB_STATUS" = 0 ]; then
    # play/build.txt names this build, so a deploy sends the browser game
    # only when it changed (deploy/client_tar.sh)
    mkdir -p "$OUT/play"
    cp "$SRC"/web/build/index.{html,js,wasm,data} "$OUT/play/"
    (cd "$OUT/play" && sha256sum index.html index.js index.wasm index.data | sha256sum | cut -c1-16) > "$OUT/play/build.txt"
    echo "[client] browser game built ($(du -sh "$OUT/play" | cut -f1))"
elif grep -q "no emsdk" "$SRC/web.log"; then
    echo "[client] note: no Emscripten here, so the browser game was not built (misc/shell_emscripten.bat)"
else
    grep -iE "error" "$SRC/web.log" | head -20 >&2
    echo "[client] the browser game did not build; full log in $SRC/web.log" >&2
    exit 1
fi

echo "[client] packaged ${#FILES[@]} files, $(du -sh "$OUT" | cut -f1) in $OUT"
