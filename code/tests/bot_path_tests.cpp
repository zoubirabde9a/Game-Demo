/* Bot path tests (server/bots/bot_paths.cpp): the way a bot walks between
   fights keeps a body's width off walls. Included by server_tests.cpp,
   which calls RunBotPathTests. */

// NOTE(zoubir): the Rimeheart Vault's corridor north to the Calving Hall
// is four tiles wide (layout columns 37 to 40). A way that hugged its west
// wall aimed the bots at a slant the movement keys round to straight up,
// into the corner of that wall: the whole party stood at the corridor's
// mouth for the rest of the run. Each spot is where a bot of the balance
// probe stood stuck; from each, the keys must also go east, round the
// corner, and the way up the corridor keeps off both its walls.
internal void
TestBotWayRoundsTheCalvingHallCorner()
{
    world *World = (world *)calloc(1, sizeof(world));
    World->MapId = MapId_Vault;
    World->TileWidth = ARENA_TILE_SIZE;
    float Tile = (float)ARENA_TILE_SIZE;
    v2 Goal = Tile * GateOrRoomMiddle(MapId_Vault, 0, 3);
    v2 Stuck[] = {{1179.8f, 836.f}, {1172.3f, 852.f}, {1202.f, 844.f}};
    for (u32 Index = 0; Index < ArrayCount(Stuck); ++Index)
    {
        v2 Way = {};
        Check(BotPathWay(World, Stuck[Index], Goal, &Way));
        Check(NetButtonsToward(Way) & NetButton_Right);
    }
    bot_path_grid *Grid = &BotPathGrid;
    for (i32 Row = 20; Row <= 25; ++Row)
    {
        u8 *Tiles = &Grid->Tiles[Row * Grid->Width];
        Check(Tiles[36] == BotTile_Closed && Tiles[41] == BotTile_Closed);
        Check(Tiles[37] == BotTile_Edge && Tiles[40] == BotTile_Edge);
        Check(Tiles[38] == BotTile_Open && Tiles[39] == BotTile_Open);
    }
    free(World);
}

internal void
RunBotPathTests()
{
    TestBotWayRoundsTheCalvingHallCorner();
}
