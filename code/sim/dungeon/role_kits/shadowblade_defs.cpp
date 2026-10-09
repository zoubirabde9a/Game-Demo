/* Shadowblade's numbers, talents and spells, included by role_talents.cpp
   before the class tables (role_kits/shadowblade.cpp has what they do). A class
   that has no spell on its first key yet (RoleSpells) is left out of the
   role picker and the bots. */

// NOTE(zoubir): Twin Strike (right click): two cuts at what is in front,
// the second CUT_GAP after the first, each this hard; a foe within reach
// (plus half its width) and inside the arc's half-angle is cut
#define TWIN_STRIKE_DAMAGE 2.65f
#define TWIN_STRIKE_REACH 70.f
#define TWIN_STRIKE_HALF_ARC 1.f
// NOTE(zoubir): the share of a cut the other foes in the arc take
#define TWIN_STRIKE_SPLASH 0.5f
#define TWIN_STRIKE_CUT_GAP 0.1f
#define TWIN_STRIKE_SHOVE 30.f
#define TWIN_STRIKE_COOLDOWN 0.5f
// NOTE(zoubir): the poison Twin Strike leaves on what it cuts, for
// BLADE_POISON_SECONDS, ticking every BLADE_POISON_TICK; a cut renews it
#define BLADE_POISON_PER_SECOND 4.f
#define BLADE_POISON_SECONDS 6.f
#define BLADE_POISON_TICK 0.5f
// NOTE(zoubir): Shadowstep (A): behind a foe this far off at most,
// landing this far past its back; the next strike within CRIT_SECONDS
// deals CRIT_SCALE times
#define SHADOWSTEP_RANGE 400.f
#define SHADOWSTEP_GAP 34.f
#define SHADOWSTEP_COOLDOWN 8.f
#define SHADOWBLADE_CRIT_SECONDS 4.f
#define SHADOWBLADE_CRIT_SCALE 2.f
// NOTE(zoubir): Fan of Knives (R): every foe this close is cut, one combo
// point each
#define FAN_OF_KNIVES_RADIUS 125.f
#define FAN_OF_KNIVES_DAMAGE 2.5f
#define FAN_OF_KNIVES_SHOVE 90.f
#define FAN_OF_KNIVES_COOLDOWN 7.f
// NOTE(zoubir): Eviscerate (W): the finisher on the foe in front, its
// damage a base and so much a combo point it spends. The foe is picked
// when the key goes down and held through the wind-up: it still lands
// on it within EVISCERATE_HOLD_REACH, however it moved
#define EVISCERATE_REACH 90.f
#define EVISCERATE_HOLD_REACH 150.f
#define EVISCERATE_DAMAGE 8.f
#define EVISCERATE_PER_POINT 10.f
#define EVISCERATE_SHOVE 140.f
#define EVISCERATE_COOLDOWN 6.f
// NOTE(zoubir): Deadly Throw (X): the ranged finisher, a dagger at the
// foe under the cursor this far off at most, a base and so much a combo
// point it spends; the foe is poisoned and slowed so long a point
#define DEADLY_THROW_RANGE 380.f
#define DEADLY_THROW_DAMAGE 5.f
#define DEADLY_THROW_PER_POINT 7.f
#define DEADLY_THROW_SHOVE 60.f
#define DEADLY_THROW_SLOW_PER_POINT 0.6f
#define DEADLY_THROW_COOLDOWN 8.f
// NOTE(zoubir): Shadow Dance (V, from the tree): for DANCE_SECONDS a
// shadow strikes again beside every Twin Strike and Eviscerate
// for DANCE_ECHO of it, and Shadowstep comes back in DANCE_STEP_COOLDOWN
#define DANCE_SECONDS 6.f
#define DANCE_ECHO 0.5f
#define DANCE_STEP_COOLDOWN 1.f
#define DANCE_COOLDOWN 45.f
// NOTE(zoubir): combo points last SHADOWBLADE_IDLE_SECONDS after the last
// strike out of a fight, then fade one every SHADOWBLADE_FADE_SECONDS
#define SHADOWBLADE_IDLE_SECONDS 6.f
#define SHADOWBLADE_FADE_SECONDS 1.f

// NOTE(zoubir): per rank, or once taken
#define LETHALITY_SHARE 0.05f
#define VENOM_SHARE 0.5f
#define VENOM_SECONDS 3.f
// NOTE(zoubir): Envenom, per rank: Fan of Knives poisons what it cuts
// from the first, and every poison bites this much harder
#define ENVENOM_SHARE 0.2f
#define OPPORTUNIST_SHARE 0.2f
// NOTE(zoubir): how far round a foe's back counts as behind it: the
// cosine between its facing and the way to the Shadowblade
#define OPPORTUNIST_BEHIND -0.2f
#define RELENTLESS_POINTS 3
// NOTE(zoubir): Knife Storm, per rank: Fan of Knives reaches this share
// farther and cuts this share harder
#define KNIFE_STORM_RADIUS_SHARE 0.1f
#define KNIFE_STORM_DAMAGE_SHARE 0.25f
// NOTE(zoubir): Kidney Shot: an Eviscerate that spends all
// SHADOWBLADE_MOST_POINTS stuns the foe it leaves alive this long
#define KIDNEY_SHOT_SECONDS 2.5f

// NOTE(zoubir): the class's bursts, as SimBurst_ShadowbladeFirst + n
// (client/dungeon/classes/shadowblade.cpp draws them)
enum shadowblade_burst
{
    ShadowbladeBurst_TwinStrike, // two crossing cuts from the player along Angle
    ShadowbladeBurst_Step,       // arriving at Position, come along Angle
    ShadowbladeBurst_Dance,      // Shadow Dance starting round the player
    ShadowbladeBurst_Fan,        // knives bursting out round the player
    ShadowbladeBurst_Eviscerate, // a flurry on the foe at Position
    ShadowbladeBurst_Empty,      // a finisher pressed with no combo points
    ShadowbladeBurst_Throw,      // a dagger flying from the player to the foe at Position
};

// NOTE(zoubir): a burst carries a small number too (the points an
// Eviscerate spent, 0 for one that found no foe), as whole steps of
// SHADOWBLADE_BURST_STEP added to its height: online a burst's position
// goes whole, no burst is drawn that high, and ShadowbladeBurstPlace
// takes it off
#define SHADOWBLADE_BURST_STEP 4096.f

inline v3
ShadowbladeBurstSpot(v3 Position, u32 Variant)
{
    v3 Result = Position;
    Result.Z += SHADOWBLADE_BURST_STEP * (float)Variant;
    return Result;
}

inline u32
ShadowbladeBurstVariant(v3 Position)
{
    float Steps = floorf((Position.Z + 0.5f * SHADOWBLADE_BURST_STEP) / SHADOWBLADE_BURST_STEP);
    u32 Result = Steps > 0.f ? (u32)Steps : 0;
    return Result;
}

inline v3
ShadowbladeBurstPlace(v3 Position)
{
    v3 Result = Position;
    Result.Z -= SHADOWBLADE_BURST_STEP * (float)ShadowbladeBurstVariant(Position);
    return Result;
}

enum shadowblade_talent
{
    ShadowbladeTalent_Lethality,
    ShadowbladeTalent_Envenom,
    ShadowbladeTalent_Venom,
    ShadowbladeTalent_Opportunist,
    ShadowbladeTalent_ShadowDance,
    ShadowbladeTalent_Relentless,
    ShadowbladeTalent_Cutthroat,
    ShadowbladeTalent_Evasion,
    ShadowbladeTalent_KnifeStorm,
    ShadowbladeTalent_QuickHands,
    ShadowbladeTalent_Siphon,
    ShadowbladeTalent_KidneyShot,
    ShadowbladeTalent_FanOfKnives,
    ShadowbladeTalent_DeadlyThrow,
    ShadowbladeTalent_Shadowstep,
};

// NOTE(zoubir): the same shape as every class's branch (role_talents.cpp);
// slot 4 unlocks the V spell. The Shadowblade has no C spell: its six keys are A, R, V, W, X
// and the right click
global_variable talent_def ShadowbladeTalentDefs[CLASS_TALENTS] =
{
    {"Lethality", "All your damage is higher", "+5% damage",
     TalentBranch_Role, 0, 0, 2, 0},
    {"Envenom", "Fan of Knives poisons every foe it cuts; all your poison bites harder",
     "+20% poison", TalentBranch_Role, 0, 1, 2, 0},
    {"Venom", "Twin Strike's poison bites harder and lasts longer", "+50% poison, +3 s",
     TalentBranch_Role, 1, 0, 1, 0},
    {"Opportunist", "Hit harder from behind a foe", "+20% damage from behind",
     TalentBranch_Role, 1, 1, 1, 0},
    {"Shadow Dance", "V: for 6 s a shadow strikes beside you and Shadowstep is back in 1 s",
     "a new spell", TalentBranch_Role, 2, 0, 1, 0},
    {"Relentless", "An Eviscerate that kills gives back 3 combo points and Shadowstep",
     "kills refund", TalentBranch_Role, 3, 0, 1, 0},
    {"Cutthroat", "Your blades cut deeper", "+4% damage",
     TalentBranch_Role, 2, 1, 4, 0},
    {"Evasion", "You take less damage", "-4% damage taken",
     TalentBranch_Role, 3, 1, 4, 0},
    {"Knife Storm", "Fan of Knives reaches farther and cuts harder",
     "+10% radius, +25% damage", TalentBranch_Role, 4, 0, 4, 0},
    {"Quick Hands", "Every spell comes back sooner", "-4% cooldowns",
     TalentBranch_Role, 4, 1, 4, 0},
    {"Siphon", "Your hits heal you", "2% of damage back as health",
     TalentBranch_Role, 5, 0, 4, 0},
    {"Kidney Shot", "An Eviscerate with all 5 combo points stuns its foe for 2.5 s",
     "a stunning finisher", TalentBranch_Role, 5, 1, 1, 0},
    {"Fan of Knives", "R: a ring of knives, a combo point for each foe cut",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Deadly Throw", "X: a poisoned dagger at a foe far off, spending every combo point",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Shadowstep", "A: appear behind a foe; your next strike in 4 s does double",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
};

// NOTE(zoubir): the stat each slot raises (role_stats.cpp), RoleStat_None
// for a talent with code of its own
global_variable u8 ShadowbladeTalentStats[CLASS_TALENTS] =
{
    RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None,
    RoleStat_Damage, RoleStat_Armor, RoleStat_None, RoleStat_Haste, RoleStat_Lifesteal, RoleStat_None,
};

// NOTE(zoubir): in RoleKeys order: A, R, C, V, W, X, right click
global_variable role_spell ShadowbladeSpells[ROLE_KEYS] =
{
    {"Shadowstep", SHADOWSTEP_COOLDOWN,
     "Shadowstep: appear behind a foe, drop all threat, take 90% less for 1 s; next strike crits",
     RoleAim_Foe, SHADOWSTEP_RANGE, ShadowbladeTalent_Shadowstep + 1},
    {"Fan of Knives", FAN_OF_KNIVES_COOLDOWN,
     "Fan of Knives: knives burst round you, a combo point for each foe cut",
     RoleAim_None, FAN_OF_KNIVES_RADIUS, ShadowbladeTalent_FanOfKnives + 1},
    {},
    {"Shadow Dance", DANCE_COOLDOWN,
     "Shadow Dance: 6 s of shadow strikes beside yours, Shadowstep back in 1 s",
     RoleAim_None, 0.f, ShadowbladeTalent_ShadowDance + 1},
    {"Eviscerate", EVISCERATE_COOLDOWN,
     "Eviscerate: spend every combo point on one big strike at the foe in front",
     RoleAim_None, EVISCERATE_REACH, 0, "All combo points, 1 at least"},
    {"Deadly Throw", DEADLY_THROW_COOLDOWN,
     "Deadly Throw: spend every combo point on a poisoned dagger thrown at a foe; it slows",
     RoleAim_Foe, DEADLY_THROW_RANGE, ShadowbladeTalent_DeadlyThrow + 1, "All combo points, 1 at least"},
    {"Twin Strike", TWIN_STRIKE_COOLDOWN,
     "Twin Strike: two quick cuts in front that poison, a combo point",
     RoleAim_None, TWIN_STRIKE_REACH, 0},
};
