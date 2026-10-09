/* Determinism and replay tests: the same inputs give the same world, tick
   by tick (sim/world_hash.cpp), with bots fighting, monsters spawning and
   all three time rewinds going off; a match recorded into memory
   (server/replay.cpp) plays back to the same hash on every tick; and a
   recording with its inputs changed is caught at the tick they change
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
    // NOTE: three tries, a second and a half apart, in case the first finds
    // the player stunned or frozen by a bot's spell; the cooldown keeps
    // it to one world rewind
    if (Slot == 0 && (Tick == 1000 || Tick == 1090 || Tick == 1180))
    {
        Buttons |= NetButton_RewindWorld;
    }
    Input.Buttons = Buttons;
    Input.AimX = (Tick % 120 < 60) ? 1.f : -0.6f;
    Input.AimY = (Tick % 90 < 45) ? 0.3f : -0.8f;
    return Input;
}

// NOTE: two scripted players and six bots on the arena, monsters and all;
// one of the players' inputs is sometimes late
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
        // NOTE: slot 1's input is late every seventh tick, so the server
        // holds it (server/input_queue.cpp) and gets it next tick
        if (Slot == 1 && Tick % 7 == 3)
        {
            GameHoldPlayer(Game, Slot);
            continue;
        }
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

// NOTE: a writer is 2 MB of LZMA scratch and buffers; cleared in place,
// since "Writer = {}" would build the empty one on the stack first
internal void
ClearReplayWriter(replay_writer *Writer)
{
    memset(Writer, 0, sizeof(*Writer));
}

// A match recorded into memory replays to the same hash on every tick,
// small (the lean layout, packed); one input changed in the recording is
// caught.
internal void
TestReplayPlaysBackTheSameMatch()
{
    static replay_writer Writer;
    static u8 Memory[4 * 1024 * 1024];
    ClearReplayWriter(&Writer);
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
    ReplayFlush(&Writer);
    Check(!Writer.Failed && !Writer.Full);
    Check(Writer.Ticks == REPLAY_TEST_TICKS);
    float PerHour = (float)Writer.Used * (3600.f * 60.f / (float)REPLAY_TEST_TICKS);
    printf("  replay: %u ticks in %u bytes, %.2f MB an hour at this pace\n",
           Writer.Ticks, Writer.Used, PerHour / (1024.f * 1024.f));
    // NOTE: eight players fighting; the old uncompressed layout made 29.6
    // MB an hour, this one about 4.6
    Check(PerHour < 8.f * 1024.f * 1024.f);

    static server_game Played;
    replay_result Result = PlayReplay(&Played, Memory, Writer.Used);
    Check(Result.Readable);
    Check(Result.Ticks == REPLAY_TEST_TICKS);
    Check(Result.Mismatches == 0);
    Check(Result.FinalHash == FinalHash);
    Check(Result.ContentId == SimContentId());

    // NOTE: the same recording read back and written again with slot 0's
    // walks turned around from half way: the replay must notice, at or
    // after the first changed tick, never before. Every walk from there,
    // not one: slot 0 may be dead for a while, and one walk then changes
    // nothing
    static replay_reader Reader;
    static replay_writer Changed;
    static u8 ChangedMemory[4 * 1024 * 1024];
    ClearReplayWriter(&Changed);
    Changed.Memory = ChangedMemory;
    Changed.Capacity = sizeof(ChangedMemory);
    Check(ReplayOpen(&Reader, Memory, Writer.Used));
    ReplayWriteHeader(&Changed, Reader.ContentId, Reader.MapId);
    replay_event Event;
    u32 Ticks = 0;
    u32 FirstChanged = 0;
    while (ReplayNextEvent(&Reader, &Event))
    {
        Ticks += Event.Type == ReplayEvent_Tick ? 1 : 0;
        if (Event.Type == ReplayEvent_Input && Event.Slot == 0 && Ticks > REPLAY_TEST_TICKS / 2 &&
            (Event.Input.Buttons & (NetButton_Left | NetButton_Right)))
        {
            Event.Input.Buttons ^= (u16)(NetButton_Left | NetButton_Right);
            FirstChanged = FirstChanged ? FirstChanged : Ticks + 1;
        }
        ReplayWriteEvent(&Changed, &Event);
    }
    ReplayFlush(&Changed);
    Check(!Reader.Failed && Ticks == REPLAY_TEST_TICKS);
    Check(FirstChanged > 0);
    Result = PlayReplay(&Played, ChangedMemory, Changed.Used);
    Check(Result.Readable);
    Check(Result.Mismatches > 0);
    Check(Result.FirstMismatch >= FirstChanged);
    // NOTE: the held ticks are part of the match: without them the
    // replay parts from the recording
    static replay_writer Unheld;
    static u8 UnheldMemory[4 * 1024 * 1024];
    ClearReplayWriter(&Unheld);
    Unheld.Memory = UnheldMemory;
    Unheld.Capacity = sizeof(UnheldMemory);
    Check(ReplayOpen(&Reader, Memory, Writer.Used));
    ReplayWriteHeader(&Unheld, Reader.ContentId, Reader.MapId);
    u32 Holds = 0;
    while (ReplayNextEvent(&Reader, &Event))
    {
        if (Event.Type == ReplayEvent_Held)
        {
            Holds++;
            continue;
        }
        ReplayWriteEvent(&Unheld, &Event);
    }
    ReplayFlush(&Unheld);
    Check(Holds > 100);
    Result = PlayReplay(&Played, UnheldMemory, Unheld.Used);
    Check(Result.Readable);
    Check(Result.Mismatches > 0);

    // NOTE: a recording cut short (a crash) plays up to its last whole
    // block and says it was cut
    Result = PlayReplay(&Played, Memory, Writer.Used - 7);
    Check(!Result.Readable && Result.Ticks < REPLAY_TEST_TICKS);
    Check(Result.Mismatches == 0);
}

// A team duel with bots replays the same: slot 0 asks for it through the
// vote bits (the bots say yes), then crosses to the other team, a bot
// making way, which only works when the replay knows the bots
// (NET_ROLE_BOT).
internal void
StepTeamReplayGame(server_game *Game, u32 Tick, u32 Asked)
{
    if (Tick == 0)
    {
        Game->BotTarget = 6;
        for (u32 Slot = 0; Slot < 2; ++Slot)
        {
            GamePlayerJoined(Game, Slot);
        }
    }
    GameKeepBots(Game, 3u, REPLAY_TEST_DT);
    for (u32 Slot = 0; Slot < 2; ++Slot)
    {
        net_input Input = ScriptedReplayInput(Slot, Tick);
        Input.Buttons &= ~(u32)(NetButton_RewindSelf | NetButton_RewindBubble | NetButton_RewindWorld);
        if (Slot == 0 && Tick >= 10 && Tick < 20)
        {
            Input.Buttons |= (u32)MapVote_Teams << NET_VOTE_SHIFT;
        }
        if (Slot == 0 && Tick >= 600 && Tick < 610)
        {
            Input.Role = (u8)(Asked << NET_ROLE_TEAM_SHIFT);
        }
        GameApplyInput(Game, Slot, &Input);
    }
    GameTick(Game, REPLAY_TEST_DT);
}

internal void
TestTeamDuelReplaysTheSame()
{
    static replay_writer Writer;
    static u8 Memory[4 * 1024 * 1024];
    ClearReplayWriter(&Writer);
    Writer.Memory = Memory;
    Writer.Capacity = sizeof(Memory);
    static server_game Recorded;
    GameInit(&Recorded, MapId_Arena);
    GameStartReplay(&Recorded, &Writer);
    // NOTE: two loops, not one with "if (Tick == 600)" in it: MSVC 2022's
    // optimizer started that loop at 600 and skipped the rest
    for (u32 Tick = 0; Tick < 600; ++Tick)
    {
        StepTeamReplayGame(&Recorded, Tick, Team_None);
    }
    Check(IsTeamDuel(Recorded.AppState));
    u32 TeamBefore = PlayerTeam(Recorded.AppState, 0);
    for (u32 Tick = 600; Tick < 900; ++Tick)
    {
        StepTeamReplayGame(&Recorded, Tick, OtherTeam(TeamBefore));
    }
    Check(IsTeamDuel(Recorded.AppState));
    Check(TeamBefore != Team_None && PlayerTeam(Recorded.AppState, 0) == OtherTeam(TeamBefore));
    Check(CountTeam(Recorded.AppState, Team_Red) == CountTeam(Recorded.AppState, Team_Blue));
    u32 FinalHash = HashWorldState(Recorded.AppState);
    GameShutdown(&Recorded);
    ReplayFlush(&Writer);
    static server_game Played;
    replay_result Result = PlayReplay(&Played, Memory, Writer.Used);
    Check(Result.Readable && Result.Mismatches == 0);
    Check(Result.FinalHash == FinalHash);
}

// The same through a file, as server --record writes it: a block at a
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
    ClearReplayWriter(&Writer);
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
    Check(Size == Writer.Used);
    static server_game Played;
    replay_result Result = PlayReplay(&Played, Memory, Size);
    Check(Result.Readable && Result.Ticks == Ticks && Result.Mismatches == 0);
    Check(Result.MapId == MapId_Wilds);
}

// A recording stops at its cap: the file stays under it, and what it
// holds, whole blocks from the start, still plays back without a mismatch.
internal u32
RecordReplayTestMatch(replay_writer *Writer, u8 *Memory, u32 Capacity, u32 Cap, u32 Ticks)
{
    ClearReplayWriter(Writer);
    Writer->Memory = Memory;
    Writer->Capacity = Capacity;
    Writer->Cap = Cap;
    static server_game Recorded;
    GameInit(&Recorded, MapId_Arena);
    GameStartReplay(&Recorded, Writer);
    for (u32 Tick = 0; Tick < Ticks; ++Tick)
    {
        StepReplayTestGame(&Recorded, Tick);
    }
    ReplayFlush(Writer);
    GameShutdown(&Recorded);
    return Writer->Used;
}

internal void
TestReplayStopsAtItsCap()
{
    static replay_writer Writer;
    static u8 Memory[4 * 1024 * 1024];
    // NOTE: long enough for several blocks of about half a minute each
    u32 Ticks = 60 * 120;
    u32 Whole = RecordReplayTestMatch(&Writer, Memory, sizeof(Memory), 0, Ticks);
    Check(!Writer.Full && Writer.Used > 3 * 1024);
    u32 Cap = Whole * 2 / 3;
    RecordReplayTestMatch(&Writer, Memory, sizeof(Memory), Cap, Ticks);
    Check(Writer.Full && !Writer.Failed);
    Check(Writer.Used <= Cap && Writer.Used > 0);
    static server_game Played;
    replay_result Result = PlayReplay(&Played, Memory, Writer.Used);
    printf("  replay cap: %u of %u bytes kept, %u of %u ticks\n", Writer.Used, Whole,
           Result.Ticks, Ticks);
    Check(Result.Readable && Result.Ticks > 0 && Result.Ticks < Ticks);
    Check(Result.Mismatches == 0);
}

internal void
RunReplayTests()
{
    GROUP(TestSameInputsGiveTheSameWorld());
    GROUP(TestReplayPlaysBackTheSameMatch());
    GROUP(TestTeamDuelReplaysTheSame());
    GROUP(TestReplayFileRoundTrip());
    GROUP(TestReplayStopsAtItsCap());
}
