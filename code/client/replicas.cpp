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
    u16 Look[MAX_REPLICAS]; // see ReplicaLook
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

// NOTE(zoubir): a hazard as AddMonsterHazard dresses it: its look and
// size come from that monster kind's ability. 0 if the server named an
// ability this build does not have.
internal world_entity *
AddHazardReplica(app_state *AppState, world *World, memory_arena *Arena,
                 v3 Position, u32 Kind, u32 AbilityIndex)
{
    if (Kind >= MonsterKind_Count)
    {
        return 0;
    }
    monster_def *Def = GetMonsterDef((monster_kind)Kind);
    if (AbilityIndex >= Def->AbilityCount)
    {
        return 0;
    }
    monster_ability *Ability = &Def->Abilities[AbilityIndex];
    world_entity *Hazard = AddEntity(AppState, World, Arena,
                                     EntityType_MonsterHazard, Position,
                                     AppState->FireBallCollision);
    Hazard->MonsterKind = (monster_kind)Kind;
    Hazard->AbilityIndex = AbilityIndex;
    float Size = 2.f * Ability->Radius;
    Hazard->Dimensions = V2(Size, Size);
    Hazard->Texture = {AssetType_MonsterHazard, (u32)Ability->HazardStyle};
    if (AppState->Monsters)
    {
        Hazard->AnimationSet =
            &AppState->Monsters->HazardAnimationSets[Ability->HazardStyle];
    }
    return Hazard;
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
        case EntityType_MonsterHazard:
        {
            Result = AddHazardReplica(AppState, World, Arena, Position,
                                      State->Variant, State->Ability);
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
    Replica->EliteAffix = State->Affix;
    Replica->AbilityIndex = State->Ability;
    // NOTE(zoubir): status pips blink under 1 s left; the client does not
    // know the real time left, so active effects read as 1.5 s
    for(u32 Effect = 1; Effect < StatusEffect_Count; Effect++)
    {
        Replica->StatusTimers[Effect] =
            (State->Status & (1 << (Effect - 1))) ? 1.5f : 0.f;
    }
    // NOTE(zoubir): keeps the chunk lists right so RemoveEntity finds it
    CheckAndChangeEntityChunk(AppState, &AppState->World, Arena,
                              OldPosition, Replica);
}

// NOTE(zoubir): player slots mirror the server's: a slot is active while
// the server lists its score, and points at that player's replica. The
// local slot stays active so the camera always has someone to follow.
internal void
ApplySnapshotScores(app_state *AppState, net_snapshot *Snapshot)
{
    bool32 Listed[MAX_PLAYERS] = {};
    for(u32 Index = 0; Index < Snapshot->ScoreCount; Index++)
    {
        net_score *Score = &Snapshot->Scores[Index];
        if (Score->Slot < MAX_PLAYERS)
        {
            player_slot *Slot = &AppState->Players[Score->Slot];
            Listed[Score->Slot] = true;
            Slot->Active = true;
            Slot->Kills = Score->Kills;
            Slot->Deaths = Score->Deaths;
            Slot->MonsterKills = Score->MonsterKills;
        }
    }
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (!Listed[SlotIndex] && SlotIndex != AppState->LocalPlayerIndex)
        {
            AppState->Players[SlotIndex].Active = false;
            AppState->Players[SlotIndex].Entity = 0;
        }
    }
}

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
                // NOTE(zoubir): a player's Variant is its slot on the server
                if (State->Type == EntityType_Player &&
                    State->Variant < MAX_PLAYERS)
                {
                    player_slot *Slot = &AppState->Players[State->Variant];
                    Slot->Active = true;
                    Slot->Entity = Replica;
                    Replica->PlayerIndex = State->Variant;
                }
            }
        }

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
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        AppState->Players[SlotIndex] = {};
    }
    AppState->LocalPlayerIndex = 0;
    u32 Slot = AppState->LocalPlayerIndex;
    world_entity *Player = AddPlayerToSlot(AppState, World, Arena, Slot,
                                           PlayerSpawnPosition(Slot));
    AddFamiliar(AppState, World, Arena, Player);
    if (AppState->Monsters)
    {
        FillMonsterPopulation(AppState, World, Arena, AppState->Monsters);
    }
}
