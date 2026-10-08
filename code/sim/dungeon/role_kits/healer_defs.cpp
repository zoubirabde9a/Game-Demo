/* Mender (healer)'s talents, included by role_talents.cpp before the class
   tables (role_kits/healer.cpp has what they do). */

enum healer_talent
{
    HealerTalent_SwiftMending,
    HealerTalent_Sanctuary,
    HealerTalent_DeepWard,
    HealerTalent_Renewal,
    HealerTalent_Radiance,
    HealerTalent_Miracle,
};

// NOTE(zoubir): per rank, or once taken
#define SWIFT_MENDING_SHARE 0.15f
#define DEEP_WARD_ABSORB 12.f
#define RENEWAL_SECONDS 4.f
#define RENEWAL_PER_SECOND 6.f
// NOTE(zoubir): Sanctuary's second rank
#define HALLOWED_RADIUS 1.3f
#define HALLOWED_HEAL 1.5f
#define MIRACLE_SECONDS 1.5f
#define MIRACLE_HP_SHARE 0.7f

global_variable talent_def HealerTalentDefs[ROLE_TALENTS] =
{
        {"Swift Mending", "Mending Bolt heals more", "+15% Mending Bolt healing",
         TalentBranch_Role, 0, 0, 2, 0},
        {"Sanctuary", "C: a circle at the cursor that heals allies inside",
         "rank 2: +30% radius, +50% healing", TalentBranch_Role, 0, 1, 2, 0},
        {"Deep Ward", "Ward absorbs more damage", "+12 absorbed",
         TalentBranch_Role, 1, 0, 1, 0},
        {"Renewal", "Mending Bolt also heals 24 over 4 s", "healing over time",
         TalentBranch_Role, 1, 1, 1, 0},
        {"Radiance", "V: heal and ward every ally around you", "a new spell",
         TalentBranch_Role, 2, 0, 1, 0},
        {"Miracle", "Revive the fallen in 1.5 s, at 70% health", "faster, stronger revives",
         TalentBranch_Role, 3, 0, 1, 0},
        {"Blessed Hands", "Your healing is stronger", "+6% healing",
         TalentBranch_Role, 2, 1, 4, 0},
        {"Inner Light", "More health", "+6% health",
         TalentBranch_Role, 3, 1, 4, 0},
        // NOTE(zoubir): slot 8, a class spell made stronger, by rank: to be written
        {"", "", "", TalentBranch_Role, 4, 0, 4, 0},
        {"Quickening", "Every spell comes back sooner", "-4% cooldowns",
         TalentBranch_Role, 4, 1, 4, 0},
        {"Light Feet", "You run faster", "+3% run speed",
         TalentBranch_Role, 5, 0, 4, 0},
        // NOTE(zoubir): slot 11, the capstone: to be written
        {"", "", "", TalentBranch_Role, 5, 1, 1, 0},
};

// NOTE(zoubir): the stat each slot raises (role_stats.cpp), RoleStat_None
// for a talent with code of its own
global_variable u8 HealerTalentStats[ROLE_TALENTS] =
{
    RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None,
    RoleStat_Healing, RoleStat_Vitality, RoleStat_None, RoleStat_Haste, RoleStat_Swiftness, RoleStat_None,
};
