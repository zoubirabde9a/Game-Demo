/* Level cap tests (dungeon_tests.cpp): in a dungeon run a player levels
   no higher than DUNGEON_LEVELS_AHEAD past the rooms the party has
   cleared (LevelCap, sim/progression/experience.cpp), and the count
   carries from one level of the dungeon to the next. */

// NOTE(zoubir): waiting in the cleared Antechamber stops at level 3 with
// the experience one point short of 4; clearing the Bone Halls lets the
// next point through, and the count goes on into the Ember Depths
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
    Check(AppState->DungeonRoomsCleared == 1 && LevelCap(AppState) == 3);

    AwardXp(AppState, Slot, XpToReach(10));
    Check(Slot->Level == 3 && Slot->Xp == XpToReach(4) - 1);
    TickCrypt(&Crypt, 60 * 60);
    Check(Slot->Level == 3 && Slot->Xp == XpToReach(4) - 1);

    MovePlayerTo(AppState, World, &Crypt.Arena, Slot->Entity, Run->RoomEntry[2]);
    TickCrypt(&Crypt, 1);
    Check(Run->FightingRoom == 2);
    KillRoomMonsters(&Crypt, 2);
    TickCrypt(&Crypt, 60);
    Check(Run->RoomStates[2] == RoomState_Cleared);
    Check(AppState->DungeonRoomsCleared == 2 && Slot->Level == 4);

    // NOTE(zoubir): the next level of the dungeon keeps the count
    StartNextRoundMap(AppState, &Crypt.Arena);
    Check(World->MapId == MapId_Depths && AppState->Dungeon);
    Check(AppState->DungeonRoomsCleared == 2 && Slot->Level == 4);
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
