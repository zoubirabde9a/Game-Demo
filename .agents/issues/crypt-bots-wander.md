found-by: agent/bossfights
files: code/server/bots.cpp
since: 2026-10-07
what: in the crypt, server bots often take minutes to walk from a cleared room into the next one, so the balance probe's 30-minute runs stop partway. On main (806a9cf), seed 2 spends 268 s before the Bone Halls and 652 s before the Ossuary between fights; a small change to how a fight ends (a new boss ability) turned a 73 s walk into 745 s. Fight times are fine; only the walks are long.
reproduce: build the probe (see the top of code/tools/dungeon_balance.cpp), then dungeon_balance 30 3 2 2; the lines "N s between fights before the X" show the walks
fix: not known; the bots seem to wander instead of heading for the next room's gate once a room is cleared
