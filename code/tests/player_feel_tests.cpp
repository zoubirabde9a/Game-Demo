/* Player feel tests: how many ticks the player takes to reach full speed,
   to stop and to turn back, and that presses made a moment early are not
   lost (a swing clicked during a swing, a dash or shockwave pressed just
   before its cooldown ends). They print the numbers, so a tuning change shows its
   effect in the test log. Included by sim_tests.cpp after
   player_ability_tests.cpp, whose helpers it uses. */

// NOTE(zoubir): ticks of 1/60 s until Done says the player got there;
// gives up after Limit
#define FEEL_TICK (1.f / 60.f)

internal u32
TicksUntilSpeed(test_world *Test, v2 Move, float Above, float Below,
                float AlongX, u32 Limit)
{
    player_slot *Slot = &Test->AppState->Players[0];
    Slot->Input.Move = Move;
    u32 Ticks = 0;
    for(; Ticks < Limit; Ticks++)
    {
        float Speed = Slot->Entity->Velocity.X * AlongX;
        if (Speed >= Above && Speed <= Below)
        {
            break;
        }
        RunPlayerFrames(Test, 0, 1);
    }
    return Ticks;
}

internal void
TestRunStartsStopsAndTurnsQuickly()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {200, 1000, 0});
    Test.Input.DeltaTime = FEEL_TICK;
    float Top = PlayerStats.RunSpeed;

    u32 ToFull = TicksUntilSpeed(&Test, V2(1.f, 0.f), 0.95f * Top, 1e9f, 1.f, 120);
    RunPlayerFrames(&Test, 0, 30);
    float RunSpeed = Player->Velocity.X;
    float StopFrom = Player->Position.X;
    u32 ToStop = TicksUntilSpeed(&Test, V2(0.f, 0.f), -1e9f, 5.f, 1.f, 120);
    float Skid = Player->Position.X - StopFrom;
    TicksUntilSpeed(&Test, V2(1.f, 0.f), 0.99f * Top, 1e9f, 1.f, 120);
    u32 ToTurn = TicksUntilSpeed(&Test, V2(-1.f, 0.f), 0.95f * Top, 1e9f, -1.f, 120);
    printf("  run %.0f: full speed in %u ticks, stop in %u ticks (%.1f units), "
           "turn back in %u ticks\n", RunSpeed, ToFull, ToStop, Skid, ToTurn);
    Check(Absolute(RunSpeed - 260.f) < 1.f);
    Check(ToFull <= 8);
    Check(ToStop <= 7);
    Check(Skid < 15.f);
    Check(ToTurn <= 12);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): ice and mud keep the top speeds they always had (ice the
// full run, mud 60%); ice is slow to get going and slow to stop
internal void
TestGroundKeepsItsTopSpeed()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {200, 1000, 0});
    Player->GroundSpeedScale = 0.25f;
    Player->GroundFriction = 0.25f;
    u32 IceToFull = TicksUntilSpeed(&Test, V2(1.f, 0.f), 0.95f * 260.f, 1e9f, 1.f, 200);
    RunPlayerFrames(&Test, 0, 30);
    Check(Absolute(Player->Velocity.X - 260.f) < 1.f);
    u32 IceToStop = TicksUntilSpeed(&Test, V2(0.f, 0.f), -1e9f, 5.f, 1.f, 200);
    printf("  ice: full speed in %u ticks, stop in %u\n", IceToFull, IceToStop);
    Check(IceToFull > 20 && IceToStop > 14);

    Player->GroundSpeedScale = 0.6f;
    Player->GroundFriction = 1.f;
    AppState->Players[0].Input.Move = V2(1.f, 0.f);
    RunPlayerFrames(&Test, 0, 60);
    Check(Absolute(Player->Velocity.X - 0.6f * 260.f) < 1.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a dash pressed a moment before it is ready goes at once
// and the next one waits that much longer; pressed earlier, nothing
internal void
TestDashPressedJustEarlyGoes()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 1000, 0});
    player_slot *Slot = &AppState->Players[0];
    float Full = PlayerMovements[PlayerMove_Dash].Cooldown;

    Player->MovementCooldowns[PlayerMove_Dash] = 0.3f;
    Slot->Input.Aim = V2(1.f, 0.f);
    Slot->Input.Pressed = PlayerButton_Dash;
    RunPlayerFrames(&Test, 0, 1);
    Check(Player->Velocity.X < 1.f);
    Check(Player->MovementCooldowns[PlayerMove_Dash] < 0.3f);

    Player->MovementCooldowns[PlayerMove_Dash] = 0.06f + FEEL_TICK;
    Slot->Input.Pressed = PlayerButton_Dash;
    RunPlayerFrames(&Test, 0, 1);
    Check(Player->Velocity.X > 1000.f);
    Check(Absolute(Player->MovementCooldowns[PlayerMove_Dash] - (Full + 0.06f)) < 0.001f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the same for an area ability: the shockwave goes off at
// once and its next cooldown is that much longer
internal void
TestAreaPressedJustEarlyGoes()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 1000, 0});
    player_slot *Slot = &AppState->Players[0];
    float *Cooldown = &Player->AreaCooldowns[PlayerArea_Shockwave];
    float Full = PlayerAreaAbilities[PlayerArea_Shockwave].Cooldown;

    *Cooldown = 0.3f;
    Slot->Input.Pressed = PlayerButton_Shockwave;
    RunPlayerFrames(&Test, 0, 1);
    Check(CountBursts(AppState, SimBurst_ShockwaveRing) == 0);
    Check(*Cooldown < 0.3f);

    *Cooldown = 0.06f + FEEL_TICK;
    Slot->Input.Pressed = PlayerButton_Shockwave;
    RunPlayerFrames(&Test, 0, 1);
    Check(CountBursts(AppState, SimBurst_ShockwaveRing) == 1);
    Check(Absolute(*Cooldown - (Full + 0.06f)) < 0.001f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a dash in the middle of a swing ends it: the player runs
// at full speed straight after, and a click swings again once the
// swing's interval is up instead of waiting for the old animation
internal void
TestDashCutsSwingShort()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    SetupAnimationSets(AppState, &Test.Arena);
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 1000, 0});
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Aim = V2(1.f, 0.f);
    Slot->Input.Pressed = PlayerButton_Attack;
    RunPlayerFrames(&Test, 0, 2);
    Check(Player->State == EntityState_Attacking);
    Slot->Input.Move = V2(0.f, 1.f);
    Slot->Input.Pressed = PlayerButton_Dash;
    RunPlayerFrames(&Test, 0, 1);
    Check(Player->State != EntityState_Attacking);
    RunPlayerFrames(&Test, 0, 40);
    Check(Absolute(Player->Velocity.Y - 260.f) < 1.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a click in a swing starts the next swing on the tick the
// first one's animation ends (0.18 s), not later
internal void
TestQueuedSwingStartsWhenSwingEnds()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    SetupAnimationSets(AppState, &Test.Arena);
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 1000, 0});
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Aim = V2(1.f, 0.f);
    Slot->Input.Pressed = PlayerButton_Attack;
    SimulateTick(AppState, &Test.Arena, FEEL_TICK);
    u32 FirstStep = Player->ComboStep;
    // NOTE(zoubir): the animation only runs in the full tick
    u32 Ticks = 1;
    for(; Ticks < 30 && Player->ComboStep == FirstStep; Ticks++)
    {
        Slot->Input.Pressed = (Ticks == 4) ? PlayerButton_Attack : 0;
        SimulateTick(AppState, &Test.Arena, FEEL_TICK);
    }
    printf("  queued swing started %u ticks after the first\n", Ticks);
    Check(Player->ComboStep != FirstStep);
    Check(Ticks <= 12);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): reversing out of a run shows the skid frames until the
// body has slowed, then the run
internal void
TestTurningBackSkids()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {600, 1000, 0});
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Move = V2(1.f, 0.f);
    RunPlayerFrames(&Test, 0, 30);
    Slot->Input.Move = V2(-1.f, 0.f);
    float AnimationSpeed;
    animation_type Type;
    animation_direction Direction = AnimationDirection_Right;
    UpdatePlayer(Slot, Test.World, &Test.Arena, FEEL_TICK, AppState,
                 &AnimationSpeed, &Type, &Direction);
    Check(Type == AnimationType_Stop);
    RunPlayerFrames(&Test, 0, 10);
    UpdatePlayer(Slot, Test.World, &Test.Arena, FEEL_TICK, AppState,
                 &AnimationSpeed, &Type, &Direction);
    Check(Type == AnimationType_Move);
    DestroyTestWorld(&Test);
}


internal void
RunPlayerFeelTests()
{
    printf("TestRunStartsStopsAndTurnsQuickly\n");
    TestRunStartsStopsAndTurnsQuickly();
    printf("TestGroundKeepsItsTopSpeed\n");
    TestGroundKeepsItsTopSpeed();
    printf("TestDashPressedJustEarlyGoes\n");
    TestDashPressedJustEarlyGoes();
    printf("TestAreaPressedJustEarlyGoes\n");
    TestAreaPressedJustEarlyGoes();
    printf("TestDashCutsSwingShort\n");
    TestDashCutsSwingShort();
    printf("TestQueuedSwingStartsWhenSwingEnds\n");
    TestQueuedSwingStartsWhenSwingEnds();
    printf("TestTurningBackSkids\n");
    TestTurningBackSkids();
}
