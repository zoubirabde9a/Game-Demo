/* Shadowblade's numbers, talents and spells, included by role_talents.cpp
   before the class tables (role_kits/shadowblade.cpp has what they do). A class
   that has no spell on its first key yet (RoleSpells) is left out of the
   role picker and the bots. */

// NOTE(zoubir): Twin Strike (right click): two cuts at what is in front,
// the second CUT_GAP after the first, each this hard; a foe within reach
// (plus half its width) and inside the arc's half-angle is cut
#define TWIN_STRIKE_DAMAGE 2.2f
#define TWIN_STRIKE_REACH 70.f
#define TWIN_STRIKE_HALF_ARC 1.f
#define TWIN_STRIKE_CUT_GAP 0.1f
#define TWIN_STRIKE_SHOVE 30.f
#define TWIN_STRIKE_COOLDOWN 0.5f
// NOTE(zoubir): Poisoned Shiv (X): a dagger thrown at a foe, then poison
// for SHIV_POISON_SECONDS, ticking every SHIV_POISON_TICK
#define SHIV_RANGE 430.f
#define SHIV_DAMAGE 6.f
#define SHIV_POISON_PER_SECOND 3.f
#define SHIV_POISON_SECONDS 6.f
#define SHIV_POISON_TICK 0.5f
#define SHIV_COOLDOWN 5.f
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
#define FAN_OF_KNIVES_DAMAGE 6.f
#define FAN_OF_KNIVES_SHOVE 90.f
#define FAN_OF_KNIVES_COOLDOWN 7.f
// NOTE(zoubir): Eviscerate (W): the finisher on the foe in front, its
// damage a base and so much a combo point it spends
#define EVISCERATE_REACH 90.f
#define EVISCERATE_DAMAGE 4.f
#define EVISCERATE_PER_POINT 4.f
#define EVISCERATE_SHOVE 140.f
#define EVISCERATE_COOLDOWN 0.8f
// NOTE(zoubir): Smoke Bomb (C, from the tree): a cloud at the feet;
// monsters inside drop the threat they have on the party in it (a tank
// keeps its own), and the party in it takes SMOKE_SHARE less
#define SMOKE_RADIUS 120.f
#define SMOKE_SECONDS 6.f
#define SMOKE_SHARE 0.2f
#define SMOKE_THICK_SHARE 0.35f
#define SMOKE_COOLDOWN 20.f
// NOTE(zoubir): Shadow Dance (V, from the tree): for DANCE_SECONDS a
// shadow strikes again beside every Twin Strike, Shiv and Eviscerate
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
#define OPPORTUNIST_SHARE 0.2f
// NOTE(zoubir): how far round a foe's back counts as behind it: the
// cosine between its facing and the way to the Shadowblade
#define OPPORTUNIST_BEHIND -0.2f
#define RELENTLESS_POINTS 3

// NOTE(zoubir): the class's bursts, as SimBurst_ShadowbladeFirst + n
// (client/dungeon/classes/shadowblade.cpp draws them)
enum shadowblade_burst
{
    ShadowbladeBurst_TwinStrike, // two crossing cuts from the player along Angle
    ShadowbladeBurst_Shiv,       // a dagger landing on the foe at Position
    ShadowbladeBurst_Step,       // arriving at Position, come along Angle
    ShadowbladeBurst_Dance,      // Shadow Dance starting round the player
    ShadowbladeBurst_Fan,        // knives bursting out round the player
    ShadowbladeBurst_Eviscerate, // a flurry on the foe at Position
    ShadowbladeBurst_Smoke,      // the cloud, for its whole life
    ShadowbladeBurst_Empty,      // a finisher pressed with no combo points
};

enum shadowblade_talent
{
    ShadowbladeTalent_Lethality,
    ShadowbladeTalent_SmokeBomb,
    ShadowbladeTalent_Venom,
    ShadowbladeTalent_Opportunist,
    ShadowbladeTalent_ShadowDance,
    ShadowbladeTalent_Relentless,
};

// NOTE(zoubir): the same shape as every class's branch (role_talents.cpp):
// two talents of two ranks, two of one, one, one; slot 1 unlocks the C
// spell, slot 4 the V spell
global_variable talent_def ShadowbladeTalentDefs[ROLE_TALENTS] =
{
    {"Lethality", "All your damage is higher", "+5% damage",
     TalentBranch_Role, 0, 0, 2, 0},
    {"Smoke Bomb", "C: a cloud at your feet; foes in it forget the party in it, who take less",
     "rank 2: 35% less taken, foes inside slowed", TalentBranch_Role, 0, 1, 2, 0},
    {"Venom", "Poisoned Shiv's poison bites harder and lasts longer", "+50% poison, +3 s",
     TalentBranch_Role, 1, 0, 1, 0},
    {"Opportunist", "Hit harder from behind a foe", "+20% damage from behind",
     TalentBranch_Role, 1, 1, 1, 0},
    {"Shadow Dance", "V: for 6 s a shadow strikes beside you and Shadowstep is back in 1 s",
     "a new spell", TalentBranch_Role, 2, 0, 1, 0},
    {"Relentless", "An Eviscerate that kills gives back 3 combo points and Shadowstep",
     "kills refund", TalentBranch_Role, 3, 0, 1, 0},
};

// NOTE(zoubir): in RoleKeys order: A, R, C, V, W, X, right click
global_variable role_spell ShadowbladeSpells[ROLE_KEYS] =
{
    {"Shadowstep", SHADOWSTEP_COOLDOWN,
     "Shadowstep: appear behind the foe under the cursor; your next strike is critical",
     RoleAim_None, SHADOWSTEP_RANGE, 0},
    {"Fan of Knives", FAN_OF_KNIVES_COOLDOWN,
     "Fan of Knives: knives burst round you, a combo point for each foe cut",
     RoleAim_None, FAN_OF_KNIVES_RADIUS, 0},
    {"Smoke Bomb", SMOKE_COOLDOWN,
     "Smoke Bomb: a cloud at your feet; foes in it forget the party in it",
     RoleAim_None, SMOKE_RADIUS, ShadowbladeTalent_SmokeBomb + 1},
    {"Shadow Dance", DANCE_COOLDOWN,
     "Shadow Dance: 6 s of shadow strikes beside yours, Shadowstep back in 1 s",
     RoleAim_None, 0.f, ShadowbladeTalent_ShadowDance + 1},
    {"Eviscerate", EVISCERATE_COOLDOWN,
     "Eviscerate: spend every combo point on a flurry at the foe in front",
     RoleAim_None, EVISCERATE_REACH, 0},
    {"Poisoned Shiv", SHIV_COOLDOWN,
     "Poisoned Shiv: throw a poisoned dagger at a foe, a combo point",
     RoleAim_None, SHIV_RANGE, 0},
    {"Twin Strike", TWIN_STRIKE_COOLDOWN,
     "Twin Strike: two quick cuts in front, a combo point",
     RoleAim_None, TWIN_STRIKE_REACH, 0},
};
