/* Replicas: while connected, the client's world shows what the server
   says instead of simulating. The arena (tiles, walls, trees, platforms)
   is built the same way on both sides and stays; every moving thing is a
   replica, a local entity that mirrors one entity of the newest snapshot,
   matched by the server's entity Id. Replicas are created when an Id
   appears, moved every snapshot, and removed when the Id disappears.

   RunWorldTick is the one call the frame makes: online it syncs replicas,
   offline it runs SimulateTick, and it switches the world between the two
   when the connection comes or goes. */

#define MAX_REPLICAS ArrayCount(((world *)0)->Entities)

struct replica_table
{
    bool32 Active;
    u32 LastAppliedTick;
    // NOTE(zoubir): indexed by server Id; local entity index + 1, 0 = none
    u32 LocalIndexPlusOne[MAX_REPLICAS];
    u8 Type[MAX_REPLICAS];
    u8 Variant[MAX_REPLICAS];
    u32 SeenTick[MAX_REPLICAS];
};

inline bool32
IsMovingEntityType(entity_type Type)
{
    bool32 Result = (Type == EntityType_Player ||
                     Type == EntityType_Monster ||
                     Type == EntityType_FireBall ||
                     Type == EntityType_Sword ||
                     Type == EntityType_Familiar ||
                     Type == EntityType_MonsterShot);
    return Result;
}

// NOTE(zoubir): leaves only the arena; used when switching between the
// local simulation and the server's world
internal void
ClearMovingEntities(app_state *AppState)
{
    world *World = &AppState->World;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (Entity->IsPresent && IsMovingEntityType(Entity->Type))
        {
            RemoveEntity(World, Entity);
        }
    }
}

// NOTE(zoubir): a monster shot as AddMonsterShot dresses it, without the
// owner and ability the server used to fire it
internal world_entity *
AddShotReplica(app_state *AppState, world *World, memory_arena *Arena,
               v3 Position, u32 ShotStyle)
{
    world_entity *Shot = AddEntity(AppState, World, Arena,
                                   EntityType_MonsterShot, Position,
                                   AppState->FireBallCollision);
    Shot->Dimensions = V2((float)SHOT_FRAME_SIZE, (float)SHOT_FRAME_SIZE);
    Shot->Texture = {AssetType_MonsterShot, ShotStyle};
    Shot->ShadowTexture = {AssetType_Shadow};
    if (AppState->Monsters && ShotStyle < ArrayCount(AppState->Monsters->ShotAnimationSets))
    {
        Shot->AnimationSet = &AppState->Monsters->ShotAnimationSets[ShotStyle];
    }
    return Shot;
}

// NOTE(zoubir): 0 for types the client has no look for
internal world_entity *
SpawnReplica(app_state *AppState, world *World, memory_arena *Arena,
             net_entity_state *State)
{
    v3 Position = V3(State->X, State->Y, State->Z);
    world_entity *Result = 0;
    switch ((entity_type)State->Type)
    {
        case EntityType_Player:
        {
            Result = AddPlayer(AppState, World, Arena, Position);
        } break;
        case EntityType_Monster:
        {
            if (State->Variant < MonsterKind_Count)
            {
                Result = AddMonster(AppState, World, Arena, Position,
                                    (monster_kind)State->Variant);
            }
        } break;
        case EntityType_FireBall:
        {
            Result = AddFireBall(AppState, World, Arena, 0, Position,
                                 V3(State->VelX, State->VelY, 0.f));
        } break;
        case EntityType_Sword:
        {
            Result = AddSword(AppState, World, Arena, Position, 0,
                              (animation_direction)State->Facing);
        } break;
        case EntityType_MonsterShot:
        {
            Result = AddShotReplica(AppState, World, Arena, Position,
                                    State->Variant);
        } break;
        default: break;
    }
    return Result;
}

// NOTE(zoubir): the replica for State, reused while the server keeps the
// same type and look under that Id, recreated when it changes
internal world_entity *
GetOrSpawnReplica(app_state *AppState, memory_arena *Arena,
                  replica_table *Table, net_entity_state *State)
{
    world *World = &AppState->World;
    u32 Id = State->Id;
    world_entity *Existing = 0;
    if (Table->LocalIndexPlusOne[Id])
    {
        Existing = &World->Entities[Table->LocalIndexPlusOne[Id] - 1];
        if (!Existing->IsPresent ||
            Table->Type[Id] != State->Type ||
            Table->Variant[Id] != State->Variant)
        {
            if (Existing->IsPresent)
            {
                RemoveEntity(World, Existing);
            }
            Existing = 0;
            Table->LocalIndexPlusOne[Id] = 0;
        }
    }

    if (!Existing)
    {
        Existing = SpawnReplica(AppState, World, Arena, State);
        if (Existing)
        {
            Table->LocalIndexPlusOne[Id] = Existing->ID + 1;
            Table->Type[Id] = State->Type;
            Table->Variant[Id] = State->Variant;
        }
    }
    return Existing;
}

internal void
ApplyStateToReplica(app_state *AppState, memory_arena *Arena,
                    world_entity *Replica, net_entity_state *State)
{
    v3 OldPosition = Replica->Position;
    Replica->Position = V3(State->X, State->Y, State->Z);
    Replica->Velocity = V3(State->VelX, State->VelY, 0.f);
    Replica->Hp = (float)State->Health;
    if (State->Facing < AnimationDirection_Count)
    {
        Replica->AnimationDirection = (animation_direction)State->Facing;
    }
    if (State->Animation < AnimationType_Count)
    {
        Replica->AnimationType = (animation_type)State->Animation;
    }
    // NOTE(zoubir): keeps the chunk lists right so RemoveEntity finds it
    CheckAndChangeEntityChunk(AppState, &AppState->World, Arena,
                              OldPosition, Replica);
}

// NOTE(zoubir): moves every replica to the newest snapshot (once per new
// server tick) and plays their animations locally every frame
internal void
SyncReplicas(app_state *AppState, memory_arena *Arena, replica_table *Table,
             net_snapshot *Snapshot, float DeltaTime)
{
    world *World = &AppState->World;
    if (!Table->Active)
    {
        ClearMovingEntities(AppState);
        *Table = {};
        Table->Active = true;
    }

    if (Snapshot->Tick != Table->LastAppliedTick)
    {
        Table->LastAppliedTick = Snapshot->Tick;
        for(u32 Index = 0; Index < Snapshot->Count; Index++)
        {
            net_entity_state *State = &Snapshot->Entities[Index];
            if (State->Id >= MAX_REPLICAS)
            {
                continue;
            }
            world_entity *Replica =
                GetOrSpawnReplica(AppState, Arena, Table, State);
            if (Replica)
            {
                ApplyStateToReplica(AppState, Arena, Replica, State);
                Table->SeenTick[State->Id] = Snapshot->Tick;
                // NOTE(zoubir): the server always sends our own player first
                if (Index == 0 && State->Type == EntityType_Player)
                {
                    player_slot *Local =
                        &AppState->Players[AppState->LocalPlayerIndex];
                    Local->Active = true;
                    Local->Entity = Replica;
                    Replica->PlayerIndex = AppState->LocalPlayerIndex;
                }
            }
        }

        for(u32 Id = 0; Id < MAX_REPLICAS; Id++)
        {
            if (Table->LocalIndexPlusOne[Id] &&
                Table->SeenTick[Id] != Snapshot->Tick)
            {
                world_entity *Gone =
                    &World->Entities[Table->LocalIndexPlusOne[Id] - 1];
                if (Gone->IsPresent)
                {
                    RemoveEntity(World, Gone);
                }
                Table->LocalIndexPlusOne[Id] = 0;
            }
        }
    }

    for(u32 Id = 0; Id < MAX_REPLICAS; Id++)
    {
        if (Table->LocalIndexPlusOne[Id])
        {
            world_entity *Replica =
                &World->Entities[Table->LocalIndexPlusOne[Id] - 1];
            if (Replica->IsPresent && Replica->AnimationSet)
            {
                AdvanceAnimation(&Replica->AnimationState,
                                 Replica->AnimationSet,
                                 Replica->AnimationType,
                                 Replica->AnimationDirection,
                                 DeltaTime, 1.f);
            }
        }
    }
}

// NOTE(zoubir): back to the local game after the connection ends: a fresh
// local player and familiar, and the monsters refilled
internal void
LeaveReplicaWorld(app_state *AppState, memory_arena *Arena,
                  replica_table *Table)
{
    ClearMovingEntities(AppState);
    *Table = {};
    world *World = &AppState->World;
    u32 Slot = AppState->LocalPlayerIndex;
    world_entity *Player = AddPlayerToSlot(AppState, World, Arena, Slot,
                                           PlayerSpawnPosition(Slot));
    AddFamiliar(AppState, World, Arena, Player);
    if (AppState->Monsters)
    {
        FillMonsterPopulation(AppState, World, Arena, AppState->Monsters);
    }
}
