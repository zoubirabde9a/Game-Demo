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

// NOTE(zoubir): simulation side. Moves State along the frames of the
// requested animation; switching direction restarts it. Needs no texture,
// so the server runs it too (attack and cast states wait on it).
internal void
AdvanceAnimation(animation_state *State, animation_set *Set,
                 animation_type Type, animation_direction Direction,
                 float DeltaTime, float SpeedRate)
{
    Assert(Type < AnimationType_Count);
    Assert(Direction < AnimationDirection_Count);
    if (State->LastAnimationDirection != Direction)
    {
        State->SlotIndex = 0;
    }
    State->LastAnimationDirection = Direction;
    State->CurrentType = Type;

    animation_slot *Animation = GetAnimation(Set, Type, Direction);
    // NOTE(zoubir): a set without this type/direction (flyers have no
    // up/down frames) simply does not advance
    if (Animation->IndicesCount == 0)
    {
        return;
    }

    u32 FrameTimeIndex = State->SlotIndex % Animation->IndicesCount;
    float SecondsPerFrame =
        SpeedRate * Animation->FramesTimeInSeconds[FrameTimeIndex];
    State->DeltaTime += DeltaTime;
    if (State->DeltaTime >= SecondsPerFrame)
    {
        State->DeltaTime -= SecondsPerFrame;
        State->SlotIndex++;
    }
}

// NOTE(zoubir): drawing side. Texture coordinates of the frame State is
// on, mirrored for reversed slots; all zero when the slot is empty.
internal v4
GetAnimationUvs(animation_state *State, animation_set *Set,
                u32 TextureWidth, u32 TextureHeight,
                u32 TileNumX, u32 TileNumY)
{
    v4 Result = {};
    animation_slot *Animation =
        GetAnimation(Set, State->CurrentType, State->LastAnimationDirection);
    if (Animation->IndicesCount == 0)
    {
        return Result;
    }

    u32 Index = Animation->FirstIndex +
        (State->SlotIndex % Animation->IndicesCount);
    Result = GetTextureUvsFromIndex(TextureWidth, TextureHeight,
                                    TileNumX, TileNumY, Index);
    if (Animation->Reversed)
    {
        float Tmp = Result.X;
        Result.X = Result.Z;
        Result.Z = Tmp;
    }
    return Result;
}

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
    // NOTE(zoubir): volumes are center + half size, so grow a min/max box
    v3 Min = Volumes[0].Offset - Volumes[0].HalfDims;
    v3 Max = Volumes[0].Offset + Volumes[0].HalfDims;
    for(u32 VolumeIndex = 1;
        VolumeIndex < VolumesCount;
        VolumeIndex++)
    {
        entity_collision_volume *Volume = &Volumes[VolumeIndex];
        Min = Minimum3(Min, Volume->Offset - Volume->HalfDims);
        Max = Maximum3(Max, Volume->Offset + Volume->HalfDims);
    }

    entity_collision_volume TotalVolume;
    TotalVolume.Offset = 0.5f * (Min + Max);
    TotalVolume.HalfDims = 0.5f * (Max - Min);
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
    Group->TotalVolume = GetTotalVolume(Group->Volumes,
                                        Group->VolumesCount);
    return Group;
}

// NOTE(zoubir): every hit goes through here so deaths are counted once
internal bool32
DamageEntity(app_state *AppState, world *World,
             world_entity *Target, float Damage, world_entity *Source)
{
    // NOTE(zoubir): a dead player waits for respawn with Hp <= 0, so
    // further hits that frame do not count as more kills
    if (!Target->IsPresent || Target->Hp <= 0.f)
    {
        return false;
    }

    Damage = ModifyIncomingDamage(Target, Source, Damage);
    Target->Hp -= Damage;
    if (Target->Hp > 0.f)
    {
        return false;
    }

    // NOTE(zoubir): credit goes to the player behind the hit, if any
    player_slot *Attacker = 0;
    if (Source && Source->Type == EntityType_Player)
    {
        Attacker = &AppState->Players[Source->PlayerIndex];
    }
    else if (Source && Source->HasOwner)
    {
        Attacker = &AppState->Players[Source->OwnerSlot];
    }

    if (Target->Type == EntityType_Monster)
    {
        RecordMonsterDeath(AppState, Target);
        RemoveEntity(World, Target);
        if (Attacker)
        {
            Attacker->MonsterKills++;
        }
    }
    else if (Target->Type == EntityType_Player)
    {
        player_slot *Victim = &AppState->Players[Target->PlayerIndex];
        Victim->Deaths++;
        Victim->RespawnTimer = PLAYER_RESPAWN_SECONDS;
        Target->Velocity = {};
        if (Attacker && Attacker != Victim)
        {
            Attacker->Kills++;
        }
    }
    return true;
}
