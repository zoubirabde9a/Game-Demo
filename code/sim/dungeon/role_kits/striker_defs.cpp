/* Striker (the fire mage, damage)'s talents, included by role_talents.cpp before the class
   tables (role_kits/striker.cpp has what they do). */

enum striker_talent
{
    StrikerTalent_Pyromancer,
    StrikerTalent_Detonate,
    StrikerTalent_Wildfire,
    StrikerTalent_Executioner,
    StrikerTalent_Combustion,
    StrikerTalent_Overload,
};

// NOTE(zoubir): per rank, or once taken
#define PYROMANCER_SHARE 0.06f
// NOTE(zoubir): Detonate's second rank: this much more a Searing stack
#define SEARING_HEAT_PER_STACK 5.f
#define WILDFIRE_SECONDS 2.f
#define EXECUTIONER_BELOW 0.3f
#define EXECUTIONER_SHARE 0.35f
#define OVERLOAD_SECONDS 3.f

global_variable talent_def StrikerTalentDefs[ROLE_TALENTS] =
{
        {"Pyromancer", "All your damage is higher", "+6% damage",
         TalentBranch_Role, 0, 0, 2, 0},
        {"Detonate", "C: blow up the Searing marks on a foe; in fire, every marked foe there",
         "rank 2: +5 a stack", TalentBranch_Role, 0, 1, 2, 0},
        {"Wildfire", "Meteor's ground burns longer, keeping marks alive", "+2 s of burning ground",
         TalentBranch_Role, 1, 0, 1, 0},
        {"Executioner", "Hit harder on monsters under 30% health", "+35% damage on them",
         TalentBranch_Role, 1, 1, 1, 0},
        {"Combustion", "V: for 6 s every hit you land is 40% harder", "a new spell",
         TalentBranch_Role, 2, 0, 1, 0},
        {"Overload", "Detonating a full mark gives back 3 s of Detonate",
         "-3 s Detonate on a full mark", TalentBranch_Role, 3, 0, 1, 0},
        {"Kindling", "Your fire burns hotter", "+4% damage",
         TalentBranch_Role, 2, 1, 4, 0},
        {"Ember Mantle", "More health", "+6% health",
         TalentBranch_Role, 3, 1, 4, 0},
        // NOTE(zoubir): slot 8, a class spell made stronger, by rank: to be written
        {"", "", "", TalentBranch_Role, 4, 0, 4, 0},
        {"Quickened Flame", "Every spell comes back sooner", "-4% cooldowns",
         TalentBranch_Role, 4, 1, 4, 0},
        {"Heat Shield", "You take less damage", "-4% damage taken",
         TalentBranch_Role, 5, 0, 4, 0},
        // NOTE(zoubir): slot 11, the capstone: to be written
        {"", "", "", TalentBranch_Role, 5, 1, 1, 0},
};

// NOTE(zoubir): the stat each slot raises (role_stats.cpp), RoleStat_None
// for a talent with code of its own
global_variable u8 StrikerTalentStats[ROLE_TALENTS] =
{
    RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None,
    RoleStat_Damage, RoleStat_Vitality, RoleStat_None, RoleStat_Haste, RoleStat_Armor, RoleStat_None,
};
