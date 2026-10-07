/* The dungeon online (sim/dungeon/, client/dungeon/dungeon_net.cpp), with
   a real server playing the Sunken Crypt and a real client: the client
   builds the crypt and its run, picks a role through the held buttons
   and sees it come back, sees the empty Antechamber cleared, and builds
   the gate walls the server has, so its prediction stops where the
   server's player does. Included by server_tests.cpp after
   round_map_tests.cpp, whose test harness it borrows. */

// NOTE(zoubir): the online session lives apart from the world, as in
// the game (app_state.WorldArena): joining the crypt rebuilds the world,
// which empties the world's arena
global_variable memory_arena DungeonSessionArena;

internal void
StartDungeonOnlineTest(round_map_test *Test, char *Name)
{
    Check(ServerStart(&Test->Server, 0, MapId_Crypt));
    Test->Client = (app_state *)calloc(1, sizeof(app_state));
    app_state *Client = Test->Client;
    memory_index Size = Megabytes(48);
    InitializeArena(&Test->Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Test->Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    InitSimulation(Client, &Test->Arena, &Test->Constants);
    AddPlayerToSlot(Client, &Client->World, &Test->Arena, 0, PlayerSpawnPosition(&Client->World, 0));
    Client->RewindFx = (rewind_fx *)calloc(1, sizeof(rewind_fx));
    SetEnvironment(ONLINE_ADDRESS_ENV, "");
    InitializeArena(&DungeonSessionArena, (memory_index *)calloc(1, Megabytes(8)), Megabytes(8));
    Client->Online = StartOnlineSession(&DungeonSessionArena);
    Test->Online = Client->Online;
    char Address[32];
    snprintf(Address, sizeof(Address), "127.0.0.1:%u", NetSocketPort(&Test->Server.Socket));
    Check(OnlineConnect(Test->Online, Address, Name));
}

// NOTE(zoubir): like RunRoundMapTest, with the role field in the held
// buttons as app.cpp sends it
internal void
RunDungeonOnlineTest(round_map_test *Test, int Frames)
{
    app_input Input = {};
    Input.DeltaTime = 1.0f / SERVER_TICK_RATE;
    for (int Frame = 0; Frame < Frames; ++Frame)
    {
        UpdateOnlineSession(Test->Online, &Input, false, {},
                            OnlineRoleBits(Test->Client, Input.DeltaTime));
        RunWorldTick(Test->Client, &Test->Arena, Input.DeltaTime);
        ServerTick(&Test->Server);
    }
}

internal u32
CountGateWalls(dungeon_run *Run)
{
    u32 Result = 0;
    for(u32 Gate = 0; Gate < DUNGEON_MAX_GATES; Gate++)
    {
        for(u32 Index = 0; Index < DUNGEON_GATE_TILES; Index++)
        {
            Result += Run->GateWalls[Gate][Index] ? 1 : 0;
        }
    }
    return Result;
}

internal void
TestDungeonRolesAndRoomsOnline()
{
    static round_map_test Test;
    StartDungeonOnlineTest(&Test, "Delver");
    RunDungeonOnlineTest(&Test, 3 * SERVER_TICK_RATE);
    app_state *Game = Test.Server.Game.AppState;
    app_state *Client = Test.Client;
    Check(Test.Online->Replicas.Active);
    Check(Client->World.MapId == MapId_Crypt);
    Check(Game->Dungeon && Client->Dungeon);
    if (!Game->Dungeon || !Client->Dungeon)
    {
        StopRoundMapTest(&Test);
        return;
    }
    // NOTE(zoubir): the Antechamber has nothing in it, so it is cleared,
    // and the gates beyond the Bone Halls stand closed on both sides
    Check(Client->Dungeon->RoomStates[1] == RoomState_Cleared);
    Check(Client->Dungeon->RoomStates[2] == RoomState_Waiting);
    Check(CountGateWalls(Client->Dungeon) == CountGateWalls(Game->Dungeon));
    Check(CountGateWalls(Client->Dungeon) > 0);

    u32 SlotIndex = Test.Online->Client.PlayerIndex;
    RequestDungeonRole(Client, PlayerRole_Healer);
    RunDungeonOnlineTest(&Test, SERVER_TICK_RATE);
    Check(Game->Players[SlotIndex].Role == PlayerRole_Healer);
    Check(Client->Players[Client->LocalPlayerIndex].Role == PlayerRole_Healer);
    world_entity *Own = GetLocalPlayer(Client);
    Check(Own && Own->MaxHp == GetRoleDef(PlayerRole_Healer)->MaxHp);
    Check(Client->Dungeon->ShownBossKind == MonsterKind_Count);
    StopRoundMapTest(&Test);
    free(DungeonSessionArena.Base);
}

internal void
RunDungeonOnlineTests()
{
    TestDungeonRolesAndRoomsOnline();
}
