/* Vault tests (dungeon_tests.cpp): the Rimeheart Vault, the dungeon's
   third level. Its rooms match its map, its monsters are tougher, harder
   hitting and faster than the depths', its bosses stand in their rooms
   on a clock with their adds, and every boss script still fits the bits
   a run keeps for it. Reaching the vault and leaving it are checked with
   the depths (TestClearedCryptGoesDown, depths_tests.cpp). */

// NOTE(zoubir): the vault's room map is the size of its layout, every
// gate tile is open ground, seven rooms with a gate between each pair,
// every spawn in the first room, and no gate or room entry on a pit
internal void
TestVaultRoomsMatchTheMap()
{
    map_def *Map = GetMapDef(MapId_Vault);
    Check(Map->Dungeon && Map->MonsterPopulation == 0);
    Check(ArrayCount(VaultRooms) == Map->Height);
    u32 RoomTiles[DUNGEON_MAX_ROOMS + 1] = {};
    u32 GateTiles[DUNGEON_MAX_GATES + 1] = {};
    for(i32 Y = 0; Y < (i32)Map->Height; Y++)
    {
        Check(strlen(VaultRooms[Y]) == Map->Width);
        Check(strlen(VaultLayout[Y]) == Map->Width);
        for(i32 X = 0; X < (i32)Map->Width; X++)
        {
            u32 Room = RoomAtTile(MapId_Vault, X, Y);
            u32 Gate = GateAtTile(MapId_Vault, X, Y);
            terrain_def *Ground = GetTerrainDef(TerrainAt(Map, X, Y));
            if (Gate < DUNGEON_MAX_GATES)
            {
                Check(!Ground->Blocks && !Ground->Hazard);
            }
            RoomTiles[Room]++;
            GateTiles[Gate]++;
        }
    }
    u32 Rooms = CountRooms(MapId_Vault);
    Check(Rooms == 7);
    for(u32 Room = 1; Room <= Rooms; Room++)
    {
        Check(RoomTiles[Room] > 0);
    }
    for(u32 Gate = 0; Gate < DUNGEON_MAX_GATES; Gate++)
    {
        Check((GateTiles[Gate] > 0) == (Gate + 1 < Rooms));
    }
    Check(Map->SpawnCount == MAX_PLAYERS);
    for(u32 Spawn = 0; Spawn < Map->SpawnCount; Spawn++)
    {
        Check(RoomAtTile(MapId_Vault, Map->SpawnX[Spawn], Map->SpawnY[Spawn]) == 1);
    }

    crypt_world Vault = CreateDungeonWorld(MapId_Vault, 1);
    dungeon_run *Run = Vault.AppState->Dungeon;
    world *World = &Vault.AppState->World;
    Check(Run && Run->RoomCount == 7);
    for(u32 Room = 1; Room <= Run->RoomCount; Room++)
    {
        v3 Spots[3] = {Run->RoomEntry[Room], Run->RoomCheckpoint[Room], Run->RoomMiddle[Room]};
        for(u32 Index = 0; Index < ArrayCount(Spots); Index++)
        {
            i32 X = (i32)floorf(Spots[Index].X / (float)World->TileWidth);
            i32 Y = (i32)floorf(Spots[Index].Y / (float)World->TileWidth);
            terrain_def *Ground = GetTerrainDef(TerrainAt(Map, X, Y));
            Check(!Ground->Blocks && !Ground->Hazard);
        }
    }
    DestroyCryptWorld(&Vault);
}

// NOTE(zoubir): each step down is harder than the one before
internal void
TestVaultIsTheHardestLevel()
{
    Check(LevelFoeHealth(MapId_Vault) > LevelFoeHealth(MapId_Depths));
    Check(LevelFoeDamage(MapId_Vault) > LevelFoeDamage(MapId_Depths));
    Check(LevelFoePace(MapId_Vault) > LevelFoePace(MapId_Depths));
    Check(LevelPackScale(MapId_Vault) >= LevelPackScale(MapId_Depths));
    Check(GetDungeonLevel(MapId_Vault)->Number == 3);
}

// NOTE(zoubir): each boss room of the vault starts with its boss in the
// middle, on an enrage clock, and its scripted adds come at their share
internal void
TestVaultBossesStand()
{
    u32 BossRooms[3] = {3, 5, 7};
    u32 BossKinds[3] = {MonsterKind_FrostColossus, MonsterKind_PaleWitch,
                        MonsterKind_Rimeheart};
    for(u32 Index = 0; Index < 3; Index++)
    {
        crypt_world Vault = CreateDungeonWorld(MapId_Vault, 1);
        app_state *AppState = Vault.AppState;
        world *World = &AppState->World;
        dungeon_run *Run = AppState->Dungeon;
        u32 Room = BossRooms[Index];
        for(u32 Before = 1; Before < Room; Before++)
        {
            Run->RoomStates[Before] = RoomState_Cleared;
        }
        TickCrypt(&Vault, 1);
        MovePlayerTo(AppState, World, &Vault.Arena, AppState->Players[0].Entity,
                     Run->RoomEntry[Room]);
        TickCrypt(&Vault, 2);
        world_entity *Boss = FightBoss(World, Run);
        Check(Run->FightingRoom == Room && Boss && Boss->MonsterKind == BossKinds[Index]);
        Check(RoomAtPosition(World, Boss->Position.XY) == Room);
        Check(GetMonsterStats(Boss->MonsterKind)->SpawnWeight == 0);
        Check(Run->Clock.Stage == BossClock_Running && Run->Clock.Limit > 60.f);
        u32 Foes = Run->FoeCount;
        Boss->Hp = Boss->MaxHp * 0.55f;
        TickCrypt(&Vault, 1);
        Check(Run->FoeCount > Foes && Run->Clock.AddCount > 0);
        DestroyCryptWorld(&Vault);
    }
}

// NOTE(zoubir): a run keeps one bit per event of its boss (BossEventsFired), and
// only the first four ability slots may make shots or ground hazards, so
// each vault boss keeps its blow nobody dodges in the fifth
internal void
TestVaultBossesFit()
{
    for(u32 Kind = 0; Kind < MonsterKind_Count; Kind++)
    {
        u32 Rows = 0;
        for(u32 Index = 0; Index < ArrayCount(BossEvents); Index++)
        {
            Rows += BossEvents[Index].Boss == (monster_kind)Kind;
        }
        Check(Rows <= 32);
    }
    monster_kind Kinds[3] = {MonsterKind_FrostColossus, MonsterKind_PaleWitch,
                    MonsterKind_Rimeheart};
    for(u32 Index = 0; Index < 3; Index++)
    {
        monster_def *Def = GetMonsterStats(Kinds[Index]);
        Check(Def->AbilityCount == 5);
        Check(Def->Abilities[4].Kind == MonsterAbility_Smite);
        Check(Def->MaxAlive == 1 && Def->EnrageHpShare > 0.f);
    }
}

internal void
RunVaultTests()
{
    TestVaultRoomsMatchTheMap();
    TestVaultIsTheHardestLevel();
    TestVaultBossesStand();
    TestVaultBossesFit();
}
