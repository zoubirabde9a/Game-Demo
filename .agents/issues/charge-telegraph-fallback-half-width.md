found-by: agent/abilityfx-bosses
files: code/art/monster_telegraphs.cpp
since: 2026-10-09
what: without the danger zone shader, a charge's fallback telegraph (DrawMonsterTelegraphs, MonsterAbility_Charge, about line 285) draws its two side lines Radius * 0.5 from the middle, so the lane shows Radius wide and stops at the end of the run. The charger hits whoever comes within Radius of it as it runs (UpdateCharge, sim/monster_abilities/movement.cpp), so the real lane is 2 x Radius wide and reaches Radius past both ends. Every charger (the Brood Queen's Pounce, the Hollow King's Shadow Rush, Kragg's Bellows Rush and the rest) shows half its real width. The shader path was fixed in client/danger_zones.cpp (DrawDangerSweep).
reproduce: delete or rename build/shaders/fx/danger_zone.frag, start a crypt run in the boss room (GAME_MAP=crypt GAME_ROOM=7) and watch Shadow Rush: the dotted lane is about half as wide as the area that hits.
fix: Side = Ability->Radius * perpendicular (not 0.5 * Radius), start the lines at Self - Radius * Aim and end them at End + Radius * Aim, and close each end with a half circle of Radius.
