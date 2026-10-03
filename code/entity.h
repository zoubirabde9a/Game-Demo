#if !defined(ENTITY_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */
#define ENTITY_H

struct world;

enum animation_type
{
    AnimationType_Move,
    AnimationType_Stand,
    AnimationType_Attack,
    AnimationType_JumpUp,
    AnimationType_JumpDown,
    AnimationType_Cast,
    AnimationType_Stop,
    AnimationType_Count
};

enum animation_direction
{
    AnimationDirection_Up,
    AnimationDirection_Down,
    AnimationDirection_Right,
    AnimationDirection_Left,
    AnimationDirection_Count
};

struct animation_slot
{
    u32 FirstIndex;
    u32 IndicesCount;
    float *FramesTimeInSeconds;    
    bool32 Reversed;
};

struct animation_state
{
    float DeltaTime;
    u32 SlotIndex;
    
    animation_direction LastAnimationDirection;
    // NOTE(zoubir): what AdvanceAnimation last played, read when drawing
    animation_type CurrentType;
};

struct animation_set
{
    animation_slot Animations[AnimationType_Count][AnimationDirection_Count];
};

struct cannonical_position
{
    u32 TileX;
    u32 TileY;
    u32 TileZ;

    v3 TileRel;
};

enum entity_type
{
    EntityType_Invalid,
    EntityType_Player,
    EntityType_StaticObject,
    EntityType_Tiled,
    EntityType_Monster,
    EntityType_Familiar,
    EntityType_FireBall,
    EntityType_Sword,
    EntityType_Count
};

enum entity_flag
{
    EntityFlag_NonCollidable = (1 << 1),
    EntityFlag_NonSpatial = (1 << 2),
};

enum entity_state
{
    EntityState_Standing,
    EntityState_Moving,
    EntityState_Attacking,
    EntityState_Casting,
    EntityState_Stopping,
    EntityState_Jumping
};

struct entity_collision_volume
{
    v3 Offset;
    v3 HalfDims;
};

struct entity_collision_volume_group
{
    entity_collision_volume TotalVolume;
    u32 VolumesCount;
    entity_collision_volume *Volumes;
};

#define PLAYER_DASH_COOLDOWN 0.8f
#define PLAYER_SHOCKWAVE_COOLDOWN 4.f
#define SWORD_DAMAGE 25.f
#define FIREBALL_DAMAGE 25.f

// NOTE(zoubir): one MonsterKind_<Name> per line of monster_list.inc, see
// code/sim/monsters/README.md
enum monster_kind
{
#define MONSTER(Name) MonsterKind_##Name,
#define MONSTER_NAME_PASS
#include "sim/monsters/monster_list.inc"
#undef MONSTER_NAME_PASS
#undef MONSTER
    MonsterKind_Count
};

#define MAX_MONSTER_ABILITIES 3
#define MAX_ABILITY_POINTS 4

// NOTE(zoubir): every monster ability runs Windup (rooted, telegraphed,
// can be dodged) -> Active (the hit or the movement) -> Recover (open to
// punishment), then goes back to Ready. See sim/monster_abilities.cpp
enum ability_phase
{
    AbilityPhase_Ready,
    AbilityPhase_Windup,
    AbilityPhase_Active,
    AbilityPhase_Recover,
};

struct world_entity
{
    u32 ID;
    entity_type Type;
    entity_state State;
    u32 Flags;
    
    v3 Position;
    float GroundZ;
    v3 Velocity;
    v2 Dimensions;
    v2 Direction;
    v2 CastingDirection;
    animation_state AnimationState;
    animation_set *AnimationSet;
    asset_id Texture;
    asset_id ShadowTexture;
    
    entity_collision_volume_group *Collision;
//    v3 CollisionHalfDims;
    float Scale;
    // WALL
    v4 Uvs;

    // Familliar
    world_entity *FollowingEntity;
    float tFlying;
    //FireBall
    float DistanceRemaining;
    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection;

    //Monster && Player
    // NOTE(zoubir): which player_slot owns this player entity
    u32 PlayerIndex;
    // NOTE(zoubir): swords and fireballs remember the slot that made them,
    // so they never hit it and its kills are credited to it
    bool32 HasOwner;
    u32 OwnerSlot;
    monster_kind MonsterKind;
    // NOTE(zoubir): multiplied with the sprite, 0 means untinted
    u32 Tint;
    float Hp;
    float MaxHp;
    // NOTE(zoubir): seconds until a monster can hit again
    float AttackCooldown;
    // NOTE(zoubir): monsters with no player in range stroll this way
    // until WanderTimer runs out, then pick a new direction (or rest)
    v2 WanderDirection;
    float WanderTimer;
    // NOTE(zoubir): the monster ability in progress, if any
    ability_phase AbilityPhase;
    u32 AbilityIndex;
    float AbilityTimer;
    float AbilityCooldowns[MAX_MONSTER_ABILITIES];
    // NOTE(zoubir): locked direction for charges, landing spots for
    // mortars and blinks
    v2 AbilityAim;
    v2 AbilityPoints[MAX_ABILITY_POINTS];
    u32 AbilityPointCount;
    bool32 AbilityHasHit;
    // NOTE(zoubir): seconds until the player can dash / shockwave again
    float DashCooldown;
    float ShockwaveCooldown;
    // NOTE(zoubir): seconds left on the shockwave ring effect
    float ShockwaveFlash;
    
    u32 UpdateID;
    
    bool32 IsPresent;

    //NOTE(zoubir): Tiled Object    
    u32 NumTilesX;
    u32 NumTilesY;
    u32 NumTilesZ;
    // 6 * 6


    u32 TileIndices[36];


    // Sword && fireball
    float TimeLeft;
};

// NOTE(zoubir): a dead player keeps its entity while waiting to respawn;
// it must not block, be hit, or be chased in the meantime
inline bool32
IsDeadPlayer(world_entity *Entity)
{
    bool32 Result = (Entity->Type == EntityType_Player && Entity->Hp <= 0.f);
    return Result;
}

internal bool32
CanOverlap(world_entity *Entity, world_entity *Region);
internal bool32
EntityOverlap(world_entity *Entity, world_entity *Region);
internal void
HandleOverlap(app_state *AppState, world *World, memory_arena *Arena,
              world_entity *Entity, world_entity *Region);

#endif
