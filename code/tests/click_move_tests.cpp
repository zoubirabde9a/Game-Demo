/* Click to move tests: a left click on the ground walks the local player
   there and stops on the spot without casting; a click on a monster, or
   with Shift held, casts and does not walk; a movement key or a wall ends
   the walk; the tile editor's clicks do neither; and online the walk is
   sent as held direction buttons, never as a held fireball. Included by
   sim_tests.cpp, which calls RunClickMoveTests. */

struct click_move_test
{
    test_world Test;
    world_entity *Player;
    bool32 Cast;
};

internal click_move_test
StartClickMoveTest()
{
    click_move_test Result = {};
    Result.Test = CreateTestWorld();
    Result.Player = AddPlayerToSlot(Result.Test.AppState, Result.Test.World,
                                    &Result.Test.Arena, 0, {300, 300, 0});
    ClickMove = {};
    return Result;
}

inline void
PressLeftButton(app_input *Input, i32 X, i32 Y)
{
    Input->MouseX = X;
    Input->MouseY = Y;
    Input->LeftButton.Pressed = true;
    Input->LeftButton.EndedDown = true;
}

// NOTE(zoubir): reads the local input and runs the player for Frames
// ticks; the button counts as pressed on the first only, and is let go
// unless Hold. Notes any fireball cast on the way
internal void
RunClickMoveFrames(click_move_test *Run, u32 Frames, bool32 Hold = false)
{
    app_input *Input = &Run->Test.Input;
    player_slot *Slot = &Run->Test.AppState->Players[0];
    for(u32 Frame = 0; Frame < Frames; Frame++)
    {
        Slot->Input = ReadKeyboardPlayerInput(Input, Run->Test.AppState);
        Run->Cast |= (Slot->Input.Pressed & PlayerButton_Cast) != 0;
        RunPlayerFrames(&Run->Test, 0, 1);
        Input->LeftButton.Pressed = false;
        Input->LeftButton.EndedDown = Hold;
    }
}

internal void
TestGroundClickWalksThereWithoutCasting()
{
    click_move_test Run = StartClickMoveTest();
    v2 Start = Run.Player->Position.XY;
    v2 Spot = Start + V2(170.f, 80.f);
    PressLeftButton(&Run.Test.Input, (i32)Spot.X, (i32)Spot.Y);
    RunClickMoveFrames(&Run, 1);
    Check(ClickMove.Active);
    // NOTE(zoubir): the cursor wanders off; the walk keeps to the spot
    Run.Test.Input.MouseX = 100;
    Run.Test.Input.MouseY = 100;
    float FarthestOffLine = 0.f;
    v2 Line = UnitOf(Spot - Start);
    for(u32 Frame = 0; Frame < 240 && ClickMove.Active; Frame++)
    {
        RunClickMoveFrames(&Run, 1);
        v2 FromStart = Run.Player->Position.XY - Start;
        float OffLine = fabsf(FromStart.X * Line.Y - FromStart.Y * Line.X);
        FarthestOffLine = Maximum(FarthestOffLine, OffLine);
    }
    Check(!ClickMove.Active);
    // NOTE(zoubir): it coasts a little after letting go of the keys
    RunClickMoveFrames(&Run, 60);
    Check(Length(Run.Player->Position.XY - Spot) < 12.f);
    Check(FarthestOffLine < 12.f);
    Check(!Run.Cast);
    Check(!FindFirstOfType(Run.Test.World, EntityType_FireBall));
    DestroyTestWorld(&Run.Test);
}

internal void
TestHoldingTheButtonSteersToTheCursor()
{
    click_move_test Run = StartClickMoveTest();
    PressLeftButton(&Run.Test.Input, 300, 500);
    RunClickMoveFrames(&Run, 20, true);
    Check(Run.Player->Position.Y > 310.f);
    Run.Test.Input.MouseX = 500;
    Run.Test.Input.MouseY = (i32)Run.Player->Position.Y;
    RunClickMoveFrames(&Run, 40, true);
    Check(Run.Player->Position.X > 340.f);
    Check(!Run.Cast);
    DestroyTestWorld(&Run.Test);
}

internal void
TestClickOnMonsterCastsInsteadOfWalking()
{
    click_move_test Run = StartClickMoveTest();
    world_entity *Monster = AddTestEntity(&Run.Test, EntityType_Monster,
                                          {420, 300, 0}, Run.Test.UnitVolume);
    Monster->Dimensions = V2(32.f, 32.f);
    Monster->MaxHp = Monster->Hp = 1000.f;
    // NOTE(zoubir): on its body, above the feet
    PressLeftButton(&Run.Test.Input, 422, 288);
    RunClickMoveFrames(&Run, 1);
    Check(ClickMove.Foe == Monster);
    Check(Run.Cast);
    Check(FindFirstOfType(Run.Test.World, EntityType_FireBall) != 0);
    Check(!ClickMove.Active);
    DestroyTestWorld(&Run.Test);
}

internal void
TestShiftClickCastsAtTheGround()
{
    click_move_test Run = StartClickMoveTest();
    Run.Test.Input.ShiftButton.EndedDown = true;
    PressLeftButton(&Run.Test.Input, 450, 300);
    RunClickMoveFrames(&Run, 1);
    Check(Run.Cast);
    Check(!ClickMove.Active);
    DestroyTestWorld(&Run.Test);
}

internal void
TestMovementKeyEndsTheWalk()
{
    click_move_test Run = StartClickMoveTest();
    PressLeftButton(&Run.Test.Input, 600, 300);
    RunClickMoveFrames(&Run, 10);
    Check(ClickMove.Active);
    Run.Test.Input.ButtonZ.EndedDown = true;
    RunClickMoveFrames(&Run, 1);
    Check(!ClickMove.Active);
    Run.Test.Input.ButtonZ.EndedDown = false;
    float X = Run.Player->Position.X;
    RunClickMoveFrames(&Run, 60);
    Check(Run.Player->Position.X < X + 20.f);
    DestroyTestWorld(&Run.Test);
}

// NOTE(zoubir): a full height wall, not a low block the player steps over
internal void
TestWallEndsTheWalk()
{
    click_move_test Run = StartClickMoveTest();
    app_state *AppState = Run.Test.AppState;
    SetupCollisionVolumes(AppState, &Run.Test.Arena);
    AppState->PlayerCollision = Run.Test.UnitVolume;
    float Y = Run.Player->Position.Y;
    float WallX = Run.Player->Position.X + 80.f;
    for(i32 Row = -3; Row <= 3; Row++)
    {
        AddTestEntity(&Run.Test, EntityType_StaticObject,
                      {WallX, Y + 32.f * (float)Row, 0}, AppState->WallCollision);
    }
    PressLeftButton(&Run.Test.Input, (i32)(WallX + 100.f), (i32)Y);
    RunClickMoveFrames(&Run, 1);
    Check(ClickMove.Active);
    RunClickMoveFrames(&Run, 120);
    Check(!ClickMove.Active);
    Check(Run.Player->Position.X + 15.f <= WallX - 16.f + 0.01f);
    DestroyTestWorld(&Run.Test);
}

internal void
TestTileEditorClicksNeitherWalkNorCast()
{
    click_move_test Run = StartClickMoveTest();
    Run.Test.AppState->TileEditing = true;
    PressLeftButton(&Run.Test.Input, 450, 300);
    RunClickMoveFrames(&Run, 10, true);
    Check(!ClickMove.Active);
    Check(!Run.Cast);
    Check(Run.Player->Position.X < 301.f);
    DestroyTestWorld(&Run.Test);
}

internal void
TestWalkIsSentAsDirectionButtons()
{
    click_move_test Run = StartClickMoveTest();
    app_input *Input = &Run.Test.Input;
    PressLeftButton(Input, 600, 600);
    player_input Local = ReadKeyboardPlayerInput(Input, Run.Test.AppState);
    app_input Sent = InputForServer(Input, &Local, false);
    u16 Buttons = NetButtonsFromKeyboard(&Sent);
    Check(Buttons == (NetButton_Right | NetButton_Down));

    // NOTE(zoubir): a screen taking the keys drops the walk
    InputForServer(Input, &Local, true);
    Check(!ClickMove.Active);
    DestroyTestWorld(&Run.Test);
}

#define CLICK_MOVE_TEST(Test) printf("%s\n", #Test); Test()

internal void
RunClickMoveTests()
{
    CLICK_MOVE_TEST(TestGroundClickWalksThereWithoutCasting);
    CLICK_MOVE_TEST(TestHoldingTheButtonSteersToTheCursor);
    CLICK_MOVE_TEST(TestClickOnMonsterCastsInsteadOfWalking);
    CLICK_MOVE_TEST(TestShiftClickCastsAtTheGround);
    CLICK_MOVE_TEST(TestMovementKeyEndsTheWalk);
    CLICK_MOVE_TEST(TestWallEndsTheWalk);
    CLICK_MOVE_TEST(TestTileEditorClicksNeitherWalkNorCast);
    CLICK_MOVE_TEST(TestWalkIsSentAsDirectionButtons);
}
