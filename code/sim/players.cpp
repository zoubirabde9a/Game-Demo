/* Player slots: joining, finding players, and respawning them. */

// NOTE(zoubir): puts a new player entity in SlotIndex; returns it
internal world_entity *
AddPlayerToSlot(app_state *AppState, world *World, memory_arena *Arena,
                u32 SlotIndex, v3 SpawnPosition)
{
    Assert(SlotIndex < MAX_PLAYERS);
    player_slot *Slot = &AppState->Players[SlotIndex];
    *Slot = {};
    Slot->Active = true;
    Slot->SpawnPosition = SpawnPosition;
    Slot->Entity = AddPlayer(AppState, World, Arena, SpawnPosition);
    Slot->Entity->PlayerIndex = SlotIndex;
    return Slot->Entity;
}

inline player_slot *
GetPlayerSlot(app_state *AppState, world_entity *PlayerEntity)
{
    Assert(PlayerEntity->Type == EntityType_Player);
    Assert(PlayerEntity->PlayerIndex < MAX_PLAYERS);
    player_slot *Result = &AppState->Players[PlayerEntity->PlayerIndex];
    return Result;
}

inline world_entity *
GetLocalPlayer(app_state *AppState)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    world_entity *Result = Slot->Active ? Slot->Entity : 0;
    return Result;
}

// NOTE(zoubir): closest live player on the ground plane, or 0
internal world_entity *
FindNearestPlayer(app_state *AppState, v2 Position, float *OutDistance)
{
    world_entity *Result = 0;
    float BestDistance = 1e30f;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Entity && Slot->Entity->IsPresent &&
            !IsDeadPlayer(Slot->Entity))
        {
            float Distance = Length(Slot->Entity->Position.XY - Position);
            if (Distance < BestDistance)
            {
                BestDistance = Distance;
                Result = Slot->Entity;
            }
        }
    }
    if (OutDistance)
    {
        *OutDistance = BestDistance;
    }
    return Result;
}

// NOTE(zoubir): the player entity is never removed. While dead it waits
// out RespawnTimer (deaths are counted in DamageEntity), then goes back to
// its slot's spawn point with full health, so slot pointers never dangle.
// Returns true while the player is still dead.
internal bool32
UpdateDeadPlayer(player_slot *Slot, world *World, memory_arena *Arena,
                 app_state *AppState, float DeltaTime)
{
    world_entity *Player = Slot->Entity;
    if (Player->Hp > 0.f)
    {
        return false;
    }

    Slot->RespawnTimer -= DeltaTime;
    if (Slot->RespawnTimer > 0.f)
    {
        return true;
    }

    v3 OldPosition = Player->Position;
    Player->Position = Slot->SpawnPosition;
    Player->Velocity = {};
    Player->Hp = Player->MaxHp;
    Slot->RespawnTimer = 0.f;
    Slot->DelayedInputCount = 0;
    CheckAndChangeEntityChunk(AppState, World, Arena,
                              OldPosition, Player);
    return false;
}
