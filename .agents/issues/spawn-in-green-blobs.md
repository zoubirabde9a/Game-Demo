found-by: claude-loop
files: code/sim/monster_population.cpp, code/sim/players.cpp (spawn point), code/sim/monsters/*
since: 2026-10-03
what: playing offline on Old Arena, the local player starts standing on two or three green blobs (slime-like, outlined) and loses health in small ticks (hit numbers show 2, 2) within about 4 s, without moving or doing anything. Every launch this session showed the same blobs at the spawn point; standing still, the player died twice in about 25 s. Not yet checked whether they are hazards or slime monsters, or why they sit on the spawn.
reproduce: set GAME_SERVER=offline, start build\win32_app.exe from build\, press F4 to close the Play screen, do not move; watch health and the numbers over the player.
fix: keep monsters and their hazards a little away from player spawn points (the population already refills away from players), or give a fresh spawn a moment of protection.
