/* Level cap tests (dungeon_tests.cpp): in a dungeon run a player earns
   one level per room with monsters the party clears (LevelCap,
   sim/progression/experience.cpp), and the count carries from one level
   of the dungeon to the next. */

// NOTE(zoubir): the empty Antechamber earns nothing, so waiting there
// stops at level 1 with the experience one point short of 2; clearing
// the Bone Halls lets the next point through, and the count goes on into
// the Ember Depths
internal void
TestDungeonLevelsWaitForRooms()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    player_slot *Slot = &AppState->Players[0];
    TickCrypt(&Crypt, 1);
    Check(Run->RoomStates[1] == RoomState_Cleared);
    Check(AppState->DungeonRoomsCleared == 0 && LevelCap(AppState) == 1);

    AwardXp(AppState, Slot, XpToReach(10));
    Check(Slot->Level == 1 && Slot->Xp == XpToReach(2) - 1);
    TickCrypt(&Crypt, 60 * 60);
    Check(Slot->Level == 1 && Slot->Xp == XpToReach(2) - 1);

    MovePlayerTo(AppState, World, &Crypt.Arena, Slot->Entity, Run->RoomEntry[2]);
    TickCrypt(&Crypt, 1);
    Check(Run->FightingRoom == 2);
    KillRoomMonsters(&Crypt, 2);
    TickCrypt(&Crypt, 60);
    Check(Run->RoomStates[2] == RoomState_Cleared);
    Check(AppState->DungeonRoomsCleared == 1 && Slot->Level == 2);

    // NOTE(zoubir): the next level of the dungeon keeps the count, and
    // its empty first room adds nothing
    StartNextRoundMap(AppState, &Crypt.Arena);
    TickCrypt(&Crypt, 60);
    Check(World->MapId == MapId_Depths && AppState->Dungeon);
    Check(AppState->DungeonRoomsCleared == 1 && Slot->Level == 2);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): away from a dungeon nothing caps the level
internal void
TestDuelLevelsAreNotCapped()
{
    test_world Test = CreateTestWorld();
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    player_slot *Slot = &Test.AppState->Players[0];
    Check(!Test.AppState->Dungeon && LevelCap(Test.AppState) == PLAYER_MAX_LEVEL);
    AwardXp(Test.AppState, Slot, XpToReach(10));
    Check(Slot->Level == 10);
    DestroyTestWorld(&Test);
}

internal void
RunLevelCapTests()
{
    TestDungeonLevelsWaitForRooms();
    TestDuelLevelsAreNotCapped();
}
