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
    // NOTE(zoubir): the ground speed the walk cycle was drawn for; the
    // cycle runs faster or slower with the real speed so the feet do not
    // slide (MoveCycleRate). 0 keeps it at its drawn pace.
    float MoveSpeed;
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
    // NOTE(zoubir): a monster ability's projectile, sim/monster_abilities.cpp
    EntityType_MonsterShot,
    // NOTE(zoubir): a lingering patch of bile, web or embers
    EntityType_MonsterHazard,
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

// NOTE(zoubir): walking accelerations of monsters and other units (an
// Acceleration; the player's walk is in player_stats.cpp) are tuned per
// 1/60 s step; the push is scaled by this, not by the frame time.
// Scaling by the frame time made speed grow with it: the 30 fps client
// walked twice as fast as the 60 Hz server, so online prediction ran
// ahead and was pulled back at every snapshot
#define ACCELERATION_STEP (1.f / 60.f)

// NOTE(zoubir): a swing hits what is within SWORD_REACH of the swinger and
// within SWORD_HALF_ANGLE (about 70 degrees) of the aim, at any angle; the
// sword entity sits SWORD_OFFSET toward the aim. The client's swing arc is
// drawn from the same numbers
#define SWORD_REACH 46.f
#define SWORD_HALF_ANGLE 1.22f
#define SWORD_OFFSET 16.f

// NOTE(zoubir): monster kinds, abilities and status effects
#include "monster_types.h"

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
    float Hp;
    float MaxHp;
    // NOTE(zoubir): seconds left on each status effect, and the clock for
    // their damage ticks
    float StatusTimers[StatusEffect_Count];
    float StatusTickTimer;
    // NOTE(zoubir): the player slot + 1 that last threw this unit (0 for
    // none), so kills by its impacts are theirs (sim/impacts.cpp)
    u32 ThrownBySlot;
    // NOTE(zoubir): hit-pause: seconds left frozen after a solid hit while
    // above 0, the grace before the next while below (sim/hit.cpp)
    float HitStop;
    // NOTE(zoubir): the last hit (sim/hit.cpp), for clients to draw: the
    // way it threw this unit in radians, whether it lifted it, and the
    // player slot + 1 who landed it (0 for a monster). All of it holds
    // while HitFresh, the seconds left since the hit, is above 0
    float HitAngle;
    bool32 HitThrown;
    u32 HitBySlot;
    float HitFresh;
    // NOTE(zoubir): set each tick from the ground underfoot
    // (sim/terrain_effects.cpp); 0 means 1
    float GroundSpeedScale;
    float GroundFriction;
    // NOTE(zoubir): monster-only state (kind, abilities, elites, summons)
    // lives in its own file so new monster features do not edit this one
#include "monster_fields.inc"
    // NOTE(zoubir): player-only state (ability cooldowns)
#include "player_fields.inc"
    // NOTE(zoubir): which player_slot owns this player entity
    u32 PlayerIndex;
    // NOTE(zoubir): swords and fireballs remember the slot that made them,
    // so they never hit it and its kills are credited to it
    bool32 HasOwner;
    u32 OwnerSlot;
    
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
// NOTE(zoubir): in sim/monster_kinds.cpp; queues the monster's death
// effect (split, ...) for the population to run next tick
internal void
RecordMonsterDeath(app_state *AppState, world_entity *Monster);
// NOTE(zoubir): in sim/monster_abilities.cpp; shells and other damage
// reductions, applied to every hit before it takes health
internal float
ModifyIncomingDamage(world_entity *Target, world_entity *Source, float Damage);
internal bool32
EntityOverlap(world_entity *Entity, world_entity *Region);
internal void
HandleOverlap(app_state *AppState, world *World, memory_arena *Arena,
              world_entity *Entity, world_entity *Region);

#endif
