/* Replicas: while connected, the client's world shows what the server
   says instead of simulating. The arena (tiles, walls, trees, platforms)
   is built the same way on both sides and stays; every moving thing is a
   replica, a local entity that mirrors one entity of the newest snapshot,
   matched by the server's entity Id. Replicas are created when an Id
   appears, moved every snapshot, and removed when the Id disappears.

   Between snapshots replicas glide rather than jump
   (replica_smoothing.cpp). How a replica is built for each entity type
   is replicas/spawn.cpp; how a snapshot's fields land on it is
   replicas/apply.cpp. This file is the table and the flow.

   RunWorldTick is the one call the frame makes: online it syncs replicas,
   offline it runs SimulateTick, and it switches the world between the two
   when the connection comes or goes. */

struct replica_table
{
    bool32 Active;
    u32 LastAppliedTick;
    // NOTE(zoubir): indexed by server Id; local entity index + 1, 0 = none
    u32 LocalIndexPlusOne[MAX_REPLICAS];
    u8 Type[MAX_REPLICAS];
    u16 Look[MAX_REPLICAS]; // see ReplicaLook
    u32 SeenTick[MAX_REPLICAS];
    // NOTE(zoubir): gliding between snapshots, client/replica_smoothing.cpp
    replica_smoothing Smoothing;
};

inline bool32
IsMovingEntityType(entity_type Type)
{
    bool32 Result = (Type == EntityType_Player ||
                     Type == EntityType_Monster ||
                     Type == EntityType_FireBall ||
                     Type == EntityType_Sword ||
                     Type == EntityType_Familiar ||
                     Type == EntityType_MonsterShot ||
                     Type == EntityType_MonsterHazard);
    return Result;
}

// NOTE(zoubir): what must match for a replica to be reused under the same
// Id. Shots and hazards are drawn from their ability, so it counts for
// them; a monster's current ability changes while it fights, so not there.
inline u16
ReplicaLook(net_entity_state *State)
{
    u16 Result = State->Variant;
    if (State->Type == EntityType_MonsterShot ||
        State->Type == EntityType_MonsterHazard)
    {
        Result |= (u16)(State->Ability << 8);
    }
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

// NOTE(zoubir): building a local entity for a server entity, by type
#include "replicas/spawn.cpp"

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
            Table->Look[Id] != ReplicaLook(State))
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
            Table->Look[Id] = ReplicaLook(State);
        }
    }
    return Existing;
}

// NOTE(zoubir): copying a snapshot's fields onto the replicas
#include "replicas/apply.cpp"

// NOTE(zoubir): moves every replica to the newest snapshot (once per new
// server tick) and plays their animations locally every frame. LocalSlot
// is the slot the server gave this client.
internal void
SyncReplicas(app_state *AppState, memory_arena *Arena, replica_table *Table,
             net_snapshot *Snapshot, float DeltaTime, u32 LocalSlot)
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
        AppState->LocalPlayerIndex = LocalSlot;
        BeginSmoothedSnapshot(&Table->Smoothing);
        for(u32 Index = 0; Index < Snapshot->Count; Index++)
        {
            net_entity_state *State = &Snapshot->Entities[Index];
            if (State->Id >= MAX_REPLICAS)
            {
                continue;
            }
            u32 LocalBefore = Table->LocalIndexPlusOne[State->Id];
            world_entity *Replica =
                GetOrSpawnReplica(AppState, Arena, Table, State);
            if (Replica)
            {
                // NOTE(zoubir): a reused replica glides from where it is
                // drawn; the local player is left to prediction
                v3 Drawn = Replica->Position;
                bool32 Reused = (LocalBefore == Replica->ID + 1);
                bool32 IsLocalPlayer = (State->Type == EntityType_Player &&
                                        State->Variant == LocalSlot);
                bool32 WasDead = Reused && IsDeadPlayer(Replica);
                ApplyStateToReplica(AppState, Arena, Replica, State);
                SetSmoothingTarget(AppState, Arena, &Table->Smoothing,
                                   State->Id, Replica, Drawn,
                                   Reused && !IsLocalPlayer);
                Table->SeenTick[State->Id] = Snapshot->Tick;
                // NOTE(zoubir): a player's Variant is its slot on the server
                if (State->Type == EntityType_Player &&
                    State->Variant < MAX_PLAYERS)
                {
                    player_slot *Slot = &AppState->Players[State->Variant];
                    Slot->Active = true;
                    Slot->Entity = Replica;
                    Replica->PlayerIndex = State->Variant;
                    // NOTE(zoubir): the server does not send respawn
                    // timers; it starts its own at the death tick, so the
                    // client starts the same one when it sees the death,
                    // at most a snapshot late
                    if (!WasDead && IsDeadPlayer(Replica))
                    {
                        Slot->RespawnTimer = PLAYER_RESPAWN_SECONDS;
                    }
                }
            }
        }

        ApplySnapshotFacings(World, Table, Snapshot);
        ApplySnapshotScores(AppState, Snapshot);
        if (Snapshot->NameSlot < MAX_PLAYERS)
        {
            CopyString(AppState->Players[Snapshot->NameSlot].Name,
                       sizeof(AppState->Players[Snapshot->NameSlot].Name),
                       Snapshot->Name);
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

    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Entity && IsDeadPlayer(Slot->Entity))
        {
            Slot->RespawnTimer = Maximum(0.f, Slot->RespawnTimer - DeltaTime);
        }
    }
    AdvanceSmoothing(&Table->Smoothing, DeltaTime);
    for(u32 Id = 0; Id < MAX_REPLICAS; Id++)
    {
        if (Table->LocalIndexPlusOne[Id])
        {
            world_entity *Replica =
                &World->Entities[Table->LocalIndexPlusOne[Id] - 1];
            if (Replica->IsPresent)
            {
                SmoothReplica(AppState, Arena, &Table->Smoothing, Id, Replica);
            }
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
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        AppState->Players[SlotIndex] = {};
    }
    AppState->LocalPlayerIndex = 0;
    u32 Slot = AppState->LocalPlayerIndex;
    world_entity *Player = AddPlayerToSlot(AppState, World, Arena, Slot,
                                           PlayerSpawnPosition(World, Slot));
    AddFamiliar(AppState, World, Arena, Player);
    if (AppState->Monsters)
    {
        FillMonsterPopulation(AppState, World, Arena, AppState->Monsters);
    }
}
