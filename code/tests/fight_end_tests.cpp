/* Fight end tests (dungeon_tests.cpp): a boss fight ends when nobody
   alive is left in its room, even with a player standing outside. */

// NOTE(zoubir): A falls in the Ossuary while B stands outside its gate:
// the fight waits while B is in the room, and wipes once nobody alive is
internal void
TestEmptyBossRoomWipes()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    Run->RoomStates[1] = Run->RoomStates[2] = RoomState_Cleared;
    world_entity *A = AppState->Players[0].Entity;
    world_entity *B = AppState->Players[1].Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, A, Run->RoomEntry[3]);
    TickCrypt(&Crypt, 1);
    Check(Run->FightingRoom == 3);
    Check(RoomAtPosition(World, B->Position.XY) == 3);

    // NOTE(zoubir): one of two outside is fine while the other fights
    MovePlayerTo(AppState, World, &Crypt.Arena, B, Run->RoomCheckpoint[3]);
    TickCrypt(&Crypt, 5 * 60);
    Check(Run->FightingRoom == 3);

    KillEntity(AppState, World, A, 0);
    TickCrypt(&Crypt, 60);
    Check(Run->FightingRoom == 3);
    TickCrypt(&Crypt, (u32)(DUNGEON_EMPTY_ROOM_SECONDS * 60.f) + 1);
    Check(Run->FightingRoom == 0 && Run->RoomStates[3] == RoomState_Waiting);
    Check(Run->Wipes == 1 && CountLiveMonsters(World) == 0);
    TickCrypt(&Crypt, 3 * 60);
    Check(!IsDeadPlayer(A) && RoomAtPosition(World, A->Position.XY) == 2);
    Check(!IsGateClosed(Run, 1));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): a player thrown onto a wall during a fight lands back in
// the fight's room, not behind its closed gate
internal void
TestStrayFightersStayInTheFight()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    Run->RoomStates[1] = Run->RoomStates[2] = RoomState_Cleared;
    world_entity *Player = AppState->Players[0].Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Player, Run->RoomEntry[3]);
    TickCrypt(&Crypt, 1);
    Check(Run->FightingRoom == 3);
    v3 From = Player->Position;
    Player->Position = TileCenter(World, 60, 0);
    Player->GroundZ = 0.f;
    CheckAndChangeEntityChunk(AppState, World, &Crypt.Arena, From, Player);
    TickCrypt(&Crypt, 2);
    Check(RoomAtPosition(World, Player->Position.XY) == 3);
    Check(Run->FightingRoom == 3);
    DestroyCryptWorld(&Crypt);
}

internal void
RunFightEndTests()
{
    TestEmptyBossRoomWipes();
    TestStrayFightersStayInTheFight();
}
