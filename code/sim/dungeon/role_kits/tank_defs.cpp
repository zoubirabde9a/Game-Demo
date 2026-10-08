/* Bulwark (tank)'s talents, included by role_talents.cpp before the class
   tables (role_kits/tank.cpp has what they do). */

enum tank_talent
{
    TankTalent_IronSkin,
    TankTalent_Intercept,
    TankTalent_Provoke,
    TankTalent_Bastion,
    TankTalent_LastStand,
    TankTalent_ShatterArmor,
    TankTalent_Fortitude,
    TankTalent_PlateMastery,
    TankTalent_Juggernaut,
    TankTalent_Vengeance,
    TankTalent_BattleRhythm,
    TankTalent_Unbroken,
};

// NOTE(zoubir): per rank, or once taken
#define IRON_SKIN_SHARE 0.06f
#define PROVOKE_COOLDOWN 1.5f
#define PROVOKE_REACH 0.15f
#define BASTION_WALL_SECONDS 2.f
#define BASTION_STUN 0.5f
// NOTE(zoubir): Intercept's second rank wards the ally it lands by
#define GUARDIAN_WARD 30.f
#define SHATTER_SHARE 0.1f
#define SHATTER_SECONDS 2.f
// NOTE(zoubir): Juggernaut, per rank: Shield Charge comes back this much
// sooner and stuns this much longer, so four ranks take it from 12 s and
// 2 s to 8 s and 3 s
#define JUGGERNAUT_COOLDOWN 1.f
#define JUGGERNAUT_STUN 0.25f
// NOTE(zoubir): Unbroken, once per fight: a blow that would down the tank
// leaves it at UNBROKEN_HEALTH_SHARE of its health behind Shield Wall for
// UNBROKEN_WALL_SECONDS. TANK_FLAG_UNBROKEN_SPENT in ClassFlags says it
// went off this fight; it clears once no fight is on
#define UNBROKEN_HEALTH_SHARE 0.1f
#define UNBROKEN_WALL_SECONDS 5.f
#define TANK_FLAG_UNBROKEN_SPENT 0x4

global_variable talent_def TankTalentDefs[ROLE_TALENTS] =
{
        {"Iron Skin", "You take less damage", "-6% damage taken",
         TalentBranch_Role, 0, 0, 2, 0},
        {"Intercept", "C: leap to an ally and pull their foes onto you",
         "rank 2: ward the ally for 30", TalentBranch_Role, 0, 1, 2, 0},
        {"Provoke", "Taunt comes back sooner and reaches farther",
         "-1.5 s Taunt cooldown, +15% reach", TalentBranch_Role, 1, 0, 1, 0},
        {"Bastion", "Shield Slam's wall holds longer, its stun lasts longer",
         "+2 s Shield Wall, +0.5 s stun", TalentBranch_Role, 1, 1, 1, 0},
        {"Last Stand", "V: heal 30% of your health and stand behind Shield Wall for 6 s",
         "a new spell", TalentBranch_Role, 2, 0, 1, 0},
        {"Shatter Armor", "Sunder bites deeper and lasts longer", "Sunder +10%, +2 s",
         TalentBranch_Role, 3, 0, 1, 0},
        {"Fortitude", "More health", "+6% health",
         TalentBranch_Role, 2, 1, 4, 0},
        {"Plate Mastery", "You take less damage", "-4% damage taken",
         TalentBranch_Role, 3, 1, 4, 0},
        {"Juggernaut", "Shield Charge comes back sooner and stuns longer",
         "-1 s Shield Charge cooldown, +0.25 s stun", TalentBranch_Role, 4, 0, 4, 0},
        {"Vengeance", "Your blows land harder", "+4% damage",
         TalentBranch_Role, 4, 1, 4, 0},
        {"Battle Rhythm", "Every spell comes back sooner", "-4% cooldowns",
         TalentBranch_Role, 5, 0, 4, 0},
        {"Unbroken", "Once a fight, a blow that would down you leaves you standing",
         "at 10% health, behind Shield Wall for 5 s", TalentBranch_Role, 5, 1, 1, 0},
};

// NOTE(zoubir): the stat each slot raises (role_stats.cpp), RoleStat_None
// for a talent with code of its own
global_variable u8 TankTalentStats[ROLE_TALENTS] =
{
    RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None,
    RoleStat_Vitality, RoleStat_Armor, RoleStat_None, RoleStat_Damage, RoleStat_Haste, RoleStat_None,
};
