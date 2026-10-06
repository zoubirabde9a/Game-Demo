/* Camera tests: running into a map edge eases the view in without a
   jolt and rests exactly on the edge, a far jump cuts instead of panning,
   and the follow comes out the same at 30 and 60 frames a second. They
   move the player by hand and call UpdateCamera, so no simulation runs.
   Included by sim_tests.cpp, which calls RunCameraTests. */

// NOTE(zoubir): a 960 by 520 view at zoom 1 on the test world (64 tiles
// of 32, so 2048 units across), the mouse outside the window so nothing
// leans
internal world_entity *
StartCameraTest(test_world *Test, app_window *View, v3 Position)
{
    *View = {960, 520};
    Test->AppState->WorldZoom = 1.f;
    Test->Input.MouseX = -1;
    Test->Input.MouseY = -1;
    world_entity *Player = AddPlayerToSlot(Test->AppState, Test->World,
                                           &Test->Arena, 0, Position);
    Player->Position = Position;
    Player->Velocity = V3(0.f);
    UpdateCamera(Test->AppState, View, &Test->Input, true);
    return Player;
}

// NOTE(zoubir): the old follow stopped the target dead at the edge, and a
// full run braked at about 2800 units/s/s; the eased edge
// keeps it under 1200 the whole way in
internal void
TestCameraEasesIntoMapEdge()
{
    test_world Test = CreateTestWorld();
    app_window View;
    world_entity *Player = StartCameraTest(&Test, &View, {600, 1000, 0});
    float Dt = Test.Input.DeltaTime;
    float EdgeX = 2048.f - 960.f;
    float Speed = 260.f;
    float History[3] = {};
    float WorstBrake = 0.f;
    bool32 PastEdge = false;
    for(u32 Frame = 0; Frame < 600; Frame++)
    {
        bool32 Running = Player->Position.X < 2020.f;
        Player->Velocity = V3(Running ? Speed : 0.f, 0.f, 0.f);
        Player->Position.X += Player->Velocity.X * Dt;
        v3 Drawn = UpdateCamera(Test.AppState, &View, &Test.Input, true);
        float X = Test.AppState->CameraOffset.X;
        PastEdge |= (X > EdgeX + 0.001f) || (Drawn.X > EdgeX + 0.001f);
        History[0] = History[1];
        History[1] = History[2];
        History[2] = X;
        // NOTE(zoubir): a second to get up to speed before measuring
        if (Frame >= 60)
        {
            float Accel = (History[2] - 2.f * History[1] + History[0]) / (Dt * Dt);
            WorstBrake = Maximum(WorstBrake, Absolute(Accel));
        }
    }
    printf("  camera into the edge: hardest change of speed %.0f units/s/s\n",
           WorstBrake);
    Check(!PastEdge);
    Check(WorstBrake < 1200.f);
    Check(Absolute(Test.AppState->CameraOffset.X - EdgeX) < 0.01f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a respawn across the map shows the new spot on the next
// frame, with no spring speed left over to carry it on
internal void
TestCameraCutsOnFarJump()
{
    test_world Test = CreateTestWorld();
    app_window View;
    world_entity *Player = StartCameraTest(&Test, &View, {1900, 1000, 0});
    for(u32 Frame = 0; Frame < 30; Frame++)
    {
        UpdateCamera(Test.AppState, &View, &Test.Input, true);
    }
    Player->Position = V3(200.f, 1000.f, 0.f);
    UpdateCamera(Test.AppState, &View, &Test.Input, true);
    Check(Test.AppState->CameraOffset.X == 0.f);
    Check(Absolute(Test.AppState->CameraOffset.Y - (1000.f - 260.f)) < 0.01f);
    Check(Test.AppState->CameraVelocity.X == 0.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a short hop is followed the same at 30 and 60 frames a
// second: the camera is within a unit of itself after a fifth of a second
internal float
CameraAfterHop(float Dt, u32 Frames)
{
    test_world Test = CreateTestWorld();
    app_window View;
    world_entity *Player = StartCameraTest(&Test, &View, {1000, 1000, 0});
    Test.Input.DeltaTime = Dt;
    Player->Position.X += 120.f;
    for(u32 Frame = 0; Frame < Frames; Frame++)
    {
        UpdateCamera(Test.AppState, &View, &Test.Input, true);
    }
    float Result = Test.AppState->CameraOffset.X;
    DestroyTestWorld(&Test);
    return Result;
}

internal void
TestCameraSameAtAnyFrameRate()
{
    float At60 = CameraAfterHop(1.f / 60.f, 12);
    float At30 = CameraAfterHop(1.f / 30.f, 6);
    Check(Absolute(At60 - At30) < 1.f);
    // NOTE(zoubir): and it moved most of the way, but did not overshoot
    float Target = 1120.f - 480.f;
    Check(At60 > Target - 40.f && At60 <= Target);
}

internal void
RunCameraTests()
{
    TestCameraEasesIntoMapEdge();
    TestCameraCutsOnFarJump();
    TestCameraSameAtAnyFrameRate();
}
