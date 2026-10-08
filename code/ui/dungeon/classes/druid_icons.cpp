/* Druid icons (ui/dungeon/role_icons.cpp): the ability bar's pictures
   for the Druid's spells, in RoleKeys order, and the talent panel's for
   its branch. 0 leaves a cell empty. */

global_variable role_icon_painter *DruidIconPainters[ROLE_KEYS] = {};
global_variable talent_icon_painter *DruidTalentIconPainters[ROLE_TALENTS] = {};
