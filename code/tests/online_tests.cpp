/* Online play tests: the client side of a connection, with no network.
   Server address and name settings, replicas built from hand-made
   snapshots (created, moved, gliding, removed, carrying scores, names and
   monster details) and prediction of the local player. Included by
   sim_tests.cpp, which calls RunOnlineTests; the real client against a
   real server is in server_tests.cpp. */

internal void
TestOnlineAddressAndButtons()
{
    char Line[64];
    CopyFirstLine(Line, sizeof(Line), (char *)"  10.0.0.5:27015 \r\nignored");
    Check(strcmp(Line, "10.0.0.5:27015") == 0);
    CopyFirstLine(Line, 6, (char *)"123456789");
    Check(strcmp(Line, "12345") == 0);

    app_input Input = {};
    Input.ButtonD.EndedDown = true;
    Input.ButtonZ.EndedDown = true;
    Input.RightButton.EndedDown = true;
    u16 Buttons = NetButtonsFromKeyboard(&Input);
    Check(Buttons == (NetButton_Right | NetButton_Up | NetButton_Sword));
}

internal void
TestOnlineSessionStartsOnlyWithAnAddress()
{
    memory_index Size = Megabytes(1);
    memory_arena Arena;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);

    TestSetEnv(ONLINE_ADDRESS_ENV, "");
    online_session *Offline = StartOnlineSession(&Arena);
    Check(!Offline->Enabled);
    Check(!IsOnline(Offline));

    // NOTE(zoubir): nothing listens on port 9, so it stays connecting
    TestSetEnv(ONLINE_ADDRESS_ENV, "127.0.0.1:9");
    online_session *Online = StartOnlineSession(&Arena);
    Check(Online->Enabled);
    Check(Online->Client.State == NetClient_Connecting);
    char Status[64];
    GetOnlineStatusText(Online, Status, sizeof(Status));
    Check(strcmp(Status, "Connecting to 127.0.0.1:9") == 0);
    NetClientDisconnect(&Online->Client);
    TestSetEnv(ONLINE_ADDRESS_ENV, "");
    free(Arena.Base);
}

internal u32
CountMovingEntities(world *World)
{
    u32 Result = 0;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent && IsMovingEntityType(Entity->Type))
        {
            Result++;
        }
    }
    return Result;
}

internal net_entity_state
SnapshotEntity(u16 Id, entity_type Type, float X, float Y, u8 Variant = 0)
{
    net_entity_state Result = {};
    Result.Id = Id;
    Result.Type = (u8)Type;
    Result.Variant = Variant;
    Result.Health = 80;
    Result.X = X;
    Result.Y = Y;
    return Result;
}

// NOTE(zoubir): other units glide to each snapshot over one interval
// instead of jumping; the local player and long jumps do not glide
internal void
TestReplicasGlideBetweenSnapshots()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));
    net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));
    float Dt = 1.f / 60.f;

    Snapshot->Tick = 1;
    Snapshot->Count = 2;
    Snapshot->Entities[0] = SnapshotEntity(5, EntityType_Player, 500, 500, 0);
    Snapshot->Entities[1] = SnapshotEntity(6, EntityType_Player, 800, 500, 1);
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    world_entity *Other =
        &Test.World->Entities[Table->LocalIndexPlusOne[6] - 1];
    Check(Other->Position.X == 800.f);

    // NOTE(zoubir): 3 frames between snapshots, as at 20 Hz and 60 fps
    for(u32 Tick = 2; Tick <= 4; Tick++)
    {
        Snapshot->Tick = Tick;
        SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
        SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
        SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    }
    Snapshot->Tick = 5;
    Snapshot->Entities[0].X = 530.f;
    Snapshot->Entities[1].X = 830.f;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    Check(GetLocalPlayer(AppState)->Position.X == 530.f);
    Check(Other->Position.X > 805.f && Other->Position.X < 815.f);
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    Check(Other->Position.X > 815.f && Other->Position.X < 825.f);
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    Check(Other->Position.X > 829.f && Other->Position.X <= 830.f);
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    Check(Other->Position.X == 830.f);

    // NOTE(zoubir): a respawn across the arena snaps
    Snapshot->Tick = 6;
    Snapshot->Entities[1].X = 830.f + 2.f * REPLICA_SNAP_DISTANCE;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    Check(Other->Position.X == 830.f + 2.f * REPLICA_SNAP_DISTANCE);

    free(Snapshot);
    free(Table);
    DestroyTestWorld(&Test);
}

internal void
TestReplicasFollowSnapshots()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *OfflinePlayer =
        AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    AddTestEntity(&Test, EntityType_Monster, {700, 700, 0}, Test.UnitVolume);
    AddTestEntity(&Test, EntityType_StaticObject, {900, 900, 0},
                  Test.WallVolume);
    replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));

    net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));
    Snapshot->Tick = 1;
    Snapshot->Count = 3;
    Snapshot->Entities[0] = SnapshotEntity(5, EntityType_Player, 500, 500);
    Snapshot->Entities[1] = SnapshotEntity(9, EntityType_Monster, 800, 500,
                                           (u8)MonsterKind_Bat);
    Snapshot->Entities[2] = SnapshotEntity(12, EntityType_FireBall, 600, 500);
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 0);

    // NOTE(zoubir): the offline player and monster are replaced by the
    // three replicas (their entity slots may be reused), the wall stays
    Check(CountMovingEntities(Test.World) == 3);
    Check(Test.World->Entities[OfflinePlayer->ID].Type != EntityType_Player ||
          Test.World->Entities[OfflinePlayer->ID].Position.X == 500.f);
    world_entity *Local = GetLocalPlayer(AppState);
    Check(Local && Local->Position.X == 500.f && Local->Hp == 80.f);
    world_entity *Monster =
        &Test.World->Entities[Table->LocalIndexPlusOne[9] - 1];
    Check(Monster->Type == EntityType_Monster);
    Check(Monster->MonsterKind == MonsterKind_Bat);

    // NOTE(zoubir): next tick the player moves, the monster is gone and the
    // server reused Id 12 for a sword
    Snapshot->Tick = 2;
    Snapshot->Count = 2;
    Snapshot->Entities[0] = SnapshotEntity(5, EntityType_Player, 520, 500);
    Snapshot->Entities[1] = SnapshotEntity(12, EntityType_Sword, 530, 500);
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 0);
    Check(CountMovingEntities(Test.World) == 2);
    Check(!Monster->IsPresent);
    Check(GetLocalPlayer(AppState)->Position.X == 520.f);
    Check(Test.World->Entities[Table->LocalIndexPlusOne[12] - 1].Type ==
          EntityType_Sword);

    // NOTE(zoubir): an unchanged tick does not move anything
    Snapshot->Entities[0].X = 9999.f;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 0);
    Check(GetLocalPlayer(AppState)->Position.X == 520.f);

    LeaveReplicaWorld(AppState, &Test.Arena, Table);
    Check(!Table->Active);
    Check(GetLocalPlayer(AppState)->Position.X ==
          PlayerSpawnPosition(&AppState->World, 0).X);
    free(Snapshot);
    free(Table);
    DestroyTestWorld(&Test);
}

internal void
TestReplicaPlayersAndScoresFillSlots()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));
    net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));

    // NOTE(zoubir): this client is slot 1; slot 0 is someone else
    Snapshot->Tick = 1;
    Snapshot->Count = 2;
    Snapshot->Entities[0] = SnapshotEntity(7, EntityType_Player, 600, 600, 1);
    Snapshot->Entities[1] = SnapshotEntity(8, EntityType_Player, 900, 600, 0);
    Snapshot->ScoreCount = 2;
    Snapshot->Scores[0] = {1, 2, 0, 5};
    Snapshot->Scores[1] = {0, 0, 2, 1};
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 1);

    Check(AppState->LocalPlayerIndex == 1);
    Check(GetLocalPlayer(AppState)->Position.X == 600.f);
    Check(AppState->Players[0].Active);
    Check(AppState->Players[0].Entity->Position.X == 900.f);
    Check(AppState->Players[0].Entity->PlayerIndex == 0);
    Check(AppState->Players[1].Kills == 2 && AppState->Players[1].MonsterKills == 5);
    Check(AppState->Players[0].Deaths == 2);
    u32 Order[MAX_PLAYERS];
    Check(RankPlayers(AppState, Order) == 2);
    Check(Order[0] == 1);

    // NOTE(zoubir): slot 0 leaves the server
    Snapshot->Tick = 2;
    Snapshot->Count = 1;
    Snapshot->ScoreCount = 1;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 1);
    Check(!AppState->Players[0].Active);
    Check(AppState->Players[1].Active);

    LeaveReplicaWorld(AppState, &Test.Arena, Table);
    Check(AppState->LocalPlayerIndex == 0);
    Check(AppState->Players[0].Active && !AppState->Players[1].Active);
    Check(AppState->Players[0].Kills == 0);
    free(Snapshot);
    free(Table);
    DestroyTestWorld(&Test);
}

internal void
TestPlayerNamesFromSnapshotsAndConfig()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));
    net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));
    Snapshot->Tick = 1;
    Snapshot->Count = 2;
    Snapshot->Entities[0] = SnapshotEntity(7, EntityType_Player, 600, 600, 0);
    Snapshot->Entities[1] = SnapshotEntity(8, EntityType_Player, 900, 600, 3);
    Snapshot->ScoreCount = 2;
    Snapshot->Scores[0] = {0, 0, 0, 0};
    Snapshot->Scores[1] = {3, 0, 0, 0};
    Snapshot->NameSlot = 3;
    snprintf(Snapshot->Name, NET_NAME_SIZE, "Mahdi");
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 0);

    char Name[24];
    GetPlayerName(AppState, 3, Name, sizeof(Name));
    Check(strcmp(Name, "Mahdi") == 0);
    GetPlayerName(AppState, 0, Name, sizeof(Name));
    Check(strcmp(Name, "Player 1") == 0);

    // NOTE(zoubir): GAME_NAME wins over server.txt's second line
    TestSetEnv(ONLINE_ADDRESS_ENV, "10.0.0.1:27015");
    TestSetEnv(ONLINE_NAME_ENV, "  Zoubir  ");
    char Address[64], Chosen[NET_NAME_SIZE];
    Check(ReadOnlineConfig(Address, sizeof(Address), Chosen, sizeof(Chosen)));
    Check(strcmp(Address, "10.0.0.1:27015") == 0);
    Check(strcmp(Chosen, "Zoubir") == 0);
    TestSetEnv(ONLINE_ADDRESS_ENV, "");
    TestSetEnv(ONLINE_NAME_ENV, "");
    Check(strcmp(SkipLines((char *)"1.2.3.4:5\r\nName here\n", 1), "Name here\n") == 0);
    Check(SkipLines((char *)"only one line", 1)[0] == 0);

    free(Snapshot);
    free(Table);
    DestroyTestWorld(&Test);
}

internal void
TestReplicasCarryMonsterDetails()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));
    net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));

    // NOTE(zoubir): any kind that has an ability, for the hazard
    u32 Kind = 0;
    while (Kind < MonsterKind_Count && GetMonsterDef((monster_kind)Kind)->AbilityCount == 0)
    {
        Kind++;
    }
    Check(Kind < MonsterKind_Count);

    Snapshot->Tick = 1;
    Snapshot->Count = 2;
    Snapshot->NameSlot = NET_NO_NAME_SLOT;
    Snapshot->Entities[0] = SnapshotEntity(4, EntityType_Monster, 500, 500, (u8)Kind);
    Snapshot->Entities[0].Affix = 3;
    Snapshot->Entities[0].Status = 5; // effects 1 and 3
    Snapshot->Entities[1] = SnapshotEntity(6, EntityType_MonsterHazard, 700, 500, (u8)Kind);
    Snapshot->Entities[1].Ability = 0;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 0);

    world_entity *Monster = &Test.World->Entities[Table->LocalIndexPlusOne[4] - 1];
    Check(Monster->EliteAffix == 3);
    Check(Monster->StatusTimers[1] == 1.5f);
    Check(Monster->StatusTimers[2] == 0.f);
    Check(Monster->StatusTimers[3] == 1.5f);

    Check(Table->LocalIndexPlusOne[6] != 0);
    world_entity *Hazard = &Test.World->Entities[Table->LocalIndexPlusOne[6] - 1];
    Check(Hazard->Type == EntityType_MonsterHazard);
    float Radius = GetMonsterDef((monster_kind)Kind)->Abilities[0].Radius;
    Check(Hazard->Dimensions.X == 2.f * Radius);

    // NOTE(zoubir): an effect wearing off clears its timer
    Snapshot->Tick = 2;
    Snapshot->Entities[0].Status = 1;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 0);
    Check(Monster->StatusTimers[1] == 1.5f && Monster->StatusTimers[3] == 0.f);

    // NOTE(zoubir): a hazard naming an ability this build lacks is skipped
    Snapshot->Tick = 3;
    Snapshot->Entities[1].Ability = 3;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 0);
    Check(Table->LocalIndexPlusOne[6] == 0);

    free(Snapshot);
    free(Table);
    DestroyTestWorld(&Test);
}

internal void
TestPredictionHistory()
{
    prediction_history *History =
        (prediction_history *)calloc(1, sizeof(prediction_history));
    for(u32 Tick = 1; Tick <= 5; Tick++)
    {
        RecordPredictedInput(History, Tick, NetButton_Right, 1.f / 60.f);
    }
    DropAcknowledgedInputs(History, 3);
    Check(History->Count == 2);
    Check(GetPredictedInput(History, 0)->Tick == 4);

    // NOTE(zoubir): a full ring forgets its oldest input
    for(u32 Tick = 6; Tick < 6 + MAX_PREDICTED_INPUTS; Tick++)
    {
        RecordPredictedInput(History, Tick, 0, 1.f / 60.f);
    }
    Check(History->Count == MAX_PREDICTED_INPUTS);
    Check(GetPredictedInput(History, History->Count - 1)->Tick ==
          5 + MAX_PREDICTED_INPUTS);
    Check(GetPredictedInput(History, 0)->Tick == 6);
    free(History);
}

// NOTE(zoubir): a server correction slides the drawn player over a few
// frames instead of snapping, and a teleport-sized one snaps
internal void
TestPredictionBlendsCorrections()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));
    prediction_history *History =
        (prediction_history *)calloc(1, sizeof(prediction_history));
    net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));
    float Dt = 1.f / 60.f;

    Snapshot->Tick = 1;
    Snapshot->Count = 1;
    Snapshot->NameSlot = NET_NO_NAME_SLOT;
    Snapshot->Entities[0] = SnapshotEntity(3, EntityType_Player, 500, 500, 0);
    Snapshot->Entities[0].Health = 100;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    PredictLocalPlayer(AppState, &Test.Arena, History, true, 0, Dt);
    world_entity *Player = GetLocalPlayer(AppState);
    Check(Player->Position.X == 500.f);

    // NOTE(zoubir): the server says 20 units left of where it was drawn
    Snapshot->Tick = 2;
    Snapshot->Entities[0].X = 480.f;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    PredictLocalPlayer(AppState, &Test.Arena, History, true, 0, Dt);
    Check(Player->Position.X > 490.f);
    Check(Player->Position.X < 500.f);
    float Previous = Player->Position.X;
    for(u32 Frame = 0; Frame < 6; Frame++)
    {
        PredictLocalPlayer(AppState, &Test.Arena, History, false, 0, Dt);
        Check(Player->Position.X < Previous);
        Previous = Player->Position.X;
    }
    Check(Player->Position.X < 482.f);
    for(u32 Frame = 0; Frame < 30; Frame++)
    {
        PredictLocalPlayer(AppState, &Test.Arena, History, false, 0, Dt);
    }
    Check(Player->Position.X == 480.f);
    Check(History->DrawError.X == 0.f);

    // NOTE(zoubir): a respawn across the arena snaps at once
    Snapshot->Tick = 3;
    Snapshot->Entities[0].X = 480.f + 10.f * PREDICTION_SNAP_DISTANCE;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    PredictLocalPlayer(AppState, &Test.Arena, History, true, 0, Dt);
    Check(Player->Position.X == 480.f + 10.f * PREDICTION_SNAP_DISTANCE);

    free(Snapshot);
    free(History);
    free(Table);
    DestroyTestWorld(&Test);
}

internal void
TestPredictionMovesNowAndReplaysAfterSnapshot()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));
    prediction_history *History =
        (prediction_history *)calloc(1, sizeof(prediction_history));
    net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));
    float Dt = 1.f / 60.f;

    Snapshot->Tick = 1;
    Snapshot->Count = 1;
    Snapshot->NameSlot = NET_NO_NAME_SLOT;
    Snapshot->Entities[0] = SnapshotEntity(3, EntityType_Player, 500, 500, 0);
    Snapshot->Entities[0].Health = 100;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    world_entity *Player = GetLocalPlayer(AppState);

    // NOTE(zoubir): holding right moves the player on the very first frame
    RecordPredictedInput(History, 1, NetButton_Right, Dt);
    PredictLocalPlayer(AppState, &Test.Arena, History, true, 0, Dt);
    float AfterOne = Player->Position.X;
    Check(AfterOne > 500.f);
    for(u32 Tick = 2; Tick <= 10; Tick++)
    {
        RecordPredictedInput(History, Tick, NetButton_Right, Dt);
        PredictLocalPlayer(AppState, &Test.Arena, History, false, 0, Dt);
    }
    Check(Player->Position.X > AfterOne);
    Check(Player->Position.Y == 500.f);

    // NOTE(zoubir): the server applied every input: the player is exactly
    // where it says
    Snapshot->Tick = 2;
    Snapshot->InputTick = 10;
    Snapshot->Entities[0].X = 530.f;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    PredictLocalPlayer(AppState, &Test.Arena, History, true, 10, Dt);
    Check(History->Count == 0);
    Check(Player->Position.X - History->DrawError.X == 530.f);

    // NOTE(zoubir): five newer inputs it has not applied: the player is
    // ahead of the server's position, as far as five frames carry it
    for(u32 Tick = 11; Tick <= 15; Tick++)
    {
        RecordPredictedInput(History, Tick, NetButton_Right, Dt);
    }
    Snapshot->Tick = 3;
    Snapshot->Entities[0].X = 540.f;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    PredictLocalPlayer(AppState, &Test.Arena, History, true, 10, Dt);
    Check(History->Count == 5);
    float Predicted = Player->Position.X - History->DrawError.X;
    Check(Predicted > 540.f);
    Check(Predicted < 540.f + 5.f * 30.f);

    free(Snapshot);
    free(History);
    free(Table);
    DestroyTestWorld(&Test);
}

internal void
TestReplicaFacingFromSnapshot()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));
    net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));
    Snapshot->Tick = 1;
    Snapshot->Count = 2;
    Snapshot->NameSlot = NET_NO_NAME_SLOT;
    Snapshot->Entities[0] = SnapshotEntity(4, EntityType_Monster, 500, 500, 0);
    Snapshot->Entities[1] = SnapshotEntity(9, EntityType_Monster, 700, 500, 0);
    Snapshot->FacingCount = 2;
    Snapshot->Facings[0] = {0, 64};  // +Y
    Snapshot->Facings[1] = {1, 128}; // -X
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 0);

    world_entity *A = &Test.World->Entities[Table->LocalIndexPlusOne[4] - 1];
    world_entity *B = &Test.World->Entities[Table->LocalIndexPlusOne[9] - 1];
    Check(Absolute(A->Direction.X) < 0.001f && Absolute(A->Direction.Y - 1.f) < 0.001f);
    Check(Absolute(B->Direction.X + 1.f) < 0.001f && Absolute(B->Direction.Y) < 0.001f);
    free(Snapshot);
    free(Table);
    DestroyTestWorld(&Test);
}

internal void
RunOnlineTests()
{
    printf("TestOnlineAddressAndButtons\n");
    TestOnlineAddressAndButtons();
    printf("TestOnlineSessionStartsOnlyWithAnAddress\n");
    TestOnlineSessionStartsOnlyWithAnAddress();
    printf("TestReplicasFollowSnapshots\n");
    TestReplicasFollowSnapshots();
    printf("TestReplicaPlayersAndScoresFillSlots\n");
    TestReplicaPlayersAndScoresFillSlots();
    printf("TestPlayerNamesFromSnapshotsAndConfig\n");
    TestPlayerNamesFromSnapshotsAndConfig();
    printf("TestReplicasCarryMonsterDetails\n");
    TestReplicasCarryMonsterDetails();
    printf("TestPredictionHistory\n");
    TestPredictionHistory();
    printf("TestPredictionMovesNowAndReplaysAfterSnapshot\n");
    TestPredictionMovesNowAndReplaysAfterSnapshot();
    printf("TestPredictionBlendsCorrections\n");
    TestPredictionBlendsCorrections();
    printf("TestReplicasGlideBetweenSnapshots\n");
    TestReplicasGlideBetweenSnapshots();
    printf("TestReplicaFacingFromSnapshot\n");
    TestReplicaFacingFromSnapshot();
}
