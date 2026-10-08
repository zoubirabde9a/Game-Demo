/* The dungeon online (sim/dungeon/, client/dungeon/dungeon_net.cpp), with
   a real server playing the Sunken Crypt and a real client: the client
   builds the crypt and its run, picks a role through the held buttons
   and sees it come back, sees a healer's sanctuary, sees the empty Antechamber cleared, and builds
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

// NOTE(zoubir): like RunRoundMapTest, with the role byte in the input as
// app.cpp sends it
internal void
RunDungeonOnlineTest(round_map_test *Test, int Frames)
{
    app_input Input = {};
    Input.DeltaTime = 1.0f / SERVER_TICK_RATE;
    for (int Frame = 0; Frame < Frames; ++Frame)
    {
        UpdateOnlineSession(Test->Online, &Input, false, {},
                            0, 0, OnlineRoleRequest(Test->Client, Input.DeltaTime));
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
    // NOTE(zoubir): the last class + 1 needs a fourth bit, which the role
    // byte has
    u32 Last = PlayerRole_Count - 1;
    RequestDungeonRole(Client, Last);
    RunDungeonOnlineTest(&Test, SERVER_TICK_RATE);
    Check(Game->Players[SlotIndex].Role == Last);
    Check(Client->Players[Client->LocalPlayerIndex].Role == Last);
    RequestDungeonRole(Client, PlayerRole_Healer);
    RunDungeonOnlineTest(&Test, SERVER_TICK_RATE);
    Check(Game->Players[SlotIndex].Role == PlayerRole_Healer);
    Check(Client->Players[Client->LocalPlayerIndex].Role == PlayerRole_Healer);
    world_entity *Own = GetLocalPlayer(Client);
    Check(Own && Own->MaxHp == GetRoleDef(PlayerRole_Healer)->MaxHp);
    Check(Client->Dungeon->ShownBossKind == MonsterKind_Count);

    // NOTE(zoubir): a sanctuary on the server is drawn on the client
    Game->Dungeon->Sanctuaries[2].Position = V3(300.f, 400.f, 0.f);
    Game->Dungeon->Sanctuaries[2].Seconds = 4.f;
    RunDungeonOnlineTest(&Test, 6);
    sanctuary *Shown = &Client->Dungeon->Sanctuaries[0];
    Check(Shown->Seconds > 3.f && Shown->Seconds <= 4.1f);
    Check(Shown->Position.X == 300.f && Shown->Position.Y == 400.f);

    // NOTE(zoubir): the meter comes over one player a snapshot; with one
    // player each snapshot has it. A new fight count zeroes the others
    Client->Players[5].MeterDamage = 77.f;
    Game->Dungeon->MeterFight = 3;
    Game->Dungeon->MeterSeconds = 12.34f;
    Game->Players[SlotIndex].MeterDamage = 1234.4f;
    Game->Players[SlotIndex].MeterHealing = 56.f;
    Game->Players[SlotIndex].MeterTaken = 78.6f;
    RunDungeonOnlineTest(&Test, 30);
    player_slot *Mine = &Client->Players[Client->LocalPlayerIndex];
    Check(Client->Dungeon->MeterFight == 3);
    Check(Client->Dungeon->MeterSeconds > 12.2f && Client->Dungeon->MeterSeconds < 12.4f);
    Check(Mine->MeterDamage == 1234.f && Mine->MeterHealing == 56.f && Mine->MeterTaken == 79.f);
    Check(Client->Players[5].MeterDamage == 0.f);
    StopRoundMapTest(&Test);
    free(DungeonSessionArena.Base);
}

// NOTE(zoubir): three bots in the crypt take a tank, a healer and a
// damage role, one each
internal void
TestDungeonBotsTakeRoles()
{
    static server_game Game;
    GameInit(&Game, MapId_Crypt);
    app_state *AppState = Game.AppState;
    float Dt = 1.f / 60.f;
    Game.BotTarget = 3;
    for (u32 Tick = 0; Tick < 120; ++Tick)
    {
        GameKeepBots(&Game, 0, Dt);
        GameTick(&Game, Dt);
    }
    u32 Seen = 0;
    u32 Damage = 0;
    for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
    {
        if (AppState->Players[Slot].Active)
        {
            Seen |= 1u << AppState->Players[Slot].Role;
            Damage += IsDamageRole(AppState->Players[Slot].Role) ? 1 : 0;
        }
    }
    Check((Seen & (1u << PlayerRole_Tank)) && (Seen & (1u << PlayerRole_Healer)) && Damage > 0);
    GameShutdown(&Game);
}

internal void
RunDungeonOnlineTests()
{
    TestDungeonBotsTakeRoles();
    TestDungeonRolesAndRoomsOnline();
}
