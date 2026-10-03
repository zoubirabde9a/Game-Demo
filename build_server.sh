#!/bin/sh
# Builds the dedicated server for Linux into build/server.
# Run it with: build/server [port]   (default port 27015)
set -e
cd "$(dirname "$0")"
mkdir -p build
# -w: the shared game code is written for MSVC and is not g++-warning clean yet.
# APP_DEV=1: the game code does not compile with APP_DEV=0 yet (file reading
# and InvalidCodePath only exist in developer builds). It also makes a failed
# Assert crash the server, which the service manager then restarts.
g++ -std=c++11 -O2 -w \
    -DAPP_SLOW=0 -DAPP_DEV=1 \
    code/server/server_main.cpp -o build/server
echo "built build/server"
