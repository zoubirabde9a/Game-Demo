/* Round maps: when the break between rounds runs out the server starts
   the next round on the next map (sim/setup.cpp StartNextRoundMap), and
   a client playing there builds the new map and follows
   (client/online.cpp). Included by server_tests.cpp, which calls
   TestRoundMovesToNextMap. */

internal void
TestRoundMovesToNextMap()
{
    static server Server;
    Check(ServerStart(&Server, 0, MapId_Arena));

    app_state *Client = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(48);
    memory_arena Arena, Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    InitSimulation(Client, &Arena, &Constants);
    AddPlayerToSlot(Client, &Client->World, &Arena, 0, PlayerSpawnPosition(&Client->World, 0));
    Client->RewindFx = (rewind_fx *)calloc(1, sizeof(rewind_fx));
    SetEnvironment(ONLINE_ADDRESS_ENV, "");
    Client->Online = StartOnlineSession(&Arena);
    online_session *Online = Client->Online;
    char Address[32];
    snprintf(Address, sizeof(Address), "127.0.0.1:%u", NetSocketPort(&Server.Socket));
    Check(OnlineConnect(Online, Address, "Rounder"));

    app_input Input = {};
    Input.DeltaTime = 1.0f / SERVER_TICK_RATE;
    for (int Frame = 0; Frame < 3 * SERVER_TICK_RATE && !Online->Replicas.Active; ++Frame)
    {
        UpdateOnlineSession(Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        ServerTick(&Server);
    }
    Check(Online->Replicas.Active);
    Check(Client->World.MapId == MapId_Arena);

    // NOTE(zoubir): a round ends with the player a level up; the break's
    // last moments run out on the server
    app_state *Game = Server.Game.AppState;
    u32 SlotIndex = Online->Client.PlayerIndex;
    player_slot *Slot = &Game->Players[SlotIndex];
    Check(Slot->Active);
    Slot->Level = 3;
    Slot->Kills = 2;
    Game->RoundBreak = 0.05f;
    for (int Frame = 0; Frame < SERVER_TICK_RATE; ++Frame)
    {
        UpdateOnlineSession(Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        ServerTick(&Server);
    }

    u32 Next = NextRoundMap(MapId_Arena);
    Check(Next != MapId_Arena);
    Check(Game->World.MapId == Next);
    Check(Server.Clients.MapId == Next);
    // NOTE(zoubir): the player kept its slot, name, level and score, and
    // stands at its spawn on the new map with full health
    Check(Slot->Active && Slot->Entity && Slot->Entity->IsPresent);
    Check(strcmp(Slot->Name, "Rounder") == 0);
    Check(Slot->Level == 3 && Slot->Kills == 2);
    Check(Slot->Entity->Hp == Slot->Entity->MaxHp);
    v3 Spawn = PlayerSpawnPosition(&Game->World, SlotIndex);
    Check(Length(Slot->Entity->Position.XY - Spawn.XY) < 64.f);
    // NOTE(zoubir): the client built the same map and its player is there
    Check(Client->World.MapId == Next);
    Check(Online->Replicas.Active);
    world_entity *Own = GetLocalPlayer(Client);
    Check(Own && Own->IsPresent);
    if (Own)
    {
        Check(Length(Own->Position.XY - Slot->Entity->Position.XY) < 16.f);
    }

    NetClientDisconnect(&Online->Client);
    ServerStop(&Server);
    free(Client->RewindFx);
    free(Arena.Base);
    free(Constants.Base);
    free(Client);
}
