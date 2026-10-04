found-by: agent/gameplay
files: code/sim/spawn.cpp (AddMonster), code/art/monster_render.cpp, code/client/draw_entities.cpp
since: 2026-10-04
what: A monster added with AddMonster after startup is drawn without its sprite for every kind tried except the brute (0): its health bar, shadow, hit numbers and stun stars show, the body does not. Seen with GAME_DUMMY=1, 2, 3 and 5; GAME_DUMMY=0 (brute) draws. Not the body poses (drawing with them switched off changes nothing), not missing frames (the toad sheet has a full standing row in build\monster_art), not upload timing (still missing at frame 150). The dummy also never moves in 150 frames, so it may not be simulated either. Summons go through AddMonster too, so they may be hit.
reproduce: after build.bat: set GAME_DUMMY=3 and GAME_SCREENSHOT_KEYS=15:F4, then misc\screenshot.bat out.png 60; a health bar sits right of the player with nothing under it. GAME_DUMMY=0 draws a brute there.
fix: unknown; compare what FillMonsterPopulation sets on its monsters with a bare AddMonster
