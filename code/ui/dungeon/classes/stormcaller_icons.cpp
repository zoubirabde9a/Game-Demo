/* Stormcaller icons (ui/dungeon/role_icons.cpp): the ability bar's pictures for
   the Stormcaller's spells, in RoleKeys order, and the talent panel's for its
   branch, painted in code on the icon canvas (ui/ability_icons/
   icon_canvas.cpp). 0 leaves a cell empty; all empty until the kit lands. */

global_variable role_icon_painter *StormcallerIconPainters[ROLE_KEYS] = {};
global_variable talent_icon_painter *StormcallerTalentIconPainters[ROLE_TALENTS] = {};
