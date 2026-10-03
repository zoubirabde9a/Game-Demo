#!/bin/sh
# Builds the dedicated server and its health-check probe for Linux:
#   build/server [port]           runs the server (default port 27015)
#   build/probe [address:port]    exits 0 if a server there lets a player in
set -e
cd "$(dirname "$0")"
mkdir -p build
# -w: the shared game code is written for MSVC and is not g++-warning clean yet.
g++ -std=c++11 -O2 -w \
    -DAPP_SLOW=0 -DAPP_DEV=0 \
    code/server/server_main.cpp -o build/server
# The probe only uses code/net, which is warning clean.
g++ -std=c++11 -O2 -Wall -Wno-unused-function \
    -DAPP_SLOW=0 -DAPP_DEV=0 \
    code/server/probe_main.cpp -o build/probe
echo "built build/server and build/probe"
