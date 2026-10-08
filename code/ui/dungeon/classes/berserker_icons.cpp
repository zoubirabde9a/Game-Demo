/* Berserker icons (ui/dungeon/role_icons.cpp): the ability bar's pictures for
   the Berserker's spells, in RoleKeys order, and the talent panel's for its
   branch, painted in code on the icon canvas (ui/ability_icons/
   icon_canvas.cpp). 0 leaves a cell empty. */

global_variable role_icon_painter *BerserkerIconPainters[ROLE_KEYS] = {};
global_variable talent_icon_painter *BerserkerTalentIconPainters[ROLE_TALENTS] = {};
