/* Determinism and replay tests: the same inputs give the same world, tick
   by tick (sim/world_hash.cpp), with bots fighting, monsters spawning and
   all three time rewinds going off; a match recorded into memory
   (server/replay.cpp) plays back to the same hash on every tick; and a
   recording with one input changed is caught at the tick it changes
   things. Included by server_tests.cpp, which calls RunReplayTests. */

#define REPLAY_TEST_TICKS 1500
#define REPLAY_TEST_DT (1.f / 60.f)

// NOTE: a scripted player: walks in a pattern that changes every second,
// swings, casts, and uses each rewind once, all by the tick number
internal net_input
ScriptedReplayInput(u32 Slot, u32 Tick)
{
    net_input Input = {};
    Input.Tick = Tick + 1;
    u32 Pattern = (Tick / 60 + Slot * 3) % 6;
    u32 Moves[6] = {NetButton_Right, NetButton_Down, NetButton_Left | NetButton_Down,
                    NetButton_Up, 0, NetButton_Right | NetButton_Up};
    u32 Buttons = Moves[Pattern];
    if ((Tick + Slot * 17) % 45 == 0) Buttons |= NetButton_Sword;
    if ((Tick + Slot * 29) % 70 == 0) Buttons |= NetButton_Fireball;
    if ((Tick + Slot * 11) % 130 == 0) Buttons |= NetButton_Dash;
    if (Tick == 300 + Slot * 40) Buttons |= NetButton_RewindSelf;
    if (Tick == 620 + Slot * 50) Buttons |= NetButton_RewindBubble;
    if (Slot == 0 && Tick == 1000) Buttons |= NetButton_RewindWorld;
    Input.Buttons = (u16)Buttons;
    Input.AimX = (Tick % 120 < 60) ? 1.f : -0.6f;
    Input.AimY = (Tick % 90 < 45) ? 0.3f : -0.8f;
    return Input;
}

// NOTE: two scripted players and six bots on the arena, monsters and all
internal void
StepReplayTestGame(server_game *Game, u32 Tick)
{
    if (Tick == 0)
    {
        Game->BotTarget = 6;
        for (u32 Slot = 0; Slot < 2; ++Slot)
        {
            GamePlayerJoined(Game, Slot);
            GamePlayerNamed(Game, Slot, (char *)(Slot ? "Two" : "One"));
        }
    }
    // NOTE: slots 0 and 1 count as connected humans, so bots keep out
    GameKeepBots(Game, 3u, REPLAY_TEST_DT);
    for (u32 Slot = 0; Slot < 2; ++Slot)
    {
        net_input Input = ScriptedReplayInput(Slot, Tick);
        GameApplyInput(Game, Slot, &Input);
    }
    GameTick(Game, REPLAY_TEST_DT);
}

// The same match twice, side by side: every tick the two worlds hash the
// same, through bot fights, deaths, respawns and all three rewinds.
internal void
TestSameInputsGiveTheSameWorld()
{
    static server_game A;
    static server_game B;
    GameInit(&A, MapId_Arena);
    GameInit(&B, MapId_Arena);
    u32 FirstDifference = 0;
    u32 Rewinds = 0;
    bool32 SawWorldFreeze = false;
    for (u32 Tick = 0; Tick < REPLAY_TEST_TICKS; ++Tick)
    {
        StepReplayTestGame(&A, Tick);
        StepReplayTestGame(&B, Tick);
        if (!FirstDifference && HashWorldState(A.AppState) != HashWorldState(B.AppState))
        {
            FirstDifference = Tick + 1;
        }
        for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
        {
            rewind_cast *Cast = &A.AppState->Rewind->Casts[Slot];
            Rewinds += (Cast->Phase == RewindPhase_Hold && Cast->PhaseLeft >
                        REWIND_HOLD_SECONDS - REPLAY_TEST_DT * 1.5f) ? 1 : 0;
        }
        SawWorldFreeze = SawWorldFreeze || A.AppState->Rewind->WorldFrozen;
    }
    if (FirstDifference)
    {
        printf("  determinism: the two runs part at tick %u\n", FirstDifference);
    }
    Check(FirstDifference == 0);
    Check(Rewinds >= 3);
    Check(SawWorldFreeze);
    GameShutdown(&A);
    GameShutdown(&B);
}

// A match recorded into memory replays to the same hash on every tick;
// one input changed in the recording is caught.
internal void
TestReplayPlaysBackTheSameMatch()
{
    static replay_writer Writer;
    static u8 Memory[4 * 1024 * 1024];
    Writer = {};
    Writer.Memory = Memory;
    Writer.Capacity = sizeof(Memory);
    static server_game Recorded;
    GameInit(&Recorded, MapId_Arena);
    GameStartReplay(&Recorded, &Writer);
    for (u32 Tick = 0; Tick < REPLAY_TEST_TICKS; ++Tick)
    {
        StepReplayTestGame(&Recorded, Tick);
    }
    u32 FinalHash = HashWorldState(Recorded.AppState);
    GameShutdown(&Recorded);
    Check(!Writer.Failed);
    Check(Writer.Ticks == REPLAY_TEST_TICKS);
    printf("  replay: %u ticks in %u bytes\n", Writer.Ticks, Writer.Used);

    static server_game Played;
    replay_result Result = PlayReplay(&Played, Memory, Writer.Used);
    Check(Result.Readable);
    Check(Result.Ticks == REPLAY_TEST_TICKS);
    Check(Result.Mismatches == 0);
    Check(Result.FinalHash == FinalHash);
    Check(Result.ContentId == SimContentId());

    // NOTE: turn slot 0's walks around from half way through: the replay
    // must notice, at or after the first changed tick, never before. Every
    // walk from there, not one: slot 0 may be dead for a while, and a
    // single changed walk then changes nothing
    u32 Ticks = 0;
    u32 At = 16;
    u32 Changed = 0;
    while (At < Writer.Used)
    {
        u8 Type = Memory[At];
        u32 Size = Type == ReplayEvent_Tick ? 9 : Type == ReplayEvent_Named ? 18 :
            Type == ReplayEvent_Input ? 16 : 2;
        if (Type == ReplayEvent_Tick) ++Ticks;
        if (Type == ReplayEvent_Input && Memory[At + 1] == 0 && Ticks > REPLAY_TEST_TICKS / 2)
        {
            u16 Buttons;
            memcpy(&Buttons, Memory + At + 6, sizeof(Buttons));
            Buttons ^= (u16)(NetButton_Left | NetButton_Right);
            memcpy(Memory + At + 6, &Buttons, sizeof(Buttons));
            Changed = Changed ? Changed : Ticks + 1;
        }
        At += Size;
    }
    Check(Changed > 0);
    Result = PlayReplay(&Played, Memory, Writer.Used);
    Check(Result.Readable);
    Check(Result.Mismatches > 0);
    Check(Result.FirstMismatch >= Changed);
    // NOTE: a recording cut short is unreadable past the cut, not wrong
    Result = PlayReplay(&Played, Memory, Writer.Used / 2 + 3);
    Check(!Result.Readable || Result.Ticks < REPLAY_TEST_TICKS);
}

// The same through a file, as server --record writes it: a buffer at a
// time, then read back whole and played.
internal void
TestReplayFileRoundTrip()
{
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // fopen
#endif
    const char *Path = "replay_tests.tmp";
    static replay_writer Writer;
    Writer = {};
    Writer.File = fopen(Path, "wb");
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    Check(Writer.File != 0);
    if (!Writer.File)
    {
        return;
    }
    static server_game Recorded;
    GameInit(&Recorded, MapId_Wilds);
    GameStartReplay(&Recorded, &Writer);
    u32 Ticks = 900;
    for (u32 Tick = 0; Tick < Ticks; ++Tick)
    {
        StepReplayTestGame(&Recorded, Tick);
    }
    ReplayFlush(&Writer);
    fclose(Writer.File);
    GameShutdown(&Recorded);
    Check(!Writer.Failed);

    static u8 Memory[4 * 1024 * 1024];
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // fopen
#endif
    FILE *File = fopen(Path, "rb");
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    u32 Size = File ? (u32)fread(Memory, 1, sizeof(Memory), File) : 0;
    if (File)
    {
        fclose(File);
    }
    remove(Path);
    static server_game Played;
    replay_result Result = PlayReplay(&Played, Memory, Size);
    Check(Result.Readable && Result.Ticks == Ticks && Result.Mismatches == 0);
    Check(Result.MapId == MapId_Wilds);
}

internal void
RunReplayTests()
{
    TestSameInputsGiveTheSameWorld();
    TestReplayPlaysBackTheSameMatch();
    TestReplayFileRoundTrip();
}
