/* Fight end (encounters.cpp): how a room's fight stops. It is won when
   no monster of it is left in the room, and lost (a wipe) when no living
   player is left in the room: everyone down, or the last ones standing
   outside it. Either way the dead come back after a short wait. */

// NOTE(zoubir): the encounter's monsters still alive, and anything they
// brought into the room (summons, a slime's split): a room is cleared
// when it holds no monster at all
internal u32
CountLiveFoes(world *World, dungeon_run *Run)
{
    u32 Result = 0;
    for(u32 Index = 0; Index < Run->FoeCount; Index++)
    {
        Result += FindMonsterBySerial(World, Run->FoeSlots[Index],
                                      Run->FoeSerials[Index]) ? 1 : 0;
    }
    // NOTE(zoubir): and the monsters in the room the list does not hold,
    // each counted once
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent || Entity->Type != EntityType_Monster ||
            RoomAtPosition(World, Entity->Position.XY) != Run->FightingRoom)
        {
            continue;
        }
        bool32 Listed = false;
        for(u32 Index = 0; Index < Run->FoeCount && !Listed; Index++)
        {
            Listed = Run->FoeSlots[Index] == EntityIndex &&
                Run->FoeSerials[Index] == Entity->MonsterSerial;
        }
        Result += Listed ? 0 : 1;
    }
    return Result;
}

// NOTE(zoubir): the fight is over, either way: the dead come back at
// Position after Seconds, and later deaths respawn there too
internal void
EndEncounter(app_state *AppState, dungeon_run *Run, v3 Position, float Seconds)
{
    Run->FightingRoom = 0;
    Run->FoeCount = 0;
    Run->PartyDamage = 0.f;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (!Slot->Active || !Slot->Entity)
        {
            continue;
        }
        Slot->SpawnPosition = Position;
        if (IsDeadPlayer(Slot->Entity))
        {
            Slot->RespawnTimer = Seconds;
        }
    }
}

internal void
WipeEncounter(app_state *AppState, world *World, dungeon_run *Run)
{
    u32 Room = Run->FightingRoom;
    for(u32 Index = 0; Index < Run->FoeCount; Index++)
    {
        world_entity *Foe = FindMonsterBySerial(World, Run->FoeSlots[Index],
                                                Run->FoeSerials[Index]);
        if (Foe)
        {
            RemoveEntity(World, Foe);
        }
    }
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster &&
            RoomAtPosition(World, Entity->Position.XY) == Room)
        {
            RemoveEntity(World, Entity);
        }
    }
    Run->RoomStates[Room] = RoomState_Waiting;
    Run->Wipes++;
    EndEncounter(AppState, Run, Run->RoomCheckpoint[Room], DUNGEON_WIPE_RESPAWN_SECONDS);
}


// NOTE(zoubir): living players standing in the room being fought
internal u32
CountPlayersInFight(app_state *AppState, dungeon_run *Run)
{
    u32 Result = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Entity && !IsDeadPlayer(Slot->Entity) &&
            RoomAtPosition(&AppState->World, Slot->Entity->Position.XY) == Run->FightingRoom)
        {
            Result++;
        }
    }
    return Result;
}

// NOTE(zoubir): once a tick while a room is fought: true when the fight
// ended this tick, cleared or wiped. A room with nobody alive in it for
// DUNGEON_EMPTY_ROOM_SECONDS wipes too, so a player left outside a closed
// gate (thrown out, joined late) cannot hold a boss fight open forever
internal bool32
UpdateFightEnd(app_state *AppState, world *World, dungeon_run *Run,
               float DeltaTime)
{
    u32 Standing;
    u32 Players = CountPartyPlayers(AppState, &Standing);
    u32 Inside = CountPlayersInFight(AppState, Run);
    Run->EmptySeconds = Inside ? 0.f : Run->EmptySeconds + DeltaTime;
    bool32 Result = true;
    if (CountLiveFoes(World, Run) == 0)
    {
        u32 Room = Run->FightingRoom;
        Run->RoomStates[Room] = RoomState_Cleared;
        EndEncounter(AppState, Run, Run->RoomEntry[Room],
                     DUNGEON_CLEAR_RESPAWN_SECONDS);
    }
    else if (Players > 0 &&
             (Standing == 0 || Run->EmptySeconds >= DUNGEON_EMPTY_ROOM_SECONDS))
    {
        WipeEncounter(AppState, World, Run);
    }
    else
    {
        Result = false;
    }
    if (Result)
    {
        Run->EmptySeconds = 0.f;
    }
    return Result;
}
