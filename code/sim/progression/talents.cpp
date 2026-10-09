/* Talents: what a player spends its level-up points on, as one table of
   three branches (Fire, Motion, Guard) of four tiers. A tier opens once
   TALENT_POINTS_PER_TIER points per tier above it are spent in that
   branch: tier 1 at 2, tier 2 at 4, tier 3 at 6.

   A talent is either an ability or a passive. An ability talent has the
   ability's button and up to three levels. The abilities the rules start
   with (GameRules.Buttons: fireball, launch, kunai, blink, shield) are level 1
   for free, so two more levels can be bought; the others (sword,
   shockwave, push, slam, and Frost Nova and Gravity Well, which only
   exist here) are locked until a point unlocks them at level 1. Dash and
   the time rewinds are out of the game: no talent, no key, and the duel
   rules leave their buttons out. Each level after the first takes
   TALENT_COOLDOWN_PER_LEVEL off the cooldown and adds the talent's
   per-level perks: shield time and kunai damage (PowerPerLevel), longer
   stuns and slows and harder shoves on area spells (AbilityLevelHit),
   faster, farther fireballs.

   A point spent is not for good: the talent field's TALENT_LEARN_RESET
   gives every point back (ResetTalents), so a player can try another
   build.

   A passive changes one thing, by rank:
   Swift Flames  fireballs fly faster and farther
   Twin Flame    each cast throws two fireballs
   Pyre          killing a player makes the fireball ready at once
   Fleet Foot    a faster run
   Momentum      a player kill readies slam and halves blink's wait, as
                 a monster kill does
   Ward          a charge that takes one hit whole, back after a while
   Second Wind   back from death sooner, with a longer shield

   Points come in through player_input.Learn (LearnTalent), so offline,
   online and in replays the same tick spends them. The client draws the
   tree (ui/talent_panel/) and the server sends each player its ranks.

   In a dungeon run the class's tree takes the place of all this
   (sim/dungeon/class_tree.cpp): two branches of twelve slots, the first
   Talent_RoleFirst on, in TalentBranch_Role, the second Talent_RunFirst
   on, in TalentBranch_Run, six tiers deep (ROLE_TALENT_TIERS). Their
   shape is the same for every class and their meaning is the class's;
   some slots roll again each run. Their ranks ride with the others;
   picking another class gives their points back, and they take no point
   outside a run.

   This file is the table, the point rules and learning; what each level
   and rank changes in play is talents/effects.cpp. */

#define TALENT_POINTS_PER_TIER 2
#define TALENT_TIERS 4
// NOTE(zoubir): the role branch goes deeper: 29 points, one a level up
// to the dungeon's top level
#define ROLE_TALENT_TIERS 6
// NOTE(zoubir): share of an ability's cooldown each level after the first
// takes off: 6 s at level 1 is 5.1 at 2 and 4.2 at 3
#define TALENT_COOLDOWN_PER_LEVEL 0.15f
// NOTE(zoubir): player_input.Learn's value that resets every talent; the
// talent field is 6 bits (NET_LEARN_MASK), and talents use 1..Talent_Count
#define TALENT_LEARN_RESET 63

enum talent_branch
{
    TalentBranch_Fire,
    TalentBranch_Motion,
    TalentBranch_Guard,
    // NOTE(zoubir): the dungeon role's own branch, shown only in a run
    TalentBranch_Role,
    // NOTE(zoubir): the class's second tree, fixed and wild talents
    // (sim/dungeon/run_tree/), shown beside the role's in a run
    TalentBranch_Run,
    TalentBranch_Count
};
// NOTE(zoubir): the branches every map shows
#define TALENT_GAME_BRANCHES 3
// NOTE(zoubir): slots in the role branch, Talent_RoleFirst on
#define ROLE_TALENTS 12
// NOTE(zoubir): slots in the second tree, Talent_RunFirst on, are
// RUN_TALENTS (sim/dungeon/run_tree/run_tree_fields.inc)

// NOTE(zoubir): by branch, then tier; the order the panel lists them in
// and the order snapshots carry their ranks in
enum talent_id
{
    Talent_Fireball,
    Talent_SwiftFlames,
    Talent_Launch,
    Talent_Shockwave,
    Talent_FrostNova,
    Talent_TwinFlame,
    Talent_Pyre,

    Talent_Kunai,
    Talent_FleetFoot,
    Talent_Blink,
    Talent_Sword,
    Talent_Push,
    Talent_Slam,
    Talent_Momentum,

    Talent_Shield,
    Talent_Ward,
    Talent_GravityWell,
    Talent_SecondWind,

    Talent_RoleFirst,
    Talent_RoleLast = Talent_RoleFirst + ROLE_TALENTS - 1,
    Talent_RunFirst,
    Talent_RunLast = Talent_RunFirst + RUN_TALENTS - 1,

    Talent_Count
};
static_assert(Talent_Count <= TALENT_SLOTS, "one rank each in player_slot");

struct talent_def
{
    char *Name;
    // NOTE(zoubir): what it does, in a line, and what one more rank adds
    char *Summary;
    char *PerRank;
    u32 Branch;
    u32 Tier;
    // NOTE(zoubir): 0 or 1, its place in the tier on the panel
    u32 Column;
    // NOTE(zoubir): an ability's highest level, or a passive's most ranks
    u32 MaxLevel;
    // NOTE(zoubir): the ability's player_button; 0 for a passive
    u32 Button;
    // NOTE(zoubir): what each level after the first adds: a share of the
    // ability row's Power (dash speed, shield seconds), seconds of stun
    // and of status on an area hit, a share of its shove, and a share of
    // the fireball's speed and range
    float PowerPerLevel;
    float StunPerLevel;
    float StatusPerLevel;
    float ShovePerLevel;
    float SpeedPerLevel;
};

global_variable talent_def TalentDefs[Talent_Count] =
{
    // NOTE(zoubir): name, summary, what a rank adds; branch, tier, column,
    // most levels, button; then per level after the first: power, stun
    // seconds, status seconds, shove share, fireball speed share
    {"Fireball", "A piercing bolt along the aim", "-15% cooldown, +8% speed and range",
     TalentBranch_Fire, 0, 0, 3, PlayerButton_Cast, 0.f, 0.f, 0.f, 0.f, 0.08f},
    {"Swift Flames", "Fireballs fly faster and farther", "+15% fireball speed and range",
     TalentBranch_Fire, 0, 1, 2, 0},
    {"Launch", "Throws everything at the aim into the air", "-15% cooldown, +0.25 s stun",
     TalentBranch_Fire, 1, 0, 3, PlayerButton_Launch, 0.f, 0.25f},
    {"Shockwave", "Blasts everything around you away", "-15% cooldown, +15% shove, +0.15 s stun",
     TalentBranch_Fire, 1, 1, 3, PlayerButton_Shockwave, 0.f, 0.15f, 0.f, 0.15f},
    {"Frost Nova", "Freezes everyone near you, then slows them", "-15% cooldown, +0.2 s freeze, +0.75 s slow",
     TalentBranch_Fire, 2, 0, 3, PlayerButton_FrostNova, 0.f, 0.2f, 0.75f},
    {"Twin Flame", "Each cast throws two fireballs", "a second fireball",
     TalentBranch_Fire, 2, 1, 1, 0},
    {"Pyre", "Killing a player makes the fireball ready at once", "fireball reset on a kill",
     TalentBranch_Fire, 3, 0, 1, 0},

    {"Kunai", "Thrown at a foe, it follows them; shields send it back", "-15% cooldown, +10% damage",
     TalentBranch_Motion, 0, 0, 3, PlayerButton_Kunai, 0.1f},
    {"Fleet Foot", "You run faster", "+6% run speed",
     TalentBranch_Motion, 0, 1, 2, 0},
    {"Blink", "Jumps through space to the cursor", "-15% cooldown",
     TalentBranch_Motion, 1, 0, 3, PlayerButton_Blink},
    {"Sword", "Three-cut combo on right click", "-15% swing interval",
     TalentBranch_Motion, 1, 1, 3, PlayerButton_Attack},
    {"Push", "A wide cone that throws a crowd back", "-15% cooldown, +15% shove",
     TalentBranch_Motion, 2, 0, 3, PlayerButton_Push, 0.f, 0.f, 0.f, 0.15f},
    {"Slam", "Dive from the air and blast where you land", "-15% cooldown, +0.2 s stun",
     TalentBranch_Motion, 2, 1, 3, PlayerButton_Slam, 0.f, 0.2f},
    {"Momentum", "A player kill readies slam, halves blink", "movement reset on a kill",
     TalentBranch_Motion, 3, 0, 1, 0},

    {"Shield", "A moment in which nothing lands", "-15% cooldown, +0.5 s shield",
     TalentBranch_Guard, 0, 0, 3, PlayerButton_Shield, 0.25f},
    {"Ward", "A charge that takes one hit whole", "recharges sooner",
     TalentBranch_Guard, 0, 1, 2, 0},
    {"Gravity Well", "Pulls everyone at the aim into one spot", "-15% cooldown, +0.15 s hold",
     TalentBranch_Guard, 1, 0, 3, PlayerButton_GravityWell, 0.f, 0.15f},
    {"Second Wind", "Back from death in half the time, shielded longer", "faster respawn",
     TalentBranch_Guard, 2, 0, 1, 0},

#include "talents/class_shape_rows.inc"
};
static_assert(ArrayCount(TalentDefs) == Talent_Count, "one row per talent");

global_variable char *TalentBranchNames[TalentBranch_Count] =
{
    "Fire", "Motion", "Guard", "Role", "Run",
};

inline bool32
IsRoleTalent(u32 Talent)
{
    bool32 Result = Talent >= Talent_RoleFirst && Talent <= Talent_RoleLast;
    return Result;
}

inline bool32
IsRunTalent(u32 Talent)
{
    bool32 Result = Talent >= Talent_RunFirst && Talent <= Talent_RunLast;
    return Result;
}

// NOTE(zoubir): a talent of either class tree, which takes points only in
// a run and means what the player's class says
inline bool32
IsClassTalent(u32 Talent)
{
    bool32 Result = IsRoleTalent(Talent) || IsRunTalent(Talent);
    return Result;
}

// NOTE(zoubir): how many tiers Branch has
inline u32
TalentBranchTiers(u32 Branch)
{
    u32 Result = Branch >= TalentBranch_Role ? ROLE_TALENT_TIERS : TALENT_TIERS;
    return Result;
}

// NOTE(zoubir): the passives' numbers, per rank
#define TALENT_SWIFT_FLAMES_SCALE 0.15f
#define TALENT_TWIN_FLAME_SPREAD 0.12f
#define TALENT_FLEET_FOOT_SCALE 0.06f
global_variable float TalentWardSeconds[] = {0.f, 18.f, 11.f};
#define TALENT_SECOND_WIND_RESPAWN 0.5f
#define TALENT_SECOND_WIND_SHIELD 3.f

// NOTE(zoubir): why a point cannot go into a talent, for the panel
enum talent_refusal
{
    TalentRefusal_None,
    TalentRefusal_NoPoints,
    TalentRefusal_MaxRank,
    TalentRefusal_TierLocked,
    // NOTE(zoubir): the other spell of the pair took a point
    TalentRefusal_OtherSpell,
};

// NOTE(zoubir): the level the rules give for free: 1 for an ability the
// match starts with, 0 for the rest and for passives
inline u32
TalentBaseLevel(u32 Talent)
{
    talent_def *Def = &TalentDefs[Talent];
    u32 Result = (Def->Button && (Def->Button & GameRules.Buttons)) ? 1 : 0;
    return Result;
}

// NOTE(zoubir): an ability's level, or a passive's rank
inline u32
TalentLevel(player_slot *Slot, u32 Talent)
{
    u32 Result = Minimum(TalentDefs[Talent].MaxLevel,
                         TalentBaseLevel(Talent) + Slot->Ranks[Talent]);
    return Result;
}

inline u32
TalentMaxRanks(u32 Talent)
{
    u32 Base = TalentBaseLevel(Talent);
    u32 Max = TalentDefs[Talent].MaxLevel;
    u32 Result = Max > Base ? Max - Base : 0;
    return Result;
}

internal u32
TalentPointsSpent(player_slot *Slot, u32 Branch = TalentBranch_Count)
{
    u32 Result = 0;
    for(u32 Talent = 0; Talent < Talent_Count; Talent++)
    {
        if (Branch == TalentBranch_Count || TalentDefs[Talent].Branch == Branch)
        {
            Result += Slot->Ranks[Talent];
        }
    }
    return Result;
}

// NOTE(zoubir): in sim/dungeon/class_tree.cpp: whether Slot took a spell
// of a class tree's pair, and whether Talent is one of those spells
internal bool32 ClassSpellTaken(player_slot *Slot);
inline bool32 IsClassSpellTalent(u32 Talent);

// NOTE(zoubir): one point per level after the first. In a run the first
// spell of a class tree's pair is free, so a class casts three spells
// from the first room and four from level 2
inline u32
TalentPointsLeft(player_slot *Slot)
{
    u32 Earned = Slot->Level > 1 ? Slot->Level - 1 : 0;
    u32 Spent = TalentPointsSpent(Slot);
    Spent -= (Spent && ClassSpellTaken(Slot)) ? 1 : 0;
    u32 Result = Earned > Spent ? Earned - Spent : 0;
    return Result;
}

// NOTE(zoubir): the points Slot may spend now: TalentPointsLeft, and in a
// run (InRun) the free first spell until it is taken
inline u32
TalentPointsToSpend(player_slot *Slot, bool32 InRun)
{
    u32 Result = TalentPointsLeft(Slot) + ((InRun && !ClassSpellTaken(Slot)) ? 1 : 0);
    return Result;
}

// NOTE(zoubir): the points in Branch that open Tier. A class tree's
// branch opens its second tier on the one point its first tier's spell
// takes, then every two more (sim/dungeon/class_tree.cpp)
inline u32
TalentTierCost(u32 Tier, u32 Branch)
{
    u32 Result = TALENT_POINTS_PER_TIER * Tier;
    if (Branch >= TalentBranch_Role && Tier > 0)
    {
        Result = TALENT_POINTS_PER_TIER * Tier - 1;
    }
    return Result;
}

inline bool32
IsTalentTierOpen(player_slot *Slot, u32 Branch, u32 Tier)
{
    bool32 Result = TalentPointsSpent(Slot, Branch) >= TalentTierCost(Tier, Branch);
    return Result;
}

// NOTE(zoubir): in sim/dungeon/class_tree.cpp: the most ranks a class
// tree slot takes for Slot, and whether Slot took the other spell of its
// pair
internal u32 ClassTalentMaxRanks(player_slot *Slot, u32 Talent);
inline bool32 ClassSpellTakenBeside(player_slot *Slot, u32 Talent);

internal talent_refusal
CanLearnTalent(player_slot *Slot, u32 Talent)
{
    talent_refusal Result = TalentRefusal_None;
    talent_def *Def = &TalentDefs[Talent];
    u32 MaxRanks = IsClassTalent(Talent) ? ClassTalentMaxRanks(Slot, Talent) : TalentMaxRanks(Talent);
    if (Slot->Ranks[Talent] >= MaxRanks)
    {
        Result = TalentRefusal_MaxRank;
    }
    else if (IsClassTalent(Talent) && ClassSpellTakenBeside(Slot, Talent))
    {
        Result = TalentRefusal_OtherSpell;
    }
    else if (!IsTalentTierOpen(Slot, Def->Branch, Def->Tier))
    {
        Result = TalentRefusal_TierLocked;
    }
    else if (TalentPointsLeft(Slot) == 0 &&
             !(IsClassSpellTalent(Talent) && !ClassSpellTaken(Slot)))
    {
        Result = TalentRefusal_NoPoints;
    }
    return Result;
}

// NOTE(zoubir): the talent that is Button's ability, Talent_Count for none
// (jump)
internal u32
TalentForButton(u32 Button)
{
    u32 Result = Talent_Count;
    for(u32 Talent = 0; Talent < Talent_Count && Button; Talent++)
    {
        if (TalentDefs[Talent].Button == Button)
        {
            Result = Talent;
            break;
        }
    }
    return Result;
}

// NOTE(zoubir): Button's ability level; 1 for an ability outside the tree
// that the rules allow (jump), 0 for one that is locked
internal u32
AbilityLevel(player_slot *Slot, u32 Button)
{
    u32 Talent = TalentForButton(Button);
    u32 Result = 0;
    if (Talent < Talent_Count)
    {
        Result = TalentLevel(Slot, Talent);
    }
    else if (Button & GameRules.Buttons)
    {
        Result = 1;
    }
    return Result;
}

// NOTE(zoubir): the buttons that do anything for this player: the rules'
// and those its talents unlocked (UpdatePlayer drops the rest)
internal u32
PlayerAllowedButtons(player_slot *Slot)
{
    u32 Result = GameRules.Buttons;
    for(u32 Talent = 0; Talent < Talent_Count; Talent++)
    {
        if (TalentDefs[Talent].Button && Slot->Ranks[Talent] > 0)
        {
            Result |= TalentDefs[Talent].Button;
        }
    }
    return Result;
}

#include "talents/effects.cpp"

// NOTE(zoubir): in sim/dungeon/role_stats.cpp: the role's health on the
// body again after its talents changed (a health talent raises it)
internal void RefreshRoleHealth(app_state *AppState, player_slot *Slot);

// NOTE(zoubir): both class trees' points back, when its role changes
// (sim/dungeon/roles.cpp, which sets the new role's health after)
internal void
ResetRoleTalents(player_slot *Slot)
{
    for(u32 Talent = Talent_RoleFirst; Talent <= Talent_RunLast; Talent++)
    {
        Slot->Ranks[Talent] = 0;
    }
}

// NOTE(zoubir): every point back: ranks to nothing, the abilities they
// unlocked locked again, the ward gone. Cooldowns under way stay
internal void
ResetTalents(app_state *AppState, u32 SlotIndex)
{
    player_slot *Slot = &AppState->Players[SlotIndex];
    if (!Slot->Active || TalentPointsSpent(Slot) == 0)
    {
        return;
    }
    for(u32 Talent = 0; Talent < TALENT_SLOTS; Talent++)
    {
        Slot->Ranks[Talent] = 0;
    }
    Slot->WardReady = false;
    Slot->WardRecharge = 0.f;
    RefreshRoleHealth(AppState, Slot);
    if (Slot->Entity && Slot->Entity->IsPresent)
    {
        EmitBurst(&AppState->Events, SimBurst_TalentLearned, (u8)SlotIndex,
                  Slot->Entity->Position);
    }
}

// NOTE(zoubir): in sim/dungeon/role_abilities.cpp: whether a dungeon
// role casts its own spell on the key Talent's ability is on, so the
// talent does nothing there
internal bool32 RoleReplacesTalent(app_state *AppState, player_slot *Slot, u32 Talent);

// NOTE(zoubir): one point into Talent, if it may take one; returns
// whether it did. In a dungeon run a point never goes into an ability the
// player's role has taken the key of
internal bool32
LearnTalent(app_state *AppState, u32 SlotIndex, u32 Talent)
{
    player_slot *Slot = &AppState->Players[SlotIndex];
    if (Talent >= Talent_Count || !Slot->Active ||
        CanLearnTalent(Slot, Talent) != TalentRefusal_None ||
        (IsClassTalent(Talent) && !AppState->Dungeon) ||
        RoleReplacesTalent(AppState, Slot, Talent))
    {
        return false;
    }
    Slot->Ranks[Talent]++;
    if (Talent == Talent_Ward)
    {
        Slot->WardReady = true;
        Slot->WardRecharge = 0.f;
    }
    if (IsClassTalent(Talent))
    {
        RefreshRoleHealth(AppState, Slot);
    }
    if (Slot->Entity && Slot->Entity->IsPresent)
    {
        EmitBurst(&AppState->Events, SimBurst_TalentLearned, (u8)SlotIndex,
                  Slot->Entity->Position);
    }
    return true;
}
