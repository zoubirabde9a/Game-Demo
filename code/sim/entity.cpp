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

// NOTE(zoubir): the SpeedRate (seconds per frame multiplier) that keeps a
// walk cycle in step with the body's ground speed; 1 for anything else.
// Slowed to a crawl the feet step slowly, a dash's run-out steps fast.
internal float
MoveCycleRate(world_entity *Entity, animation_type Type)
{
    float Result = 1.f;
    animation_set *Set = Entity->AnimationSet;
    if (Type == AnimationType_Move && Set && Set->MoveSpeed > 0.f)
    {
        float Speed = Length(Entity->Velocity.XY);
        Result = Set->MoveSpeed / Maximum(Speed, 1.f);
        Result = Minimum(2.5f, Maximum(0.5f, Result));
    }
    return Result;
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

// NOTE(zoubir): only the trunk blocks. The canopy had a box of its own,
// 86 wide from 50 units up: walking under it was fine, but any jump near a
// tree hit that invisible wall of leaves
entity_collision_volume_group *
MakeGroundedTreeCollisionVolume(memory_arena *Arena)
{
    entity_collision_volume_group *Group =
        AllocateStruct(Arena, entity_collision_volume_group);
    Group->VolumesCount = 1;
    Group->Volumes =
        AllocateArray(Arena, Group->VolumesCount, entity_collision_volume);
    Group->Volumes[0].HalfDims = V3(18.f, 10.f, 63.f);
    Group->Volumes[0].Offset.Z = 63.f;
    Group->TotalVolume = GetTotalVolume(Group->Volumes,
                                        Group->VolumesCount);
    return Group;
}

// NOTE(zoubir): a player mid-dash or mid-blink (while its streak shows,
// DashFlash) cannot be hurt or shoved: dashing through an attack dodges
// it. Nor can one just back from the dead (SpawnShield)
inline bool32
IsDodging(world_entity *Entity)
{
    bool32 Result = Entity->Type == EntityType_Player &&
        (Entity->DashFlash > 0.f || Entity->SpawnShield > 0.f);
    return Result;
}

// NOTE(zoubir): high enough in a jump to clear ground hazards (bile, webs,
// embers); a jump spends about 0.4 of its 0.46 s above this
#define CLEARS_GROUND_HEIGHT 8.f

inline bool32
IsClearOfGround(world_entity *Entity)
{
    bool32 Result = Entity->Position.Z > CLEARS_GROUND_HEIGHT;
    return Result;
}

// NOTE(zoubir): a monster shot flies at chest height (14 units); a player
// whose feet are this far above it lets it pass underneath. A jump peaks
// at 36 and spends about 0.3 s of its 0.42 s high enough
#define CLEARS_SHOT_MARGIN 4.f

inline bool32
IsAboveShot(world_entity *Player, world_entity *Shot)
{
    bool32 Result = Player->Position.Z > Shot->Position.Z + CLEARS_SHOT_MARGIN;
    return Result;
}

// NOTE(zoubir): every hit goes through here so deaths are counted once
// NOTE(zoubir): a player's kill refunds cooldowns
// (player_abilities/movement_abilities.cpp, included later)
internal void RefundOnKill(world_entity *Player);

internal bool32
DamageEntity(app_state *AppState, world *World,
             world_entity *Target, float Damage, world_entity *Source)
{
    // NOTE(zoubir): a dead player waits for respawn with Hp <= 0, so
    // further hits that frame do not count as more kills
    if (!Target->IsPresent || Target->Hp <= 0.f || IsDodging(Target))
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

    // NOTE(zoubir): monsters and players alike burst as they die
    v3 Chest = Target->Position;
    Chest.Z += 14.f;
    EmitBurst(&AppState->Events, SimBurst_Death,
              Attacker ? (u8)(Attacker - AppState->Players) : SIM_NOBODY,
              Chest);

    if (Target->Type == EntityType_Monster)
    {
        RecordMonsterDeath(AppState, Target);
        RemoveEntity(World, Target);
        if (Attacker)
        {
            Attacker->MonsterKills++;
            if (Attacker->Entity)
            {
                RefundOnKill(Attacker->Entity);
            }
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
        u8 KillerMonster = (Source && Source->Type == EntityType_Monster) ?
            (u8)Source->MonsterKind : SIM_NOBODY;
        EmitKill(&AppState->Events,
                 (Attacker && Attacker != Victim) ?
                 (u8)(Attacker - AppState->Players) : SIM_NOBODY,
                 (u8)Target->PlayerIndex, KillerMonster);
    }
    return true;
}
