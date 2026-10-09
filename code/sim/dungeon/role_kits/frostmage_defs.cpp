/* Frost Mage's numbers, talents and spells, included by role_talents.cpp
   before the class tables (role_kits/frostmage.cpp has what they do). A
   class that has no spell on its first key yet (RoleSpells) is left out
   of the role picker and the bots. */

// NOTE(zoubir): Icicles, the Frost Mage's resource: each Frostbolt that
// lands grows one, Glacial Spike spends them all
#define FROSTMAGE_ICICLES_MOST 5
// NOTE(zoubir): between fights, this long after the last one grew, they
// melt one a second
#define FROSTMAGE_ICICLE_HOLD 6.f

// NOTE(zoubir): Shatter: every hit of the Frost Mage on a frozen foe
// (rooted or stunned) deals this much more
#define FROST_SHATTER_SHARE 0.4f

// NOTE(zoubir): bolts fly at this speed; a hit lands when its bolt gets
// there (client/dungeon/classes/frostmage.cpp flies the same speed)
#define FROSTBOLT_SPEED 1100.f

// NOTE(zoubir): Frostbolt (X): a bolt at the foe aimed at, the filler; it
// chills the foe (slowed) and grows an Icicle
#define FROSTBOLT_RANGE 560.f
#define FROSTBOLT_DAMAGE 24.f
#define FROSTBOLT_CHILL_SECONDS 2.f
#define FROSTBOLT_COOLDOWN 1.f

// NOTE(zoubir): Blizzard (A): ice falls on the circle at the cursor for
// BLIZZARD_SECONDS, every foe inside struck each BLIZZARD_TICK and chilled
#define BLIZZARD_RADIUS 95.f
#define BLIZZARD_SECONDS 3.f
#define BLIZZARD_TICK 0.5f
#define BLIZZARD_TICK_DAMAGE 4.f
#define BLIZZARD_CHILL_SECONDS 1.f
#define BLIZZARD_COOLDOWN 14.f

// NOTE(zoubir): Glacial Spike (R): after its cast (PlayerSpell_FrostMageA)
// a spike at the foe aimed at, GLACIAL_SPIKE_DAMAGE plus
// GLACIAL_SPIKE_PER_ICICLE for each Icicle it spends (all of them); on
// five Icicles it freezes the foe (stunned) for GLACIAL_SPIKE_FREEZE
#define GLACIAL_SPIKE_RANGE 600.f
#define GLACIAL_SPIKE_DAMAGE 20.f
#define GLACIAL_SPIKE_PER_ICICLE 9.f
#define GLACIAL_SPIKE_FREEZE 1.5f
#define GLACIAL_SPIKE_COOLDOWN 7.f
// NOTE(zoubir): the spike flies faster than a bolt
#define GLACIAL_SPIKE_SPEED 1400.f

// NOTE(zoubir): Frost Nova (W): every foe within FROST_NOVA_RADIUS of the
// mage is frozen in place (rooted) for FROST_NOVA_ROOT and takes a little
#define FROST_NOVA_RADIUS 150.f
#define FROST_NOVA_DAMAGE 6.f
#define FROST_NOVA_ROOT 3.f
#define FROST_NOVA_COOLDOWN 16.f

// NOTE(zoubir): Ice Barrier (C, from the tree): a shield of ice that takes
// the next ICE_BARRIER_ABSORB damage for ICE_BARRIER_SECONDS; its second
// rank makes it ICE_BARRIER_ABSORB_2
#define ICE_BARRIER_ABSORB 45.f
#define ICE_BARRIER_ABSORB_2 70.f
#define ICE_BARRIER_SECONDS 10.f
#define ICE_BARRIER_COOLDOWN 20.f

// NOTE(zoubir): Frozen Orb (V, from the tree): an orb that rolls along the
// aim for FROZEN_ORB_SECONDS, striking and chilling every foe within
// FROZEN_ORB_RADIUS each FROZEN_ORB_TICK, and growing an Icicle on each
// tick that strikes anything
#define FROZEN_ORB_SPEED 120.f
#define FROZEN_ORB_SECONDS 3.f
#define FROZEN_ORB_RADIUS 70.f
#define FROZEN_ORB_TICK 0.4f
#define FROZEN_ORB_DAMAGE 3.f
#define FROZEN_ORB_COOLDOWN 18.f

enum frostmage_talent
{
    FrostMageTalent_Frostbite,
    FrostMageTalent_IceBarrier,
    FrostMageTalent_Permafrost,
    FrostMageTalent_SplittingIce,
    FrostMageTalent_FrozenOrb,
    FrostMageTalent_FingersOfFrost,
    FrostMageTalent_IceShards,
    FrostMageTalent_GlacialArmor,
    FrostMageTalent_DeepFreeze,
    FrostMageTalent_ColdSnap,
    FrostMageTalent_WintersGrace,
    FrostMageTalent_AbsoluteZero,
};

// NOTE(zoubir): per rank, or once taken
#define FROSTBITE_SHARE 0.06f
// NOTE(zoubir): Permafrost: Frostbolt's and Blizzard's chill lasts longer
#define PERMAFROST_SECONDS 1.5f
// NOTE(zoubir): Splitting Ice: Glacial Spike also strikes the nearest
// other foe within this for this share
#define SPLITTING_ICE_REACH 140.f
#define SPLITTING_ICE_SHARE 0.5f
// NOTE(zoubir): Fingers of Frost: every this many Frostbolts, one
// shatters as if its foe were frozen and grows two Icicles
#define FINGERS_OF_FROST_EVERY 4
// NOTE(zoubir): Deep Freeze, per rank: Frost Nova holds this much longer
#define DEEP_FREEZE_SECONDS 0.5f
// NOTE(zoubir): Absolute Zero, the capstone: a five-Icicle Glacial Spike
// freezes every foe within this of its foe
#define ABSOLUTE_ZERO_RADIUS 120.f

// NOTE(zoubir): the same shape as every class's branch (role_talents.cpp);
// slot 1 unlocks the C spell, slot 4 the V spell
global_variable talent_def FrostMageTalentDefs[ROLE_TALENTS] =
{
    {"Frostbite", "All your damage is higher", "+6% damage",
     TalentBranch_Role, 0, 0, 2, 0},
    {"Ice Barrier", "C: a shield of ice that takes the next 45 damage; rank 2, 70",
     "a new spell", TalentBranch_Role, 0, 1, 2, 0},
    {"Permafrost", "Frostbolt and Blizzard keep foes slowed longer", "+1.5 s chill",
     TalentBranch_Role, 1, 0, 1, 0},
    {"Splitting Ice", "Glacial Spike also strikes the nearest other foe for half",
     "a second spike", TalentBranch_Role, 1, 1, 1, 0},
    {"Frozen Orb", "V: an orb of ice rolls along your aim, striking and chilling foes near it",
     "a new spell", TalentBranch_Role, 2, 0, 1, 0},
    {"Fingers of Frost", "Every fourth Frostbolt shatters as if its foe were frozen, and grows two Icicles",
     "shatter every 4th", TalentBranch_Role, 3, 0, 1, 0},
    {"Ice Shards", "Your spells hit harder", "+4% damage",
     TalentBranch_Role, 2, 1, 4, 0},
    {"Glacial Armor", "You take less damage", "-4% damage taken",
     TalentBranch_Role, 3, 1, 4, 0},
    {"Deep Freeze", "Frost Nova holds foes longer", "+0.5 s freeze",
     TalentBranch_Role, 4, 0, 4, 0},
    {"Cold Snap", "Every spell comes back sooner", "-4% cooldowns",
     TalentBranch_Role, 4, 1, 4, 0},
    {"Winter's Grace", "More health", "+6% health",
     TalentBranch_Role, 5, 0, 4, 0},
    {"Absolute Zero", "A five-Icicle Glacial Spike freezes every foe near its target",
     "freezes all within 120", TalentBranch_Role, 5, 1, 1, 0},
};

// NOTE(zoubir): the stat each slot raises (role_stats.cpp), RoleStat_None
// for a talent with code of its own
global_variable u8 FrostMageTalentStats[ROLE_TALENTS] =
{
    RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None,
    RoleStat_Damage, RoleStat_Armor, RoleStat_None, RoleStat_Haste, RoleStat_Vitality, RoleStat_None,
};

// NOTE(zoubir): in RoleKeys order: A, R, C, V, W, X, right click
global_variable role_spell FrostMageSpells[ROLE_KEYS] =
{
    {"Blizzard", BLIZZARD_COOLDOWN, "Blizzard: ice falls on the circle at the cursor for 3 s, chilling",
     RoleAim_Ground, BLIZZARD_RADIUS, 0},
    {"Glacial Spike", GLACIAL_SPIKE_COOLDOWN,
     "Glacial Spike: 1.25 s cast, a spike at a foe that spends your Icicles; five freeze it",
     RoleAim_Foe, GLACIAL_SPIKE_RANGE, 0, "All Icicles"},
    {"Ice Barrier", ICE_BARRIER_COOLDOWN, "Ice Barrier: a shield of ice that takes the next 45 damage",
     RoleAim_None, 0.f, FrostMageTalent_IceBarrier + 1},
    {"Frozen Orb", FROZEN_ORB_COOLDOWN,
     "Frozen Orb: an orb rolls along your aim, striking and chilling what is near it",
     RoleAim_Line, FROZEN_ORB_SPEED * FROZEN_ORB_SECONDS, FrostMageTalent_FrozenOrb + 1},
    {"Frost Nova", FROST_NOVA_COOLDOWN, "Frost Nova: freeze every foe near you in place for 3 s",
     RoleAim_None, 0.f, 0},
    {"Frostbolt", FROSTBOLT_COOLDOWN,
     "Frostbolt: a bolt that chills a foe and grows an Icicle; frozen foes take 40% more",
     RoleAim_Foe, FROSTBOLT_RANGE, 0},
    {},
};
