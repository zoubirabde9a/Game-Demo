/* Level cap tests (dungeon_tests.cpp): in a dungeon run a player earns
   one level per room with monsters the party clears on top of the level
   it started the run at (LevelCap, sim/progression/experience.cpp), the
   count carries from one level of the dungeon to the next, and a new run
   keeps the character (StartNextRoundMap, sim/setup.cpp). */

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
    Check(AppState->DungeonRoomsCleared == 0 && LevelCap(AppState, Slot) == 1);

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
    Check(!Test.AppState->Dungeon && LevelCap(Test.AppState, Slot) == DUEL_MAX_LEVEL);
    AwardXp(Test.AppState, Slot, XpToReach(10));
    Check(Slot->Level == 10);
    AwardXp(Test.AppState, Slot, XpToReach(PLAYER_MAX_LEVEL));
    Check(Slot->Level == DUEL_MAX_LEVEL);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a vote from one dungeon map to another starts a new run
// with the level and class talents the party has, and the cap counts
// from there; a duel in between sets the dungeon character aside and
// the next dungeon gives it back
internal void
TestNewRunKeepsTheCharacter()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    player_slot *Slot = &AppState->Players[0];
    TickCrypt(&Crypt, 1);
    AppState->DungeonRoomsCleared = 6;
    AwardXp(AppState, Slot, XpToReach(7));
    Check(Slot->Level == 7);
    Slot->Ranks[Talent_RoleFirst] = 2;

    AppState->NextMapVoted = true;
    AppState->NextMap = MapId_Depths;
    StartNextRoundMap(AppState, &Crypt.Arena);
    Check(World->MapId == MapId_Depths && AppState->DungeonRoomsCleared == 0);
    Check(Slot->Level == 7 && Slot->RunStartLevel == 7);
    Check(Slot->Ranks[Talent_RoleFirst] == 2);
    Check(LevelCap(AppState, Slot) == 7);
    AwardXp(AppState, Slot, XpToReach(12));
    Check(Slot->Level == 7 && Slot->Xp == XpToReach(8) - 1);
    AppState->DungeonRoomsCleared = 1;
    AwardXp(AppState, Slot, 1);
    Check(Slot->Level == 8);

    AppState->NextMapVoted = true;
    AppState->NextMap = MapId_Arena;
    StartNextRoundMap(AppState, &Crypt.Arena);
    Check(!AppState->Dungeon && Slot->Level == 1 && Slot->Ranks[Talent_RoleFirst] == 0);

    AppState->NextMapVoted = true;
    AppState->NextMap = MapId_Crypt;
    StartNextRoundMap(AppState, &Crypt.Arena);
    Check(AppState->Dungeon && Slot->Level == 8 && Slot->RunStartLevel == 8);
    Check(Slot->Ranks[Talent_RoleFirst] == 2);
    DestroyCryptWorld(&Crypt);
}

internal void
RunLevelCapTests()
{
    TestDungeonLevelsWaitForRooms();
    TestNewRunKeepsTheCharacter();
    TestDuelLevelsAreNotCapped();
}
