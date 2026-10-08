found-by: agent/depthsloop
files: code/server/bots.cpp
since: 2026-10-08
what: in the Ember Depths, server bots keep dying after the Ashfall Bridge is cleared with no monster near them, about every 10 to 20 s for minutes; they seem to walk into the lava lake beside the causeway on their way to the Throne of Embers.
reproduce: PROBE_MAP=depths PROBE_DEATHS=1 build\dungeon_balance.exe 12 3 6 3 (seed 1: lines "fell ... at 0% of the room's foes alive" with nothing listed)
fix: have bots path around lava tiles (or treat lava as a wall) when walking between rooms
