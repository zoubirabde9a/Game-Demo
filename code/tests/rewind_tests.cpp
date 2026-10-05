/* Time rewind tests: the three rewinds (sim/time_rewind/) run through the
   server's game, as they do online. Self takes the caster back two
   seconds, health and all, and leaves its cooldown spent; what a rewind
   froze takes no hits; a bubble takes what is inside it and nothing else;
   a world rewind brings back the dead, stops everything while it holds
   and ends other rewinds; a stun ends a cast; the snapshot says which
   entities a rewind froze; and the history keeps its frames in order.
   Included by server_tests.cpp, which calls RunRewindTests. */

#define REWIND_TEST_DT (1.f / 60.f)

// NOTE: the arena with no monsters, so nothing bumps into the players
internal app_state *
StartQuietRewindGame(server_game *Game)
{
    GameInit(Game, MapId_Arena);
    app_state *AppState = Game->AppState;
    AppState->Monsters->Target = 0;
    world *World = &AppState->World;
    for (u32 Index = 0; Index < World->EntityCount; ++Index)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster)
        {
            RemoveEntity(World, Entity);
        }
    }
    // NOTE: the first tick makes the history (SimulateTick)
    GameTick(Game, REWIND_TEST_DT);
    return AppState;
}

internal void
PlaceForRewindTest(server_game *Game, world_entity *Entity, v2 XY)
{
    v3 Old = Entity->Position;
    Entity->Position.XY = XY;
    CheckAndChangeEntityChunk(Game->AppState, &Game->AppState->World, Game->Arena,
                              Old, Entity);
}

internal void
HoldForRewindTest(server_game *Game, u32 Slot, u16 Buttons)
{
    static u32 Tick;
    net_input Input = {};
    Input.Tick = ++Tick;
    Input.Buttons = Buttons;
    Input.AimX = 1.f;
    GameApplyInput(Game, Slot, &Input);
}

inline rewind_cast *
RewindCastOf(server_game *Game, u32 Slot)
{
    return &Game->AppState->Rewind->Casts[Slot];
}

// The caster runs back along its own path to where it was two seconds
// before the freeze, with the health it had then, and its cooldown stays
// spent. While frozen it does not move and takes no hits.
internal void
TestSelfRewindGoesBackTwoSeconds()
{
    static server_game Game;
    app_state *AppState = StartQuietRewindGame(&Game);
    GamePlayerJoined(&Game, 0);
    world_entity *Player = AppState->Players[0].Entity;
    PlaceForRewindTest(&Game, Player, V2(30.f * 32.f, 20.f * 32.f));

    static v2 Positions[600];
    static float Health[600];
    u32 Tick = 0;
    u32 HoldTick = 0;
    for (; Tick < 600; ++Tick)
    {
        // NOTE: walking right, then down, so the way back has a corner
        u16 Buttons = (u16)(Tick < 100 ? NetButton_Right : NetButton_Down);
        if (Tick == 150) Buttons = (u16)(Buttons | NetButton_RewindSelf);
        HoldForRewindTest(&Game, 0, Buttons);
        if (Tick == 120)
        {
            DamageEntity(AppState, &AppState->World, Player, 30.f, 0);
        }
        GameTick(&Game, REWIND_TEST_DT);
        Positions[Tick] = Player->Position.XY;
        Health[Tick] = Player->Hp;
        rewind_cast *Cast = RewindCastOf(&Game, 0);
        if (!HoldTick && Cast->Phase == RewindPhase_Hold)
        {
            HoldTick = Tick;
            Check(Cast->AffectedCount == 1 && Cast->AffectedID[0] == Player->ID);
        }
        if (HoldTick && Cast->Phase == RewindPhase_None)
        {
            break;
        }
    }
    Check(HoldTick > 150 && HoldTick <= 182);
    if (!HoldTick || Tick >= 600)
    {
        GameShutdown(&Game);
        return;
    }
    // NOTE: frozen through the hold, on the spot it was when it began
    Check(Positions[HoldTick + 25].X == Positions[HoldTick - 1].X ||
          Length(Positions[HoldTick + 25] - Positions[HoldTick]) < 0.01f);
    // NOTE: the landing is the frame two seconds before the hold, which
    // the history took within one record of that tick
    v2 Expected = Positions[HoldTick - 120];
    float Error = Length(Player->Position.XY - Expected);
    printf("  self rewind: back %.0f units, %.1f from where it was 2 s before\n",
           Length(Positions[HoldTick] - Player->Position.XY), Error);
    Check(Length(Positions[HoldTick] - Expected) > 100.f);
    // NOTE: at most a tick off: frames are taken every other tick
    Check(Error < 6.f);
    Check(Health[HoldTick] == Player->MaxHp - 30.f);
    Check(Player->Hp == Player->MaxHp);
    Check(Player->RewindCooldowns[RewindKind_Self] > 0.f);
    Check(!IsTimeLocked(AppState, Player));
    // NOTE: and it walks again
    v2 Before = Player->Position.XY;
    for (u32 Step = 0; Step < 20; ++Step)
    {
        HoldForRewindTest(&Game, 0, NetButton_Down);
        GameTick(&Game, REWIND_TEST_DT);
    }
    Check(Length(Player->Position.XY - Before) > 20.f);
    GameShutdown(&Game);
}

// What a rewind froze is outside time: hits neither hurt nor shove it.
internal void
TestFrozenTakesNoHits()
{
    static server_game Game;
    app_state *AppState = StartQuietRewindGame(&Game);
    GamePlayerJoined(&Game, 0);
    world_entity *Player = AppState->Players[0].Entity;
    for (u32 Tick = 0; Tick < 200 && RewindCastOf(&Game, 0)->Phase != RewindPhase_Hold; ++Tick)
    {
        HoldForRewindTest(&Game, 0, (u16)(Tick == 60 ? NetButton_RewindSelf : 0));
        GameTick(&Game, REWIND_TEST_DT);
    }
    Check(RewindCastOf(&Game, 0)->Phase == RewindPhase_Hold);
    Check(IsTimeLocked(AppState, Player));
    float Hp = Player->Hp;
    Check(!DamageEntity(AppState, &AppState->World, Player, 500.f, 0));
    hit Shove = {10.f, 900.f, 300.f, 0.f, 1.f, SimBurst_Count};
    ApplyHit(AppState, &AppState->World, Player, &Shove, V2(1.f, 0.f), 0, SIM_NOBODY);
    Check(Player->Hp == Hp);
    Check(Player->Velocity.X == 0.f && Player->Velocity.Z == 0.f);
    Check(!HasStatus(Player, StatusEffect_Stunned));
    GameShutdown(&Game);
}

// A bubble takes what is inside it when it closes: the player inside
// goes back, the player outside never stops, and a monster that came
// into being inside it since the moment it goes back to is gone again.
internal void
TestBubbleTakesWhatIsInside()
{
    static server_game Game;
    app_state *AppState = StartQuietRewindGame(&Game);
    world *World = &AppState->World;
    GamePlayerJoined(&Game, 0);
    GamePlayerJoined(&Game, 1);
    GamePlayerJoined(&Game, 2);
    world_entity *Caster = AppState->Players[0].Entity;
    world_entity *Inside = AppState->Players[1].Entity;
    world_entity *Outside = AppState->Players[2].Entity;
    v2 Centre = V2(40.f * 32.f, 20.f * 32.f);
    PlaceForRewindTest(&Game, Caster, Centre);
    PlaceForRewindTest(&Game, Inside, Centre + V2(-60.f, -40.f));
    PlaceForRewindTest(&Game, Outside, Centre + V2(-600.f, -100.f));

    static v2 InsideAt[400];
    world_entity *Newcomer = 0;
    u32 NewcomerID = 0;
    u32 HoldTick = 0;
    u32 Tick = 0;
    bool32 OutsideStopped = false;
    for (; Tick < 400; ++Tick)
    {
        HoldForRewindTest(&Game, 0, (u16)(Tick == 150 ? NetButton_RewindBubble : 0));
        // NOTE: up and down, so it stays inside the bubble
        HoldForRewindTest(&Game, 1, (u16)(((Tick / 40) & 1) ? NetButton_Up : NetButton_Down));
        HoldForRewindTest(&Game, 2, (u16)(((Tick / 50) & 1) ? NetButton_Left : NetButton_Right));
        if (Tick == 140)
        {
            Newcomer = AddMonster(AppState, World, Game.Arena,
                                  V3(Centre.X + 40.f, Centre.Y + 50.f, 0.f), (monster_kind)0);
            NewcomerID = Newcomer->ID;
        }
        v2 OutsideBefore = Outside->Position.XY;
        GameTick(&Game, REWIND_TEST_DT);
        InsideAt[Tick] = Inside->Position.XY;
        rewind_cast *Cast = RewindCastOf(&Game, 0);
        if (Cast->Phase >= RewindPhase_Hold &&
            LengthSq(Outside->Position.XY - OutsideBefore) == 0.f)
        {
            OutsideStopped = true;
        }
        if (!HoldTick && Cast->Phase == RewindPhase_Hold)
        {
            HoldTick = Tick;
            Check(IsTimeLocked(AppState, Inside));
            Check(IsTimeLocked(AppState, Caster));
            Check(!IsTimeLocked(AppState, Outside));
            Check(Newcomer->IsPresent && IsTimeLocked(AppState, Newcomer));
        }
        if (HoldTick && Cast->Phase == RewindPhase_None)
        {
            break;
        }
    }
    Check(HoldTick > 0 && Tick < 400);
    if (HoldTick && Tick < 400)
    {
        Check(Length(Inside->Position.XY - InsideAt[HoldTick - 120]) < 6.f);
        Check(!OutsideStopped);
        // NOTE: the monster was made 10 ticks before the cast, well after
        // the two seconds back, so the bubble unmade it
        world_entity *Slot = &World->Entities[NewcomerID];
        Check(!Slot->IsPresent || Slot->Type != EntityType_Monster ||
              Slot->RewindSerial != Newcomer->RewindSerial);
    }
    GameShutdown(&Game);
}

// A world rewind freezes everything while it holds, brings back a monster
// killed since the moment it goes back to, sets the clock back, and ends
// another player's rewind that was under way.
internal void
TestWorldRewindBringsBackTheDead()
{
    static server_game Game;
    app_state *AppState = StartQuietRewindGame(&Game);
    world *World = &AppState->World;
    GamePlayerJoined(&Game, 0);
    GamePlayerJoined(&Game, 1);
    world_entity *Far = AppState->Players[1].Entity;
    world_entity *Monster = AddMonster(AppState, World, Game.Arena,
                                       AppState->Players[0].Entity->Position +
                                       V3(300.f, 0.f, 0.f), (monster_kind)0);
    u32 MonsterID = Monster->ID;
    u32 MonsterSerial = 0;
    u32 HoldTick = 0;
    bool32 FarMovedInHold = false;
    bool32 OtherEnded = false;
    float HoldClock = 0.f;
    u32 Tick = 0;
    for (; Tick < 500; ++Tick)
    {
        u16 Buttons = 0;
        if (Tick == 200) Buttons = (u16)NetButton_RewindWorld;
        HoldForRewindTest(&Game, 0, Buttons);
        u16 FarButtons = (u16)(((Tick / 30) & 1) ? NetButton_Left : NetButton_Right);
        if (Tick == 210) FarButtons = (u16)(FarButtons | NetButton_RewindSelf);
        HoldForRewindTest(&Game, 1, FarButtons);
        if (Tick == 150)
        {
            MonsterSerial = Monster->RewindSerial;
            DamageEntity(AppState, World, Monster, 100000.f, 0);
            Check(!Monster->IsPresent);
        }
        v2 FarBefore = Far->Position.XY;
        GameTick(&Game, REWIND_TEST_DT);
        rewind_cast *Cast = RewindCastOf(&Game, 0);
        if (Cast->Phase >= RewindPhase_Hold)
        {
            if (!HoldTick)
            {
                HoldTick = Tick;
                HoldClock = Cast->HoldClock;
                OtherEnded = RewindCastOf(&Game, 1)->Phase == RewindPhase_None;
                Check(AppState->Rewind->WorldFrozen);
            }
            else if (Cast->Phase == RewindPhase_Hold &&
                     LengthSq(Far->Position.XY - FarBefore) > 0.f)
            {
                FarMovedInHold = true;
            }
        }
        if (HoldTick && Cast->Phase == RewindPhase_None)
        {
            break;
        }
    }
    Check(HoldTick > 0 && Tick < 500);
    Check(!FarMovedInHold);
    Check(OtherEnded);
    Check(!AppState->Rewind->WorldFrozen);
    world_entity *Back = &World->Entities[MonsterID];
    Check(Back->IsPresent && Back->Type == EntityType_Monster &&
          Back->RewindSerial == MonsterSerial);
    Check(Back->Hp > 0.f);
    // NOTE: the clock went back with the world, and what came after the
    // landing frame is gone from the history
    float Clock = AppState->Rewind->Clock;
    Check(Clock <= HoldClock - REWIND_SECONDS + REWIND_RECORD_INTERVAL + 0.001f);
    rewind_frame *Newest = GetRewindFrame(AppState->Rewind, AppState->Rewind->FrameCount - 1);
    Check(Newest->Time <= Clock + 0.0001f);
    // NOTE: its own cooldown stays spent; the other player's too
    Check(AppState->Players[0].Entity->RewindCooldowns[RewindKind_World] > 0.f);
    Check(Far->RewindCooldowns[RewindKind_Self] > 0.f);
    GameShutdown(&Game);
}

// A stun during the cast ends it; the cooldown stays spent.
internal void
TestStunEndsTheCast()
{
    static server_game Game;
    app_state *AppState = StartQuietRewindGame(&Game);
    GamePlayerJoined(&Game, 0);
    world_entity *Player = AppState->Players[0].Entity;
    for (u32 Tick = 0; Tick < 80; ++Tick)
    {
        HoldForRewindTest(&Game, 0, (u16)(Tick == 60 ? NetButton_RewindBubble : 0));
        GameTick(&Game, REWIND_TEST_DT);
    }
    Check(RewindCastOf(&Game, 0)->Phase == RewindPhase_Cast);
    ApplyStatus(Player, StatusEffect_Stunned, 1.f);
    GameTick(&Game, REWIND_TEST_DT);
    Check(RewindCastOf(&Game, 0)->Phase == RewindPhase_None);
    Check(Player->RewindCooldowns[RewindKind_Bubble] > 0.f);
    Check(!IsTimeLocked(AppState, Player));
    GameShutdown(&Game);
}

// The snapshot carries the rewind, with the caster's own entity marked
// frozen, and it survives the wire.
internal void
TestSnapshotSaysWhatIsFrozen()
{
    static server_game Game;
    StartQuietRewindGame(&Game);
    GamePlayerJoined(&Game, 0);
    for (u32 Tick = 0; Tick < 200 && RewindCastOf(&Game, 0)->Phase != RewindPhase_Hold; ++Tick)
    {
        HoldForRewindTest(&Game, 0, (u16)(Tick == 60 ? NetButton_RewindSelf : 0));
        GameTick(&Game, REWIND_TEST_DT);
    }
    static net_packet Packet;
    Packet = {};
    Packet.Header.Type = NetPacket_Snapshot;
    GameWriteSnapshot(&Game, 0, &Packet.Snapshot);
    Check(Packet.Snapshot.RewindCount == 1);
    net_rewind *Sent = &Packet.Snapshot.Rewinds[0];
    Check(Sent->Slot == 0 && Sent->Kind == RewindKind_Self && Sent->Phase == RewindPhase_Hold);
    // NOTE: the viewer's own player is the snapshot's first entity
    Check(Sent->Frozen[0] & 1);
    static u8 Buffer[NET_MAX_PACKET_SIZE];
    u32 Size = NetWritePacket(&Packet, Buffer, sizeof(Buffer));
    static net_packet Read;
    Check(Size > 0 && NetReadPacket(Buffer, Size, &Read));
    Check(Read.Snapshot.RewindCount == 1 && (Read.Snapshot.Rewinds[0].Frozen[0] & 1));
    Check(Read.Snapshot.Rewinds[0].PhaseLeft > 0.f &&
          Read.Snapshot.Rewinds[0].PhaseLeft <= REWIND_HOLD_SECONDS + 0.01f);
    GameShutdown(&Game);
}

// With eight bots fighting for a while the history stays a run of frames
// in time order, each record a present entity it really took, and the
// frame found for a moment is the nearest one to it.
internal void
TestHistoryStaysInOrder()
{
    static server_game Game;
    GameInit(&Game, MapId_Arena);
    Game.BotTarget = 8;
    for (u32 Tick = 0; Tick < 900; ++Tick)
    {
        GameKeepBots(&Game, 0, REWIND_TEST_DT);
        GameTick(&Game, REWIND_TEST_DT);
    }
    time_rewind *Rewind = Game.AppState->Rewind;
    Check(Rewind->FrameCount > 90);
    float Previous = -1.f;
    bool32 Ordered = true;
    bool32 Whole = true;
    for (u32 Age = 0; Age < Rewind->FrameCount; ++Age)
    {
        rewind_frame *Frame = GetRewindFrame(Rewind, Age);
        Ordered = Ordered && Frame->Time > Previous;
        Previous = Frame->Time;
        Whole = Whole && Frame->FirstRecord + Frame->RecordCount <= Rewind->RecordCapacity;
        for (u32 Index = 0; Index < Frame->RecordCount; ++Index)
        {
            world_entity *Record = &Rewind->Records[Frame->FirstRecord + Index];
            Whole = Whole && Record->IsPresent && Record->RewindSerial != 0;
        }
    }
    Check(Ordered);
    Check(Whole);
    float Span = Previous - GetRewindFrame(Rewind, 0)->Time;
    printf("  rewind history: %u frames over %.2f s\n", Rewind->FrameCount, Span);
    Check(Span >= REWIND_SECONDS + REWIND_CAST_SECONDS + REWIND_HOLD_SECONDS);
    float Moment = Rewind->Clock - 1.f;
    rewind_frame *Found = FindRewindFrame(Rewind, Moment);
    Check(Found && Absolute(Found->Time - Moment) <= 0.5f * REWIND_RECORD_INTERVAL + 0.001f);
    GameShutdown(&Game);
}

// Online, a real client casts a self rewind while holding a walk key: once
// it sees the hold its own player stops where the server froze it (no
// prediction walks it on), the playback runs, and afterwards prediction
// and the server agree again.
internal void
TestOnlineRewindFreezesThePlayer()
{
    static server Server;
    Check(ServerStart(&Server, 0));
    ClearMonsters(&Server);

    app_state *Client = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(48);
    memory_arena Arena, Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    InitSimulation(Client, &Arena, &Constants);
    AddPlayerToSlot(Client, &Client->World, &Arena, 0, PlayerSpawnPosition(&Client->World, 0));
    // NOTE: the game makes it in MemoryArena, which this client has not
    Client->RewindFx = (rewind_fx *)calloc(1, sizeof(rewind_fx));
    char Address[32];
    snprintf(Address, sizeof(Address), "127.0.0.1:%u", NetSocketPort(&Server.Socket));
    SetEnvironment(ONLINE_ADDRESS_ENV, Address);
    Client->Online = StartOnlineSession(&Arena);
    SetEnvironment(ONLINE_ADDRESS_ENV, "");

    app_input Input = {};
    Input.DeltaTime = 1.0f / SERVER_TICK_RATE;
    #define REWIND_FRAME() do { UpdateOnlineSession(Client->Online, &Input); \
                                RunWorldTick(Client, &Arena, Input.DeltaTime); \
                                UpdateRewindFx(Client, Input.DeltaTime); \
                                ServerTick(&Server); } while (0)
    for (int Frame = 0; Frame < 120 && !IsOnline(Client->Online); ++Frame) REWIND_FRAME();
    Check(IsOnline(Client->Online));
    for (int Frame = 0; Frame < 20; ++Frame) REWIND_FRAME();

    // NOTE: walking left the whole time; T pressed after a second
    Input.ButtonQ.EndedDown = true;
    for (int Frame = 0; Frame < 60; ++Frame) REWIND_FRAME();
    Input.ButtonT.EndedDown = true;
    REWIND_FRAME();
    Input.ButtonT.EndedDown = false;

    rewind_fx_cast *Seen = &Client->RewindFx->Casts[0];
    world_entity *Authority = Server.Game.AppState->Players[0].Entity;
    bool32 SawLocked = false;
    bool32 SawPlayback = false;
    float FrozenX = 0.f;
    float WorstDrift = 0.f;
    u32 LockedFrames = 0;
    for (int Frame = 0; Frame < 150; ++Frame)
    {
        REWIND_FRAME();
        world_entity *Predicted = GetLocalPlayer(Client);
        if (Seen->Active && Seen->Phase == RewindPhase_Hold && IsLocalPlayerTimeLocked(Client))
        {
            if (!SawLocked)
            {
                FrozenX = Predicted->Position.X;
            }
            SawLocked = true;
            LockedFrames++;
            WorstDrift = Maximum(WorstDrift, Absolute(Predicted->Position.X - FrozenX));
        }
        SawPlayback = SawPlayback || (Seen->Active && Seen->Phase == RewindPhase_Playback);
    }
    printf("  online rewind: frozen %u frames, drifted %.2f with the key held\n",
           LockedFrames, WorstDrift);
    Check(SawLocked && LockedFrames >= 20);
    Check(WorstDrift < 1.f);
    Check(SawPlayback);
    Check(!IsLocalPlayerTimeLocked(Client));
    // NOTE: walking again, and in step with the server once it stops
    Input.ButtonQ.EndedDown = false;
    for (int Frame = 0; Frame < 60; ++Frame) REWIND_FRAME();
    #undef REWIND_FRAME
    world_entity *Predicted = GetLocalPlayer(Client);
    Check(Absolute(Predicted->Position.X - Authority->Position.X) < 1.f);
    Check(Absolute(Predicted->Position.Y - Authority->Position.Y) < 1.f);

    NetClientDisconnect(&Client->Online->Client);
    ServerStop(&Server);
    free(Client->RewindFx);
    free(Arena.Base);
    free(Constants.Base);
    free(Client);
}

internal void
RunRewindTests()
{
    TestOnlineRewindFreezesThePlayer();
    TestSelfRewindGoesBackTwoSeconds();
    TestFrozenTakesNoHits();
    TestBubbleTakesWhatIsInside();
    TestWorldRewindBringsBackTheDead();
    TestStunEndsTheCast();
    TestSnapshotSaysWhatIsFrozen();
    TestHistoryStaysInOrder();
}
