/* Monster kinds: every monster is EntityType_Monster (so sword, fireball
   and collision rules apply to all of them). A kind is a monster_def:
   stats, size, how often it spawns, up to MAX_MONSTER_ABILITIES abilities,
   and (client only, art/monster_art.cpp) a function that draws its
   sprite frames. Each kind lives in its own
   file under sim/monsters/, listed once in sim/monsters/monster_list.inc.
   See sim/monsters/README.md for adding one. */

#include "monster_affixes.cpp"
#include "status_effects.cpp"

enum monster_ability_kind
{
    MonsterAbility_None,
    // NOTE(zoubir): hits every player within Radius of the monster
    MonsterAbility_Slam,
    // NOTE(zoubir): runs along the aim locked at windup start, hitting the
    // first player it touches; a wall cuts it short and stuns the monster
    MonsterAbility_Charge,
    // NOTE(zoubir): marks Count spots around the target during windup,
    // each blows up for Damage within Radius when the windup ends
    MonsterAbility_Mortar,
    // NOTE(zoubir): marks a spot behind the target during windup, then
    // appears there and strikes everything within Radius
    MonsterAbility_Blink,
    // NOTE(zoubir): throws Count shots fanned over Spread degrees along
    // the aim locked at windup start. Shots fly at Speed for Active
    // seconds, stop at walls and hit the first player within Radius
    MonsterAbility_Volley,
    // NOTE(zoubir): raises Count monsters of SummonKind on spots marked
    // during windup, while fewer than MaxActive of its summons live.
    // Summons crumble when their summoner dies
    MonsterAbility_Summon,
    // NOTE(zoubir): heals the most hurt ally within Radius by Heal; only
    // starts when an ally is below MEND_THRESHOLD of its health
    MonsterAbility_Mend,
    // NOTE(zoubir): digs in when the windup ends and stays underground
    // (immune) for Active seconds while a ripple tunnels toward the
    // target. The landing spot follows the target until the last
    // BURROW_LOCK_SHARE of Active, then locks and is marked; the monster
    // erupts there, hitting everything within Radius
    MonsterAbility_Burrow,
    MonsterAbility_Count
};

// NOTE(zoubir): how a shot looks; one sheet row each in art/monster_fx.cpp
enum monster_shot_style
{
    ShotStyle_Ember,
    ShotStyle_Bile,
    ShotStyle_Spine,
    ShotStyle_Count
};
#define SHOT_FRAME_SIZE 16
#define SHOT_FRAMES 4

// NOTE(zoubir): how a lingering ground patch looks, art/monster_fx.cpp
enum monster_hazard_style
{
    HazardStyle_Bile,
    HazardStyle_Web,
    HazardStyle_Embers,
    HazardStyle_Goo,
    HazardStyle_Count
};
#define HAZARD_FRAME_SIZE 48
#define HAZARD_FRAMES 4

struct monster_ability
{
    monster_ability_kind Kind;
    char *Name;
    // NOTE(zoubir): used only when the target is between these distances
    float MinRange;
    float MaxRange;
    float Cooldown;
    float Windup;
    float Active;
    float Recover;
    float Damage;
    float Radius;
    // NOTE(zoubir): speed for charges, push on players hit for the rest
    float Speed;
    float Knockback;
    u32 Count;
    // NOTE(zoubir): mortar scatter, blink distance behind the target,
    // volley fan width in degrees
    float Spread;
    monster_shot_style ShotStyle;
    // NOTE(zoubir): put on every player this ability hits (and on anyone
    // standing in its hazard)
    status_effect Status;
    float StatusSeconds;
    // NOTE(zoubir): slams and mortar spots leave a hazard of Radius for
    // this long
    float HazardSeconds;
    monster_hazard_style HazardStyle;
    monster_kind SummonKind;
    u32 MaxActive;
    float Heal;
    // NOTE(zoubir): which phases may use it; bit 0 = before enrage,
    // bit 1 = after. 0 means every phase
    u32 PhaseMask;
};
#define PHASE_CALM (1 << 0)
#define PHASE_ENRAGED (1 << 1)

#define MONSTER_SHEET_COLUMNS 6
// NOTE(zoubir): sheet rows, top to bottom
enum monster_sheet_row
{
    MonsterRow_Idle,
    MonsterRow_Move,
    MonsterRow_Windup,
    MonsterRow_Attack,
    MonsterRow_Recover,
    // NOTE(zoubir): optional, for kinds with an odd state such as being
    // underground; FrameCounts 0 leaves it empty. Played as
    // AnimationType_JumpDown, which monsters never use otherwise
    MonsterRow_Special,
    MonsterRow_Count
};


// NOTE(zoubir): what happens when a monster of this kind dies
enum monster_death_effect
{
    DeathEffect_None,
    // NOTE(zoubir): SplitCount monsters of SplitKind pop out of the corpse
    DeathEffect_Split,
    DeathEffect_Count
};

struct monster_def
{
    char *Name;
    float MaxHp;
    float Acceleration;
    float AggroRange;
    float StopRange;
    // NOTE(zoubir): the plain bite every monster has, off cooldown
    float AttackRange;
    float AttackDamage;
    float AttackInterval;
    // NOTE(zoubir): 0 for walkers, hover height for flyers
    float FlyHeight;
    // NOTE(zoubir): multiplied with the sprite, 0 means untinted
    u32 Tint;
    // NOTE(zoubir): relative odds of this kind when the arena refills
    u32 SpawnWeight;
    // NOTE(zoubir): square frame, in pixels; drawn at one unit per pixel
    u32 FrameSize;
    u32 FrameCounts[MonsterRow_Count];
    float SecondsPerFrame[MonsterRow_Count];

    u32 AbilityCount;
    monster_ability Abilities[MAX_MONSTER_ABILITIES];

    monster_death_effect DeathEffect;
    monster_kind SplitKind;
    u32 SplitCount;

    // NOTE(zoubir): share of damage a hit loses when it comes from inside
    // the front arc (centered on the monster's Direction)
    float FrontArmor;
    float FrontArcDegrees;
    // NOTE(zoubir): radians per second the facing turns toward the target;
    // 0 turns instantly
    float TurnRate;

    // NOTE(zoubir): below this share of health the monster enrages once:
    // moves EnrageSpeedScale faster, recharges in EnrageCooldownScale of
    // the time, takes EnrageTint and unlocks PHASE_ENRAGED abilities.
    // 0 never enrages
    float EnrageHpShare;
    float EnrageSpeedScale;
    float EnrageCooldownScale;
    u32 EnrageTint;

    // NOTE(zoubir): at most this many alive at once when the arena refills;
    // 0 means no limit
    u32 MaxAlive;
};
// NOTE(zoubir): older code calls the def "stats"
typedef monster_def monster_stats;

inline monster_ability *
AddMonsterAbility(monster_def *Def, monster_ability_kind Kind, char *Name)
{
    Assert(Def->AbilityCount < MAX_MONSTER_ABILITIES);
    monster_ability *Result = &Def->Abilities[Def->AbilityCount++];
    // NOTE(zoubir): ZeroSize, not = {}, so padding is zero too and
    // ComputeMonsterTableHash sees the same bytes in every program
    ZeroSize(Result, sizeof(*Result));
    Result->Kind = Kind;
    Result->Name = Name;
    Result->Count = 1;
    return Result;
}

// NOTE(zoubir): defaults every kind starts from before its Define function
inline void
DefaultMonsterDef(monster_def *Def)
{
    ZeroSize(Def, sizeof(*Def));
    Def->SpawnWeight = 1;
    Def->FrameSize = 48;
    Def->FrameCounts[MonsterRow_Idle] = 4;
    Def->FrameCounts[MonsterRow_Move] = 6;
    Def->FrameCounts[MonsterRow_Windup] = 4;
    Def->FrameCounts[MonsterRow_Attack] = 4;
    Def->FrameCounts[MonsterRow_Recover] = 4;
    Def->SecondsPerFrame[MonsterRow_Idle] = 0.18f;
    Def->SecondsPerFrame[MonsterRow_Move] = 0.1f;
    Def->SecondsPerFrame[MonsterRow_Windup] = 0.1f;
    Def->SecondsPerFrame[MonsterRow_Attack] = 0.08f;
    Def->SecondsPerFrame[MonsterRow_Recover] = 0.15f;
    Def->FrameCounts[MonsterRow_Special] = 0;
    Def->SecondsPerFrame[MonsterRow_Special] = 0.1f;
}

#include "monsters/monster_list.inc"

// NOTE(zoubir): runtime state shared by every monster in the arena; the
// refill logic lives in monster_population.cpp
// NOTE(zoubir): a death waiting for its effect; DamageEntity cannot spawn
// entities itself (it has no arena), so the population does it next tick
struct monster_death_record
{
    monster_kind Kind;
    v3 Position;
    u32 EliteAffix;
};
#define MAX_PENDING_DEATHS 32

struct monster_population
{
    u32 Target;
    float RespawnTimer;
    random_series Series;
    monster_death_record PendingDeaths[MAX_PENDING_DEATHS];
    u32 PendingDeathCount;
    u32 NextMonsterSerial;
    animation_set AnimationSets[MonsterKind_Count];
    animation_set ShotAnimationSets[ShotStyle_Count];
    animation_set HazardAnimationSets[HazardStyle_Count];
};

typedef void monster_define_function(monster_def *Def);

global_variable monster_define_function *MonsterDefineFunctions[MonsterKind_Count] =
{
#define MONSTER(Name) DefineMonster_##Name,
#define MONSTER_NAME_PASS
#include "monsters/monster_list.inc"
#undef MONSTER_NAME_PASS
#undef MONSTER
};

// NOTE(zoubir): filled on first use. Defs are plain data, so rebuilding
// them after a hot reload of app.dll gives the same table
global_variable monster_def MonsterDefTable[MonsterKind_Count];
global_variable bool32 MonsterDefTableReady;

inline monster_def *
GetMonsterDef(monster_kind Kind)
{
    Assert(Kind < MonsterKind_Count);
    if (!MonsterDefTableReady)
    {
        for(u32 KindIndex = 0; KindIndex < MonsterKind_Count; KindIndex++)
        {
            monster_def *Def = &MonsterDefTable[KindIndex];
            DefaultMonsterDef(Def);
            MonsterDefineFunctions[KindIndex](Def);
        }
        MonsterDefTableReady = true;
    }
    monster_def *Result = &MonsterDefTable[Kind];
    return Result;
}

inline monster_stats *
GetMonsterStats(monster_kind Kind)
{
    return GetMonsterDef(Kind);
}

inline u32
CountAliveOfKind(world *World, monster_kind Kind)
{
    u32 Result = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        Result += Entity->IsPresent && Entity->Type == EntityType_Monster &&
            Entity->MonsterKind == Kind;
    }
    return Result;
}

// NOTE(zoubir): how much a kind weighs in the next refill: its SpawnWeight,
// or 0 once MaxAlive of it are already in the arena
inline u32
GetSpawnWeightNow(world *World, monster_kind Kind)
{
    monster_def *Def = GetMonsterDef(Kind);
    // NOTE(zoubir): the map scales each kind; four steps per unit of
    // SpawnWeight keep fractional map weights meaningful
    float MapWeight = GetMapDef((map_id)World->MapId)->MonsterWeight[Kind];
    u32 Result = (u32)(4.f * (float)Def->SpawnWeight * MapWeight + 0.5f);
    if (Def->MaxAlive && CountAliveOfKind(World, Kind) >= Def->MaxAlive)
    {
        Result = 0;
    }
    return Result;
}

// NOTE(zoubir): weighted by SpawnWeight, skipping kinds at their MaxAlive
internal monster_kind
PickMonsterKind(random_series *Series, world *World)
{
    u32 Weights[MonsterKind_Count];
    u32 TotalWeight = 0;
    for(u32 KindIndex = 0; KindIndex < MonsterKind_Count; KindIndex++)
    {
        Weights[KindIndex] = GetSpawnWeightNow(World, (monster_kind)KindIndex);
        TotalWeight += Weights[KindIndex];
    }
    monster_kind Result = (monster_kind)0;
    if (TotalWeight > 0)
    {
        u32 Roll = RandomChoice(Series, TotalWeight);
        for(u32 KindIndex = 0; KindIndex < MonsterKind_Count; KindIndex++)
        {
            u32 Weight = Weights[KindIndex];
            if (Roll < Weight)
            {
                Result = (monster_kind)KindIndex;
                break;
            }
            Roll -= Weight;
        }
    }
    return Result;
}

// NOTE(zoubir): sheet rows map onto the shared animation types; every
// direction plays the right-facing frames, left mirrors them
internal void
SetupMonsterAnimationSet(animation_set *Set, memory_arena *Arena,
                         monster_def *Def)
{
    animation_type RowTypes[MonsterRow_Count] =
        {
            AnimationType_Stand,
            AnimationType_Move,
            AnimationType_Cast,
            AnimationType_Attack,
            AnimationType_Stop,
            AnimationType_JumpDown,
        };
    for(u32 Row = 0; Row < MonsterRow_Count; Row++)
    {
        u32 FirstIndex = Row * MONSTER_SHEET_COLUMNS;
        u32 Count = Def->FrameCounts[Row];
        if (Count == 0)
        {
            continue;
        }
        float Seconds = Def->SecondsPerFrame[Row];
        AddAnimation(Set, Arena, RowTypes[Row], AnimationDirection_Right,
                     FirstIndex, Count, Seconds);
        AddAnimation(Set, Arena, RowTypes[Row], AnimationDirection_Up,
                     FirstIndex, Count, Seconds);
        AddAnimation(Set, Arena, RowTypes[Row], AnimationDirection_Down,
                     FirstIndex, Count, Seconds);
        AddAnimation(Set, Arena, RowTypes[Row], AnimationDirection_Left,
                     FirstIndex, Count, Seconds, true);
    }
    // NOTE(zoubir): jumps never happen to monsters, but keep the slots
    // valid in case something asks
    for(u32 Direction = 0; Direction < AnimationDirection_Count; Direction++)
    {
        Set->Animations[AnimationType_JumpUp][Direction] =
            Set->Animations[AnimationType_Stand][Direction];
        if (Def->FrameCounts[MonsterRow_Special] == 0)
        {
            Set->Animations[AnimationType_JumpDown][Direction] =
                Set->Animations[AnimationType_Stand][Direction];
        }
    }
}

internal void
RecordMonsterDeath(app_state *AppState, world_entity *Monster)
{
    monster_population *Population = AppState->Monsters;
    if (!Population ||
        GetMonsterDef(Monster->MonsterKind)->DeathEffect == DeathEffect_None ||
        Population->PendingDeathCount >= MAX_PENDING_DEATHS)
    {
        return;
    }
    monster_death_record *Record =
        &Population->PendingDeaths[Population->PendingDeathCount++];
    Record->Kind = Monster->MonsterKind;
    Record->Position = Monster->Position;
    Record->EliteAffix = Monster->EliteAffix;
}

inline u32
HashBytes(u32 Hash, void *Data, memory_index Size)
{
    u8 *Bytes = (u8 *)Data;
    for(memory_index Index = 0; Index < Size; Index++)
    {
        Hash = (Hash ^ Bytes[Index]) * 16777619u;
    }
    return Hash;
}

inline u32
HashString(u32 Hash, char *String)
{
    for(; String && *String; String++)
    {
        Hash = (Hash ^ (u8)*String) * 16777619u;
    }
    return Hash;
}

// NOTE(zoubir): a fingerprint of every monster kind, ability and affix.
// Client and server must agree on it, since kinds travel as numbers.
// Names are hashed by text; the pointers to them differ per program
internal u32
ComputeMonsterTableHash()
{
    u32 Hash = 2166136261u;
    u32 KindCount = MonsterKind_Count;
    Hash = HashBytes(Hash, &KindCount, sizeof(KindCount));
    for(u32 KindIndex = 0; KindIndex < MonsterKind_Count; KindIndex++)
    {
        monster_def Def;
        memcpy(&Def, GetMonsterDef((monster_kind)KindIndex), sizeof(Def));
        Hash = HashString(Hash, Def.Name);
        Def.Name = 0;
        for(u32 AbilityIndex = 0; AbilityIndex < MAX_MONSTER_ABILITIES; AbilityIndex++)
        {
            Hash = HashString(Hash, Def.Abilities[AbilityIndex].Name);
            Def.Abilities[AbilityIndex].Name = 0;
        }
        Hash = HashBytes(Hash, &Def, sizeof(Def));
    }
    for(u32 AffixIndex = 0; AffixIndex < MonsterAffix_Count; AffixIndex++)
    {
        monster_affix_def Affix;
        memcpy(&Affix, GetAffix(AffixIndex), sizeof(Affix));
        Hash = HashString(Hash, Affix.Name);
        Affix.Name = 0;
        Hash = HashBytes(Hash, &Affix, sizeof(Affix));
    }
    return Hash;
}
