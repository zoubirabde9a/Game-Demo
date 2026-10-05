/* Online play tests: the client side of a connection, with no network.
   Server address and name settings, replicas built from hand-made
   snapshots (created, moved, gliding, removed, carrying scores, names,
   monster details and the last hit) and prediction of the local player. Included by
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
    Input.ButtonE.EndedDown = true;
    // NOTE(zoubir): the sword's key is sent whether or not the talent tree
    // has unlocked it; the simulation decides (sim/progression/talents.cpp)
    Input.RightButton.EndedDown = true;
    u32 Buttons = NetButtonsFromKeyboard(&Input);
    Check(Buttons == (NetButton_Right | NetButton_Up | NetButton_Shield | NetButton_Sword));
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
    // NOTE(zoubir): an elite's health bar and tint match the server's
    Check(Monster->MaxHp == GetMonsterDef((monster_kind)Kind)->MaxHp * GetAffix(3)->HpScale);
    Check(Monster->Tint == GetAffix(3)->Tint || GetAffix(3)->Tint == 0);
    Check(Monster->Hp == 80.f);
    // NOTE(zoubir): and only once, not again every snapshot
    float EliteMaxHp = Monster->MaxHp;
    Snapshot->Tick = 100;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 0);
    Check(Monster->MaxHp == EliteMaxHp);
    Snapshot->Tick = 1;
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

// NOTE(zoubir): a shove only the server ran (sim/hit.cpp) reaches the
// client as the player's speed and stagger; the replay slides it out with
// the keys off, as the server does. Without the stagger, keys held the
// other way braked it at once and every shove was pulled back
internal void
TestPredictionReplaysAStagger()
{
    float Dt = 1.f / 60.f;
    float Speeds[2];
    for(u32 Staggered = 0; Staggered < 2; Staggered++)
    {
        test_world Test = CreateTestWorld();
        app_state *AppState = Test.AppState;
        replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));
        prediction_history *History =
            (prediction_history *)calloc(1, sizeof(prediction_history));
        net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));

        Snapshot->Tick = 1;
        Snapshot->Count = 1;
        Snapshot->NameSlot = NET_NO_NAME_SLOT;
        Snapshot->Entities[0] = SnapshotEntity(3, EntityType_Player, 500, 500, 0);
        Snapshot->Entities[0].Health = 100;
        Snapshot->Entities[0].VelX = 250.f;
        Snapshot->Stagger = Staggered ? 255 : 0;
        for(u32 Tick = 1; Tick <= 5; Tick++)
        {
            RecordPredictedInput(History, Tick, NetButton_Left, Dt);
        }
        SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
        PredictLocalPlayer(AppState, &Test.Arena, History, true, 0, Dt);
        world_entity *Player = GetLocalPlayer(AppState);
        Speeds[Staggered] = Player->Velocity.X;
        if (Staggered)
        {
            Check(Absolute(Player->Stagger -
                           (PlayerStats.StaggerSeconds - 5.f * Dt)) < 0.001f);
        }

        free(Snapshot);
        free(History);
        free(Table);
        DestroyTestWorld(&Test);
    }
    printf("  speed after 5 replayed ticks against a shove of 250: "
           "%.0f without the stagger, %.0f with it\n", Speeds[0], Speeds[1]);
    Check(Speeds[0] < 0.f);
    Check(Speeds[1] > 80.f);
}

// NOTE(zoubir): walking the local player through a fireball replica must
// not hurt it on the client: the server decides whether the shot hit, and
// says so in its snapshot. A replica fireball has no owner, so the
// player's own shots used to count as well.
internal void
TestPredictionTakesNoFireballDamage()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));
    prediction_history *History =
        (prediction_history *)calloc(1, sizeof(prediction_history));
    net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));
    float Dt = 1.f / 60.f;

    Snapshot->Tick = 1;
    Snapshot->Count = 2;
    Snapshot->NameSlot = NET_NO_NAME_SLOT;
    Snapshot->Entities[0] = SnapshotEntity(3, EntityType_Player, 500, 500, 0);
    Snapshot->Entities[0].Health = 100;
    Snapshot->Entities[1] = SnapshotEntity(4, EntityType_FireBall, 520, 500);
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    world_entity *Player = GetLocalPlayer(AppState);
    Check(Player->Hp == 100.f);

    RecordPredictedInput(History, 1, NetButton_Right, Dt);
    PredictLocalPlayer(AppState, &Test.Arena, History, true, 0, Dt);
    for(u32 Tick = 2; Tick <= 40; Tick++)
    {
        RecordPredictedInput(History, Tick, NetButton_Right, Dt);
        PredictLocalPlayer(AppState, &Test.Arena, History, false, 0, Dt);
    }
    // NOTE(zoubir): it went through the fireball, which does not block
    Check(Player->Position.X - History->DrawError.X > 540.f);
    Check(Player->Hp == 100.f);

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

// NOTE(zoubir): the respawn countdown runs online too: it starts when a
// snapshot first shows the local player dead and counts down every frame
internal void
TestRespawnCountdownOnline()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));
    net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));
    float Dt = 1.f / 60.f;

    Snapshot->Tick = 1;
    Snapshot->Count = 1;
    Snapshot->NameSlot = NET_NO_NAME_SLOT;
    Snapshot->Entities[0] = SnapshotEntity(5, EntityType_Player, 500, 500, 0);
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    player_slot *Slot = &AppState->Players[0];
    Check(!IsDeadPlayer(Slot->Entity));

    Snapshot->Tick = 2;
    Snapshot->Entities[0].Health = 0;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    Check(IsDeadPlayer(Slot->Entity));
    Check(Slot->RespawnTimer > PLAYER_RESPAWN_SECONDS - 0.1f);
    for(u32 Frame = 0; Frame < 60; Frame++)
    {
        SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    }
    Check(Slot->RespawnTimer < PLAYER_RESPAWN_SECONDS - 0.9f);
    Check(Slot->RespawnTimer > PLAYER_RESPAWN_SECONDS - 1.1f);

    // NOTE(zoubir): a later snapshot of the same death does not restart it
    Snapshot->Tick = 3;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    Check(Slot->RespawnTimer < PLAYER_RESPAWN_SECONDS - 0.9f);

    free(Snapshot);
    free(Table);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a player's death is reported with who did it, and the
// kill feed keeps the newest first and forgets them after a while
internal void
TestKillsReachTheKillFeed()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Attacker = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                             0, {300, 300, 0});
    world_entity *Victim = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {330, 300, 0});
    AppState->Events = {};
    Check(DamageEntity(AppState, Test.World, Victim, Victim->Hp + 1.f, Attacker));
    bool32 Found = false;
    for(u32 Index = 0; Index < AppState->Events.Count; Index++)
    {
        sim_event *Event = &AppState->Events.Events[Index];
        if (Event->Type == SimEvent_Kill && Event->Killer == 0 &&
            Event->Victim == 1 && Event->KillerMonster == SIM_NOBODY)
        {
            Found = true;
        }
    }
    Check(Found);

    kill_feed *Feed = (kill_feed *)calloc(1, sizeof(kill_feed));
    for(u8 Victim = 0; Victim < KILL_FEED_SIZE + 2; Victim++)
    {
        sim_event Kill = {};
        Kill.Type = SimEvent_Kill;
        Kill.Killer = SIM_NOBODY;
        Kill.Victim = Victim;
        Kill.KillerMonster = 3;
        AddToKillFeed(Feed, &Kill);
        AgeKillFeed(Feed, 1.f);
    }
    Check(Feed->Count == KILL_FEED_SIZE);
    Check(Feed->Entries[0].Victim == KILL_FEED_SIZE + 1);
    Check(Feed->Entries[0].Age == 1.f);
    AgeKillFeed(Feed, KILL_FEED_SECONDS - 2.5f);
    Check(Feed->Count == 2);
    AgeKillFeed(Feed, 10.f);
    Check(Feed->Count == 0);
    free(Feed);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the round trip counts the client's own frames, whatever
// its frame rate: 14 inputs behind at 144 fps is about 97 ms, not 233
internal void
TestRoundTripFollowsTheFrameRate()
{
    online_quality Quality = {};
    for(u32 Frame = 0; Frame < 200; Frame++)
    {
        NoteOnlineFrame(&Quality, 1.f / 144.f);
    }
    RecordSnapshotQuality(&Quality, 300, 1014, 1000);
    Check(Quality.RoundTripMs > 90.f && Quality.RoundTripMs < 104.f);

    online_quality Slow = {};
    for(u32 Frame = 0; Frame < 200; Frame++)
    {
        NoteOnlineFrame(&Slow, 1.f / 30.f);
    }
    RecordSnapshotQuality(&Slow, 300, 1003, 1000);
    Check(Slow.RoundTripMs > 95.f && Slow.RoundTripMs < 105.f);

    // NOTE(zoubir): no frame seen yet counts 60 fps
    online_quality Fresh = {};
    RecordSnapshotQuality(&Fresh, 300, 1006, 1000);
    Check(Fresh.RoundTripMs > 99.f && Fresh.RoundTripMs < 101.f);
}

// NOTE(zoubir): snapshot loss without knowing the server's send rate: the
// step is learned from the tick gaps, every 3rd or every 5th tick alike
internal void
TestLossLearnsTheSnapshotStep()
{
    u32 Steps[] = {3, 5};
    for(u32 Case = 0; Case < 2; Case++)
    {
        u32 Step = Steps[Case];
        online_quality Quality = {};
        // NOTE(zoubir): one snapshot in four is lost
        u32 Sent = 0;
        for(u32 Tick = 0; Tick <= 2400; Tick += Step, Sent++)
        {
            if (Sent % 4 == 3) continue;
            RecordSnapshotQuality(&Quality, Tick, 0, 0);
        }
        Check(Quality.SnapshotStep == Step);
        Check(Quality.Loss > 0.2f && Quality.Loss < 0.3f);
    }
}

// NOTE(zoubir): a monster's wind-up from the snapshot reaches its replica,
// so its warning is drawn online; it counts down between snapshots and is
// gone once the snapshot no longer lists it
internal void
TestReplicasShowMonsterWindups()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));
    net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));
    u32 Kind = 0;
    while (Kind < MonsterKind_Count && GetMonsterDef((monster_kind)Kind)->AbilityCount == 0) Kind++;
    Check(Kind < MonsterKind_Count);

    Snapshot->Tick = 1;
    Snapshot->Count = 1;
    Snapshot->NameSlot = NET_NO_NAME_SLOT;
    Snapshot->Entities[0] = SnapshotEntity(4, EntityType_Monster, 500, 500, (u8)Kind);
    Snapshot->AbilityCount = 1;
    net_ability_state *A = &Snapshot->Abilities[0];
    A->EntityIndex = 0;
    A->Phase = AbilityPhase_Windup;
    A->Ability = 0;
    A->TimeLeft = 0.5f;
    A->AimX = 1.f;
    A->PointCount = 2;
    A->PointX[1] = 620.f;
    A->PointY[1] = 480.f;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 0);
    world_entity *Monster = &Test.World->Entities[Table->LocalIndexPlusOne[4] - 1];
    Check(Monster->AbilityPhase == AbilityPhase_Windup);
    Check(Monster->AbilityAim.X == 1.f && Monster->AbilityPointCount == 2);
    Check(Monster->AbilityPoints[1].X == 620.f && Monster->AbilityPoints[1].Y == 480.f);
    float Before = Monster->AbilityTimer;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 0);
    Check(Monster->AbilityTimer < Before);

    Snapshot->Tick = 2;
    Snapshot->AbilityCount = 0;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, 1.f / 60.f, 0);
    Check(Monster->AbilityPhase == AbilityPhase_Ready);

    free(Snapshot);
    free(Table);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a boss's enrage burst reaches its replica: it starts when
// the snapshot's Flash bit comes on, is not restarted while the bit stays
// on, and plays out by itself
internal void
TestEnrageBurstOnline()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));
    net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));
    float Dt = 1.f / 60.f;
    Snapshot->Tick = 1;
    Snapshot->Count = 1;
    Snapshot->NameSlot = NET_NO_NAME_SLOT;
    Snapshot->Entities[0] = SnapshotEntity(4, EntityType_Monster, 500, 500, 0);
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    world_entity *Monster = &Test.World->Entities[Table->LocalIndexPlusOne[4] - 1];
    Check(Monster->PhaseFlash == 0.f);

    Snapshot->Tick = 2;
    Snapshot->Entities[0].Flash = 1;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    Check(Monster->PhaseFlash > ENRAGE_FLASH_SECONDS - 0.05f);
    float Started = Monster->PhaseFlash;
    for(u32 Frame = 0; Frame < 6; Frame++) SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    Snapshot->Tick = 3;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    Check(Monster->PhaseFlash < Started - 0.05f);
    for(u32 Frame = 0; Frame < 60; Frame++) SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    Check(Monster->PhaseFlash == 0.f);

    free(Snapshot);
    free(Table);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a hit's data from the snapshot drives the body: the
// flinch goes the way the hit threw it, a hit-pause holds it white and
// still, it tumbles only when the hit lifted it, and a solid hit the
// local player landed freezes the player an instant and nudges the camera
internal void
TestHitsReadFromSnapshots()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->BodyPoses = (body_poses *)calloc(1, sizeof(body_poses));
    replica_table *Table = (replica_table *)calloc(1, sizeof(replica_table));
    net_snapshot *Snapshot = (net_snapshot *)calloc(1, sizeof(net_snapshot));
    float Dt = 1.f / 60.f;

    Snapshot->Tick = 1;
    Snapshot->Count = 2;
    Snapshot->NameSlot = NET_NO_NAME_SLOT;
    Snapshot->Entities[0] = SnapshotEntity(5, EntityType_Player, 500, 500, 0);
    Snapshot->Entities[1] = SnapshotEntity(4, EntityType_Monster, 600, 500, 0);
    // NOTE(zoubir): facing right, so the old guess would tip it left, back
    // from where it faces
    Snapshot->Entities[1].Facing = AnimationDirection_Right;
    u32 Tick = 1;
    for(u32 Frame = 0; Frame < 6; Frame++)
    {
        Snapshot->Tick = Tick++;
        SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
        UpdateBodyPoses(AppState, Dt);
    }
    world_entity *Monster = &Test.World->Entities[Table->LocalIndexPlusOne[4] - 1];
    world_entity *Player = GetLocalPlayer(AppState);
    Check(Player && !IsBodyFrozen(AppState, Monster));

    // NOTE(zoubir): the local player's sword throws it right (angle 0)
    // and pauses it 60 ms, in the air but not lifted
    net_entity_state *Hit = &Snapshot->Entities[1];
    Hit->Health = 60;
    Hit->Z = 20.f;
    Hit->Hit = 1;
    Hit->HitStop = 60;
    Hit->HitAngle = 0;
    Hit->HitBy = 1;
    Snapshot->Tick = Tick++;
    SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
    UpdateBodyPoses(AppState, Dt);
    Check(Monster->HitFresh > 0.f && Monster->HitBySlot == 1 && !Monster->HitThrown);
    Check(Monster->HitStop > 0.04f && Monster->HitStop < 0.06f);
    Check(IsBodyFrozen(AppState, Monster));
    body_pose_draw Pose = GetBodyPose(AppState, Monster);
    Check(Pose.White == 1.f && Pose.Angle > 0.05f);
    // NOTE(zoubir): the player's pose may come before the monster's in
    // the frame, so its freeze shows from the next
    UpdateBodyPoses(AppState, Dt);
    Check(IsBodyFrozen(AppState, Player));
    Check(GetHitNudge(AppState).X > 0.5f);

    // NOTE(zoubir): the pause runs out between snapshots; still white
    // until it does, never tumbling, as the hit did not lift it
    Hit->Hit = 0;
    Hit->HitStop = 0;
    u32 Frozen = 0;
    for(u32 Frame = 0; Frame < 20; Frame++)
    {
        SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
        UpdateBodyPoses(AppState, Dt);
        Frozen += IsBodyFrozen(AppState, Monster) ? 1 : 0;
    }
    Check(Frozen >= 1 && Frozen <= 3);
    Check(!IsBodyFrozen(AppState, Player));
    Check(GetHitNudge(AppState).X == 0.f);
    Check(Absolute(GetBodyPose(AppState, Monster).Angle) < 0.1f);

    // NOTE(zoubir): a hit that lifts it tumbles it, even with no pause
    Hit->Health = 40;
    Hit->Hit = 1;
    Hit->HitThrown = 1;
    Hit->HitBy = 0;
    Hit->Z = 30.f;
    Snapshot->Tick = Tick++;
    for(u32 Frame = 0; Frame < 10; Frame++)
    {
        SyncReplicas(AppState, &Test.Arena, Table, Snapshot, Dt, 0);
        UpdateBodyPoses(AppState, Dt);
    }
    Check(GetBodyPose(AppState, Monster).Angle > 0.2f);

    free(Snapshot);
    free(Table);
    free(AppState->BodyPoses);
    AppState->BodyPoses = 0;
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
    printf("TestPredictionReplaysAStagger\n");
    TestPredictionReplaysAStagger();
    printf("TestPredictionTakesNoFireballDamage\n");
    TestPredictionTakesNoFireballDamage();
    printf("TestPredictionBlendsCorrections\n");
    TestPredictionBlendsCorrections();
    printf("TestReplicasGlideBetweenSnapshots\n");
    TestReplicasGlideBetweenSnapshots();
    printf("TestReplicaFacingFromSnapshot\n");
    TestReplicaFacingFromSnapshot();
    printf("TestRespawnCountdownOnline\n");
    TestRespawnCountdownOnline();
    printf("TestKillsReachTheKillFeed\n");
    TestKillsReachTheKillFeed();
    printf("TestRoundTripFollowsTheFrameRate\n");
    TestRoundTripFollowsTheFrameRate();
    printf("TestLossLearnsTheSnapshotStep\n");
    TestLossLearnsTheSnapshotStep();
    printf("TestReplicasShowMonsterWindups\n");
    TestReplicasShowMonsterWindups();
    printf("TestEnrageBurstOnline\n");
    TestEnrageBurstOnline();
    printf("TestHitsReadFromSnapshots\n");
    TestHitsReadFromSnapshots();
}
