/* Player ability tests: the sword and fireball go toward the aim (the
   cursor) at any angle, the body faces the aim whichever way the player
   walks, a swing roots the player only for a moment, and each shockwave
   and sword swing draws one ring or arc, and hits show one number each.
   Included by sim_tests.cpp, which calls RunPlayerAbilityTests. */

inline v2
UnitOf(v2 V)
{
    v2 Result = V * (1.f / Length(V));
    return Result;
}

internal world_entity *
FindFirstOfType(world *World, entity_type Type)
{
    world_entity *Result = 0;
    for(u32 Index = 0; Index < World->EntityCount && !Result; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent && Entity->Type == Type)
        {
            Result = Entity;
        }
    }
    return Result;
}

// NOTE(zoubir): runs the slot's player for Frames ticks; presses only
// count on the first
internal animation_direction
RunPlayerFrames(test_world *Test, u32 SlotIndex, u32 Frames)
{
    player_slot *Slot = &Test->AppState->Players[SlotIndex];
    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection = AnimationDirection_Right;
    for(u32 Frame = 0; Frame < Frames; Frame++)
    {
        UpdatePlayer(Slot, Test->World, &Test->Arena, Test->Input.DeltaTime,
                     Test->AppState, &AnimationSpeed, &AnimationType,
                     &AnimationDirection);
        Slot->Entity->AnimationState.LastAnimationDirection = AnimationDirection;
        Slot->Input.Pressed = 0;
    }
    return AnimationDirection;
}

internal void
TestFireBallFliesTowardAim()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Caster = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    v2 Aim = UnitOf(V2(3.f, -1.f));
    AppState->Players[0].Input.Aim = Aim;
    AppState->Players[0].Input.Pressed = PlayerButton_Cast;
    RunPlayerFrames(&Test, 0, 1);

    world_entity *FireBall = FindFirstOfType(Test.World, EntityType_FireBall);
    Check(FireBall != 0);
    if (FireBall)
    {
        v2 Flight = UnitOf(FireBall->Velocity.XY);
        Check(DotProduct(Flight, Aim) > 0.999f);
        Check(FireBall->Position.Y < Caster->Position.Y);
    }
    DestroyTestWorld(&Test);
}

internal void
TestSwordSwingsTowardAim()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Attacker = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                             0, {300, 300, 0});
    AppState->Players[0].Input.Aim = V2(0.f, 1.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Attack;
    animation_direction Facing = RunPlayerFrames(&Test, 0, 1);

    world_entity *Sword = FindFirstOfType(Test.World, EntityType_Sword);
    Check(Sword != 0);
    if (Sword)
    {
        Check(Sword->Position.Y > Attacker->Position.Y + 10.f);
        Check(Sword->AnimationDirection == AnimationDirection_Down);
    }
    Check(Facing == AnimationDirection_Down);
    DestroyTestWorld(&Test);
}

internal void
TestBodyFacesAimWhileWalkingAway()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Walker = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    AppState->Players[0].Input.Move = V2(1.f, 0.f);
    AppState->Players[0].Input.Aim = V2(-1.f, 0.f);
    animation_direction Facing = RunPlayerFrames(&Test, 0, 30);
    Check(Walker->Position.X > 320.f);
    Check(Facing == AnimationDirection_Left);

    // NOTE(zoubir): no cursor input keeps the last aim
    AppState->Players[0].Input.Aim = {};
    Facing = RunPlayerFrames(&Test, 0, 5);
    Check(Facing == AnimationDirection_Left);
    DestroyTestWorld(&Test);
}

internal void
TestPlayerWalksDuringSwing()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Walker = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    AppState->Players[0].Input.Move = V2(1.f, 0.f);
    AppState->Players[0].Input.Aim = V2(0.f, -1.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Attack;
    RunPlayerFrames(&Test, 0, 1);
    Check(Walker->State == EntityState_Attacking);
    float XAfterLock = Walker->Position.X;
    // NOTE(zoubir): a swing used to root the player for its whole
    // animation; now only for PLAYER_SWING_LOCK
    RunPlayerFrames(&Test, 0, 8);
    Check(Walker->Position.X > XAfterLock + 5.f);
    DestroyTestWorld(&Test);
}

internal void
TestShockwaveStartsOneRing()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Local = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                          0, {300, 300, 0});
    world_entity *Replica = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                            1, {600, 300, 0});
    shockwave_rings Fx = {};
    float Dt = 1.f / 60.f;
    Local->ShockwaveFlash = SHOCKWAVE_FLASH_SECONDS;
    UpdateShockwaveRings(&Fx, AppState, Dt);
    UpdateShockwaveRings(&Fx, AppState, Dt);
    Check(Fx.Count == 1);

    // NOTE(zoubir): a server's replica carries the flag in AbilityIndex
    Replica->AbilityIndex = PLAYER_FLASH_SHOCKWAVE;
    UpdateShockwaveRings(&Fx, AppState, Dt);
    Check(Fx.Count == 2);
    Check(Fx.Rings[1].Center.X == 600.f);

    for(u32 Frame = 0; Frame < 60; Frame++)
    {
        UpdateShockwaveRings(&Fx, AppState, Dt);
    }
    Check(Fx.Count == 0);
    DestroyTestWorld(&Test);
}

internal void
TestSwordSwingStartsOneArc()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    AppState->Players[0].Input.Aim = V2(0.f, -1.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Attack;
    RunPlayerFrames(&Test, 0, 1);

    sword_arcs Fx = {};
    float Dt = 1.f / 60.f;
    UpdateSwordArcs(&Fx, AppState, Dt);
    UpdateSwordArcs(&Fx, AppState, Dt);
    Check(Fx.Count == 1);
    // NOTE(zoubir): aimed up, so the arc is centred on -90 degrees
    Check(Absolute(Fx.Arcs[0].Angle + 0.5f * Pi32) < 0.01f);

    for(u32 Frame = 0; Frame < 30; Frame++)
    {
        SimulateTick(AppState, &Test.Arena, Dt);
        UpdateSwordArcs(&Fx, AppState, Dt);
    }
    Check(Fx.Count == 0);
    DestroyTestWorld(&Test);
}

internal void
TestDashGoesWhereKeysPointElseTowardAim()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Still = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                          0, {300, 300, 0});
    world_entity *Runner = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {300, 700, 0});
    // NOTE(zoubir): standing still, the dash used to go nowhere
    AppState->Players[0].Input.Aim = V2(0.f, 1.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Dash;
    RunPlayerFrames(&Test, 0, 20);
    Check(Still->Position.Y > 340.f);
    Check(Absolute(Still->Position.X - 300.f) < 1.f);
    Check(Still->DashCooldown > 0.f);

    // NOTE(zoubir): moving, the keys win over the aim
    AppState->Players[1].Input.Move = V2(-1.f, 0.f);
    AppState->Players[1].Input.Aim = V2(1.f, 0.f);
    AppState->Players[1].Input.Pressed = PlayerButton_Dash;
    RunPlayerFrames(&Test, 1, 20);
    Check(Runner->Position.X < 300.f - 60.f);
    DestroyTestWorld(&Test);
}

internal void
TestBlinkLandsAtCursorOrStopsAtWall()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Blinker = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                            0, {300, 300, 0});
    // NOTE(zoubir): cursor 100 units right, inside the reach
    AppState->Players[0].Input.Aim = V2(100.f / PLAYER_AIM_REACH, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Blink;
    RunPlayerFrames(&Test, 0, 1);
    Check(Absolute(Blinker->Position.X - 400.f) < 3.f);
    Check(Blinker->BlinkCooldown > 0.f);
    Check(Blinker->DashFlash > 0.f);

    // NOTE(zoubir): on cooldown, a second press does nothing
    float X = Blinker->Position.X;
    AppState->Players[0].Input.Pressed = PlayerButton_Blink;
    RunPlayerFrames(&Test, 0, 1);
    Check(Absolute(Blinker->Position.X - X) < 3.f);

    // NOTE(zoubir): a wall on the way stops it at the wall's face
    world_entity *Walled = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {300, 700, 0});
    AddTestEntity(&Test, EntityType_StaticObject, {400, 700, 0}, Test.WallVolume);
    AppState->Players[1].Input.Aim = V2(1.f, 0.f);
    AppState->Players[1].Input.Pressed = PlayerButton_Blink;
    RunPlayerFrames(&Test, 1, 1);
    Check(Walled->Position.X > 350.f);
    Check(Walled->Position.X + 15.f <= 384.01f);
    DestroyTestWorld(&Test);
}

internal void
TestHitsShowOneNumberEach()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {400, 300, 0}, Test.UnitVolume);
    Monster->MaxHp = Monster->Hp = 100.f;
    hit_numbers *Fx = (hit_numbers *)calloc(1, sizeof(hit_numbers));
    float Dt = 1.f / 60.f;
    UpdateHitNumbers(Fx, AppState, Dt);
    Check(Fx->Count == 0);

    DamageEntity(AppState, Test.World, Monster, 12.f, 0);
    UpdateHitNumbers(Fx, AppState, Dt);
    Check(Fx->Count == 1);
    Check(Fx->Numbers[0].Amount == 12);

    // NOTE(zoubir): a burn ticking every frame adds up instead of
    // showing a number per frame
    for(u32 Frame = 0; Frame < 30; Frame++)
    {
        Monster->Hp -= 0.5f;
        UpdateHitNumbers(Fx, AppState, Dt);
    }
    Check(Fx->Count >= 2 && Fx->Count <= 4);
    free(Fx);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): counts sword entities that appear over Frames, by ID
internal u32
CountSwordsOver(test_world *Test, u32 SlotIndex, u32 Frames)
{
    u32 Seen[16] = {};
    u32 SeenCount = 0;
    for(u32 Frame = 0; Frame < Frames; Frame++)
    {
        SimulateTick(Test->AppState, &Test->Arena, 1.f / 60.f);
        Test->AppState->Players[SlotIndex].Input.Pressed = 0;
        world *World = Test->World;
        for(u32 Index = 0; Index < World->EntityCount; Index++)
        {
            world_entity *Entity = &World->Entities[Index];
            if (!Entity->IsPresent || Entity->Type != EntityType_Sword)
            {
                continue;
            }
            bool32 Known = false;
            for(u32 K = 0; K < SeenCount; K++)
            {
                Known = Known || Seen[K] == Entity->ID;
            }
            if (!Known && SeenCount < ArrayCount(Seen))
            {
                Seen[SeenCount++] = Entity->ID;
            }
        }
    }
    return SeenCount;
}

// NOTE(zoubir): a second click early in a swing used to wait less than
// the swing lasts and was dropped
internal void
TestClickDuringSwingQueuesNextSwing()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    // NOTE(zoubir): the real swing length, 6 frames of 0.03 s
    SetupAnimationSets(AppState, &Test.Arena);
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    AppState->Players[0].Input.Aim = V2(1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Attack;
    SimulateTick(AppState, &Test.Arena, 1.f / 60.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Attack;
    Check(CountSwordsOver(&Test, 0, 40) == 2);
    DestroyTestWorld(&Test);
}

internal u32
CountPresent(world *World, entity_type Type)
{
    u32 Result = 0;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        Result += (Entity->IsPresent && Entity->Type == Type) ? 1 : 0;
    }
    return Result;
}

// NOTE(zoubir): two clicks a frame apart give one fireball now and the
// second once the interval has passed, not two at once
internal void
TestFastClicksFireAtSteadyRate()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    AppState->Players[0].Input.Aim = V2(1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Cast;
    RunPlayerFrames(&Test, 0, 1);
    AppState->Players[0].Input.Pressed = PlayerButton_Cast;
    RunPlayerFrames(&Test, 0, 5);
    Check(CountPresent(Test.World, EntityType_FireBall) == 1);
    RunPlayerFrames(&Test, 0, 20);
    Check(CountPresent(Test.World, EntityType_FireBall) == 2);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): casting while walking roots for a moment, then the walk
// goes on at full speed with the walk animation, as if no cast happened
internal void
TestWalkingCutsCastAnimation()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    SetupAnimationSets(AppState, &Test.Arena);
    world_entity *Caster = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Walker = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {300, 700, 0});
    for(u32 SlotIndex = 0; SlotIndex < 2; SlotIndex++)
    {
        AppState->Players[SlotIndex].Input.Move = V2(1.f, 0.f);
        AppState->Players[SlotIndex].Input.Aim = V2(0.f, -1.f);
    }
    RunPlayerFrames(&Test, 0, 10);
    RunPlayerFrames(&Test, 1, 10);
    AppState->Players[0].Input.Pressed = PlayerButton_Cast;
    animation_type Animation = AnimationType_Stand;
    for(u32 Frame = 0; Frame < 30; Frame++)
    {
        for(u32 SlotIndex = 0; SlotIndex < 2; SlotIndex++)
        {
            float AnimationSpeed;
            animation_direction Direction;
            animation_type Type;
            player_slot *Slot = &AppState->Players[SlotIndex];
            UpdatePlayer(Slot, Test.World, &Test.Arena, Test.Input.DeltaTime,
                         AppState, &AnimationSpeed, &Type, &Direction);
            Slot->Input.Pressed = 0;
            if (SlotIndex == 0) Animation = Type;
        }
    }
    Check(Caster->State == EntityState_Moving);
    Check(Animation == AnimationType_Move);
    // NOTE(zoubir): only the short root is lost against a plain walk
    Check(Walker->Position.X - Caster->Position.X < 8.f);
    DestroyTestWorld(&Test);
}

internal void
TestSwordShovesSurvivorAway()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Attacker = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                             0, {300, 300, 0});
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {330, 300, 0}, Test.UnitVolume);
    Monster->MaxHp = Monster->Hp = 100.f;
    world_entity *Sword = AddSword(AppState, Test.World, &Test.Arena,
                                   {316, 300, 0}, Attacker,
                                   AnimationDirection_Right);
    UpdateSword(Sword, Test.World, &Test.Arena, AppState, Test.Input.DeltaTime);
    Check(Monster->Hp == 100.f - SWORD_DAMAGE);
    Check(Monster->Velocity.X > 0.9f * SWORD_KNOCKBACK);
    Check(Absolute(Monster->Velocity.Y) < 1.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a diagonal swing hits along the diagonal, and nothing
// behind the swinger is hit (the old box reached 15 units behind)
internal void
TestSwordHitsItsSliceAtAnyAngle()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    world_entity *Diagonal = AddTestEntity(&Test, EntityType_Monster,
                                           {326, 274, 0}, Test.UnitVolume);
    world_entity *Behind = AddTestEntity(&Test, EntityType_Monster,
                                         {272, 300, 0}, Test.UnitVolume);
    world_entity *Far = AddTestEntity(&Test, EntityType_Monster,
                                      {370, 230, 0}, Test.UnitVolume);
    Diagonal->MaxHp = Diagonal->Hp = 100.f;
    Behind->MaxHp = Behind->Hp = 100.f;
    Far->MaxHp = Far->Hp = 100.f;
    AppState->Players[0].Input.Aim = V2(0.7071f, -0.7071f);
    AppState->Players[0].Input.Pressed = PlayerButton_Attack;
    for(u32 Frame = 0; Frame < 20; Frame++)
    {
        SimulateTick(AppState, &Test.Arena, 1.f / 60.f);
        AppState->Players[0].Input.Pressed = 0;
    }
    Check(Diagonal->Hp == 100.f - SWORD_DAMAGE);
    Check(Behind->Hp == 100.f);
    Check(Far->Hp == 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a hit landing mid-dash does nothing; once the dash
// streak is over, hits land again
internal void
TestDashDodgesHits()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Dodger = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    AppState->Players[0].Input.Aim = V2(1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Dash;
    RunPlayerFrames(&Test, 0, 1);
    float Hp = Dodger->Hp;
    Check(!DamageEntity(AppState, Test.World, Dodger, 30.f, 0));
    Check(Dodger->Hp == Hp);
    RunPlayerFrames(&Test, 0, 20);
    DamageEntity(AppState, Test.World, Dodger, 30.f, 0);
    Check(Dodger->Hp == Hp - 30.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): standing in a hazard gives its status; jumping over it
// does not
internal void
TestJumpClearsGroundHazards()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Owner = AddTestEntity(&Test, EntityType_Monster,
                                        {600, 600, 0}, Test.UnitVolume);
    world_entity *Standing = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                             0, {300, 300, 0});
    world_entity *Jumping = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                            1, {300, 400, 0});
    Jumping->Position.Z = 20.f;
    // NOTE(zoubir): any monster ability that leaves a patch with a status
    monster_ability *Ability = 0;
    for(u32 Kind = 0; Kind < MonsterKind_Count && !Ability; Kind++)
    {
        monster_def *Def = GetMonsterDef((monster_kind)Kind);
        for(u32 Index = 0; Index < Def->AbilityCount && !Ability; Index++)
        {
            monster_ability *Candidate = &Def->Abilities[Index];
            if (Candidate->HazardSeconds > 0.f && Candidate->Radius > 0.f &&
                Candidate->Status != StatusEffect_None)
            {
                Ability = Candidate;
                Owner->MonsterKind = (monster_kind)Kind;
                Owner->AbilityIndex = Index;
            }
        }
    }
    Check(Ability != 0);
    if (!Ability)
    {
        DestroyTestWorld(&Test);
        return;
    }
    status_effect Status = Ability->Status;
    world_entity *Under = AddMonsterHazard(AppState, Test.World, &Test.Arena,
                                           Owner, Ability, V2(300.f, 300.f));
    world_entity *Over = AddMonsterHazard(AppState, Test.World, &Test.Arena,
                                          Owner, Ability, V2(300.f, 400.f));
    UpdateMonsterHazard(Under, Test.World, AppState, 1.f / 60.f);
    UpdateMonsterHazard(Over, Test.World, AppState, 1.f / 60.f);
    Check(Standing->StatusTimers[Status] > 0.f);
    Check(Jumping->StatusTimers[Status] == 0.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): in the air the jump state used to replace the swing
// state each tick, so every click swung at once; swings are paced the
// same as on the ground
internal void
TestSwingsInTheAirArePaced()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Jumper = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    AppState->Players[0].Input.Aim = V2(1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Jump;
    RunPlayerFrames(&Test, 0, 3);
    Check(Jumper->Position.Z > 0.f);
    for(u32 Click = 0; Click < 4; Click++)
    {
        AppState->Players[0].Input.Pressed = PlayerButton_Attack;
        RunPlayerFrames(&Test, 0, 2);
    }
    // NOTE(zoubir): four clicks over 8 frames (0.13 s), under one swing
    Check(CountPresent(Test.World, EntityType_Sword) == 1);
    DestroyTestWorld(&Test);
}

internal void
RunPlayerAbilityTests()
{
    printf("TestFireBallFliesTowardAim\n");
    TestFireBallFliesTowardAim();
    printf("TestSwordSwingsTowardAim\n");
    TestSwordSwingsTowardAim();
    printf("TestBodyFacesAimWhileWalkingAway\n");
    TestBodyFacesAimWhileWalkingAway();
    printf("TestPlayerWalksDuringSwing\n");
    TestPlayerWalksDuringSwing();
    printf("TestShockwaveStartsOneRing\n");
    TestShockwaveStartsOneRing();
    printf("TestSwordSwingStartsOneArc\n");
    TestSwordSwingStartsOneArc();
    printf("TestDashGoesWhereKeysPointElseTowardAim\n");
    TestDashGoesWhereKeysPointElseTowardAim();
    printf("TestBlinkLandsAtCursorOrStopsAtWall\n");
    TestBlinkLandsAtCursorOrStopsAtWall();
    printf("TestHitsShowOneNumberEach\n");
    TestHitsShowOneNumberEach();
    printf("TestClickDuringSwingQueuesNextSwing\n");
    TestClickDuringSwingQueuesNextSwing();
    printf("TestFastClicksFireAtSteadyRate\n");
    TestFastClicksFireAtSteadyRate();
    printf("TestWalkingCutsCastAnimation\n");
    TestWalkingCutsCastAnimation();
    printf("TestSwordShovesSurvivorAway\n");
    TestSwordShovesSurvivorAway();
    printf("TestSwordHitsItsSliceAtAnyAngle\n");
    TestSwordHitsItsSliceAtAnyAngle();
    printf("TestDashDodgesHits\n");
    TestDashDodgesHits();
    printf("TestJumpClearsGroundHazards\n");
    TestJumpClearsGroundHazards();
    printf("TestSwingsInTheAirArePaced\n");
    TestSwingsInTheAirArePaced();
}
