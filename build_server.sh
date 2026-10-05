#!/bin/sh
# Builds the dedicated server and its health-check probe for Linux:
#   build/server [port]           runs the server (default port 27015)
#   build/probe [address:port]    exits 0 if a server there lets a player in
#   build/bots address:port id [n] [s]  load test: n bot players for s seconds
#   build/replay file             plays a replay (server --record file) back and checks it
set -e
cd "$(dirname "$0")"
mkdir -p build
# -w: the shared game code is written for MSVC and is not g++-warning clean yet.
# -ffp-contract=off: g++ fuses a * b + c into one instruction on ARM by
# default, which rounds differently from x86 and from MSVC; off, every build
# of the simulation does the same arithmetic, so replays and the hashes in
# them agree (server/replay.cpp).
g++ -std=c++11 -O2 -w -ffp-contract=off \
    -DAPP_SLOW=0 -DAPP_DEV=0 \
    code/server/server_main.cpp -o build/server
# The probe only uses code/net, which is warning clean.
g++ -std=c++11 -O2 -Wall -Wno-unused-function \
    -DAPP_SLOW=0 -DAPP_DEV=0 \
    code/server/probe_main.cpp -o build/probe
g++ -std=c++11 -O2 -Wall -Wno-unused-function \
    -DAPP_SLOW=0 -DAPP_DEV=0 \
    code/tools/bots_main.cpp -o build/bots
g++ -std=c++11 -O2 -w -ffp-contract=off \
    -DAPP_SLOW=0 -DAPP_DEV=0 \
    code/tools/replay_main.cpp -o build/replay
echo "built build/server, build/probe, build/bots and build/replay"
