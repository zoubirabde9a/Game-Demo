/* Talents: what a player spends its level-up points on, as one table of
   three branches (Fire, Motion, Guard) of four tiers. A tier opens once
   TALENT_POINTS_PER_TIER points per tier above it are spent in that
   branch: tier 1 at 2, tier 2 at 4, tier 3 at 6.

   A talent is either an ability or a passive. An ability talent has the
   ability's button and up to three levels. The abilities the rules start
   with (GameRules.Buttons: fireball, launch, dash, blink, shield, world
   rewind) are level 1 for free, so two more levels can be bought; the
   others (sword, shockwave, push, slam, the self and bubble rewinds, and
   Frost Nova and Gravity Well, which only exist here) are locked until a
   point unlocks them at level 1. Each level after the first takes
   TALENT_COOLDOWN_PER_LEVEL off the cooldown, and adds PowerPerLevel to
   what its row's Power is (dash speed, shield seconds).

   A passive changes one thing, by rank:
   Swift Flames  fireballs fly faster and farther
   Twin Flame    each cast throws two fireballs
   Pyre          killing a player makes the fireball ready at once
   Fleet Foot    a faster run
   Momentum      a player kill readies dash and slam and halves blink's
                 wait, as a monster kill does
   Ward          a charge that takes one hit whole, back after a while
   Second Wind   back from death sooner, with a longer shield

   Points come in through player_input.Learn (LearnTalent), so offline,
   online and in replays the same tick spends them. The client draws the
   tree (ui/talent_panel/) and the server sends each player its ranks. */

#define TALENT_POINTS_PER_TIER 2
#define TALENT_TIERS 4
// NOTE(zoubir): share of an ability's cooldown each level after the first
// takes off: 6 s at level 1 is 5.1 at 2 and 4.2 at 3
#define TALENT_COOLDOWN_PER_LEVEL 0.15f

enum talent_branch
{
    TalentBranch_Fire,
    TalentBranch_Motion,
    TalentBranch_Guard,
    TalentBranch_Count
};

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

    Talent_Dash,
    Talent_FleetFoot,
    Talent_Blink,
    Talent_Sword,
    Talent_Push,
    Talent_Slam,
    Talent_Momentum,

    Talent_Shield,
    Talent_Ward,
    Talent_RewindWorld,
    Talent_RewindSelf,
    Talent_GravityWell,
    Talent_RewindBubble,
    Talent_SecondWind,

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
    // NOTE(zoubir): share added to the ability row's Power per level
    // after the first
    float PowerPerLevel;
};

global_variable talent_def TalentDefs[Talent_Count] =
{
    {"Fireball", "A piercing bolt along the aim", "-15% cooldown",
     TalentBranch_Fire, 0, 0, 3, PlayerButton_Cast, 0.f},
    {"Swift Flames", "Fireballs fly faster and farther", "+15% fireball speed and range",
     TalentBranch_Fire, 0, 1, 2, 0, 0.f},
    {"Launch", "Throws everything at the aim into the air", "-15% cooldown",
     TalentBranch_Fire, 1, 0, 3, PlayerButton_Launch, 0.f},
    {"Shockwave", "Blasts everything around you away", "-15% cooldown",
     TalentBranch_Fire, 1, 1, 3, PlayerButton_Shockwave, 0.f},
    {"Frost Nova", "Freezes everyone near you, then slows them", "-15% cooldown",
     TalentBranch_Fire, 2, 0, 3, PlayerButton_FrostNova, 0.f},
    {"Twin Flame", "Each cast throws two fireballs", "a second fireball",
     TalentBranch_Fire, 2, 1, 1, 0, 0.f},
    {"Pyre", "Killing a player makes the fireball ready at once", "fireball reset on a kill",
     TalentBranch_Fire, 3, 0, 1, 0, 0.f},

    {"Dash", "A burst of speed", "-15% cooldown, +10% speed",
     TalentBranch_Motion, 0, 0, 3, PlayerButton_Dash, 0.1f},
    {"Fleet Foot", "You run faster", "+6% run speed",
     TalentBranch_Motion, 0, 1, 2, 0, 0.f},
    {"Blink", "Jumps through space to the cursor", "-15% cooldown",
     TalentBranch_Motion, 1, 0, 3, PlayerButton_Blink, 0.f},
    {"Sword", "Three-cut combo on right click", "-15% swing interval",
     TalentBranch_Motion, 1, 1, 3, PlayerButton_Attack, 0.f},
    {"Push", "A wide cone that throws a crowd back", "-15% cooldown",
     TalentBranch_Motion, 2, 0, 3, PlayerButton_Push, 0.f},
    {"Slam", "Dive from the air and blast where you land", "-15% cooldown",
     TalentBranch_Motion, 2, 1, 3, PlayerButton_Slam, 0.f},
    {"Momentum", "A player kill readies dash and slam, halves blink", "movement reset on a kill",
     TalentBranch_Motion, 3, 0, 1, 0, 0.f},

    {"Shield", "A moment in which nothing lands", "-15% cooldown, +0.5 s shield",
     TalentBranch_Guard, 0, 0, 3, PlayerButton_Shield, 0.25f},
    {"Ward", "A charge that takes one hit whole", "recharges sooner",
     TalentBranch_Guard, 0, 1, 2, 0, 0.f},
    {"World Rewind", "Everyone goes back 2 seconds", "-15% cooldown",
     TalentBranch_Guard, 1, 0, 3, PlayerButton_RewindWorld, 0.f},
    {"Rewind", "You go back to where you were 2 seconds ago", "-15% cooldown",
     TalentBranch_Guard, 1, 1, 3, PlayerButton_RewindSelf, 0.f},
    {"Gravity Well", "Pulls everyone at the aim into one spot", "-15% cooldown",
     TalentBranch_Guard, 2, 0, 3, PlayerButton_GravityWell, 0.f},
    {"Rewind Bubble", "Everything around you goes back 2 seconds", "-15% cooldown",
     TalentBranch_Guard, 2, 1, 3, PlayerButton_RewindBubble, 0.f},
    {"Second Wind", "Back from death in half the time, shielded longer", "faster respawn",
     TalentBranch_Guard, 3, 0, 1, 0, 0.f},
};

global_variable char *TalentBranchNames[TalentBranch_Count] =
{
    "Fire", "Motion", "Guard",
};

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

// NOTE(zoubir): one point per level after the first
inline u32
TalentPointsLeft(player_slot *Slot)
{
    u32 Earned = Slot->Level > 1 ? Slot->Level - 1 : 0;
    u32 Spent = TalentPointsSpent(Slot);
    u32 Result = Earned > Spent ? Earned - Spent : 0;
    return Result;
}

inline u32
TalentTierCost(u32 Tier)
{
    u32 Result = TALENT_POINTS_PER_TIER * Tier;
    return Result;
}

inline bool32
IsTalentTierOpen(player_slot *Slot, u32 Branch, u32 Tier)
{
    bool32 Result = TalentPointsSpent(Slot, Branch) >= TalentTierCost(Tier);
    return Result;
}

internal talent_refusal
CanLearnTalent(player_slot *Slot, u32 Talent)
{
    talent_refusal Result = TalentRefusal_None;
    talent_def *Def = &TalentDefs[Talent];
    if (Slot->Ranks[Talent] >= TalentMaxRanks(Talent))
    {
        Result = TalentRefusal_MaxRank;
    }
    else if (!IsTalentTierOpen(Slot, Def->Branch, Def->Tier))
    {
        Result = TalentRefusal_TierLocked;
    }
    else if (TalentPointsLeft(Slot) == 0)
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

inline player_slot *
TalentSlotOf(app_state *AppState, world_entity *Player)
{
    player_slot *Result = (Player->Type == EntityType_Player &&
                           Player->PlayerIndex < MAX_PLAYERS) ?
        &AppState->Players[Player->PlayerIndex] : 0;
    return Result;
}

// NOTE(zoubir): what Button's cooldown is multiplied by at its level
inline float
CooldownScaleForLevel(u32 Level)
{
    float Result = Level > 1 ? 1.f - TALENT_COOLDOWN_PER_LEVEL * (float)(Level - 1) : 1.f;
    return Result;
}

internal float
PlayerCooldownScale(app_state *AppState, world_entity *Player, u32 Button)
{
    player_slot *Slot = TalentSlotOf(AppState, Player);
    float Result = Slot ? CooldownScaleForLevel(AbilityLevel(Slot, Button)) : 1.f;
    return Result;
}

internal float
PlayerPowerScale(app_state *AppState, world_entity *Player, u32 Button)
{
    player_slot *Slot = TalentSlotOf(AppState, Player);
    u32 Talent = TalentForButton(Button);
    float Result = 1.f;
    if (Slot && Talent < Talent_Count)
    {
        u32 Level = TalentLevel(Slot, Talent);
        if (Level > 1)
        {
            Result += TalentDefs[Talent].PowerPerLevel * (float)(Level - 1);
        }
    }
    return Result;
}

// NOTE(zoubir): a passive's rank on the player behind Player, 0 for none
inline u32
PlayerTalentRank(app_state *AppState, world_entity *Player, u32 Talent)
{
    player_slot *Slot = TalentSlotOf(AppState, Player);
    u32 Result = Slot ? Slot->Ranks[Talent] : 0;
    return Result;
}

inline float
FireballSpeedScale(app_state *AppState, world_entity *Player)
{
    float Result = 1.f + TALENT_SWIFT_FLAMES_SCALE *
        (float)PlayerTalentRank(AppState, Player, Talent_SwiftFlames);
    return Result;
}

inline float
RunSpeedScale(app_state *AppState, world_entity *Player)
{
    float Result = 1.f + TALENT_FLEET_FOOT_SCALE *
        (float)PlayerTalentRank(AppState, Player, Talent_FleetFoot);
    return Result;
}

inline float
RespawnSeconds(player_slot *Slot)
{
    float Result = PLAYER_RESPAWN_SECONDS;
    if (Slot->Ranks[Talent_SecondWind])
    {
        Result *= TALENT_SECOND_WIND_RESPAWN;
    }
    return Result;
}

inline float
RespawnShieldSeconds(player_slot *Slot)
{
    float Result = Slot->Ranks[Talent_SecondWind] ?
        TALENT_SECOND_WIND_SHIELD : PLAYER_SPAWN_SHIELD_SECONDS;
    return Result;
}

// NOTE(zoubir): the Ward talent taking a hit of Damage on Target instead
// of it; true when it did (DamageEntity, ApplyHit). A spent ward comes
// back after its rank's seconds (UpdateProgression)
internal bool32
WardTakesHit(app_state *AppState, world_entity *Target, float Damage)
{
    player_slot *Slot = TalentSlotOf(AppState, Target);
    bool32 Result = false;
    if (Slot && Damage > 0.f && Slot->WardReady && Slot->Ranks[Talent_Ward] &&
        !Slot->Predicted)
    {
        Slot->WardReady = false;
        Slot->WardRecharge = TalentWardSeconds[Slot->Ranks[Talent_Ward]];
        v3 Chest = Target->Position;
        Chest.Z += 16.f;
        EmitBurst(&AppState->Events, SimBurst_WardBreak,
                  (u8)Target->PlayerIndex, Chest);
        EmitSound(&AppState->Events, AssetType_Dash, Target->Position);
        Result = true;
    }
    return Result;
}

// NOTE(zoubir): one point into Talent, if it may take one; returns
// whether it did
internal bool32
LearnTalent(app_state *AppState, u32 SlotIndex, u32 Talent)
{
    player_slot *Slot = &AppState->Players[SlotIndex];
    if (Talent >= Talent_Count || !Slot->Active ||
        CanLearnTalent(Slot, Talent) != TalentRefusal_None)
    {
        return false;
    }
    Slot->Ranks[Talent]++;
    if (Talent == Talent_Ward)
    {
        Slot->WardReady = true;
        Slot->WardRecharge = 0.f;
    }
    if (Slot->Entity && Slot->Entity->IsPresent)
    {
        EmitBurst(&AppState->Events, SimBurst_TalentLearned, (u8)SlotIndex,
                  Slot->Entity->Position);
    }
    return true;
}
