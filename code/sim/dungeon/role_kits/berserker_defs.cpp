/* Berserker's numbers, talents and spells, included by role_talents.cpp
   before the class tables (role_kits/berserker.cpp has what they do). A class
   that has no spell on its first key yet (RoleSpells) is left out of the
   role picker and the bots. */

// NOTE(zoubir): Rage, 0..BERSERKER_RAGE_MAX in ClassMeter: a hit the
// Berserker lands gives RAGE_PER_DAMAGE a point of damage dealt, a hit it
// takes RAGE_PER_DAMAGE_TAKEN a point of health lost. After
// BERSERKER_CALM_SECONDS with neither it drains RAGE_DECAY a second
#define BERSERKER_RAGE_MAX 100
#define RAGE_PER_DAMAGE 0.3f
#define RAGE_PER_DAMAGE_TAKEN 0.6f
#define BERSERKER_CALM_SECONDS 4.f
#define RAGE_DECAY 6.f
// NOTE(zoubir): how long the HUD flashes after a spell lacked Rage
#define BERSERKER_NO_RAGE_SECONDS 0.45f

// NOTE(zoubir): Cleave (right click): every foe in a wide arc in front,
// as far as CLEAVE_REACH, its body's width counting
#define CLEAVE_REACH 84.f
#define CLEAVE_HALF_ANGLE 1.5f
#define CLEAVE_DAMAGE 8.f
#define CLEAVE_SHOVE 70.f
#define CLEAVE_COOLDOWN 0.6f
// NOTE(zoubir): the foe most squarely in front takes the whole Cleave,
// every other one it catches this share
#define CLEAVE_SPLASH 0.6f
// NOTE(zoubir): Leap (A): a high jump to the cursor, LEAP_SECONDS in the
// air; landing strikes and stuns every foe within LEAP_RADIUS
#define LEAP_SECONDS 0.55f
#define LEAP_RADIUS 95.f
#define LEAP_DAMAGE 22.f
#define LEAP_STUN 1.f
#define LEAP_SHOVE 160.f
#define LEAP_COOLDOWN 12.f
// NOTE(zoubir): a leap that never lands (a pit, a wall) gives up after this
#define LEAP_MOST_SECONDS 1.6f
// NOTE(zoubir): Whirlwind (R): WHIRLWIND_HITS hits on every foe within
// WHIRLWIND_RADIUS over the cast (PlayerSpell_BerserkerA), the last as it
// ends; it costs WHIRLWIND_RAGE
#define WHIRLWIND_RADIUS 100.f
#define WHIRLWIND_DAMAGE 7.f
#define WHIRLWIND_HITS 5
#define WHIRLWIND_RAGE 30
#define WHIRLWIND_COOLDOWN 9.f
// NOTE(zoubir): Execute (W): a chop on the foe in front after the raised
// axe's wind-up (PlayerSpell_BerserkerB); it needs EXECUTE_MIN_RAGE, spends
// all of it for EXECUTE_DAMAGE plus EXECUTE_PER_RAGE a point, and deals
// EXECUTE_LOW_SCALE times that on a foe under EXECUTE_LOW_SHARE health
#define EXECUTE_REACH 95.f
#define EXECUTE_MIN_RAGE 20
#define EXECUTE_DAMAGE 16.f
#define EXECUTE_PER_RAGE 0.6f
#define EXECUTE_LOW_SHARE 0.25f
#define EXECUTE_LOW_SCALE 1.75f
#define EXECUTE_SHOVE 120.f
#define EXECUTE_COOLDOWN 6.f
// NOTE(zoubir): Berserk (V, from the tree): this long, this much more
// damage dealt and less taken, and no Rage decay
#define BERSERK_SECONDS 8.f
#define BERSERK_DEALT_SHARE 0.25f
#define BERSERK_TAKEN_SHARE 0.15f
#define BERSERK_COOLDOWN 45.f

// NOTE(zoubir): the talents, per rank or once taken
#define BRUTALITY_SHARE 0.06f
// NOTE(zoubir): Bloodthirst: Execute heals the Berserker this share of
// what it dealt, at rank 1 and rank 2
#define BLOODTHIRST_HEAL_SHARE 0.3f
#define BLOODTHIRST_RANK2_HEAL 0.45f
#define UNBRIDLED_WRATH_SHARE 0.35f
// NOTE(zoubir): Sweeping Strikes: Cleave and Whirlwind hit this much
// harder for each other foe they catch, counting SWEEPING_MOST, and Cleave
// reaches this much farther
#define SWEEPING_PER_FOE 0.1f
#define SWEEPING_MOST 3
#define SWEEPING_REACH 1.2f
// NOTE(zoubir): Massacre: Execute's big hit from this share of health, and
// this much Rage back when it kills
#define MASSACRE_LOW_SHARE 0.35f
#define MASSACRE_REFUND 30

enum berserker_talent
{
    BerserkerTalent_Brutality,
    BerserkerTalent_Bloodthirst,
    BerserkerTalent_UnbridledWrath,
    BerserkerTalent_SweepingStrikes,
    BerserkerTalent_Berserk,
    BerserkerTalent_Massacre,
};

// NOTE(zoubir): the same shape as every class's branch (role_talents.cpp):
// two talents of two ranks, two of one, one, one; slot 4 unlocks the V
// spell. The Berserker has no C spell: its five keys are A, R, V, W and
// the right click
global_variable talent_def BerserkerTalentDefs[ROLE_TALENTS] =
{
    {"Brutality", "All your damage is higher", "+6% damage", TalentBranch_Role, 0, 0, 2, 0},
    {"Bloodthirst", "Execute heals you for 30% of the damage it deals",
     "rank 2: heals 45%", TalentBranch_Role, 0, 1, 2, 0},
    {"Unbridled Wrath", "Every hit you land or take gives more Rage", "+35% Rage",
     TalentBranch_Role, 1, 0, 1, 0},
    {"Sweeping Strikes", "Cleave reaches farther; Cleave and Whirlwind hit harder for every foe they catch",
     "+10% a foe, Cleave +20% reach", TalentBranch_Role, 1, 1, 1, 0},
    {"Berserk", "V: for 8 s deal 25% more, take 15% less, and Rage holds",
     "a new spell", TalentBranch_Role, 2, 0, 1, 0},
    {"Massacre", "Execute's big hit starts at 35% health; a kill gives 30 Rage back",
     "Execute under 35%, Rage on a kill", TalentBranch_Role, 3, 0, 1, 0},
};

// NOTE(zoubir): in RoleKeys order: A, R, C, V, W, X, right click
global_variable role_spell BerserkerSpells[ROLE_KEYS] =
{
    {"Leap", LEAP_COOLDOWN, "Leap: jump to the cursor and slam down, stunning what is there",
     RoleAim_Ground, LEAP_RADIUS, 0},
    {"Whirlwind", WHIRLWIND_COOLDOWN,
     "Whirlwind: 30 Rage, spin for 1.5 s hitting everything around you five times",
     RoleAim_None, WHIRLWIND_RADIUS, 0},
    {},
    {"Berserk", BERSERK_COOLDOWN, "Berserk: 8 s of 25% more damage, 15% less taken, Rage holds",
     RoleAim_None, 0.f, BerserkerTalent_Berserk + 1},
    {"Execute", EXECUTE_COOLDOWN,
     "Execute: spend all Rage on one chop, twice as hard under 25% health",
     RoleAim_None, EXECUTE_REACH, 0},
    {},
    {"Cleave", CLEAVE_COOLDOWN, "Cleave: a wide swing of the axe through everything in front",
     RoleAim_None, CLEAVE_REACH, 0},
};
