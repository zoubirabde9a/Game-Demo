/* Druid's numbers, talents and spells, included by role_talents.cpp
   before the class tables (role_kits/druid.cpp has what they do). A class
   that has no spell on its first key yet (RoleSpells) is left out of the
   role picker and the bots. */

// NOTE(zoubir): Bloom, the Druid's resource: its damage spells grow it,
// its heals spend it all for more healing, so the Druid swings between
// the two halves of its kit
#define DRUID_BLOOM_MOST 5
// NOTE(zoubir): between fights, this long after it last grew, it fades
// one a second
#define DRUID_BLOOM_HOLD 6.f

// NOTE(zoubir): bolts fly at this speed; a hit lands when its bolt gets
// there (client/dungeon/classes/druid.cpp flies the same speed)
#define DRUID_BOLT_SPEED 1000.f
// NOTE(zoubir): a Moonfire or a Rejuvenation is sent to clients again
// this often while it lasts, so its look never drops out of their short
// list of bursts (client/dungeon/role_fx.cpp) and follows who it is on
#define DRUID_KEEP_SECONDS 1.5f

// NOTE(zoubir): Wrath (right click): a quick bolt of nature at the foe
// aimed at, the filler; grows a Bloom
#define WRATH_RANGE 520.f
#define WRATH_DAMAGE 10.f
#define WRATH_COOLDOWN 1.2f

// NOTE(zoubir): Moonfire (X): moonlight burns the foe aimed at, a little
// at once and MOONFIRE_TICK_DAMAGE each second for MOONFIRE_SECONDS
#define MOONFIRE_RANGE 560.f
#define MOONFIRE_DAMAGE 6.f
#define MOONFIRE_TICK_DAMAGE 2.5f
#define MOONFIRE_SECONDS 12.f
#define MOONFIRE_COOLDOWN 6.f

// NOTE(zoubir): Rejuvenation (A): an ally heals REJUVENATION_PER_SECOND a
// second for REJUVENATION_SECONDS, and at once REJUVENATION_PER_BLOOM for
// each Bloom spent (all of it)
#define REJUVENATION_RANGE 500.f
#define REJUVENATION_PER_SECOND 8.f
#define REJUVENATION_SECONDS 8.f
#define REJUVENATION_PER_BLOOM 6.f
#define REJUVENATION_COOLDOWN 4.f

// NOTE(zoubir): Starfire (R): after its cast (PlayerSpell_DruidA) a heavy
// star at the foe aimed at; grows STARFIRE_BLOOM Blooms
#define STARFIRE_RANGE 560.f
#define STARFIRE_DAMAGE 26.f
#define STARFIRE_BLOOM 2
// NOTE(zoubir): the star falls this long before it strikes
#define STARFIRE_FALL 0.35f
#define STARFIRE_COOLDOWN 6.f

// NOTE(zoubir): Regrowth (W): an ally heals REGROWTH_HEAL at once and
// REGROWTH_PER_BLOOM more for each Bloom spent (all of it)
#define REGROWTH_RANGE 500.f
#define REGROWTH_HEAL 30.f
#define REGROWTH_PER_BLOOM 8.f
#define REGROWTH_COOLDOWN 5.f

// NOTE(zoubir): Entangling Roots (C, from the tree): roots every foe in
// the circle at the cursor for ROOTS_SECONDS and hurts them each second;
// rank 2 holds ROOTS_SECONDS_2
#define ROOTS_RADIUS 80.f
#define ROOTS_SECONDS 3.f
#define ROOTS_SECONDS_2 4.5f
#define ROOTS_TICK_DAMAGE 4.f
#define ROOTS_COOLDOWN 18.f

// NOTE(zoubir): Tranquility (V, from the tree): a channel
// (PlayerSpell_DruidB) that heals every ally within TRANQUILITY_RADIUS of
// the Druid for TRANQUILITY_HEAL each TRANQUILITY_TICK
#define TRANQUILITY_RADIUS 260.f
// NOTE(zoubir): Grove's pair sets it against Regrowth, the big single
// heal, so it heals more and comes back sooner than it did beside it
#define TRANQUILITY_HEAL 10.f
#define TRANQUILITY_TICK 0.5f
#define TRANQUILITY_COOLDOWN 20.f

enum druid_talent
{
    DruidTalent_NaturesWrath,
    DruidTalent_EntanglingRoots,
    DruidTalent_Verdancy,
    DruidTalent_Eclipse,
    DruidTalent_Tranquility,
    DruidTalent_Symbiosis,
    DruidTalent_GiftOfTheWild,
    DruidTalent_Barkskin,
    DruidTalent_Overgrowth,
    DruidTalent_Swiftmend,
    DruidTalent_StarlitFury,
    DruidTalent_WildGrowth,
    DruidTalent_Rejuvenation,
    DruidTalent_Starfire,
    DruidTalent_Moonfire,
    DruidTalent_Regrowth,
    DruidTalent_LunarBloom,
};

// NOTE(zoubir): per rank, or once taken
#define NATURES_WRATH_SHARE 0.06f
// NOTE(zoubir): Verdancy: Rejuvenation heals this much more and longer
#define VERDANCY_SHARE 0.3f
#define VERDANCY_SECONDS 3.f
// NOTE(zoubir): Eclipse: Wrath and Starfire on a foe under the Druid's
// Moonfire hit this share harder, and Starfire leaves the Druid's
// Moonfire on what it strikes, so the talent pays off either spell of
// the Moon branch's pair
#define ECLIPSE_SHARE 0.25f
// NOTE(zoubir): Symbiosis: Wrath and Starfire heal the most hurt ally
// within this for this share of what they deal
#define SYMBIOSIS_REACH 400.f
#define SYMBIOSIS_SHARE 0.25f
// NOTE(zoubir): Overgrowth, per rank: Regrowth heals this much more
#define OVERGROWTH_SHARE 0.1f
// NOTE(zoubir): Wild Growth, the capstone: Rejuvenation also lands on the
// WILD_GROWTH_ALLIES most hurt other allies within this of its target
#define WILD_GROWTH_REACH 260.f
#define WILD_GROWTH_ALLIES 2

// NOTE(zoubir): the same shape as every class's branch (role_talents.cpp);
// slot 1 unlocks the C spell, slot 4 the V spell
global_variable talent_def DruidTalentDefs[CLASS_TALENTS] =
{
    {"Nature's Wrath", "All your damage is higher", "+6% damage",
     TalentBranch_Role, 0, 0, 2, 0},
    {"Entangling Roots", "C: roots every foe in a circle and hurts them; rank 2, a longer hold",
     "a new spell", TalentBranch_Role, 0, 1, 2, 0},
    {"Verdancy", "Rejuvenation heals 30% more and lasts 3 s longer", "+30%, +3 s",
     TalentBranch_Role, 1, 0, 1, 0},
    {"Eclipse", "Wrath and Starfire hit a foe under your Moonfire 25% harder; Starfire leaves a Moonfire",
     "+25% on Moonfire",
     TalentBranch_Role, 1, 1, 1, 0},
    {"Tranquility", "V: a 3 s channel that heals every ally around you", "a new spell",
     TalentBranch_Role, 2, 0, 1, 0},
    {"Symbiosis", "Wrath and Starfire heal the most hurt ally for a quarter of what they deal",
     "damage heals 25%", TalentBranch_Role, 3, 0, 1, 0},
    {"Gift of the Wild", "Your heals give more", "+6% healing",
     TalentBranch_Role, 2, 1, 4, 0},
    {"Barkskin", "More health", "+6% health",
     TalentBranch_Role, 3, 1, 4, 0},
    {"Overgrowth", "Regrowth heals more", "+10% Regrowth",
     TalentBranch_Role, 4, 0, 4, 0},
    {"Swiftmend", "Every spell comes back sooner", "-4% cooldowns",
     TalentBranch_Role, 4, 1, 4, 0},
    {"Starlit Fury", "Your spells hit harder", "+4% damage",
     TalentBranch_Role, 5, 0, 4, 0},
    {"Wild Growth", "Rejuvenation also lands on the two most hurt allies near its target",
     "Rejuvenation on 3", TalentBranch_Role, 5, 1, 1, 0},
    {"Rejuvenation", "A: an ally heals over 8 s, more for each Bloom spent",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Starfire", "R: a 1.5 s cast, then a star at a foe that grows two Bloom",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Moonfire", "X: moonlight burns a foe for 12 s",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Regrowth", "W: a big heal on an ally, more for each Bloom spent",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Lunar Bloom", "Wrath and Starfire on a foe under your Moonfire grow a Bloom more",
     "+1 Bloom on Moonfire", TalentBranch_Role, 0, 0, 1, 0},
};

// NOTE(zoubir): the stat each slot raises (role_stats.cpp), RoleStat_None
// for a talent with code of its own
global_variable u8 DruidTalentStats[CLASS_TALENTS] =
{
    RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None,
    RoleStat_Healing, RoleStat_Vitality, RoleStat_None, RoleStat_Haste, RoleStat_Damage, RoleStat_None,
};

// NOTE(zoubir): in RoleKeys order: A, R, C, V, W, X, right click, G, T
global_variable role_spell DruidSpells[ROLE_KEYS] =
{
    {"Rejuvenation", REJUVENATION_COOLDOWN,
     "Rejuvenation: an ally heals over 8 s, and at once for each Bloom spent",
     RoleAim_Ally, REJUVENATION_RANGE, 0, "All Bloom"},
    {"Starfire", STARFIRE_COOLDOWN, "Starfire: 1.5 s cast, a falling star on a foe; grows two Bloom",
     RoleAim_Foe, STARFIRE_RANGE, 0},
    {"Entangling Roots", ROOTS_COOLDOWN, "Entangling Roots: hold every foe in the circle and hurt them",
     RoleAim_Ground, ROOTS_RADIUS, DruidTalent_EntanglingRoots + 1},
    {"Tranquility", TRANQUILITY_COOLDOWN, "Tranquility: 3 s channel that heals every ally around you",
     RoleAim_None, 0.f, DruidTalent_Tranquility + 1},
    {"Regrowth", REGROWTH_COOLDOWN, "Regrowth: heal an ally at once, more for each Bloom spent",
     RoleAim_Ally, REGROWTH_RANGE, 0, "All Bloom"},
    {"Moonfire", MOONFIRE_COOLDOWN, "Moonfire: burn a foe with moonlight for 12 s",
     RoleAim_Foe, MOONFIRE_RANGE, 0},
    {"Wrath", WRATH_COOLDOWN, "Wrath: a quick bolt of nature at a foe; grows a Bloom",
     RoleAim_None, WRATH_RANGE, 0},
};
