/* Run talents (run_tree.cpp): every talent the second tree can hold, as
   one table. A talent is one or two effects, each a kind and an amount a
   rank. The same talent can sit fixed in one class's tree and turn up in
   another's wild slot; nothing in it is class code, so every effect is
   read in a handful of places (run_effects.cpp, and RoleStatShare for the
   seven that are plain stats).

   A negative amount is a cost: Glass Cannon deals more and takes more.

   Kinds says which role kinds may roll a talent in a wild slot (a healer
   never rolls Execute); a fixed talent is placed by hand and ignores it.
   Keystones are one-rank talents with a cost, rolled only in the wild
   keystone slot. */

enum run_effect
{
    RunEffect_None,
    // NOTE(zoubir): the seven plain stats, read through RoleStatShare
    RunEffect_Damage,
    RunEffect_Armor,
    RunEffect_Health,
    RunEffect_Haste,
    RunEffect_Healing,
    RunEffect_Speed,
    RunEffect_Leech,
    // NOTE(zoubir): more damage on some hits (RunDealtScale)
    RunEffect_Execute,
    RunEffect_Opener,
    RunEffect_Bossbane,
    RunEffect_Packbane,
    RunEffect_Desperate,
    RunEffect_Cadence,
    RunEffect_Frenzy,
    // NOTE(zoubir): less damage taken while low (RunTakenScale)
    RunEffect_LastBreath,
    // NOTE(zoubir): on a kill (OnRunKill)
    RunEffect_Feast,
    RunEffect_Refund,
    // NOTE(zoubir): over time and round the player (UpdateRunTrees)
    RunEffect_Thorns,
    RunEffect_Regen,
    RunEffect_Aura,
    RunEffect_Anthem,
    // NOTE(zoubir): threat made, and healing received and overhealing
    // (HealPlayer)
    RunEffect_Threat,
    RunEffect_HealTaken,
    RunEffect_Overflow,
    // NOTE(zoubir): how the run goes (run_tree.cpp, run_effects.cpp):
    // more damage early in a fight, more for every room cleared this run,
    // a heal once a fight when low, kills that heal allies
    RunEffect_Vanguard,
    RunEffect_Glory,
    RunEffect_Lifeline,
    RunEffect_SharedFeast,
    RunEffect_Count
};

// NOTE(zoubir): the numbers the effects share
#define RUN_EXECUTE_BELOW 0.35f
#define RUN_OPENER_ABOVE 0.8f
#define RUN_DESPERATE_BELOW 0.4f
#define RUN_CADENCE_EVERY 5
#define RUN_FRENZY_SECONDS 6.f
#define RUN_AURA_REACH 320.f
#define RUN_VANGUARD_SECONDS 8.f
#define RUN_GLORY_ROOMS 10
#define RUN_LIFELINE_BELOW 0.3f

#define RUN_KIND_TANK (1u << RoleKind_Tank)
#define RUN_KIND_HEALER (1u << RoleKind_Healer)
#define RUN_KIND_RANGED (1u << RoleKind_Ranged)
#define RUN_KIND_MELEE (1u << RoleKind_Melee)
#define RUN_KIND_DAMAGE (RUN_KIND_RANGED | RUN_KIND_MELEE)
#define RUN_KIND_ALL (RUN_KIND_TANK | RUN_KIND_HEALER | RUN_KIND_DAMAGE)

struct run_mod_def
{
    char *Name;
    // NOTE(zoubir): what it does, in a line
    char *Summary;
    u8 Effect[2];
    float PerRank[2];
    u32 Kinds;
    bool32 Keystone;
};

enum run_mod
{
    RunMod_None,
    // NOTE(zoubir): the wild pool
    RunMod_KeenEdge, RunMod_Toughened, RunMod_Vigor, RunMod_Quickened, RunMod_MendingTouch,
    RunMod_Wayfarer, RunMod_Leeching, RunMod_Finisher, RunMod_Ambusher, RunMod_Giantslayer,
    RunMod_Cleaver, RunMod_Cornered, RunMod_Stubborn, RunMod_Feast, RunMod_Reprisal,
    RunMod_Cadence, RunMod_Spikes, RunMod_SecondBreath, RunMod_Bloodrush, RunMod_Rallying,
    RunMod_WarSong, RunMod_Menace, RunMod_Unseen, RunMod_Overflowing, RunMod_Receptive,
    RunMod_OpeningSalvo, RunMod_RisingGlory, RunMod_Lifeline, RunMod_SharedSpoils,
    // NOTE(zoubir): the wild keystones
    RunMod_GlassCannon, RunMod_Colossus, RunMod_BloodPact, RunMod_Zealotry, RunMod_Headsman,
    RunMod_Martyr, RunMod_Unyielding, RunMod_Warlord, RunMod_Bloodbath, RunMod_Thornwall,
    RunMod_Blitz, RunMod_Legend,
    // NOTE(zoubir): the classes' fixed talents (run_tree.cpp lays them out)
    RunMod_KindledWrath, RunMod_CinderSkin, RunMod_FireWithin, RunMod_Pyroclasm, RunMod_Smoulder,
    RunMod_PhoenixHeart,
    RunMod_Menacing, RunMod_Stoneform, RunMod_Retaliation, RunMod_SteadyHeart, RunMod_ShieldBrother,
    RunMod_LivingFortress,
    RunMod_Bountiful, RunMod_SpiritWard, RunMod_Serenity, RunMod_Hymn, RunMod_Sanctified,
    RunMod_SaintsVigil,
    RunMod_Patience, RunMod_BigGame, RunMod_Trailwise, RunMod_KillingRhythm, RunMod_Trophy,
    RunMod_ApexPredator,
    RunMod_CorneredBeast, RunMod_Gorge, RunMod_ThickHide, RunMod_Savagery, RunMod_RedMist,
    RunMod_UndyingFury,
    RunMod_Assassinate, RunMod_Shroud, RunMod_Ambush, RunMod_QuickHands, RunMod_Slip,
    RunMod_DeathMark,
    RunMod_StaticBuild, RunMod_StormFront, RunMod_Conductor, RunMod_Grounded, RunMod_Surge,
    RunMod_EyeOfTheStorm,
    RunMod_Footwork, RunMod_Precision, RunMod_Measured, RunMod_CoupDeGrace, RunMod_Panache,
    RunMod_PerfectForm,
    RunMod_ShatterPoint, RunMod_GlacialSkin, RunMod_DeepWinter, RunMod_ColdCalculation,
    RunMod_Permafrost, RunMod_AbsoluteZero,
    RunMod_Overgrowth, RunMod_Barkskin, RunMod_WildBloom, RunMod_Verdant, RunMod_Moonlit,
    RunMod_HeartOfTheWild,
    RunMod_Count,
    RunMod_WildFirst = RunMod_KeenEdge,
    RunMod_WildLast = RunMod_Legend,
};

#define RUN_MOD(Name, Summary, E0, P0, E1, P1, Kinds, Keystone) \
    {Name, Summary, {RunEffect_##E0, RunEffect_##E1}, {P0, P1}, Kinds, Keystone}

global_variable run_mod_def RunModDefs[RunMod_Count] =
{
    RUN_MOD("", "", None, 0.f, None, 0.f, 0, false),

    RUN_MOD("Keen Edge", "You deal more damage", Damage, 0.05f, None, 0.f, RUN_KIND_ALL, false),
    RUN_MOD("Toughened", "You take less damage", Armor, 0.05f, None, 0.f, RUN_KIND_ALL, false),
    RUN_MOD("Vigor", "More health", Health, 0.07f, None, 0.f, RUN_KIND_ALL, false),
    RUN_MOD("Quickened", "Every class spell comes back sooner", Haste, 0.05f, None, 0.f, RUN_KIND_ALL, false),
    RUN_MOD("Mending Touch", "Your heals heal more", Healing, 0.07f, None, 0.f, RUN_KIND_HEALER, false),
    RUN_MOD("Wayfarer", "You run faster and shrug off a little", Speed, 0.05f, Armor, 0.02f, RUN_KIND_ALL, false),
    RUN_MOD("Leeching Strikes", "Some of the damage you deal heals you", Leech, 0.025f, None, 0.f,
            RUN_KIND_TANK | RUN_KIND_DAMAGE, false),
    RUN_MOD("Finisher", "More damage to foes under 35% health", Execute, 0.16f, None, 0.f,
            RUN_KIND_DAMAGE, false),
    RUN_MOD("Ambusher", "More damage to foes above 80% health", Opener, 0.2f, None, 0.f,
            RUN_KIND_DAMAGE, false),
    RUN_MOD("Giantslayer", "More damage to bosses", Bossbane, 0.1f, None, 0.f,
            RUN_KIND_TANK | RUN_KIND_DAMAGE, false),
    RUN_MOD("Cleaver", "More damage to everything but bosses", Packbane, 0.1f, None, 0.f,
            RUN_KIND_TANK | RUN_KIND_DAMAGE, false),
    RUN_MOD("Cornered", "More damage while you are under 40% health", Desperate, 0.22f, None, 0.f,
            RUN_KIND_TANK | RUN_KIND_MELEE, false),
    RUN_MOD("Stubborn", "Less damage taken while you are under 40% health", LastBreath, 0.1f, None, 0.f,
            RUN_KIND_ALL, false),
    RUN_MOD("Feast", "A kill heals you", Feast, 0.03f, None, 0.f,
            RUN_KIND_TANK | RUN_KIND_DAMAGE, false),
    RUN_MOD("Reprisal", "A kill takes time off every class spell", Refund, 0.5f, None, 0.f,
            RUN_KIND_DAMAGE, false),
    RUN_MOD("Cadence", "Every fifth hit lands much harder", Cadence, 0.25f, None, 0.f,
            RUN_KIND_DAMAGE, false),
    RUN_MOD("Spikes", "Monsters that hit you take some of it back", Thorns, 0.15f, None, 0.f,
            RUN_KIND_TANK | RUN_KIND_MELEE, false),
    RUN_MOD("Second Breath", "Heal a little every second of a fight", Regen, 0.004f, None, 0.f,
            RUN_KIND_ALL, false),
    RUN_MOD("Bloodrush", "A kill makes you deal more damage for 6 s", Frenzy, 0.11f, None, 0.f,
            RUN_KIND_DAMAGE, false),
    RUN_MOD("Rallying Presence", "Allies near you take less damage", Aura, 0.04f, None, 0.f,
            RUN_KIND_TANK | RUN_KIND_HEALER, false),
    RUN_MOD("War Song", "Allies near you deal more damage", Anthem, 0.03f, None, 0.f,
            RUN_KIND_TANK | RUN_KIND_HEALER, false),
    RUN_MOD("Menace", "Your damage makes more threat", Threat, 0.25f, None, 0.f, RUN_KIND_TANK, false),
    RUN_MOD("Unseen", "Your damage makes less threat", Threat, -0.15f, None, 0.f, RUN_KIND_DAMAGE, false),
    RUN_MOD("Overflowing Grace", "Healing past full health becomes a ward", Overflow, 0.25f, None, 0.f,
            RUN_KIND_HEALER, false),
    RUN_MOD("Receptive", "Heals on you heal more", HealTaken, 0.08f, None, 0.f, RUN_KIND_ALL, false),
    RUN_MOD("Opening Salvo", "More damage in the first 8 s of a fight", Vanguard, 0.2f, None, 0.f,
            RUN_KIND_TANK | RUN_KIND_DAMAGE, false),
    RUN_MOD("Rising Glory", "More damage for every room cleared this run", Glory, 0.005f, None, 0.f,
            RUN_KIND_ALL, false),
    RUN_MOD("Lifeline", "Once a fight, falling under 30% health heals you", Lifeline, 0.15f, None, 0.f,
            RUN_KIND_ALL, false),
    RUN_MOD("Shared Spoils", "Your kills heal allies near you", SharedFeast, 0.015f, None, 0.f,
            RUN_KIND_TANK | RUN_KIND_DAMAGE, false),

    RUN_MOD("Glass Cannon", "Deal much more damage, and take more", Damage, 0.18f, Armor, -0.12f,
            RUN_KIND_DAMAGE, true),
    RUN_MOD("Colossus", "Much more health, and a slower run", Health, 0.25f, Speed, -0.08f,
            RUN_KIND_TANK | RUN_KIND_HEALER | RUN_KIND_MELEE, true),
    RUN_MOD("Blood Pact", "Heal from your hits, and less from heals", Leech, 0.06f, HealTaken, -0.3f,
            RUN_KIND_DAMAGE, true),
    RUN_MOD("Zealotry", "Spells come back much sooner, for less health", Haste, 0.15f, Health, -0.1f,
            RUN_KIND_DAMAGE, true),
    RUN_MOD("Headsman", "Far more damage to foes under 35%, less to fresh ones", Execute, 0.4f,
            Opener, -0.15f, RUN_KIND_DAMAGE, true),
    RUN_MOD("Martyr", "Heal much more, and take more damage", Healing, 0.25f, Armor, -0.06f,
            RUN_KIND_HEALER, true),
    RUN_MOD("Unyielding", "Far less damage taken when low, and less dealt", LastBreath, 0.3f,
            Damage, -0.06f, RUN_KIND_TANK, true),
    RUN_MOD("Warlord", "Allies near you deal more, and you deal less", Anthem, 0.06f, Damage, -0.1f,
            RUN_KIND_TANK | RUN_KIND_HEALER, true),
    RUN_MOD("Bloodbath", "A kill makes you far deadlier, bosses take less", Frenzy, 0.2f,
            Bossbane, -0.08f, RUN_KIND_DAMAGE, true),
    RUN_MOD("Thornwall", "Monsters that hit you take half of it back, and a slower run", Thorns, 0.5f,
            Speed, -0.05f, RUN_KIND_TANK, true),
    RUN_MOD("Blitz", "A huge opening burst each fight, and less damage after it", Vanguard, 0.6f,
            Damage, -0.08f, RUN_KIND_DAMAGE, true),
    RUN_MOD("Legend", "Far more damage for every room cleared this run, for less health", Glory, 0.015f,
            Health, -0.1f, RUN_KIND_ALL, true),

    // NOTE(zoubir): Fire Mage, Ashbringer
    RUN_MOD("Kindled Wrath", "More damage to foes above 80% health", Opener, 0.16f, None, 0.f, 0, false),
    RUN_MOD("Cinder Skin", "You take less damage", Armor, 0.04f, None, 0.f, 0, false),
    RUN_MOD("Fire Within", "More damage to bosses", Bossbane, 0.08f, None, 0.f, 0, false),
    RUN_MOD("Pyroclasm", "More damage to everything but bosses", Packbane, 0.08f, None, 0.f, 0, false),
    RUN_MOD("Smoulder", "A kill makes you deal more damage for 6 s", Frenzy, 0.09f, None, 0.f, 0, false),
    RUN_MOD("Phoenix Heart", "Low on health, you burn hotter and take less", Desperate, 0.25f,
            LastBreath, 0.15f, 0, false),
    // NOTE(zoubir): Bulwark, Iron Vanguard
    RUN_MOD("Menacing", "More threat from your damage, and a harder hide", Threat, 0.2f, Armor, 0.02f, 0, false),
    RUN_MOD("Stoneform", "You take less damage", Armor, 0.06f, None, 0.f, 0, false),
    RUN_MOD("Retaliation", "Monsters that hit you take some back, and more health", Thorns, 0.12f, Health, 0.03f, 0, false),
    RUN_MOD("Steady Heart", "Heal a little every second of a fight", Regen, 0.005f, None, 0.f, 0, false),
    RUN_MOD("Shield Brother", "You and the allies near you take less damage", Aura, 0.03f, Armor, 0.02f, 0, false),
    RUN_MOD("Living Fortress", "More health, and far less damage taken when low", Health, 0.2f,
            LastBreath, 0.2f, 0, false),
    // NOTE(zoubir): Mender, Grace
    RUN_MOD("Bountiful", "Your heals heal more", Healing, 0.07f, None, 0.f, 0, false),
    RUN_MOD("Spirit Ward", "Healing past full health becomes a ward", Overflow, 0.2f, None, 0.f, 0, false),
    RUN_MOD("Serenity", "Every class spell comes back sooner", Haste, 0.04f, None, 0.f, 0, false),
    RUN_MOD("Hymn", "Allies near you deal more and take less", Anthem, 0.025f, Aura, 0.015f, 0, false),
    RUN_MOD("Sanctified", "You take less damage", Armor, 0.05f, None, 0.f, 0, false),
    RUN_MOD("Saint's Vigil", "Allies near you take less, and heals on you heal more", Aura, 0.06f,
            HealTaken, 0.15f, 0, false),
    // NOTE(zoubir): Ranger, Wildstalker
    RUN_MOD("Patience", "More damage to foes above 80% health", Opener, 0.17f, None, 0.f, 0, false),
    RUN_MOD("Big Game", "More damage to bosses", Bossbane, 0.08f, None, 0.f, 0, false),
    RUN_MOD("Trailwise", "You run faster and shrug off a little", Speed, 0.04f, Armor, 0.02f, 0, false),
    RUN_MOD("Killing Rhythm", "Every fifth hit lands much harder", Cadence, 0.2f, None, 0.f, 0, false),
    RUN_MOD("Trophy", "A kill takes time off every class spell", Refund, 0.4f, None, 0.f, 0, false),
    RUN_MOD("Apex Predator", "Far more damage to weak foes, and a kill makes you deadlier",
            Execute, 0.3f, Frenzy, 0.1f, 0, false),
    // NOTE(zoubir): Berserker, Bloodrage
    RUN_MOD("Cornered Beast", "More damage while you are under 40% health", Desperate, 0.2f, None, 0.f, 0, false),
    RUN_MOD("Gorge", "A kill heals you", Feast, 0.025f, None, 0.f, 0, false),
    RUN_MOD("Thick Hide", "More health", Health, 0.06f, None, 0.f, 0, false),
    RUN_MOD("Savagery", "More damage to everything but bosses", Packbane, 0.09f, None, 0.f, 0, false),
    RUN_MOD("Red Mist", "A kill makes you deal more damage for 6 s", Frenzy, 0.1f, None, 0.f, 0, false),
    RUN_MOD("Undying Fury", "Your hits heal you, and you take far less when low", Leech, 0.05f,
            LastBreath, 0.2f, 0, false),
    // NOTE(zoubir): Shadowblade, Nightfall
    RUN_MOD("Assassinate", "More damage to foes under 35% health", Execute, 0.14f, None, 0.f, 0, false),
    RUN_MOD("Shroud", "Less threat, and less damage taken when low", Threat, -0.12f, LastBreath, 0.08f, 0, false),
    RUN_MOD("Ambush", "More damage to foes above 80% health", Opener, 0.18f, None, 0.f, 0, false),
    RUN_MOD("Quick Hands", "Every class spell comes back sooner", Haste, 0.04f, None, 0.f, 0, false),
    RUN_MOD("Slip", "You run faster and shrug off a little", Speed, 0.04f, Armor, 0.02f, 0, false),
    RUN_MOD("Death Mark", "More damage to bosses, and every fifth hit lands harder", Bossbane, 0.12f,
            Cadence, 0.15f, 0, false),
    // NOTE(zoubir): Stormcaller, Tempest
    RUN_MOD("Static Build", "Every fifth hit lands much harder", Cadence, 0.2f, None, 0.f, 0, false),
    RUN_MOD("Storm Front", "More damage to everything but bosses", Packbane, 0.09f, None, 0.f, 0, false),
    RUN_MOD("Conductor", "Every class spell comes back sooner", Haste, 0.04f, None, 0.f, 0, false),
    RUN_MOD("Grounded", "You take less damage", Armor, 0.04f, None, 0.f, 0, false),
    RUN_MOD("Surge", "A kill takes time off every class spell", Refund, 0.4f, None, 0.f, 0, false),
    RUN_MOD("Eye of the Storm", "More damage, and a kill makes you deadlier", Damage, 0.1f,
            Frenzy, 0.1f, 0, false),
    // NOTE(zoubir): Duelist, Flourish
    RUN_MOD("Footwork", "You run faster and shrug off a little", Speed, 0.04f, Armor, 0.02f, 0, false),
    RUN_MOD("Precision", "Every fifth hit lands much harder", Cadence, 0.2f, None, 0.f, 0, false),
    RUN_MOD("Measured", "You take less damage", Armor, 0.04f, None, 0.f, 0, false),
    RUN_MOD("Coup de Grace", "More damage to foes under 35% health", Execute, 0.14f, None, 0.f, 0, false),
    RUN_MOD("Panache", "A kill makes you deal more damage for 6 s", Frenzy, 0.09f, None, 0.f, 0, false),
    RUN_MOD("Perfect Form", "More damage, and less taken when low", Damage, 0.08f, LastBreath, 0.15f, 0, false),
    // NOTE(zoubir): Frost Mage, Rime
    RUN_MOD("Shatter Point", "More damage to foes under 35% health", Execute, 0.14f, None, 0.f, 0, false),
    RUN_MOD("Glacial Skin", "You take less damage", Armor, 0.05f, None, 0.f, 0, false),
    RUN_MOD("Deep Winter", "More damage to bosses", Bossbane, 0.08f, None, 0.f, 0, false),
    RUN_MOD("Cold Calculation", "Every class spell comes back sooner", Haste, 0.04f, None, 0.f, 0, false),
    RUN_MOD("Permafrost", "Heal a little every second of a fight", Regen, 0.003f, None, 0.f, 0, false),
    RUN_MOD("Absolute Zero", "Far more damage to packs, and allies near you take less", Packbane, 0.15f,
            Aura, 0.04f, 0, false),
    // NOTE(zoubir): Druid, Grove
    RUN_MOD("Overgrowth", "Healing past full health becomes a ward", Overflow, 0.2f, None, 0.f, 0, false),
    RUN_MOD("Barkskin", "You take less damage", Armor, 0.04f, None, 0.f, 0, false),
    RUN_MOD("Wild Bloom", "Your heals heal more", Healing, 0.07f, None, 0.f, 0, false),
    RUN_MOD("Verdant", "Heal a little every second of a fight", Regen, 0.003f, None, 0.f, 0, false),
    RUN_MOD("Moonlit", "You deal more damage", Damage, 0.04f, None, 0.f, 0, false),
    RUN_MOD("Heart of the Wild", "Heal more, and more health", Healing, 0.1f, Health, 0.1f, 0, false),
};
#undef RUN_MOD

// NOTE(zoubir): an effect's amount as the tooltip writes it: "+5%
// damage", "-0.5 s cooldowns on a kill". Shares are fractions, Refund is
// seconds
global_variable char *RunEffectFormats[RunEffect_Count] =
{
    "",
    "%+.0f%% damage",
    "%+.0f%% damage resisted",
    "%+.0f%% health",
    "%+.0f%% spell cooldown speed",
    "%+.0f%% healing",
    "%+.0f%% run speed",
    "%.1f%% of damage healed",
    "%+.0f%% damage to foes under 35%%",
    "%+.0f%% damage to foes above 80%%",
    "%+.0f%% damage to bosses",
    "%+.0f%% damage to non-bosses",
    "%+.0f%% damage under 40%% health",
    "%+.0f%% on every fifth hit",
    "%+.0f%% damage for 6 s after a kill",
    "%+.0f%% damage resisted under 40%% health",
    "%.1f%% health back on a kill",
    "%.1f s off spells on a kill",
    "%.0f%% of a hit sent back",
    "%.1f%% health a second in a fight",
    "%+.0f%% damage resisted by allies near",
    "%+.0f%% damage by allies near",
    "%+.0f%% threat",
    "%+.0f%% healing received",
    "%.0f%% of overhealing as a ward",
    "%+.0f%% damage in a fight's first 8 s",
    "%+.1f%% damage a room cleared, 10 at most",
    "%.0f%% health back once a fight when low",
    "%.1f%% health to allies near on a kill",
};

// NOTE(zoubir): Amount as RunEffectFormats prints it: a share as
// percent, seconds as they are
inline float
RunEffectShown(u32 Effect, float Amount)
{
    float Result = Effect == RunEffect_Refund ? Amount : 100.f * Amount;
    return Result;
}
