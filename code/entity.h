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

internal bool32
CanOverlap(world_entity *Entity, world_entity *Region);
internal bool32
EntityOverlap(world_entity *Entity, world_entity *Region);
internal void
HandleOverlap(app_state *AppState, world *World, memory_arena *Arena,
              world_entity *Entity, world_entity *Region);

#endif
