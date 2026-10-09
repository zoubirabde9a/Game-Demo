/* Duelist's numbers, talents and spells, included by role_talents.cpp
   before the class tables (role_kits/duelist.cpp has what they do). */

// NOTE(zoubir): Thrust (right click): a quick stab at the nearest foe
// within reach (plus half its width) and inside the half-angle in front;
// one foe only
#define THRUST_DAMAGE 5.5f
#define THRUST_REACH 100.f
#define THRUST_HALF_ARC 0.45f
#define THRUST_SHOVE 25.f
#define THRUST_COOLDOWN 0.5f
// NOTE(zoubir): Lunge (A): to just in front of a foe this far off at
// most, LUNGE_GAP past its half width, and a strike
#define LUNGE_RANGE 300.f
#define LUNGE_GAP 30.f
#define LUNGE_DAMAGE 10.f
#define LUNGE_SHOVE 40.f
#define LUNGE_COOLDOWN 8.f
// NOTE(zoubir): Riposte (R): RIPOSTE_GUARD_SECONDS on guard. The first
// hit taken in it is parried and the nearest foe within RIPOSTE_REACH
// (the attacker first) countered: COUNTER_DAMAGE, stunned COUNTER_STUN,
// COUNTER_TEMPO more Tempo, and Riposte is ready RIPOSTE_READY_SECONDS
// later; a guard that parries nothing keeps the whole cooldown
#define RIPOSTE_GUARD_SECONDS 0.75f
#define RIPOSTE_REACH 160.f
#define RIPOSTE_COOLDOWN 9.f
#define RIPOSTE_READY_SECONDS 2.f
#define COUNTER_DAMAGE 22.f
#define COUNTER_STUN 1.f
#define COUNTER_SHOVE 60.f
#define COUNTER_TEMPO 2
// NOTE(zoubir): Heartseeker (W): on the foe in front when pressed, held
// through the wind-up within HEARTSEEKER_HOLD_REACH; a base and so much
// a Tempo stack (Tempo is not spent), HEARTSEEKER_LOW_SCALE times on a
// foe under HEARTSEEKER_LOW_HEALTH of its health
#define HEARTSEEKER_REACH 95.f
#define HEARTSEEKER_HOLD_REACH 150.f
#define HEARTSEEKER_DAMAGE 24.f
#define HEARTSEEKER_PER_TEMPO 12.f
#define HEARTSEEKER_LOW_HEALTH 0.3f
#define HEARTSEEKER_LOW_SCALE 1.5f
#define HEARTSEEKER_SHOVE 60.f
#define HEARTSEEKER_COOLDOWN 6.f
// NOTE(zoubir): Perfect Form (V, from the tree): for FORM_SECONDS Tempo
// cannot drop, every key gains it (repeats too), and a second Thrust
// follows each one FORM_ECHO_DELAY later
#define FORM_SECONDS 8.f
#define FORM_ECHO_DELAY 0.12f
#define FORM_COOLDOWN 50.f
// NOTE(zoubir): Tempo: each stack is TEMPO_SHARE more damage; a hit Riposte
// did not stop takes TEMPO_HIT_LOSS. Out of a fight it lasts
// DUELIST_IDLE_SECONDS after the last hit dealt, then goes one stack every
// DUELIST_FADE_SECONDS
#define TEMPO_SHARE 0.16f
#define TEMPO_HIT_LOSS 2
#define DUELIST_IDLE_SECONDS 5.f
#define DUELIST_FADE_SECONDS 1.f

// NOTE(zoubir): per rank, or once taken
#define FINESSE_SHARE 0.05f
// NOTE(zoubir): Footwork, per rank: Lunge's cooldown this much shorter;
// after a Lunge the Duelist is Hasted (half again as fast) this long,
// about what 2 s at 30% faster gives
#define FOOTWORK_COOLDOWN 2.f
#define FOOTWORK_HASTE_SECONDS 1.2f
#define PRECISION_LOW_HEALTH 0.45f
#define BAIT_GUARD_SECONDS 1.2f
#define BAIT_HEAL_SHARE 0.1f
// NOTE(zoubir): Feint (C, from Guard's pair against Riposte): a quick
// step, Hasted for FEINT_HASTE_SECONDS, and for FEINT_SECONDS the next
// blow on the Duelist misses whole and gives FEINT_TEMPO; with Bait the
// dodge heals as a counter does
#define FEINT_SECONDS 1.f
#define FEINT_HASTE_SECONDS 0.8f
#define FEINT_TEMPO 1
#define FEINT_COOLDOWN 7.f
// NOTE(zoubir): Crescendo: at full Tempo Heartseeker also cuts every other
// foe within its reach and this half-angle in front for CRESCENDO_SPLASH of
// its base and Tempo damage (no low-health bonus)
#define CRESCENDO_HALF_ARC 1.f
#define CRESCENDO_SPLASH 0.5f
#define FLURRY_SHARE 0.12f
// NOTE(zoubir): Masterstroke: at full Tempo Heartseeker strikes again
// MASTERSTROKE_DELAY later for this share of it
#define MASTERSTROKE_SHARE 0.6f
#define MASTERSTROKE_DELAY 0.15f

// NOTE(zoubir): the class's bursts, as SimBurst_DuelistFirst + n
// (client/dungeon/classes/duelist.cpp draws them):
// - Thrust: the stab from the chest along Angle; variant 1 when it struck
// - Lunge: arriving at Position along Angle; variant the way it came, in
//   DUELIST_LUNGE_STEP units
// - Guard: Riposte's guard going up; variant 1 with Bait (the longer guard)
// - Parry: the clang at the Duelist's chest, Angle toward the blow
// - Counter: the riposte landing on the foe at Position
// - Heartseeker: the strike on the foe at Position, along Angle; variant
//   the Tempo it struck with + 1 (0 for a strike that found no foe), plus
//   DUELIST_BURST_SWEEP for a Crescendo sweep and DUELIST_BURST_SECOND for
//   Masterstroke's second strike
// - Form: Perfect Form starting round the Duelist
// - Break: Tempo lost to a hit, over the head; variant the Tempo before
enum duelist_burst
{
    DuelistBurst_Thrust,
    DuelistBurst_Lunge,
    DuelistBurst_Guard,
    DuelistBurst_Parry,
    DuelistBurst_Counter,
    DuelistBurst_Heartseeker,
    DuelistBurst_Form,
    DuelistBurst_Break,
};

#define DUELIST_BURST_SWEEP 8
#define DUELIST_BURST_SECOND 16
#define DUELIST_LUNGE_STEP 8.f

// NOTE(zoubir): a burst carries its small number as whole steps of
// DUELIST_BURST_STEP added to its height: online a burst's position goes
// whole, no burst is drawn that high, and DuelistBurstPlace takes it off
#define DUELIST_BURST_STEP 4096.f

inline v3
DuelistBurstSpot(v3 Position, u32 Variant)
{
    v3 Result = Position;
    Result.Z += DUELIST_BURST_STEP * (float)Variant;
    return Result;
}

inline u32
DuelistBurstVariant(v3 Position)
{
    float Steps = floorf((Position.Z + 0.5f * DUELIST_BURST_STEP) / DUELIST_BURST_STEP);
    u32 Result = Steps > 0.f ? (u32)Steps : 0;
    return Result;
}

inline v3
DuelistBurstPlace(v3 Position)
{
    v3 Result = Position;
    Result.Z -= DUELIST_BURST_STEP * (float)DuelistBurstVariant(Position);
    return Result;
}

enum duelist_talent
{
    DuelistTalent_Finesse,
    DuelistTalent_Footwork,
    DuelistTalent_Precision,
    DuelistTalent_Bait,
    DuelistTalent_PerfectForm,
    DuelistTalent_Crescendo,
    DuelistTalent_KeenEdge,
    DuelistTalent_Parade,
    DuelistTalent_Flurry,
    DuelistTalent_QuickWrist,
    DuelistTalent_Stamina,
    DuelistTalent_Masterstroke,
    DuelistTalent_Lunge,
    DuelistTalent_Riposte,
    DuelistTalent_Feint,
};

// NOTE(zoubir): the same shape as every class's branch (role_talents.cpp);
// slot 4 unlocks the V spell. The Duelist has no C spell (X and C do
// nothing for it), so slot 1 is a passive
global_variable talent_def DuelistTalentDefs[CLASS_TALENTS] =
{
    {"Finesse", "All your damage is higher", "+5% damage",
     TalentBranch_Role, 0, 0, 2, 0},
    {"Footwork", "Lunge comes back 2 s sooner a rank, and you run faster just after it",
     "-2 s Lunge", TalentBranch_Role, 0, 1, 2, 0},
    {"Precision", "Heartseeker's bonus on a hurt foe starts at 45% health", "below 45% health",
     TalentBranch_Role, 1, 0, 1, 0},
    {"Bait", "Riposte's guard lasts 1.2 s; a counter or a Feint's dodge heals 10% of your health",
     "longer guard, heals", TalentBranch_Role, 1, 1, 1, 0},
    {"Perfect Form", "V: 8 s where Tempo cannot drop, every key builds it and Thrust strikes twice",
     "a new spell", TalentBranch_Role, 2, 0, 1, 0},
    {"Crescendo", "At 5 Tempo, Heartseeker cuts every foe in front and readies Lunge",
     "a sweeping finish", TalentBranch_Role, 3, 0, 1, 0},
    {"Keen Edge", "Your rapier cuts deeper", "+4% damage",
     TalentBranch_Role, 2, 1, 4, 0},
    {"Parade", "You take less damage", "-4% damage taken",
     TalentBranch_Role, 3, 1, 4, 0},
    {"Flurry", "Thrust hits harder", "+12% Thrust damage",
     TalentBranch_Role, 4, 0, 4, 0},
    {"Quick Wrist", "Every spell comes back sooner", "-4% cooldowns",
     TalentBranch_Role, 4, 1, 4, 0},
    {"Stamina", "You have more health", "+6% health",
     TalentBranch_Role, 5, 0, 4, 0},
    {"Masterstroke", "At 5 Tempo, Heartseeker strikes again for 60%; a kill readies it",
     "a second strike", TalentBranch_Role, 5, 1, 1, 0},
    {"Lunge", "A: dash to a foe and strike it",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Riposte", "R: a moment on guard; the first blow is parried and countered",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Feint", "C: a quick step; the next blow on you in 1 s misses and gives a Tempo",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
};

// NOTE(zoubir): the stat each slot raises (role_stats.cpp), RoleStat_None
// for a talent with code of its own
global_variable u8 DuelistTalentStats[CLASS_TALENTS] =
{
    RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None,
    RoleStat_Damage, RoleStat_Armor, RoleStat_None, RoleStat_Haste, RoleStat_Vitality, RoleStat_None,
};

// NOTE(zoubir): in RoleKeys order: A, R, C, V, W, X, right click, G, T
global_variable role_spell DuelistSpells[ROLE_KEYS] =
{
    {"Lunge", LUNGE_COOLDOWN, "Lunge: dash to the foe under the cursor and strike it",
     RoleAim_Foe, LUNGE_RANGE, 0},
    {"Riposte", RIPOSTE_COOLDOWN,
     "Riposte: 0.75 s on guard; a hit in it is parried and countered, and Riposte is back in 2 s",
     RoleAim_None, RIPOSTE_REACH, DuelistTalent_Riposte + 1},
    {"Feint", FEINT_COOLDOWN,
     "Feint: a quick step; the next blow on you in 1 s misses and gives a Tempo",
     RoleAim_None, 0.f, DuelistTalent_Feint + 1},
    {"Perfect Form", FORM_COOLDOWN,
     "Perfect Form: 8 s where Tempo holds, every key builds it and Thrust strikes twice",
     RoleAim_None, 0.f, DuelistTalent_PerfectForm + 1},
    {"Heartseeker", HEARTSEEKER_COOLDOWN,
     "Heartseeker: a piercing strike at the foe in front, harder with Tempo and on a hurt foe",
     RoleAim_None, HEARTSEEKER_REACH, 0},
    {},
    {"Thrust", THRUST_COOLDOWN, "Thrust: a quick stab at the first foe in front",
     RoleAim_None, THRUST_REACH, 0},
};
