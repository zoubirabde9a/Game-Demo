/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: zoubir $
   ======================================================================== */
#include "entity.h"

inline animation_slot *
GetAnimation(animation_set *Set, animation_type Type,
             animation_direction Direction)
{
    animation_slot *Result =
        &Set->Animations[Type][Direction];
    return Result;
}

inline bool32
IsAnimationFinished(animation_set *Set,
                    animation_state *State,
                    animation_type Type,
                    animation_direction Direction)
{
    animation_slot *Animation =
        GetAnimation(Set, Type, Direction);
    bool32 Result = (State->SlotIndex >= Animation->IndicesCount);
    return Result;
}

internal animation_slot *
AddAnimation(animation_set *Set,
             memory_arena *Arena,
             animation_type Type,
             animation_direction Direction,
             u32 FirstIndex,
             u32 IndicesCount,
             float SecondsPerFrame,
             bool32 Reversed = 0)
{
    
    animation_slot *Animation =
        GetAnimation(Set, Type, Direction);
    Animation->FirstIndex = FirstIndex;
    Animation->IndicesCount = IndicesCount;
    Animation->FramesTimeInSeconds = AllocateArray(Arena,
                                               IndicesCount,
                                               float);
    for(u32 CurrentIndex = 0;
        CurrentIndex < IndicesCount;
        CurrentIndex++)
    {
        Animation->FramesTimeInSeconds[CurrentIndex] =
            SecondsPerFrame;
    }
    Animation->Reversed = Reversed;

    return Animation;
}

inline v4
GetTextureUvsFromIndex(u32 TextureWidth, u32 TextureHeight,
                          u32 TileNumX, u32 TileNumY,
                          u32 Index)
{

    v4 Result;
    
    Assert(TileNumX * (TextureWidth / TileNumX) == TextureWidth);
    Assert(TileNumY * (TextureHeight / TileNumY) == TextureHeight);
    
    float UvWidth = 1.f / TileNumX;
    float UvHeight = 1.f / TileNumY;
    
    Result.X = ((UvWidth) * (Index % TileNumX));
    Result.Y = ((UvHeight) * ((Index / TileNumX) % TileNumY));
    Result.Z = Result.X + UvWidth;
    Result.W = Result.Y + UvHeight;
    
    return Result;
}

internal v4
DoAnimation(animation_state *State,
            u32 TextureWidth, u32 TextureHeight,
            u32 TileNumX, u32 TileNumY,
            float DeltaTime,
            float SpeedRate, animation_slot Animation)
{
    v4 Result;

    u32 Index = Animation.FirstIndex +
        (State->SlotIndex % Animation.IndicesCount);
    Result = GetTextureUvsFromIndex(TextureWidth, TextureHeight,
                                    TileNumX, TileNumY, Index);

    u32 FrameTimeIndex =
        State->SlotIndex % Animation.IndicesCount;
    float SecondsPerFrame = SpeedRate * Animation.FramesTimeInSeconds[FrameTimeIndex];
    State->DeltaTime += DeltaTime;
    if (State->DeltaTime >= SecondsPerFrame)
    {
        State->DeltaTime -= SecondsPerFrame;
        State->SlotIndex++;
    }

    if (Animation.Reversed)
    {
        float Tmp = Result.X;
        Result.X = Result.Z;
        Result.Z = Tmp;
    }
    return Result;
}

inline v4
DoAnimation(animation_state *State,
            u32 TextureWidth, u32 TextureHeight,
            u32 TileNumX, u32 TileNumY,
            float DeltaTime,
            float SpeedRate, animation_set *Set,
            animation_type AnimationType,
            animation_direction AnimationDirection)
{
    Assert(AnimationType < AnimationType_Count);
    Assert(AnimationDirection < AnimationDirection_Count);
    Assert(State);
    v4 Result;
    
    animation_slot *Animation =
        GetAnimation(Set, AnimationType, AnimationDirection);
    
    // NOTE(zoubir): Animation Does Not Exit
    Assert(Animation->IndicesCount > 0);

    #if 0
    if (State->LastAnimationDirection != AnimationDirection)
    {
        State->DeltaTime = 0.f;
        //TODO(zoubir): do we want this to be 1 instead of 0
        State->SlotIndex = 0;
        u32 Index = Animation->FirstIndex;
        Result = GetTextureUvsFromIndex(TextureWidth, TextureHeight,
                                        TileNumX, TileNumY, Index);
    }
    else
    {
        Result = DoAnimation(State, TextureWidth, TextureHeight,
                    TileNumX, TileNumY, DeltaTime, SpeedRate,
                    *Animation);
                    
    }
    #endif
    
    if (State->LastAnimationDirection != AnimationDirection)
    {
        State->SlotIndex = 0;
    }    
    Result = DoAnimation(State, TextureWidth, TextureHeight,
                         TileNumX, TileNumY, DeltaTime, SpeedRate,
                         *Animation);
    
    State->LastAnimationDirection = AnimationDirection;
    return Result;
};

inline bool32
IsSet(world_entity *Entity, u32 Flag)
{
    bool32 Result = Entity->Flags & Flag;
    return Result;
}

inline void
AddFlags(world_entity *Entity, u32 Flags)
{
    Entity->Flags |= Flags;
}

inline void
ClearFlag(world_entity *Entity, u32 Flag)
{
    Entity->Flags &= ~Flag;
}

inline entity_collision_volume_group *
MakeSimpleCollisionVolume(memory_arena *Arena, v3 HalfDims)
{
    entity_collision_volume_group *Group =
        AllocateStruct(Arena, entity_collision_volume_group);
    Group->VolumesCount = 1;
    Group->Volumes =
        AllocateArray(Arena, 1, entity_collision_volume);
    Group->Volumes[0].HalfDims = HalfDims;
    Group->TotalVolume = Group->Volumes[0];
    
    Assert(Group->Volumes[0].Offset.X == 0 &&
           Group->Volumes[0].Offset.Y == 0 &&
           Group->Volumes[0].Offset.Z == 0);
       
    return Group;
}

inline entity_collision_volume_group *
MakeSimpleGroundedCollisionVolume(memory_arena *Arena, v3 HalfDims)
{
    entity_collision_volume_group *Group =
        AllocateStruct(Arena, entity_collision_volume_group);
    Group->VolumesCount = 1;
    Group->Volumes =
        AllocateArray(Arena, 1, entity_collision_volume);
    Group->Volumes[0].Offset.Z = HalfDims.Z;
    Group->Volumes[0].HalfDims = HalfDims;
    Group->TotalVolume = Group->Volumes[0];
    
    Assert(Group->Volumes[0].Offset.X == 0 &&
           Group->Volumes[0].Offset.Y == 0);
       
    return Group;
}

inline entity_collision_volume
GetTotalVolume(entity_collision_volume *Volumes,
               u32 VolumesCount)
{
    Assert(VolumesCount > 0);
    entity_collision_volume TotalVolume = Volumes[0];

    for(u32 VolumeIndex = 0;
        VolumeIndex < VolumesCount;
        VolumeIndex++)
    {
        entity_collision_volume *Volume =
            &Volumes[VolumeIndex];
        v3 Min = Minimum3(TotalVolume.Offset, Volume->Offset);
        v3 Max = Maximum3(TotalVolume.Offset + TotalVolume.HalfDims,
                Volume->Offset + Volume->HalfDims);
        TotalVolume.Offset = Min;
        TotalVolume.HalfDims = Max - Min;
    }
    
    return TotalVolume;
}

entity_collision_volume_group *
MakeGroundedTreeCollisionVolume(memory_arena *Arena)
{
    entity_collision_volume_group *Group =
        AllocateStruct(Arena, entity_collision_volume_group);

    float LowestHalfZ = 63;
    
    Group->VolumesCount = 2;
    Group->Volumes =
        AllocateArray(Arena, Group->VolumesCount, entity_collision_volume);
    Group->Volumes[0].HalfDims = V3(18.f, 10.f, 63.f);
    Group->Volumes[0].Offset.Z += LowestHalfZ;
    
    Group->Volumes[1].HalfDims = V3(43.f, 10.f, 28.f);
    Group->Volumes[1].Offset.Z = 15;
    Group->Volumes[1].Offset.Z += LowestHalfZ;
    //TODO(zoubir): this is not the total volume
    // make a function that returns one
    Group->TotalVolume = GetTotalVolume(Group->Volumes,
                                        Group->VolumesCount);
    return Group;
}

internal bool32
HandleCollision(world *World,
                world_entity *A,
                world_entity *B)
{
    bool32 Result = true;

    if (A->Type == EntityType_FireBall &&
        B->Type == EntityType_Monster)
    {
        B->Hp -= 25;
        if (B->Hp <= 0.f)
        {
            RemoveEntity(World, B);
        }
        Result = false;
    }
    
    if (B->Type == EntityType_FireBall &&
        A->Type == EntityType_Monster)
    {
        A->Hp -= 25;
        if (A->Hp <= 0.f)
        {
            RemoveEntity(World, A);            
        }
        Result = false;        
    }
    return Result;
}

struct test_wall
{
    float X;
    v3 Rel;
    v3 Delta;
    float MinY;
    float MaxY;
    float MinZ;
    float MaxZ;
    v3 Normal;
};

internal bool32
TestWall(float WallX, float RelX, float RelY, float RelZ,
         float PlayerDeltaX, float PlayerDeltaY, float PlayerDeltaZ,
         float *tMin,
         float MinY, float MaxY,
         float MinZ, float MaxZ)
{
    bool32 Hit = false;
    float tEpsilon = 0.01f;
    if(PlayerDeltaX != 0.0f)
    {
        float tResult = (WallX - RelX) / PlayerDeltaX;
        float Y = RelY + tResult*PlayerDeltaY;
        float Z = RelZ + tResult*PlayerDeltaZ;
        if((tResult >= 0.0f) && (*tMin > tResult))
        {
            if((Y >= MinY) && (Y <= MaxY) &&
               (Z >= MinZ) && (Z <= MaxZ))
            {
                *tMin = Maximum(0.0f, tResult - tEpsilon);
                Hit = true;
            }
        }
    }
    
    return Hit;
}

internal bool32
CanOverlap(world_entity *Entity, world_entity *Region)
{
    bool32 Result = false;
    if (Entity->Type == EntityType_Player &&
        Region->Type == EntityType_Familiar)
    {
        Result = true;
    }
    
    if (Entity->Type == EntityType_Sword &&
        Region->Type == EntityType_Monster)
    {
        Result = true;
    }
    
    return Result;
}

internal bool32
EntityOverlap(world_entity *Entity, world_entity *Region)
{
    bool32 Result = false;
    
    for(u32 VolumeIndex = 0;
        VolumeIndex < Entity->Collision->VolumesCount;
        VolumeIndex++)
    {
        entity_collision_volume *Volume =
            &Entity->Collision->Volumes[VolumeIndex];
        
        rectangle3 EntityRect =
            RectCenterHalfDims(Entity->Position +
                               Volume->Offset, Volume->HalfDims);
                                                                     
        for(u32 RegionVolumeIndex = 0;
            RegionVolumeIndex < Region->Collision->VolumesCount;
            RegionVolumeIndex++)
        {
            entity_collision_volume *RegionVolume =
                &Region->Collision->Volumes[RegionVolumeIndex];
            rectangle3 RegionRect =
                RectCenterHalfDims(Region->Position +
                                   RegionVolume->Offset, RegionVolume->HalfDims);
            if (RectanglesIntersect(EntityRect, RegionRect))
            {
                Result = true;
            }
        }
    }
    return Result;
}

internal void
HandleOverlap(app_state *AppState, world *World, memory_arena *Arena,
              world_entity *Entity, world_entity *Region)
{
        
    if (Entity->Type == EntityType_Sword)
    {
        Region->Hp -= 25;
        if (Region->Hp <= 0.f)
        {
            RemoveEntity(World, Region);
        }
        else
        {
            AddCollisionRule(AppState, Arena,
                             Entity->ID, Region->ID,
                             false);
        }
    }
}

internal void
MoveEntity(world_entity *Entity, world *World,
           memory_arena *Arena,
           app_input *Input, app_state *AppState,
           v3 DDEntity, float *MaxDistance)
{
    // NOTE(zoubir): jumping code
    
//    DDEntityZ -= (10.f * Entity->VelocityZ);
    
    // NOTE(zoubir): New Position
    v3 EntityDelta = 0.5f * DDEntity * Square(Input->DeltaTime) +
        Entity->Velocity * Input->DeltaTime;

    float SmallNumber = 0.01f;


    #if 0
    for(u32 CoordIndex = 0;
        CoordIndex < ArrayCount(EntityDelta.Data);
        CoordIndex++)
    {
        float *Coord = &EntityDelta.Data[CoordIndex];
        if (*Coord > 0)
        {
            if (*Coord < SmallNumber)
            {
                *Coord = 0.f;
            }
        }
        else
        {
            if (*Coord > -SmallNumber)
            {
                *Coord = 0.f;
            }
        }
    }
    #endif


    Entity->Velocity = DDEntity * Input->DeltaTime +
        Entity->Velocity;

    int NumIterations = 4;
    Assert(NumIterations > 0);
    u32 MinChunkX;
    u32 MinChunkY;
    u32 MinChunkZ;
    u32 MaxChunkX;
    u32 MaxChunkY;
    u32 MaxChunkZ;        
        
    for(int Iteration = 0;
        Iteration < NumIterations;
        Iteration++)
    {
        //TODO(zoubir): optimise this shit show
        
        v3 EntityOldPosition = Entity->Position;
        v3 EntityNewPosition = EntityOldPosition + EntityDelta;
        EntityNewPosition.Z = Maximum(0.f, EntityNewPosition.Z);

        float Distance = Length(EntityDelta);
        float EpsilonDistance = 0.000001f;
        if (Distance <= EpsilonDistance)
        {
            // Nothing to do here
            break;
        }

        if (Distance > *MaxDistance)
        {
            
            float Ratio = *MaxDistance / Distance;
            EntityDelta = Ratio * EntityDelta;
            *MaxDistance = 0.f;
        }
        else
        {
            *MaxDistance -= Distance;
        }

        u32 CollisionTileWidth = World->CollisionWidth;
        u32 CollisionTileHeight = World->CollisionHeight;
        u32 CollisionTileDepth = World->CollisionDepth;
    
        cannonical_position NewCollisionP =
            CannonicalizePosition(EntityNewPosition,
                                  World->CollisionWidth,
                                  World->CollisionHeight,
                                  World->CollisionDepth);
        cannonical_position OldCollisionP =
            CannonicalizePosition(EntityOldPosition,
                                  World->CollisionWidth,
                                  World->CollisionHeight,
                                  World->CollisionDepth);
    
    
        entity_collision_volume *EntityCollisionTotal =
            &Entity->Collision->TotalVolume;
        
        v3 MinPos = Minimum3(EntityOldPosition, EntityNewPosition);
        v3 MaxPos = Maximum3(EntityOldPosition, EntityNewPosition);
        MinPos += EntityCollisionTotal->Offset;
        MaxPos += EntityCollisionTotal->Offset;
        
        MinPos -= EntityCollisionTotal->HalfDims;
        MaxPos += EntityCollisionTotal->HalfDims;        
        rectangle3 Box = RectMinMax(MinPos,
                                    MaxPos);
        GetChunksFromBox(World, Box,
                         &MinChunkX, &MinChunkY, &MinChunkZ,
                         &MaxChunkX, &MaxChunkY, &MaxChunkZ);

#if 0
        u32 MinTileX = Minimum(OldCollisionP.TileX,
                               NewCollisionP.TileX);
        u32 MinTileY = Minimum(OldCollisionP.TileY,
                               NewCollisionP.TileY);
        u32 MinTileZ = Minimum(OldCollisionP.TileZ,
                               NewCollisionP.TileZ);
        
        u32 MaxTileX = Maximum(OldCollisionP.TileX,
                               NewCollisionP.TileX);
        u32 MaxTileY = Maximum(OldCollisionP.TileY,
                               NewCollisionP.TileY);
        u32 MaxTileZ = Maximum(OldCollisionP.TileZ,
                               NewCollisionP.TileZ);
        
        u32 EntityTileWidth = CeilFloatToUInt32(Entity->CollisionHalfDims.X /
                                                CollisionTileWidth);
        u32 EntityTileHeight = CeilFloatToUInt32(Entity->CollisionHalfDims.Y /
                                                 CollisionTileHeight);    
        u32 EntityTileDepth = CeilFloatToUInt32(Entity->CollisionHalfDims.Z /
                                                CollisionTileDepth);
        MinTileX -= EntityTileWidth;
        MinTileY -= EntityTileHeight;
        MinTileZ -= EntityTileDepth;
        
        MaxTileX += EntityTileWidth;
        MaxTileY += EntityTileHeight;
        MaxTileZ += EntityTileDepth;
#endif
        world_entity *CollidedEntity = 0;
        v3 WallNormal = {};
        float tMin = 1.f;

        //TODO(zoubir): make this spot collision
        // with different objects
        // and do something about it
        for(u32 ChunkY = MinChunkY;
            ChunkY <= MaxChunkY;
            ChunkY++)
        {
            for(u32 ChunkX = MinChunkX;
                ChunkX <= MaxChunkX;
                ChunkX++)
            {
                for(u32 ChunkZ = MinChunkZ;
                    ChunkZ <= MaxChunkZ;
                    ChunkZ++)
                {
                    world_chunk *ThisChunk = GetChunk(World, ChunkX,
                                                      ChunkY, ChunkZ);
                    for (world_entity_chunk *EntityChunk = &ThisChunk->FirstEntityChunk;
                         EntityChunk;
                         EntityChunk = EntityChunk->Next)
                    {
                        for(u32 EntityIndex = 0;
                            EntityIndex < EntityChunk->EntityCount;
                            EntityIndex++)
                        {
                            world_entity *ThisEntity = EntityChunk->Entities[EntityIndex];
                            if ((ThisEntity != Entity) &&
                                CanCollide(AppState, Entity, ThisEntity) &&
                                CanCollide(AppState, Entity->Type,
                                           ThisEntity->Type))
                            {
                                
                                for(u32 VolumeIndex = 0;
                                    VolumeIndex < Entity->Collision->VolumesCount;
                                    VolumeIndex++)
                                {
                                    entity_collision_volume *Volume =
                                        Entity->Collision->Volumes +
                                        VolumeIndex;
                                    for(u32 TestVolumeIndex = 0;
                                        TestVolumeIndex < ThisEntity->Collision->VolumesCount;
                                        TestVolumeIndex++)
                                    {
                                        entity_collision_volume *TestVolume =
                                            ThisEntity->Collision->Volumes +
                                            TestVolumeIndex;
                                        v3 MinkowskiDiameter = Volume->HalfDims
                                            + TestVolume->HalfDims;
                                        v3 MinCorner = -MinkowskiDiameter;
                                        v3 MaxCorner = MinkowskiDiameter;
                                        
                                        v3 Rel = (EntityOldPosition + Volume->Offset) -
                                            (ThisEntity->Position + TestVolume->Offset);

                                        test_wall Walls[] =
                                            {
                                                {MinCorner.X, Rel.X, Rel.Y, Rel.Z, EntityDelta.X, EntityDelta.Y, EntityDelta.Z, MinCorner.Y, MaxCorner.Y, MinCorner.Z, MaxCorner.Z, {-1, 0, 0}},
                                                {MaxCorner.X, Rel.X, Rel.Y, Rel.Z, EntityDelta.X, EntityDelta.Y, EntityDelta.Z, MinCorner.Y, MaxCorner.Y, MinCorner.Z, MaxCorner.Z, {1, 0, 0}},
                                                {MinCorner.Y, Rel.Y, Rel.X, Rel.Z, EntityDelta.Y, EntityDelta.X, EntityDelta.Z, MinCorner.X, MaxCorner.X, MinCorner.Z, MaxCorner.Z, {0, 1, 0}},
                                                {MaxCorner.Y, Rel.Y, Rel.X, Rel.Z, EntityDelta.Y, EntityDelta.X, EntityDelta.Z, MinCorner.X, MaxCorner.X, MinCorner.Z, MaxCorner.Z, {0, -1, 0}},
                                                {MinCorner.Z, Rel.Z, Rel.Y, Rel.X, EntityDelta.Z, EntityDelta.Y, EntityDelta.X, MinCorner.Y, MaxCorner.Y, MinCorner.X, MaxCorner.X, {0, 0, -1}},
                                                {MaxCorner.Z, Rel.Z, Rel.Y, Rel.X, EntityDelta.Z, EntityDelta.Y, EntityDelta.X, MinCorner.Y, MaxCorner.Y, MinCorner.X, MaxCorner.X, {0, 0, 1}},
                                                {MinCorner.Z, Rel.Z, Rel.X, Rel.Y, EntityDelta.Z, EntityDelta.X, EntityDelta.Y, MinCorner.X, MaxCorner.X, MinCorner.Y, MaxCorner.Y, {0, 0, 1}},
                                                {MaxCorner.Z, Rel.Z, Rel.X, Rel.Y, EntityDelta.Z, EntityDelta.X, EntityDelta.Y, MinCorner.X, MaxCorner.X, MinCorner.Y, MaxCorner.Y, {0, 0, -1}}
                                            };
                                        
                                        for(u32 WallIndex = 0;
                                            WallIndex < ArrayCount(Walls);
                                            WallIndex++)
                                        {
                                            test_wall *Wall = &Walls[WallIndex];
                                            if (TestWall(Wall->X,
                                                         Wall->Rel.X, Wall->Rel.Y, Wall->Rel.Z,
                                                         Wall->Delta.X, Wall->Delta.Y, Wall->Delta.Z,
                                                         &tMin,
                                                         Wall->MinY, Wall->MaxY,
                                                         Wall->MinZ, Wall->MaxZ))
                                            {
                                                WallNormal = Wall->Normal;
                                                CollidedEntity = ThisEntity;                                                
                                            }
                                        }
                                    }                    
                                }                    
                            }
                        }
                    }
                }
            }
        }

        v3 AllowedDelta = tMin * EntityDelta;
        Entity->Position = EntityOldPosition + AllowedDelta;
        
        // NOTE(zoubir): Cannot Go Lower Than The Ground Level
        if (Entity->Position.Z < 0.f)
        {
            Entity->Position.Z = 0.f;
            Entity->Velocity.Z = 0.f;
        }

        // TODO(zoubir): Do This Once Per MoveEntityCall RatherThan
        // Every Iteration        
        CheckAndChangeEntityChunk(AppState,
                                  World, Arena,
                                  EntityOldPosition, Entity);

        if (CollidedEntity)
        {
            bool32 StopsOnCollision =
                HandleCollision(World, Entity,
                                CollidedEntity);
            if (StopsOnCollision)
            {
                Entity->Velocity = Entity->Velocity - 1.f * DotProduct(Entity->Velocity, WallNormal) * WallNormal;
                v3 DeltaLeft = EntityDelta - AllowedDelta;
                EntityDelta = DeltaLeft - 1.f * DotProduct(DeltaLeft, WallNormal) * WallNormal;
            }
            else
            {
                AddCollisionRule(AppState, Arena,
                                 Entity->ID, CollidedEntity->ID,
                                 false);
            }
        }
        else
        {
            break;
        }

        for(u32 ChunkZ = MinChunkZ;
            ChunkZ < MaxChunkZ;
            ChunkZ++)
        {        
            for(u32 ChunkY = MinChunkY;
                ChunkY < MaxChunkY;
                ChunkY++)
            {        
                for(u32 ChunkX = MinChunkX;
                    ChunkX < MaxChunkX;
                    ChunkX++)
                {        
                    world_chunk *CurrentChunk = GetChunk(World, ChunkX, ChunkY, ChunkZ);
                    CheckEntityOverlapInChunk(AppState, World, Arena,
                                              Entity, &CurrentChunk->FirstEntityChunk);
                }
            }
        }
        
    }

    // NOTE(zoubir): New Velocity

    
    // Gravity / friction
//    AppState->DEntity *= 1.f;
    //TODO(zoubir): fix this false velocity
#if 0
    v2 MaxVelocity = {300, 300};
    if (AppState->DEntity.X > MaxVelocity.X)
    {
        AppState->DEntity.X = MaxVelocity.X;
    }
    if (AppState->DEntity.X < -MaxVelocity.X)
    {
        AppState->DEntity.X = -MaxVelocity.X;
    }
    if (AppState->DEntity.Y > MaxVelocity.Y)
    {
        AppState->DEntity.Y = MaxVelocity.Y;
    }
    if (AppState->DEntity.Y < -MaxVelocity.Y)
    {
        AppState->DEntity.Y = -MaxVelocity.Y;
    }
#endif

    
#if 0
// TODO(zoubir): Spatial partition here!
for(u32 EntityIndex = 0;
    EntityIndex < World->EntityCount;
    EntityIndex++)
{        
    world_entity *TestEntity = &World->Entities[EntityIndex];
    if (Entity != TestEntity &&
        CanOverlap(Entity, TestEntity) &&
        EntityOverlap(Entity, TestEntity))
    {
        HandleOverlap(AppState, World, Arena,
                      Entity, TestEntity);
    }                    
}
#endif
float NearestDistance = 100000.f;
entity_collision_volume *NearestVolume = 0;
world_entity *NearestEntity = 0;
// NOTE(zoubir): For Shadow And Ground Position
// TODO(zoubir): Spatial partition here!
for(u32 EntityIndex = 0;
    EntityIndex < World->EntityCount;
    EntityIndex++)
{
    world_entity *TestEntity = &World->Entities[EntityIndex];
    if (TestEntity != Entity &&
        CanCollide(AppState, Entity->Type, TestEntity->Type) &&
        CanCollide(AppState, Entity, TestEntity))
    {
            
        for(u32 VolumeIndex = 0;
            VolumeIndex < Entity->Collision->VolumesCount;
            VolumeIndex++)
        {                
            entity_collision_volume *Volume =
                &Entity->Collision->Volumes[VolumeIndex];
                
            rectangle2 EntityRect =
                RectCenterHalfDims(Entity->Position.XY + Volume->Offset.XY,
                                   Volume->HalfDims.XY);
            for(u32 TestVolumeIndex = 0;
                TestVolumeIndex < TestEntity->Collision->VolumesCount;
                TestVolumeIndex++)
            {                    
                entity_collision_volume *TestVolume =
                    &TestEntity->Collision->Volumes[TestVolumeIndex];
                    
                rectangle2 TestEntityRect =
                    RectCenterHalfDims(TestEntity->Position.XY +
                                       TestVolume->Offset.XY,
                                       TestVolume->HalfDims.XY);
                if (RectanglesIntersect(EntityRect, TestEntityRect))
                {
                    float Distance =
                        (Entity->Position.Z -
                         (TestEntity->Position.Z +
                          TestVolume->Offset.Z +
                          TestVolume->HalfDims.Z));
                    Assert(Distance < 100000.f);
                    if (Distance > 0.f &&
                        Distance < NearestDistance)
                    {
                        NearestDistance = Distance;
                        NearestVolume = TestVolume;
                        NearestEntity = TestEntity;
                    }
                }
            }
        }
    }
}

if (NearestVolume)
{
    Entity->GroundZ = NearestEntity->Position.Z +
        NearestVolume->Offset.Z + NearestVolume->HalfDims.Z;        
}
else
{
    Entity->GroundZ = 0.f;
}
}
    


