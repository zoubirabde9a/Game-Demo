/* Combo move tests (sim/player_abilities/combos.cpp): each combo fires
   from its moves in order and within its window, the same moves the
   other way round are a different combo, a slow follow-up is no combo,
   combos chain, and a client predicting its own player keeps the trail
   but leaves hits to the server. Included by player_ability_tests.cpp,
   whose RunPlayerAbilityTests calls RunPlayerComboTests. */

// NOTE(zoubir): a player at (300, 300) aiming right, with an empty event
// queue
internal world_entity *
AddComboTestPlayer(test_world *Test)
{
    app_state *AppState = Test->AppState;
    AppState->PlayerCollision = Test->UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test->World, &Test->Arena,
                                           0, {300, 300, 0});
    AppState->Players[0].Input.Aim = V2(1.f, 0.f);
    AppState->Events.Count = 0;
    return Player;
}

internal world_entity *
AddComboTestTarget(test_world *Test, v2 At)
{
    world_entity *Target = AddTestEntity(Test, EntityType_Monster,
                                         V3(At.X, At.Y, 0.f), Test->UnitVolume);
    Target->MaxHp = Target->Hp = 1000.f;
    return Target;
}

// NOTE(zoubir): presses Button for one tick, then runs Frames more
internal void
PressFor(test_world *Test, u32 Button, u32 Frames)
{
    Test->AppState->Players[0].Input.Pressed = Button;
    RunPlayerFrames(Test, 0, 1 + Frames);
}

// NOTE(zoubir): a swing's blade hits on its own update, which the player's
// does not run
internal void
UpdateTestSwords(test_world *Test)
{
    for(u32 Index = 0; Index < Test->World->EntityCount; Index++)
    {
        world_entity *Sword = &Test->World->Entities[Index];
        if (Sword->IsPresent && Sword->Type == EntityType_Sword)
        {
            UpdateSword(Sword, Test->World, &Test->Arena, Test->AppState,
                        Test->Input.DeltaTime);
        }
    }
}

// NOTE(zoubir): dash then attack is a lunge: it reaches what a plain swing
// cannot and leaves the player free to move
internal void
TestDashThenAttackLunges()
{
    for(u32 WithDash = 0; WithDash < 2; WithDash++)
    {
        test_world Test = CreateTestWorld();
        world_entity *Player = AddComboTestPlayer(&Test);
        player_slot *Slot = &Test.AppState->Players[0];
        if (WithDash)
        {
            // NOTE(zoubir): down, so the dash does not carry the player
            // into the target on the right
            Slot->Input.Move = V2(0.f, 1.f);
            PressFor(&Test, PlayerButton_Dash, 0);
            Slot->Input.Move = V2(0.f, 0.f);
            RunPlayerFrames(&Test, 0, 4);
        }
        world_entity *Target = AddComboTestTarget(&Test, Player->Position.XY +
                                                  V2(80.f, 0.f));
        PressFor(&Test, PlayerButton_Attack, 0);
        UpdateTestSwords(&Test);
        if (WithDash)
        {
            Check(Target->Hp <= 1000.f - 1.5f * SWORD_DAMAGE + 0.1f);
            Check(CountBursts(Test.AppState, SimBurst_Lunge) == 1);
            Check(CountBursts(Test.AppState, SimBurst_SwingArc) == 0);
            Check(Player->ActionLock == 0.f);
            // NOTE(zoubir): thrown along the thrust
            Check(Target->Velocity.X > 400.f);
        }
        else
        {
            Check(Target->Hp == 1000.f);
            Check(CountBursts(Test.AppState, SimBurst_Lunge) == 0);
        }
        DestroyTestWorld(&Test);
    }
}

// NOTE(zoubir): attack then dash is a different combo: the dash cuts what
// it passes and throws it to the side; no lunge
internal void
TestAttackThenDashCuts()
{
    test_world Test = CreateTestWorld();
    world_entity *Player = AddComboTestPlayer(&Test);
    world_entity *Beside = AddComboTestTarget(&Test, V2(350.f, 330.f));
    world_entity *Far = AddComboTestTarget(&Test, V2(350.f, 420.f));
    PressFor(&Test, PlayerButton_Attack, 3);
    PressFor(&Test, PlayerButton_Dash, 0);
    Check(CountBursts(Test.AppState, SimBurst_CuttingDash) == 1);
    Check(CountBursts(Test.AppState, SimBurst_Lunge) == 0);
    Check(Beside->Hp <= 1000.f - CuttingDashHit.Damage + 0.1f);
    Check(Beside->Velocity.Y > 200.f);
    Check(Far->Hp == 1000.f);
    Check(Player->ComboTrail[0] == ComboMove_Dash);
    Check(Player->ComboTrail[1] == ComboMove_Attack);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a follow-up slower than the window is a plain move
internal void
TestSlowFollowUpIsNoCombo()
{
    test_world Test = CreateTestWorld();
    AddComboTestPlayer(&Test);
    PressFor(&Test, PlayerButton_Dash, 40);
    PressFor(&Test, PlayerButton_Attack, 0);
    Check(CountBursts(Test.AppState, SimBurst_Lunge) == 0);
    Check(CountBursts(Test.AppState, SimBurst_SwingArc) == 1);
    DestroyTestWorld(&Test);
}

internal u32
CountOfType(world *World, entity_type Type)
{
    u32 Result = 0;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        Result += (Entity->IsPresent && Entity->Type == Type);
    }
    return Result;
}

internal void
TestDashThenCastFans()
{
    test_world Test = CreateTestWorld();
    AddComboTestPlayer(&Test);
    PressFor(&Test, PlayerButton_Dash, 3);
    PressFor(&Test, PlayerButton_Cast, 0);
    Check(CountOfType(Test.World, EntityType_FireBall) == 3);
    Check(CountBursts(Test.AppState, SimBurst_FlameFan) == 1);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): how far the player travels between leaving the ground and
// landing, jumping DashFrames after a dash (none when 0)
internal float
JumpDistance(u32 DashFrames)
{
    test_world Test = CreateTestWorld();
    world_entity *Player = AddComboTestPlayer(&Test);
    player_slot *Slot = &Test.AppState->Players[0];
    Slot->Input.Move = V2(1.f, 0.f);
    RunPlayerFrames(&Test, 0, 30);
    if (DashFrames)
    {
        PressFor(&Test, PlayerButton_Dash, DashFrames - 1);
    }
    float From = Player->Position.X;
    PressFor(&Test, PlayerButton_Jump, 0);
    for(u32 Frame = 0; Frame < 120 && Player->Position.Z > 0.f; Frame++)
    {
        RunPlayerFrames(&Test, 0, 1);
    }
    Check(Player->LongJump == false);
    float Result = Player->Position.X - From;
    DestroyTestWorld(&Test);
    return Result;
}

// NOTE(zoubir): dash then jump keeps the dash's speed in the air
internal void
TestDashThenJumpGoesLong()
{
    float Plain = JumpDistance(0);
    float Long = JumpDistance(3);
    Check(Long > 2.f * Plain);
    Check(Long > Plain + 60.f);
}

// NOTE(zoubir): jump, dash, attack is the skewer, which beats the lunge
// (dash, attack) because it is longer, and throws the target up
internal void
TestSkewerBeatsLunge()
{
    test_world Test = CreateTestWorld();
    world_entity *Player = AddComboTestPlayer(&Test);
    player_slot *Slot = &Test.AppState->Players[0];
    PressFor(&Test, PlayerButton_Jump, 4);
    Slot->Input.Move = V2(0.f, 1.f);
    PressFor(&Test, PlayerButton_Dash, 0);
    Slot->Input.Move = V2(0.f, 0.f);
    RunPlayerFrames(&Test, 0, 2);
    world_entity *Target = AddComboTestTarget(&Test, Player->Position.XY +
                                              V2(60.f, 0.f));
    PressFor(&Test, PlayerButton_Attack, 0);
    UpdateTestSwords(&Test);
    Check(CountBursts(Test.AppState, SimBurst_Skewer) == 1);
    Check(CountBursts(Test.AppState, SimBurst_Lunge) == 0);
    Check(Target->Velocity.Z > 300.f);
    Check(HasStatus(Target, StatusEffect_Stunned));
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the swing after a blink is the finisher at once
internal void
TestBlinkThenAttackAmbushes()
{
    test_world Test = CreateTestWorld();
    world_entity *Player = AddComboTestPlayer(&Test);
    PressFor(&Test, PlayerButton_Blink, 2);
    world_entity *Target = AddComboTestTarget(&Test, Player->Position.XY +
                                              V2(30.f, 0.f));
    PressFor(&Test, PlayerButton_Attack, 0);
    UpdateTestSwords(&Test);
    Check(CountBursts(Test.AppState, SimBurst_Ambush) == 1);
    Check(Player->ComboStep == SwordCut_Finisher);
    Check(HasStatus(Target, StatusEffect_Stunned));
    Check(Target->Velocity.Z > 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): attack, dash, attack, attack: the cutting dash, the lunge
// (a combo counts as the move it ended on), then the chain's second cut
internal void
TestCombosChain()
{
    test_world Test = CreateTestWorld();
    world_entity *Player = AddComboTestPlayer(&Test);
    PressFor(&Test, PlayerButton_Attack, 3);
    PressFor(&Test, PlayerButton_Dash, 3);
    PressFor(&Test, PlayerButton_Attack, 11);
    Check(CountBursts(Test.AppState, SimBurst_CuttingDash) == 1);
    Check(CountBursts(Test.AppState, SimBurst_Lunge) == 1);
    PressFor(&Test, PlayerButton_Attack, 6);
    Check(Player->ComboStep == SwordCut_Second);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): predicting, an attack press left to the server still marks
// the trail; the cutting dash it makes hits nobody until the server says
// so, while the long jump, which moves only the player, runs at once
internal void
TestPredictedCombos()
{
    test_world Test = CreateTestWorld();
    world_entity *Player = AddComboTestPlayer(&Test);
    player_slot *Slot = &Test.AppState->Players[0];
    world_entity *Beside = AddComboTestTarget(&Test, V2(350.f, 330.f));
    Slot->Predicted = true;
    Slot->Input.ServerPressed = PlayerButton_Attack;
    RunPlayerFrames(&Test, 0, 1);
    Slot->Input.ServerPressed = 0;
    Check(Player->ComboTrail[0] == ComboMove_Attack);
    Check(CountOfType(Test.World, EntityType_Sword) == 0);
    PressFor(&Test, PlayerButton_Dash, 2);
    Check(Beside->Hp == 1000.f);
    Check(CountBursts(Test.AppState, SimBurst_CuttingDash) == 0);
    Check(Player->ComboTrail[0] == ComboMove_Dash);
    PressFor(&Test, PlayerButton_Jump, 0);
    Check(Player->LongJump);
    Check(CountBursts(Test.AppState, SimBurst_LongJump) == 1);
    Slot->Predicted = false;
    DestroyTestWorld(&Test);
}

internal void
RunPlayerComboTests()
{
    printf("TestDashThenAttackLunges\n");
    TestDashThenAttackLunges();
    printf("TestAttackThenDashCuts\n");
    TestAttackThenDashCuts();
    printf("TestSlowFollowUpIsNoCombo\n");
    TestSlowFollowUpIsNoCombo();
    printf("TestDashThenCastFans\n");
    TestDashThenCastFans();
    printf("TestDashThenJumpGoesLong\n");
    TestDashThenJumpGoesLong();
    printf("TestSkewerBeatsLunge\n");
    TestSkewerBeatsLunge();
    printf("TestBlinkThenAttackAmbushes\n");
    TestBlinkThenAttackAmbushes();
    printf("TestCombosChain\n");
    TestCombosChain();
    printf("TestPredictedCombos\n");
    TestPredictedCombos();
}
