found-by: agent/abilityfx-bosses
files: code/art/monster_telegraphs.cpp
since: 2026-10-09
what: without the danger zone shader, a volley's fallback telegraph (MonsterAbility_Volley, about line 384) is one dotted line per shot, with no width. A shot hits whoever comes within its ability's Radius (16 to 20 on the bosses, UpdateMonsterShot), so each path is 2 x Radius wide and reaches Radius past where the shot starts (12 in front of the caster, AddMonsterShot) and stops. The shader path was fixed in client/danger_zones.cpp.
reproduce: delete or rename build/shaders/fx/danger_zone.frag and watch Sskarra's Magma Spit (GAME_MAP=depths GAME_ROOM=5): each shot's lane is a single dotted line.
fix: draw each lane as two dotted lines Radius each side of the shot's path, from Self + 12 * Direction - Radius * Direction to the end of the flight + Radius * Direction.
