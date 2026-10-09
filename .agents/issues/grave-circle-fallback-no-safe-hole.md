found-by: agent/abilityfx-bosses
files: code/art/monster_telegraphs.cpp
since: 2026-10-09
what: without the danger zone shader, a Mortar's fallback telegraph (MonsterAbility_Mortar, about line 300) draws a full dotted circle of Radius and a filling one, ignoring InnerRadius. The Gravecaller's Grave Circle (Radius 130, InnerRadius 45) spares whoever holds still in the middle, but the fallback shows the whole disc as danger, so a player told to stand in the middle sees themselves standing in it.
reproduce: delete or rename build/shaders/fx/danger_zone.frag and watch a Gravecaller's Grave Circle in the crypt.
fix: when Ability->InnerRadius > 0, also draw a dotted circle at InnerRadius in TELEGRAPH_COLOR_MEND (the safe ground), and fill from the rim inward toward it instead of from the middle out.
