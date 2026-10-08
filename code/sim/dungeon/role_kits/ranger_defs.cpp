/* Ranger's numbers, talents and spells, included by role_talents.cpp
   before the class tables (role_kits/ranger.cpp has what they do). A class
   that has no spell on its first key yet (RoleSpells) is left out of the
   role picker and the bots. */

enum ranger_talent
{
    RangerTalent_0,
    RangerTalent_1,
    RangerTalent_2,
    RangerTalent_3,
    RangerTalent_4,
    RangerTalent_5,
};

// NOTE(zoubir): the same shape as every class's branch (role_talents.cpp):
// two talents of two ranks, two of one, one, one; slot 1 unlocks the C
// spell, slot 4 the V spell
global_variable talent_def RangerTalentDefs[ROLE_TALENTS] =
{
    {"", "", "", TalentBranch_Role, 0, 0, 2, 0},
    {"", "", "", TalentBranch_Role, 0, 1, 2, 0},
    {"", "", "", TalentBranch_Role, 1, 0, 1, 0},
    {"", "", "", TalentBranch_Role, 1, 1, 1, 0},
    {"", "", "", TalentBranch_Role, 2, 0, 1, 0},
    {"", "", "", TalentBranch_Role, 3, 0, 1, 0},
};

// NOTE(zoubir): in RoleKeys order: A, R, C, V, W, X, right click
global_variable role_spell RangerSpells[ROLE_KEYS] =
{
    {}, {}, {}, {}, {}, {}, {},
};
