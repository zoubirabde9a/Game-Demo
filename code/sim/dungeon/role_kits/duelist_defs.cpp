/* Duelist's numbers, talents and spells, included by role_talents.cpp
   before the class tables (role_kits/duelist.cpp has what they do). A
   class that has no spell on its first key yet (RoleSpells) is left out
   of the role picker and the bots, so until the kit lands the spell
   table is empty and the talents below are placeholders in the branch's
   shape. */

enum duelist_talent
{
    DuelistTalent_Finesse,
    DuelistTalent_Footwork,
    DuelistTalent_Precision,
    DuelistTalent_Bait,
    DuelistTalent_PerfectForm,
    DuelistTalent_Crescendo,
    DuelistTalent_KeenEdge,
    DuelistTalent_Parade,
    DuelistTalent_Flurry,
    DuelistTalent_QuickWrist,
    DuelistTalent_Stamina,
    DuelistTalent_Masterstroke,
};

// NOTE(zoubir): the same shape as every class's branch (role_talents.cpp);
// slot 4 will unlock the V spell. The Duelist has no C spell (X and C do
// nothing for it), so slot 1 is a passive
global_variable talent_def DuelistTalentDefs[ROLE_TALENTS] =
{
    {"Finesse", "All your damage is higher", "+5% damage",
     TalentBranch_Role, 0, 0, 2, 0},
    {"Footwork", "Lunge comes back sooner and you run faster after it", "-2 s Lunge",
     TalentBranch_Role, 0, 1, 2, 0},
    {"Precision", "Heartseeker's bonus on a hurt foe starts sooner", "below 45% health",
     TalentBranch_Role, 1, 0, 1, 0},
    {"Bait", "Riposte's guard lasts longer and a counter heals you", "longer guard",
     TalentBranch_Role, 1, 1, 1, 0},
    {"Perfect Form", "V: 8 s where Tempo cannot drop and Thrust strikes twice",
     "a new spell", TalentBranch_Role, 2, 0, 1, 0},
    {"Crescendo", "At 5 Tempo, Heartseeker cuts every foe in front and readies Lunge",
     "a sweeping finish", TalentBranch_Role, 3, 0, 1, 0},
    {"Keen Edge", "Your rapier cuts deeper", "+4% damage",
     TalentBranch_Role, 2, 1, 4, 0},
    {"Parade", "You take less damage", "-4% damage taken",
     TalentBranch_Role, 3, 1, 4, 0},
    {"Flurry", "Thrust hits harder", "+12% Thrust damage",
     TalentBranch_Role, 4, 0, 4, 0},
    {"Quick Wrist", "Every spell comes back sooner", "-4% cooldowns",
     TalentBranch_Role, 4, 1, 4, 0},
    {"Stamina", "You have more health", "+6% health",
     TalentBranch_Role, 5, 0, 4, 0},
    {"Masterstroke", "At 5 Tempo, Heartseeker strikes a second time", "a second strike",
     TalentBranch_Role, 5, 1, 1, 0},
};

// NOTE(zoubir): the stat each slot raises (role_stats.cpp), RoleStat_None
// for a talent with code of its own
global_variable u8 DuelistTalentStats[ROLE_TALENTS] =
{
    RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None,
    RoleStat_Damage, RoleStat_Armor, RoleStat_None, RoleStat_Haste, RoleStat_Vitality, RoleStat_None,
};

// NOTE(zoubir): in RoleKeys order: A, R, C, V, W, X, right click
global_variable role_spell DuelistSpells[ROLE_KEYS] =
{
    {}, {}, {}, {}, {}, {}, {},
};
