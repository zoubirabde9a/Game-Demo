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
    HealerTalent_BlessedHands,
    HealerTalent_InnerLight,
    HealerTalent_SteadfastWard,
    HealerTalent_Quickening,
    HealerTalent_LightFeet,
    HealerTalent_GuardianAngel,
    HealerTalent_HolyFire,
    HealerTalent_SmiteBolt,
    HealerTalent_Atonement,
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
// NOTE(zoubir): Steadfast Ward, per rank: more absorbed, seconds off
// Ward's cooldown
#define STEADFAST_WARD_ABSORB 8.f
#define STEADFAST_WARD_SECONDS 0.75f
// NOTE(zoubir): Guardian Angel: a blow that takes an ally under this share
// of their health cannot kill them; once it lands they heal this share of
// their health and hold at least this ward, once per ally per fight
#define GUARDIAN_ANGEL_HP_SHARE 0.3f
#define GUARDIAN_ANGEL_HEAL_SHARE 0.4f
#define GUARDIAN_ANGEL_WARD 40.f
// NOTE(zoubir): Atonement, per rank: the heal Holy Fire and Smite Bolt give
// the most hurt ally (OnHealerShot) is this share more
#define ATONEMENT_SHARE 0.2f

global_variable talent_def HealerTalentDefs[CLASS_TALENTS] =
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
        {"Steadfast Ward", "Ward absorbs more and comes back sooner",
         "+8 absorbed, -0.75 s cooldown", TalentBranch_Role, 4, 0, 4, 0},
        {"Quickening", "Every spell comes back sooner", "-4% cooldowns",
         TalentBranch_Role, 4, 1, 4, 0},
        {"Light Feet", "You run faster", "+3% run speed",
         TalentBranch_Role, 5, 0, 4, 0},
        {"Guardian Angel", "A blow taking an ally under 30% health can't kill: heal 40%, ward 40",
         "once per ally per fight", TalentBranch_Role, 5, 1, 1, 0},
    {"Holy Fire", "W: strike a foe with light; the most hurt ally heals for it",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Smite Bolt", "Right click: a quick bolt of light at a foe; the most hurt ally heals a little",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Atonement", "The light you strike foes with heals the most hurt ally more",
     "+20% of Holy Fire's and Smite Bolt's heal", TalentBranch_Role, 0, 0, 3, 0},
};

// NOTE(zoubir): the stat each slot raises (role_stats.cpp), RoleStat_None
// for a talent with code of its own
global_variable u8 HealerTalentStats[CLASS_TALENTS] =
{
    RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None,
    RoleStat_Healing, RoleStat_Vitality, RoleStat_None, RoleStat_Haste, RoleStat_Swiftness, RoleStat_None,
};
