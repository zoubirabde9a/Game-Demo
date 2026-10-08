found-by: newclasses
files: code/ui/dungeon/classes/shadowblade_hud.cpp
since: 2026-10-09
what: the Shadowblade HUD's critical-strike and Shadow Dance timer rings are timed from bursts, which the client drops after half a second or less, so both rings vanish long before the buff ends.
reproduce: GAME_ROLE=shadowblade, Shadowstep a foe and watch the crit ring beside the combo gems; it is gone well before the 4 s crit window ends.
fix: time the rings from when SHADOWBLADE_FLAG_CRIT / SHADOWBLADE_FLAG_DANCE go up in ClassFlags, as ui/dungeon/classes/duelist_hud.cpp does for its guard and Perfect Form rings.
