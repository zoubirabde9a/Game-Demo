/* Dungeon tests (sim/dungeon/): roles change health and damage only in a
   dungeon run, and leave the duel alone. Included by sim_tests.cpp,
   which calls RunDungeonTests. */

internal void
TestRolesDoNothingOutsideADungeon()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    Player->SpawnShield = 0.f;
    float MaxHp = Player->MaxHp;
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Tank);
    Check(Player->MaxHp == MaxHp);
    DamageEntity(AppState, Test.World, Player, 10.f, 0);
    Check(Player->Hp == MaxHp - 10.f);
    DestroyTestWorld(&Test);
}

internal void
TestRolesScaleHealthAndDamage()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    dungeon_run Run = {};
    AppState->Dungeon = &Run;
    world_entity *Tank = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    world_entity *Healer = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1, {400, 300, 0});
    Tank->SpawnShield = Healer->SpawnShield = 0.f;
    // NOTE(zoubir): a slot joins as a damage player
    Check(Tank->MaxHp == GetRoleDef(PlayerRole_Damage)->MaxHp);
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Tank);
    SetPlayerRole(AppState, &AppState->Players[1], PlayerRole_Healer);
    Check(Tank->MaxHp == 180.f && Tank->Hp == 180.f);
    Check(Healer->MaxHp == 100.f);

    DamageEntity(AppState, Test.World, Tank, 10.f, 0);
    Check(Tank->Hp == 180.f - 7.f);

    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {500, 300, 0}, Test.UnitVolume);
    Monster->MaxHp = Monster->Hp = 100.f;
    DamageEntity(AppState, Test.World, Monster, 20.f, Healer);
    Check(Monster->Hp == 90.f);
    DamageEntity(AppState, Test.World, Monster, 20.f, Tank);
    Check(Monster->Hp == 76.f);

    AppState->Dungeon = 0;
    DestroyTestWorld(&Test);
}

internal void
TestNoDuelMapIsADungeon()
{
    for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
    {
        map_def *Map = GetMapDef((map_id)MapIndex);
        if (Map->Dungeon)
        {
            Check(Map->MonsterPopulation == 0);
        }
    }
}

// NOTE(zoubir): the room layout matches the map: every room and gate
// tile is open ground, the rooms are numbered 1 up with a gate between
// each pair, and the party spawns in the first room
internal void
TestCryptRoomsMatchTheMap()
{
    map_def *Map = GetMapDef(MapId_Crypt);
    Check(Map->Dungeon && Map->MonsterPopulation == 0);
    Check(ArrayCount(CryptRooms) == Map->Height);
    u32 RoomTiles[DUNGEON_MAX_ROOMS + 1] = {};
    u32 GateTiles[DUNGEON_MAX_GATES + 1] = {};
    for(i32 Y = 0; Y < (i32)Map->Height; Y++)
    {
        Check(strlen(CryptRooms[Y]) == Map->Width);
        for(i32 X = 0; X < (i32)Map->Width; X++)
        {
            u32 Room = RoomAtTile(MapId_Crypt, X, Y);
            u32 Gate = GateAtTile(MapId_Crypt, X, Y);
            bool32 Open = !GetTerrainDef(TerrainAt(Map, X, Y))->Blocks;
            char Symbol = CryptRooms[Y][X];
            Check(Symbol == '.' || Room || Gate < DUNGEON_MAX_GATES);
            if (Gate < DUNGEON_MAX_GATES)
            {
                Check(Open);
            }
            RoomTiles[Room]++;
            GateTiles[Gate]++;
        }
    }
    u32 Rooms = CountRooms(MapId_Crypt);
    Check(Rooms == 7);
    for(u32 Room = 1; Room <= Rooms; Room++)
    {
        Check(RoomTiles[Room] > 0);
    }
    for(u32 Gate = 0; Gate < DUNGEON_MAX_GATES; Gate++)
    {
        Check((GateTiles[Gate] > 0) == (Gate + 1 < Rooms));
    }
    for(u32 Spawn = 0; Spawn < Map->SpawnCount; Spawn++)
    {
        Check(RoomAtTile(MapId_Crypt, Map->SpawnX[Spawn], Map->SpawnY[Spawn]) == 1);
    }
    Check(Map->SpawnCount == MAX_PLAYERS);
}

// NOTE(zoubir): the duel's vote never offers the dungeon, and a run
// offers nothing
internal void
TestTheMapVoteLeavesTheDungeonAlone()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->World.MapId = MapId_Arena;
    Check(IsVotableMap(AppState, MapId_Keep));
    Check(!IsVotableMap(AppState, MapId_Crypt));
    dungeon_run Run = {};
    AppState->Dungeon = &Run;
    AppState->World.MapId = MapId_Crypt;
    Check(!IsVotableMap(AppState, MapId_Keep));
    AppState->Dungeon = 0;
    DestroyTestWorld(&Test);
}

internal void
RunDungeonTests()
{
    TestCryptRoomsMatchTheMap();
    TestTheMapVoteLeavesTheDungeonAlone();
    TestRolesDoNothingOutsideADungeon();
    TestRolesScaleHealthAndDamage();
    TestNoDuelMapIsADungeon();
}
