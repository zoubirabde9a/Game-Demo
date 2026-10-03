/* Monster kinds: every monster is EntityType_Monster (so sword, fireball
   and collision rules apply to all of them). A kind is a monster_def:
   stats, size, how often it spawns, up to MAX_MONSTER_ABILITIES abilities,
   and a function that draws its sprite frames. Each kind lives in its own
   file under sim/monsters/, listed once in sim/monsters/monster_list.inc.
   See sim/monsters/README.md for adding one. */

#include "../art/sprite_canvas.cpp"
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
    // NOTE(zoubir): mortar spots leave a hazard of Radius for this long
    float HazardSeconds;
    monster_hazard_style HazardStyle;
};

#define MONSTER_SHEET_COLUMNS 6
// NOTE(zoubir): sheet rows, top to bottom
enum monster_sheet_row
{
    MonsterRow_Idle,
    MonsterRow_Move,
    MonsterRow_Windup,
    MonsterRow_Attack,
    MonsterRow_Recover,
    MonsterRow_Count
};

typedef void monster_draw_function(sprite_canvas *Canvas, monster_pose Pose);

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
};
// NOTE(zoubir): older code calls the def "stats"
typedef monster_def monster_stats;

inline monster_ability *
AddMonsterAbility(monster_def *Def, monster_ability_kind Kind, char *Name)
{
    Assert(Def->AbilityCount < MAX_MONSTER_ABILITIES);
    monster_ability *Result = &Def->Abilities[Def->AbilityCount++];
    *Result = {};
    Result->Kind = Kind;
    Result->Name = Name;
    Result->Count = 1;
    return Result;
}

// NOTE(zoubir): defaults every kind starts from before its Define function
inline void
DefaultMonsterDef(monster_def *Def)
{
    *Def = {};
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
}

#include "monsters/monster_list.inc"

// NOTE(zoubir): runtime state shared by every monster in the arena; the
// refill logic lives in monster_population.cpp
struct monster_population
{
    u32 Target;
    float RespawnTimer;
    random_series Series;
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

global_variable monster_draw_function *MonsterDrawFunctions[MonsterKind_Count] =
{
#define MONSTER(Name) DrawMonster_##Name,
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

// NOTE(zoubir): weighted by SpawnWeight
internal monster_kind
PickMonsterKind(random_series *Series)
{
    u32 TotalWeight = 0;
    for(u32 KindIndex = 0; KindIndex < MonsterKind_Count; KindIndex++)
    {
        TotalWeight += GetMonsterDef((monster_kind)KindIndex)->SpawnWeight;
    }
    monster_kind Result = (monster_kind)0;
    if (TotalWeight > 0)
    {
        u32 Roll = RandomChoice(Series, TotalWeight);
        for(u32 KindIndex = 0; KindIndex < MonsterKind_Count; KindIndex++)
        {
            u32 Weight = GetMonsterDef((monster_kind)KindIndex)->SpawnWeight;
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
        };
    for(u32 Row = 0; Row < MonsterRow_Count; Row++)
    {
        u32 FirstIndex = Row * MONSTER_SHEET_COLUMNS;
        u32 Count = Def->FrameCounts[Row];
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
        Set->Animations[AnimationType_JumpDown][Direction] =
            Set->Animations[AnimationType_Stand][Direction];
    }
}

