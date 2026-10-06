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

// The caster is invulnerable for the whole ability, the freeze and the
// playback, in all three rewinds: on every one of those ticks a lethal
// blow, a shove and a stun change nothing, and it lands alive with the
// health it had two seconds before.
internal void
TestCasterInvulnerableThroughTheRewind(u16 Button)
{
    static server_game Game;
    app_state *AppState = StartQuietRewindGame(&Game);
    GamePlayerJoined(&Game, 0);
    world_entity *Player = AppState->Players[0].Entity;
    for (u32 Tick = 0; Tick < 200 && RewindCastOf(&Game, 0)->Phase != RewindPhase_Hold; ++Tick)
    {
        HoldForRewindTest(&Game, 0, (u16)(Tick == 60 ? Button : 0));
        GameTick(&Game, REWIND_TEST_DT);
    }
    Check(RewindCastOf(&Game, 0)->Phase == RewindPhase_Hold);
    float Hp = Player->Hp;
    hit Shove = {10.f, 900.f, 300.f, 0.f, 1.f, SimBurst_Count};
    u32 Ticks = 0;
    bool32 Hurt = false;
    for (; Ticks < 200 && RewindCastOf(&Game, 0)->Phase != RewindPhase_None; ++Ticks)
    {
        v3 Velocity = Player->Velocity;
        bool32 Killed = DamageEntity(AppState, &AppState->World, Player, 5000.f, 0);
        ApplyHit(AppState, &AppState->World, Player, &Shove, V2(1.f, 0.f), 0, SIM_NOBODY);
        Hurt = Hurt || Killed || Player->Hp != Hp || HasStatus(Player, StatusEffect_Stunned) ||
            Player->Velocity.X != Velocity.X || Player->Velocity.Z != Velocity.Z;
        HoldForRewindTest(&Game, 0, 0);
        GameTick(&Game, REWIND_TEST_DT);
    }
    // NOTE: half a second of freeze, half a second of playback
    Check(Ticks >= 55 && Ticks < 200);
    Check(!Hurt);
    Check(Player->Hp > 0.f && Player->Hp == Hp);
    GameShutdown(&Game);
}

internal void
TestFrozenTakesNoHits()
{
    TestCasterInvulnerableThroughTheRewind((u16)NetButton_RewindSelf);
    TestCasterInvulnerableThroughTheRewind((u16)NetButton_RewindBubble);
    TestCasterInvulnerableThroughTheRewind((u16)NetButton_RewindWorld);
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

// The cast is the caster's player cast (sim/player_casts.cpp), so the
// cast pose and bar show it; a dash cuts it, which ends the rewind, and
// the cooldown stays spent.
internal void
TestDashEndsTheCast()
{
    static server_game Game;
    app_state *AppState = StartQuietRewindGame(&Game);
    GamePlayerJoined(&Game, 0);
    world_entity *Player = AppState->Players[0].Entity;
    for (u32 Tick = 0; Tick < 66; ++Tick)
    {
        HoldForRewindTest(&Game, 0, (u16)(Tick == 60 ? NetButton_RewindBubble : 0));
        GameTick(&Game, REWIND_TEST_DT);
    }
    Check(RewindCastOf(&Game, 0)->Phase == RewindPhase_Cast);
    Check(Player->CastSpell == PlayerSpell_RewindBubble);
    Check(Absolute(RewindCastOf(&Game, 0)->PhaseLeft - Player->CastLeft) < 0.0001f);
    HoldForRewindTest(&Game, 0, NetButton_Dash);
    GameTick(&Game, REWIND_TEST_DT);
    Check(!IsPlayerCasting(Player));
    Check(RewindCastOf(&Game, 0)->Phase == RewindPhase_None);
    Check(Player->RewindCooldowns[RewindKind_Bubble] > 0.f);
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

internal void
RunRewindTests()
{
    TestSelfRewindGoesBackTwoSeconds();
    TestFrozenTakesNoHits();
    TestBubbleTakesWhatIsInside();
    TestWorldRewindBringsBackTheDead();
    TestStunEndsTheCast();
    TestDashEndsTheCast();
    TestSnapshotSaysWhatIsFrozen();
    TestHistoryStaysInOrder();
}
