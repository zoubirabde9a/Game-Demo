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

// NOTE(zoubir): how many bursts of Kind the simulation asked for since the
// queue was last emptied
internal u32
CountBursts(app_state *AppState, sim_burst Kind)
{
    u32 Result = 0;
    for(u32 Index = 0; Index < AppState->Events.Count; Index++)
    {
        sim_event *Event = &AppState->Events.Events[Index];
        Result += (Event->Type == SimEvent_Burst && Event->Burst == Kind);
    }
    return Result;
}

// NOTE(zoubir): the walking kind with the least (Heaviest false) or most
// health, which is how heavy it is to throw. Tests that measure a throw
// use the lightest, which takes a hit's full knockback (hit.cpp
// KnockbackScale); kind 0 may be heavy
internal monster_kind
FindWalkerByWeight(bool32 Heaviest)
{
    monster_kind Result = MonsterKind_Count;
    for(u32 Kind = 0; Kind < MonsterKind_Count; Kind++)
    {
        monster_def *Def = GetMonsterDef((monster_kind)Kind);
        if (Def->FlyHeight > 0.f)
        {
            continue;
        }
        if (Result == MonsterKind_Count ||
            (Heaviest ? Def->MaxHp > GetMonsterDef(Result)->MaxHp :
                        Def->MaxHp < GetMonsterDef(Result)->MaxHp))
        {
            Result = (monster_kind)Kind;
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

// NOTE(zoubir): runs the slot's player until its wind-up ends (a blink
// takes 0.5 s, player_casts.cpp); returns the frames it took
internal u32
FinishTestCast(test_world *Test, u32 SlotIndex)
{
    u32 Frames = 0;
    while (IsPlayerCasting(Test->AppState->Players[SlotIndex].Entity) &&
           Frames < 120)
    {
        RunPlayerFrames(Test, SlotIndex, 1);
        Frames++;
    }
    return Frames;
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
    // animation; now only for its Lock (spawn_actions.cpp)
    RunPlayerFrames(&Test, 0, 8);
    Check(Walker->Position.X > XAfterLock + 5.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a shockwave winds up, then asks clients for one ring,
// and none while it recharges
internal void
TestShockwaveStartsOneRing()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    AppState->Events.Count = 0;
    AppState->Players[0].Input.Pressed = PlayerButton_Shockwave;
    RunPlayerFrames(&Test, 0, 2);
    Check(Player->CastSpell == PlayerSpell_Shockwave);
    Check(CountBursts(AppState, SimBurst_ShockwaveRing) == 0);
    RunPlayerFrames(&Test, 0, 30);
    AppState->Players[0].Input.Pressed = PlayerButton_Shockwave;
    RunPlayerFrames(&Test, 0, 30);
    Check(CountBursts(AppState, SimBurst_ShockwaveRing) == 1);
    Check(CountBursts(AppState, SimBurst_CastGather) == 1);
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
    AppState->Events.Count = 0;
    RunPlayerFrames(&Test, 0, 1);

    // NOTE(zoubir): aimed up, so the arc is centred on -90 degrees
    Check(CountBursts(AppState, SimBurst_SwingArc) == 1);
    sim_event *Arc = 0;
    for(u32 Index = 0; Index < AppState->Events.Count; Index++)
    {
        if (AppState->Events.Events[Index].Burst == SimBurst_SwingArc)
        {
            Arc = &AppState->Events.Events[Index];
        }
    }
    Check(Arc && Absolute(Arc->Angle + 0.5f * Pi32) < 0.01f);
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
    // NOTE(zoubir): from a stand it coasts about 94 in a third of a second
    // (the drag down to a run, then the skid); at the faster pace before
    // it was 118, and 62 before that
    RunPlayerFrames(&Test, 0, 20);
    printf("  dash from a stand: %.1f\n", Still->Position.Y - 300.f);
    Check(Still->Position.Y > 300.f + 92.f);
    Check(Still->Position.Y < 300.f + 105.f);
    Check(Absolute(Still->Position.X - 300.f) < 1.f);
    Check(Still->MovementCooldowns[PlayerMove_Dash] > 0.f);

    // NOTE(zoubir): moving, the keys win over the aim
    AppState->Players[1].Input.Move = V2(-1.f, 0.f);
    AppState->Players[1].Input.Aim = V2(1.f, 0.f);
    AppState->Players[1].Input.Pressed = PlayerButton_Dash;
    // NOTE(zoubir): about 154 with the run under it
    RunPlayerFrames(&Test, 1, 20);
    printf("  dash from a run: %.1f\n", 300.f - Runner->Position.X);
    Check(Runner->Position.X < 300.f - 136.f);
    DestroyTestWorld(&Test);
}

internal void
TestBlinkLandsAtCursorThroughWalls()
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
    // NOTE(zoubir): the press starts a half second wind-up; the cooldown
    // is spent but the player has not moved or started dodging
    Check(Blinker->CastSpell == PlayerSpell_Blink);
    Check(Absolute(Blinker->Position.X - 300.f) < 3.f);
    Check(Blinker->MovementCooldowns[PlayerMove_Blink] > 0.f);
    Check(Blinker->DashFlash == 0.f);
    float Seconds = (1 + FinishTestCast(&Test, 0)) * Test.Input.DeltaTime;
    Check(Absolute(Seconds - 0.5f) < 0.03f);
    Check(Absolute(Blinker->Position.X - 400.f) < 3.f);
    Check(Blinker->DashFlash > 0.f);

    // NOTE(zoubir): on cooldown, a second press does nothing
    float X = Blinker->Position.X;
    AppState->Players[0].Input.Pressed = PlayerButton_Blink;
    RunPlayerFrames(&Test, 0, 1);
    Check(Absolute(Blinker->Position.X - X) < 3.f);

    // NOTE(zoubir): a wall on the way is jumped through: the full 320
    world_entity *Walled = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {300, 700, 0});
    AddTestEntity(&Test, EntityType_StaticObject, {400, 700, 0}, Test.WallVolume);
    AppState->Players[1].Input.Aim = V2(1.f, 0.f);
    AppState->Players[1].Input.Pressed = PlayerButton_Blink;
    RunPlayerFrames(&Test, 1, 1);
    FinishTestCast(&Test, 1);
    Check(Absolute(Walled->Position.X - (300.f + 320.f)) < 3.f);

    // NOTE(zoubir): the cursor at or past the reach blinks the whole 320
    world_entity *Far = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                        2, {300, 1100, 0});
    AppState->Players[2].Input.Aim = V2(1.f, 0.f);
    AppState->Players[2].Input.Pressed = PlayerButton_Blink;
    RunPlayerFrames(&Test, 2, 1);
    FinishTestCast(&Test, 2);
    Check(Absolute(Far->Position.X - (300.f + 320.f)) < 3.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a dash during the blink's wind-up cuts it: no jump, and
// the blink's cooldown stays spent
internal void
TestDashCutsBlinkWindUp()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    AppState->Players[0].Input.Aim = V2(0.f, 1.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Blink;
    RunPlayerFrames(&Test, 0, 10);
    AppState->Players[0].Input.Aim = V2(1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Dash;
    RunPlayerFrames(&Test, 0, 60);
    Check(!IsPlayerCasting(Player));
    Check(Player->Position.Y < 320.f);
    Check(Player->MovementCooldowns[PlayerMove_Blink] > 0.f);
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

// NOTE(zoubir): two clicks a frame apart give one fireball, not two at
// once; the second is dropped, as the 6 s interval is too long to hold a
// click for (duel_tests.cpp has the next one)
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
    Check(CountPresent(Test.World, EntityType_FireBall) == 1);
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
    // NOTE(zoubir): only the short root is lost against a plain walk: 0.05 s
    // of a 260 run is 13 units
    Check(Walker->Position.X - Caster->Position.X < 16.f);
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
    Monster->MonsterKind = FindWalkerByWeight(false);
    Monster->MaxHp = Monster->Hp = 100.f;
    world_entity *Sword = AddSword(AppState, Test.World, &Test.Arena,
                                   {316, 300, 0}, Attacker,
                                   AnimationDirection_Right);
    UpdateSword(Sword, Test.World, &Test.Arena, AppState, Test.Input.DeltaTime);
    Check(Monster->Hp == 100.f - PlayerStats.SwordDamage);
    Check(Monster->Velocity.X > 0.9f * PlayerStats.SwordShove);
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
    Check(Diagonal->Hp == 100.f - PlayerStats.SwordDamage);
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

// NOTE(zoubir): the landing goes through a wall with room behind it; a
// target inside a wall lands on the nearest clear spot back toward the
// player, in a gap between two walls when there is one, and never past
// the edge of a bounded map
internal void
TestBlinkLandingGoesThroughWalls()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    // NOTE(zoubir): walls 32 wide at 400 and 480 leave a 48 unit gap,
    // room for the 30 unit wide player between X 431 and 449
    AddTestEntity(&Test, EntityType_StaticObject, {400, 300, 0}, Test.WallVolume);
    AddTestEntity(&Test, EntityType_StaticObject, {480, 300, 0}, Test.WallVolume);
    v2 Behind = FindBlinkLanding(AppState, Player, V2(560.f, 300.f));
    Check(Absolute(Behind.X - 560.f) < 0.01f);
    v2 Gap = FindBlinkLanding(AppState, Player, V2(480.f, 300.f));
    Check(Gap.X - 15.f >= 416.f - 0.01f && Gap.X + 15.f <= 464.f + 0.01f);
    v2 Short = FindBlinkLanding(AppState, Player, V2(400.f, 300.f));
    Check(Short.X + 15.f <= 384.f + 0.01f && Short.X > 360.f);
    v2 Clear = FindBlinkLanding(AppState, Player, V2(300.f, 420.f));
    Check(Absolute(Clear.Y - 420.f) < 0.01f);

    // NOTE(zoubir): the map is 2048 wide
    world_entity *Edge = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                         1, {1950, 700, 0});
    v2 Outside = FindBlinkLanding(AppState, Edge, V2(2150.f, 700.f));
    Check(Outside.X + 15.f <= 2048.f + 0.01f && Outside.X > 2020.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): walking for one second covers the same ground whatever
// the frame time; the 30 fps client used to walk twice as far as the
// 60 Hz server, so online prediction ran ahead and was pulled back
internal void
TestWalkSpeedDoesNotDependOnFrameRate()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Slow = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                         0, {300, 300, 0});
    world_entity *Fast = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                         1, {300, 700, 0});
    float Rates[2] = {30.f, 60.f};
    for(u32 SlotIndex = 0; SlotIndex < 2; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        Slot->Input.Move = V2(1.f, 0.f);
        for(u32 Frame = 0; Frame < (u32)Rates[SlotIndex]; Frame++)
        {
            float AnimationSpeed;
            animation_type Type;
            animation_direction Direction;
            UpdatePlayer(Slot, Test.World, &Test.Arena, 1.f / Rates[SlotIndex],
                         AppState, &AnimationSpeed, &Type, &Direction);
        }
    }
    float SlowWalked = Slow->Position.X - 300.f;
    float FastWalked = Fast->Position.X - 300.f;
    printf("  walked in 1 s: %.1f at 30 fps, %.1f at 60 fps\n",
           SlowWalked, FastWalked);
    // NOTE(zoubir): a 208 run less half the tenth of a second it takes to
    // get up to speed (player_stats.cpp)
    Check(FastWalked > 176.f && FastWalked < 200.f);
    Check(Absolute(SlowWalked - FastWalked) < 0.05f * FastWalked);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): one jump from the ground and one in the air, then none
// until the player lands; the second goes higher than one jump can
internal void
TestDoubleJump()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    player_slot *Slot = &AppState->Players[0];
    float SinglePeak = 0.f;
    Slot->Input.Pressed = PlayerButton_Jump;
    for(u32 Frame = 0; Frame < 40; Frame++)
    {
        RunPlayerFrames(&Test, 0, 1);
        SinglePeak = Maximum(SinglePeak, Player->Position.Z);
    }
    Check(SinglePeak > 32.f);
    Check(Player->Position.Z == 0.f);

    Slot->Input.Pressed = PlayerButton_Jump;
    RunPlayerFrames(&Test, 0, 12);
    Slot->Input.Pressed = PlayerButton_Jump;
    RunPlayerFrames(&Test, 0, 1);
    Check(Player->JumpsUsed == 2);
    float DoublePeak = 0.f;
    for(u32 Frame = 0; Frame < 60; Frame++)
    {
        // NOTE(zoubir): a third press in the air does nothing
        Slot->Input.Pressed = (Frame == 20) ? PlayerButton_Jump : 0;
        RunPlayerFrames(&Test, 0, 1);
        DoublePeak = Maximum(DoublePeak, Player->Position.Z);
        Check(Frame <= 20 || Player->Velocity.Z <= 0.f ||
              Player->Position.Z == 0.f);
    }
    Check(DoublePeak > 60.f);
    Check(Player->Position.Z == 0.f);
    Check(Player->JumpsUsed == 0);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): running at a boulder and jumping clears it; running at a
// wall and double jumping does not
internal void
TestJumpOverBoulderButNotWall()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    SetupCollisionVolumes(AppState, &Test.Arena);
    AppState->PlayerCollision = Test.UnitVolume;
    entity_collision_volume_group *Boulder =
        MakeSimpleGroundedCollisionVolume(&Test.Arena, {13.f, 8.f, 14.f});
    AddTestEntity(&Test, EntityType_StaticObject, {400, 300, 0}, Boulder);
    AddTestEntity(&Test, EntityType_StaticObject, {400, 600, 0},
                  AppState->WallCollision);
    world_entity *Jumper = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {330, 300, 0});
    world_entity *Climber = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                            1, {330, 600, 0});
    for(u32 SlotIndex = 0; SlotIndex < 2; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        Slot->Input.Move = V2(1.f, 0.f);
        Slot->Input.Pressed = PlayerButton_Jump;
        RunPlayerFrames(&Test, SlotIndex, 12);
        Slot->Input.Pressed = PlayerButton_Jump;
        RunPlayerFrames(&Test, SlotIndex, 90);
    }
    Check(Jumper->Position.X > 450.f);
    Check(Climber->Position.X + 15.f <= 400.f - 16.f + 0.01f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): Push waits out its short cast, then throws the monsters
// in its cone away and apart, leaving the one behind the player alone
internal void
TestPushThrowsCrowdApart()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Upper = AddTestEntity(&Test, EntityType_Monster,
                                        {360, 280, 0}, Test.UnitVolume);
    world_entity *Lower = AddTestEntity(&Test, EntityType_Monster,
                                        {360, 320, 0}, Test.UnitVolume);
    world_entity *Behind = AddTestEntity(&Test, EntityType_Monster,
                                         {240, 300, 0}, Test.UnitVolume);
    Upper->MaxHp = Upper->Hp = Lower->MaxHp = Lower->Hp = 100.f;
    Behind->MaxHp = Behind->Hp = 100.f;
    AppState->Players[0].Input.Aim = V2(1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Push;
    RunPlayerFrames(&Test, 0, 1);
    Check(IsPlayerCasting(Player));
    Check(Upper->Hp == 100.f);
    RunPlayerFrames(&Test, 0, 14);
    Check(!IsPlayerCasting(Player));
    Check(Upper->Hp < 100.f && Lower->Hp < 100.f);
    Check(Behind->Hp == 100.f);
    Check(Upper->Velocity.X > 200.f && Upper->Velocity.Y < 0.f);
    Check(Lower->Velocity.X > 200.f && Lower->Velocity.Y > 0.f);
    Check(HasStatus(Upper, StatusEffect_Stunned));
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a stunned monster's move: no steering, the drag the
// game gives it (lighter in the air) and gravity, and its status clocks
internal void
Tumble(test_world *Test, world_entity *Entity, u32 Frames)
{
    for(u32 Frame = 0; Frame < Frames && Entity->IsPresent; Frame++)
    {
        v3 DDEntity = -10.f * GetGroundFriction(Entity) * Entity->Velocity;
        DDEntity.Z = -1000.f;
        float MaxDistance = 10000.f;
        MoveEntity(Entity, Test->World, &Test->Arena, Test->Input.DeltaTime,
                   Test->AppState, DDEntity, &MaxDistance);
        CountDownStatusTimers(Entity, Test->Input.DeltaTime);
    }
}

// NOTE(zoubir): Push throws a monster through the air, far past the
// sword's reach, and it comes out of the stun slowed; one thrown into a
// wall slams it, is hurt and bounces back off
internal void
TestPushThrowsFarAndOffWalls()
{
    for(u32 Walled = 0; Walled < 2; Walled++)
    {
        test_world Test = CreateTestWorld();
        app_state *AppState = Test.AppState;
        AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
        world_entity *Target = AddTestEntity(&Test, EntityType_Monster,
                                             {360, 300, 0}, Test.UnitVolume);
        Target->MonsterKind = FindWalkerByWeight(false);
        Target->MaxHp = Target->Hp = 100.f;
        if (Walled)
        {
            AddTestEntity(&Test, EntityType_StaticObject, {440, 300, 0},
                          Test.WallVolume);
        }
        AppState->Players[0].Input.Aim = V2(1.f, 0.f);
        AppState->Players[0].Input.Pressed = PlayerButton_Push;
        RunPlayerFrames(&Test, 0, 15);
        Check(Target->Velocity.Z > 0.f);
        float HpAfterPush = Target->Hp;
        float Farthest = Target->Position.X;
        float Back = 0.f;
        for(u32 Frame = 0; Frame < 60; Frame++)
        {
            Tumble(&Test, Target, 1);
            Farthest = Maximum(Farthest, Target->Position.X);
            Back = Minimum(Back, Target->Velocity.X);
        }
        Check(!HasStatus(Target, StatusEffect_Stunned));
        Check(HasStatus(Target, StatusEffect_Slowed));
        if (Walled)
        {
            Check(Farthest + 15.f <= 424.f + 0.01f);
            Check(Back < -100.f);
            Check(Target->Hp < HpAfterPush);
            Check(Target->Position.X < Farthest - 20.f);
        }
        else
        {
            Check(Target->Position.X > 360.f + 180.f);
        }
        DestroyTestWorld(&Test);
    }
}

// NOTE(zoubir): Launch throws what stands at the aim into the air and
// stuns it; a stunned monster stays still where it lands
internal void
TestLaunchThrowsUpAndStuns()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    world_entity *Target = AddTestEntity(&Test, EntityType_Monster,
                                         {370, 300, 0}, Test.UnitVolume);
    Target->MonsterKind = FindWalkerByWeight(false);
    Target->MaxHp = Target->Hp = 100.f;
    AppState->Players[0].Input.Aim = V2(1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Launch;
    RunPlayerFrames(&Test, 0, 30);
    Check(Target->Hp < 100.f);
    Check(Target->Velocity.Z > 300.f);
    Check(HasStatus(Target, StatusEffect_Stunned));
    float Peak = 0.f;
    for(u32 Frame = 0; Frame < 60; Frame++)
    {
        Walk(&Test, Target, {0, 0}, 1);
        Peak = Maximum(Peak, Target->Position.Z);
    }
    Check(Peak > 60.f);
    Check(Target->Position.Z == 0.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a dash cuts a cast; nothing is hit and the cooldown stays
internal void
TestDashCutsAreaCast()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Target = AddTestEntity(&Test, EntityType_Monster,
                                         {370, 300, 0}, Test.UnitVolume);
    Target->MonsterKind = FindWalkerByWeight(false);
    Target->MaxHp = Target->Hp = 100.f;
    AppState->Players[0].Input.Aim = V2(1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Launch;
    RunPlayerFrames(&Test, 0, 2);
    AppState->Players[0].Input.Aim = V2(-1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Dash;
    RunPlayerFrames(&Test, 0, 30);
    Check(Target->Hp == 100.f);
    Check(Player->AreaCooldowns[PlayerArea_Launch] > 0.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a stunned player ignores its keys until the stun ends
internal void
TestStunnedPlayerCannotAct()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    ApplyStatus(Player, StatusEffect_Stunned, 1.f);
    AppState->Players[0].Input.Move = V2(1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Jump;
    RunPlayerFrames(&Test, 0, 10);
    Check(Absolute(Player->Position.X - 300.f) < 0.5f);
    Check(Player->Position.Z == 0.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a second jump, a Launch and the hard landings each ask for
// their burst once
internal void
TestAbilitiesAskForBursts()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    AddTestEntity(&Test, EntityType_Monster, {370, 300, 0}, Test.UnitVolume)
        ->MaxHp = 1000.f;
    player_slot *Slot = &AppState->Players[0];
    AppState->Events.Count = 0;
    Slot->Input.Aim = V2(1.f, 0.f);
    Slot->Input.Pressed = PlayerButton_Jump;
    RunPlayerFrames(&Test, 0, 10);
    Slot->Input.Pressed = PlayerButton_Jump;
    RunPlayerFrames(&Test, 0, 70);
    Check(CountBursts(AppState, SimBurst_AirJump) == 1);
    Check(CountBursts(AppState, SimBurst_Land) == 1);

    AppState->Events.Count = 0;
    Slot->Input.Pressed = PlayerButton_Launch;
    RunPlayerFrames(&Test, 0, 30);
    Check(CountBursts(AppState, SimBurst_CastGather) == 1);
    Check(CountBursts(AppState, SimBurst_LaunchColumn) == 1);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a monster pushed into a wall and killed by the slam is
// the pusher's kill
internal void
TestImpactKillIsThePushers()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    AddTestEntity(&Test, EntityType_StaticObject, {420, 300, 0},
                  Test.WallVolume);
    world_entity *Victim = AddTestEntity(&Test, EntityType_Monster,
                                         {350, 300, 0}, Test.UnitVolume);
    float PushDamage = PlayerAreaAbilities[PlayerArea_Push].Hit.Damage;
    Victim->MonsterKind = FindWalkerByWeight(false);
    Victim->MaxHp = 100.f;
    Victim->Hp = PushDamage + 1.f;
    AppState->Players[0].Input.Aim = V2(1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Push;
    for(u32 Frame = 0; Frame < 30 && Victim->IsPresent; Frame++)
    {
        RunPlayerFrames(&Test, 0, 1);
        if (Victim->IsPresent)
        {
            Walk(&Test, Victim, {0, 0}, 1);
        }
    }
    Check(!Victim->IsPresent);
    Check(AppState->Players[0].MonsterKills == 1);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the walk cycle keeps step with the ground speed, within
// limits, and only the walk cycle
internal void
TestWalkCycleFollowsSpeed()
{
    animation_set Set = {};
    Set.MoveSpeed = 93.f;
    world_entity Body = {};
    Body.AnimationSet = &Set;
    Body.Velocity = V3(93.f, 0.f, 0.f);
    Check(Absolute(MoveCycleRate(&Body, AnimationType_Move) - 1.f) < 0.01f);
    Body.Velocity = V3(0.f, 46.5f, 0.f);
    Check(Absolute(MoveCycleRate(&Body, AnimationType_Move) - 2.f) < 0.01f);
    // NOTE(zoubir): a full run, 260, still keeps step; a dash is past the
    // fastest the frames play
    Body.Velocity = V3(260.f, 0.f, 0.f);
    Check(Absolute(MoveCycleRate(&Body, AnimationType_Move) - 93.f / 260.f) < 0.01f);
    Body.Velocity = V3(PlayerStats.DashSpeed, 0.f, 0.f);
    Check(MoveCycleRate(&Body, AnimationType_Move) == 0.35f);
    Check(MoveCycleRate(&Body, AnimationType_Attack) == 1.f);
}

// NOTE(zoubir): a swing in the air shows the swing, not the jump frame
internal void
TestSwingInTheAirShowsSwing()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Pressed = PlayerButton_Jump;
    RunPlayerFrames(&Test, 0, 4);
    Slot->Input.Pressed = PlayerButton_Attack;
    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection = AnimationDirection_Right;
    UpdatePlayer(Slot, Test.World, &Test.Arena, Test.Input.DeltaTime, AppState,
                 &AnimationSpeed, &AnimationType, &AnimationDirection);
    Check(Slot->Entity->Position.Z > 0.f);
    Check(AnimationType == AnimationType_Attack);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a slam only works in the air; the player hangs there while
// it winds up, then it drives the player down and on landing throws and
// stuns what is around
internal void
TestSlamFromTheAir()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Near = AddTestEntity(&Test, EntityType_Monster,
                                       {350, 300, 0}, Test.UnitVolume);
    Near->MaxHp = Near->Hp = 100.f;
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Pressed = PlayerButton_Slam;
    RunPlayerFrames(&Test, 0, 2);
    Check(Player->MovementCooldowns[PlayerMove_Slam] == 0.f);

    Slot->Input.Pressed = PlayerButton_Jump;
    RunPlayerFrames(&Test, 0, 12);
    Check(Player->Position.Z > 20.f);
    Slot->Input.Pressed = PlayerButton_Slam;
    RunPlayerFrames(&Test, 0, 1);
    Check(Player->CastSpell == PlayerSpell_Slam);
    float Height = Player->Position.Z;
    RunPlayerFrames(&Test, 0, 10);
    Check(IsPlayerCasting(Player));
    Check(Absolute(Player->Position.Z - Height) < 0.01f);
    Check(Near->Hp == 100.f);
    for(u32 Frame = 0; Frame < 30 && IsPlayerCasting(Player); Frame++)
    {
        RunPlayerFrames(&Test, 0, 1);
    }
    Check(Player->Velocity.Z < -500.f);
    RunPlayerFrames(&Test, 0, 6);
    Check(Player->Position.Z == 0.f);
    Check(Near->Hp < 100.f);
    Check(HasStatus(Near, StatusEffect_Stunned));
    Check(Near->Velocity.Z > 0.f && Near->Velocity.X > 0.f);
    Check(Player->PendingLandArea == 0);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a predicted slam dives and lands but leaves the hit to the
// server
internal void
TestPredictedSlamLeavesHitToServer()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Near = AddTestEntity(&Test, EntityType_Monster,
                                       {350, 300, 0}, Test.UnitVolume);
    Near->MaxHp = Near->Hp = 100.f;
    player_slot *Slot = &AppState->Players[0];
    Slot->Predicted = true;
    Slot->Input.Pressed = PlayerButton_Jump;
    RunPlayerFrames(&Test, 0, 12);
    Slot->Input.Pressed = PlayerButton_Slam;
    RunPlayerFrames(&Test, 0, 25);
    Check(Player->Position.Z == 0.f);
    Check(Player->PendingLandArea == 0);
    Check(Near->Hp == 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): three quick swings are a combo whose last hit throws the
// target up and stuns it; after a pause the chain starts over
internal void
TestSwordComboFinisher()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Aim = V2(1.f, 0.f);
    AppState->Events.Count = 0;
    float Lifts[3];
    for(u32 Swing = 0; Swing < 3; Swing++)
    {
        world_entity *Target = AddTestEntity(&Test, EntityType_Monster,
                                             {340, 300, 0}, Test.UnitVolume);
        Target->MaxHp = Target->Hp = 1000.f;
        Slot->Input.Pressed = PlayerButton_Attack;
        RunPlayerFrames(&Test, 0, 1);
        // NOTE(zoubir): the swing's blade hits on its own update
        for(u32 Index = 0; Index < Test.World->EntityCount; Index++)
        {
            world_entity *Sword = &Test.World->Entities[Index];
            if (Sword->IsPresent && Sword->Type == EntityType_Sword)
            {
                UpdateSword(Sword, Test.World, &Test.Arena, AppState,
                            Test.Input.DeltaTime);
            }
        }
        Lifts[Swing] = Target->Velocity.Z;
        RunPlayerFrames(&Test, 0, 11);
        Check(Player->ComboStep == Swing);
        if (Swing == 2)
        {
            Check(HasStatus(Target, StatusEffect_Stunned));
        }
        RemoveEntity(Test.World, Target);
    }
    Check(Lifts[0] <= 0.f && Lifts[1] <= 0.f);
    Check(Lifts[2] > 100.f);
    Check(CountBursts(AppState, SimBurst_Finisher) == 1);
    // NOTE(zoubir): each step draws its own arc
    Check(CountBursts(AppState, SimBurst_SwingArc) == 1);
    Check(CountBursts(AppState, SimBurst_SwingArcBack) == 1);
    Check(CountBursts(AppState, SimBurst_SwingArcFinisher) == 1);

    RunPlayerFrames(&Test, 0, 60);
    Slot->Input.Pressed = PlayerButton_Attack;
    RunPlayerFrames(&Test, 0, 2);
    Check(Player->ComboStep == 0);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): with both jumps spent, a press just before landing jumps
// on landing; a press long before does not
internal void
TestJumpPressedJustBeforeLanding()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    player_slot *Slot = &AppState->Players[0];
    for(u32 Early = 0; Early < 2; Early++)
    {
        Slot->Input.Pressed = PlayerButton_Jump;
        RunPlayerFrames(&Test, 0, 10);
        Slot->Input.Pressed = PlayerButton_Jump;
        RunPlayerFrames(&Test, 0, 1);
        // NOTE(zoubir): fall until just above the ground
        u32 Frames = 0;
        while (Player->Velocity.Z >= 0.f ||
               Player->Position.Z > (Early ? 60.f : 6.f))
        {
            RunPlayerFrames(&Test, 0, 1);
            Check(++Frames < 200);
            if (Frames >= 200) break;
        }
        Slot->Input.Pressed = PlayerButton_Jump;
        bool32 JumpedAgain = false;
        bool32 Landed = false;
        for(u32 Frame = 0; Frame < 40; Frame++)
        {
            RunPlayerFrames(&Test, 0, 1);
            Landed = Landed || Player->Position.Z == 0.f;
            JumpedAgain = JumpedAgain || (Landed && Player->Velocity.Z > 0.f);
        }
        Check(Landed);
        Check(JumpedAgain == !Early);
        RunPlayerFrames(&Test, 0, 60);
    }
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a sword hit on a target in the air knocks it up; the same
// hit on the ground does not
internal void
TestSwordJugglesAirborneTarget()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Attacker = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                             0, {300, 300, 0});
    world_entity *Grounded = AddTestEntity(&Test, EntityType_Monster,
                                           {330, 300, 0}, Test.UnitVolume);
    world_entity *Flying = AddTestEntity(&Test, EntityType_Monster,
                                         {330, 330, 30}, Test.UnitVolume);
    Grounded->MaxHp = Grounded->Hp = Flying->MaxHp = Flying->Hp = 100.f;
    Flying->Velocity.Z = -100.f;
    hit *Hit = &SwordCuts[SwordCut_First].Hit;
    ApplyHit(AppState, Test.World, Grounded, Hit, V2(1.f, 0.f), Attacker, 0);
    ApplyHit(AppState, Test.World, Flying, Hit, V2(1.f, 0.f), Attacker, 0);
    Check(Grounded->Velocity.Z == 0.f);
    Check(Flying->Velocity.Z == KnockbackScale(Flying) * Hit->AirLift);
    Check(Flying->Velocity.Z > 0.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the same slam throws a light monster farther and higher
// than a heavy one, and both come down and stop
internal void
TestHeavyUnitsFlyLess()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    monster_kind Kinds[2] = {FindWalkerByWeight(false), FindWalkerByWeight(true)};
    Check(Kinds[0] != Kinds[1]);
    float Distance[2] = {};
    float Peak[2] = {};
    for(u32 Index = 0; Index < 2; Index++)
    {
        world_entity *Unit = AddTestEntity(&Test, EntityType_Monster,
                                           {300.f, 300.f + 200.f * Index, 0.f},
                                           Test.UnitVolume);
        Unit->MonsterKind = Kinds[Index];
        Unit->MaxHp = Unit->Hp = 1000.f;
        hit *Hit = &PlayerAreaAbilities[PlayerArea_Slam].Hit;
        Check(ApplyHit(AppState, Test.World, Unit, Hit, V2(1.f, 0.f), 0,
                       SIM_NOBODY));
        float AnimationSpeed;
        animation_type AnimationType;
        animation_direction AnimationDirection;
        float Dt = Test.Input.DeltaTime;
        for(u32 Frame = 0; Frame < 120; Frame++)
        {
            if (!TickHitStop(Unit, Dt))
            {
                UpdateMonster(Unit, Test.World, &Test.Arena, Dt, AppState,
                              &AnimationSpeed, &AnimationType,
                              &AnimationDirection);
            }
            Unit->StatusTimers[StatusEffect_Stunned] =
                Maximum(0.f, Unit->StatusTimers[StatusEffect_Stunned] - Dt);
            Peak[Index] = Maximum(Peak[Index], Unit->Position.Z);
        }
        Distance[Index] = Unit->Position.X - 300.f;
        Check(Unit->Position.Z == 0.f);
        Check(Length(Unit->Velocity.XY) < 1.f);
    }
    Check(Distance[1] < 0.8f * Distance[0]);
    Check(Peak[1] < Peak[0]);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the share of its speed a thrown body passes on to the
// unit it hits
internal float
ImpactTransfer(monster_kind Thrown, monster_kind Struck)
{
    test_world Test = CreateTestWorld();
    world_entity *Body = AddTestEntity(&Test, EntityType_Monster,
                                       {300, 300, 0}, Test.UnitVolume);
    world_entity *Other = AddTestEntity(&Test, EntityType_Monster,
                                        {340, 300, 0}, Test.UnitVolume);
    Body->MonsterKind = Thrown;
    Other->MonsterKind = Struck;
    Body->MaxHp = Body->Hp = Other->MaxHp = Other->Hp = 1000.f;
    ApplyStatus(Body, StatusEffect_Stunned, 1.f);
    Body->Velocity = V3(700.f, 0.f, 0.f);
    ImpactOnHit(Test.AppState, Test.World, Body, Other, V3(-1.f, 0.f, 0.f));
    float Result = Other->Velocity.X / 700.f;
    DestroyTestWorld(&Test);
    return Result;
}

// NOTE(zoubir): between equal weights a thrown body passes on what it
// always did; into a heavier unit much less, into a lighter one more
internal void
TestThrownWeightCarries()
{
    monster_kind Light = FindWalkerByWeight(false);
    monster_kind Heavy = FindWalkerByWeight(true);
    float Same = ImpactTransfer(Light, Light);
    float IntoHeavy = ImpactTransfer(Light, Heavy);
    float IntoLight = ImpactTransfer(Heavy, Light);
    printf("  speed passed on: equal %.2f, light into heavy %.2f, "
           "heavy into light %.2f\n", Same, IntoHeavy, IntoLight);
    Check(Absolute(Same - IMPACT_TRANSFER) < 0.01f);
    Check(IntoHeavy < 0.5f * Same);
    Check(IntoLight > 1.4f * Same && IntoLight < IMPACT_MAX_TRANSFER + 0.01f);
}

// NOTE(zoubir): a solid hit freezes the monster for a few ticks, never
// longer than HITSTOP_MAX; another hit right after does not freeze it
// again until the grace is over. Players never freeze
internal void
TestHitStopEnds()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Attacker = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                             0, {300, 300, 0});
    world_entity *Target = AddTestEntity(&Test, EntityType_Monster,
                                         {330, 300, 0}, Test.UnitVolume);
    Target->MaxHp = Target->Hp = 1000.f;
    hit *Hit = &SwordCuts[SwordCut_Finisher].Hit;
    float Dt = Test.Input.DeltaTime;
    ApplyHit(AppState, Test.World, Target, Hit, V2(1.f, 0.f), Attacker, 0);
    Check(Target->HitStop > 0.f);
    Check(Attacker->HitStop == 0.f);
    u32 FrozenTicks = 0;
    while (TickHitStop(Target, Dt) && FrozenTicks < 1000)
    {
        FrozenTicks++;
    }
    Check(FrozenTicks > 0);
    Check(FrozenTicks <= (u32)(HITSTOP_MAX / Dt) + 1);
    ApplyHit(AppState, Test.World, Target, Hit, V2(1.f, 0.f), Attacker, 0);
    Check(!TickHitStop(Target, Dt));
    for(u32 Frame = 0; Frame * Dt < HITSTOP_GRACE + Dt; Frame++)
    {
        Check(!TickHitStop(Target, Dt));
    }
    Check(Target->HitStop == 0.f);
    ApplyHit(AppState, Test.World, Target, Hit, V2(1.f, 0.f), Attacker, 0);
    Check(TickHitStop(Target, Dt));

    // NOTE(zoubir): a monster hitting a player pauses itself, not the player
    monster_ability Slam = {};
    Slam.Kind = MonsterAbility_Slam;
    Slam.Damage = 20.f;
    Slam.Radius = 80.f;
    Slam.Knockback = 600.f;
    Attacker->SpawnShield = 0.f;
    world_entity *Brute = AddTestEntity(&Test, EntityType_Monster,
                                        {260, 300, 0}, Test.UnitVolume);
    Brute->MaxHp = Brute->Hp = 100.f;
    Check(HurtPlayersInRadius(AppState, Test.World, Brute,
                              Brute->Position.XY, &Slam) == 1);
    Check(Attacker->HitStop == 0.f);
    Check(Brute->HitStop > 0.f);
    // NOTE(zoubir): and an area hit throws the player up as well as out
    Check(Attacker->Velocity.Z > 0.f);
    Check(Attacker->Velocity.X > 0.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): letting go after a run raises one skid; a short step none
internal void
TestStoppingFromARunSkids()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Move = V2(1.f, 0.f);
    RunPlayerFrames(&Test, 0, 2);
    Slot->Input.Move = V2(0.f, 0.f);
    AppState->Events.Count = 0;
    RunPlayerFrames(&Test, 0, 30);
    Check(CountBursts(AppState, SimBurst_Skid) == 0);

    Slot->Input.Move = V2(1.f, 0.f);
    RunPlayerFrames(&Test, 0, 40);
    Slot->Input.Move = V2(0.f, 0.f);
    AppState->Events.Count = 0;
    RunPlayerFrames(&Test, 0, 5);
    Check(CountBursts(AppState, SimBurst_Skid) == 1);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): body poses read from motion: a fast sideways move
// stretches a body long, a hard landing squashes it short, a hit flashes
// it, and being put far away starts it over at rest
internal void
TestBodyPosesFollowMotion()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->BodyPoses = (body_poses *)calloc(1, sizeof(body_poses));
    world_entity *Body = AddTestEntity(&Test, EntityType_Monster,
                                       {300, 300, 0}, Test.UnitVolume);
    Body->MaxHp = Body->Hp = 100.f;
    float Dt = 1.f / 60.f;
    UpdateBodyPoses(AppState, Dt);
    body_pose_draw Rest = GetBodyPose(AppState, Body);
    // NOTE(zoubir): at rest it only breathes, a little taller at most
    Check(Rest.Scale.X == 1.f && Rest.Flash == 0.f);
    Check(Rest.Scale.Y >= 1.f && Rest.Scale.Y <= 1.f + BODY_BREATH_DEPTH + 0.001f);

    // NOTE(zoubir): a dash's speed, 10 units a frame
    for(u32 Frame = 0; Frame < 10; Frame++)
    {
        Body->Position.X += 10.f;
        UpdateBodyPoses(AppState, Dt);
    }
    body_pose_draw Rush = GetBodyPose(AppState, Body);
    Check(Rush.Scale.X > 1.1f && Rush.Scale.Y < 0.95f);

    // NOTE(zoubir): falling, then on the ground
    for(u32 Frame = 0; Frame < 30; Frame++)
    {
        UpdateBodyPoses(AppState, Dt);
    }
    Body->Position.Z = 40.f;
    UpdateBodyPoses(AppState, Dt);
    Body->Velocity.Z = -480.f;
    for(u32 Frame = 0; Frame < 5; Frame++)
    {
        Body->Position.Z -= 8.f;
        UpdateBodyPoses(AppState, Dt);
    }
    Body->Position.Z = 0.f;
    Body->Velocity.Z = 0.f;
    UpdateBodyPoses(AppState, Dt);
    Check(GetBodyPose(AppState, Body).Scale.Y < 0.95f);

    Body->Hp -= 10.f;
    UpdateBodyPoses(AppState, Dt);
    Check(GetBodyPose(AppState, Body).Flash > 0.5f);

    // NOTE(zoubir): a kick upward in the air spins it; a jump off the
    // ground does not
    for(u32 Frame = 0; Frame < 30; Frame++)
    {
        UpdateBodyPoses(AppState, Dt);
    }
    Body->Velocity.Z = 340.f;
    Body->Position.Z = 6.f;
    UpdateBodyPoses(AppState, Dt);
    Check(GetBodyPose(AppState, Body).Angle == 0.f);
    Body->Velocity.Z = 70.f;
    Body->Position.Z = 30.f;
    UpdateBodyPoses(AppState, Dt);
    Body->Velocity.Z = 320.f;
    Body->Position.Z = 36.f;
    UpdateBodyPoses(AppState, Dt);
    UpdateBodyPoses(AppState, Dt);
    Check(Absolute(GetBodyPose(AppState, Body).Angle) > 0.3f);
    Body->Position.Z = 0.f;
    Body->Velocity.Z = 0.f;

    // NOTE(zoubir): turning squeezes it thin for a moment
    for(u32 Frame = 0; Frame < 30; Frame++)
    {
        UpdateBodyPoses(AppState, Dt);
    }
    float Before = GetBodyPose(AppState, Body).Scale.X;
    Body->AnimationState.LastAnimationDirection =
        Body->AnimationState.LastAnimationDirection == AnimationDirection_Left ?
        AnimationDirection_Right : AnimationDirection_Left;
    UpdateBodyPoses(AppState, Dt);
    Check(GetBodyPose(AppState, Body).Scale.X < Before - 0.1f);

    // NOTE(zoubir): put 500 units away: no stretch from it
    for(u32 Frame = 0; Frame < 60; Frame++)
    {
        UpdateBodyPoses(AppState, Dt);
    }
    Body->Position.X += 500.f;
    UpdateBodyPoses(AppState, Dt);
    UpdateBodyPoses(AppState, Dt);
    body_pose_draw Moved = GetBodyPose(AppState, Body);
    Check(Absolute(Moved.Scale.X - 1.f) < 0.01f);
    free(AppState->BodyPoses);
    AppState->BodyPoses = 0;
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): killing a monster readies the dash at once and halves
// the blink's wait
internal void
TestMonsterKillRefundsMovement()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {500, 300, 0}, Test.UnitVolume);
    Monster->MaxHp = Monster->Hp = 1.f;
    Player->MovementCooldowns[PlayerMove_Dash] = 0.6f;
    Player->MovementCooldowns[PlayerMove_Blink] = 2.f;
    AppState->Events.Count = 0;
    DamageEntity(AppState, Test.World, Monster, 10.f, Player);
    Check(!Monster->IsPresent);
    Check(CountBursts(AppState, SimBurst_Death) == 1);
    Check(Player->MovementCooldowns[PlayerMove_Dash] == 0.f);
    Check(Absolute(Player->MovementCooldowns[PlayerMove_Blink] - 1.f) < 0.001f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a respawned player cannot be hurt for a moment
internal void
TestRespawnIsShielded()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    DamageEntity(AppState, Test.World, Player, 1000.f, 0);
    Check(IsDeadPlayer(Player));
    for(u32 Frame = 0; Frame < 60 * 4 && IsDeadPlayer(Player); Frame++)
    {
        SimulateTick(AppState, &Test.Arena, Test.Input.DeltaTime);
    }
    Check(!IsDeadPlayer(Player));
    float Hp = Player->Hp;
    DamageEntity(AppState, Test.World, Player, 10.f, 0);
    Check(Player->Hp == Hp);
    for(u32 Frame = 0; Frame < 120; Frame++)
    {
        SimulateTick(AppState, &Test.Arena, Test.Input.DeltaTime);
    }
    DamageEntity(AppState, Test.World, Player, 10.f, 0);
    Check(Player->Hp == Hp - 10.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): an area ability's effects take their size and length from
// its row, so retuning the row moves them too
internal void
TestAreaEffectsFollowTheirRows()
{
    player_area_ability *Push = &PlayerAreaAbilities[PlayerArea_Push];
    player_area_ability *Launch = &PlayerAreaAbilities[PlayerArea_Launch];
    player_area_ability *Slam = &PlayerAreaAbilities[PlayerArea_Slam];
    burst_area PushMark = BurstArea(SimBurst_PushMark);
    Check(PushMark.Radius == Push->Radius);
    Check(PushMark.Seconds == PlayerSpells[Push->Spell].CastTime);
    Check(Absolute(Cos(PushMark.HalfAngle) - Push->ConeCos) < 0.001f);
    Check(BurstArea(SimBurst_LaunchMark).Radius == Launch->Radius);
    Check(BurstArea(SimBurst_LaunchColumn).Radius == Launch->Radius);
    Check(BurstArea(SimBurst_SlamRing).Radius == Slam->Radius);
    Check(BurstArea(SimBurst_ShockwaveRing).Radius ==
          PlayerAreaAbilities[PlayerArea_Shockwave].Radius);
    for(u32 Kind = 0; Kind < SimBurst_Count; Kind++)
    {
        burst_area Area = BurstArea((sim_burst)Kind);
        Check(Area.Radius > 0.f && Area.Seconds > 0.f);
    }
}

// NOTE(zoubir): a dash while falling stops the fall, and a jump with a
// dash at its top lands farther than one without
internal void
TestAirDashCarriesFarther()
{
    float Landed[2];
    for(u32 WithDash = 0; WithDash < 2; WithDash++)
    {
        test_world Test = CreateTestWorld();
        app_state *AppState = Test.AppState;
        AppState->PlayerCollision = Test.UnitVolume;
        world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                               0, {300, 300, 0});
        player_slot *Slot = &AppState->Players[0];
        Slot->Input.Move = V2(1.f, 0.f);
        Slot->Input.Pressed = PlayerButton_Jump;
        RunPlayerFrames(&Test, 0, 16);
        Check(Player->Velocity.Z < 0.f);
        if (WithDash)
        {
            Slot->Input.Pressed = PlayerButton_Dash;
            RunPlayerFrames(&Test, 0, 1);
            Check(Player->Velocity.Z > 0.f);
        }
        for(u32 Frame = 0; Frame < 120 && Player->Position.Z > 0.f; Frame++)
        {
            RunPlayerFrames(&Test, 0, 1);
        }
        Landed[WithDash] = Player->Position.X;
        DestroyTestWorld(&Test);
    }
    Check(Landed[1] > Landed[0] + 80.f);
}

// NOTE(zoubir): a stunned flyer falls to the ground, then climbs back to
// its hover height once the stun ends
internal void
TestStunGroundsFlyers()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    u32 FlyerKind = MonsterKind_Count;
    for(u32 Kind = 0; Kind < MonsterKind_Count && FlyerKind == MonsterKind_Count; Kind++)
    {
        if (GetMonsterStats((monster_kind)Kind)->FlyHeight > 10.f)
        {
            FlyerKind = Kind;
        }
    }
    Check(FlyerKind != MonsterKind_Count);
    if (FlyerKind == MonsterKind_Count)
    {
        DestroyTestWorld(&Test);
        return;
    }
    float FlyHeight = GetMonsterStats((monster_kind)FlyerKind)->FlyHeight;
    world_entity *Flyer = AddTestEntity(&Test, EntityType_Monster,
                                        {300, 300, FlyHeight}, Test.UnitVolume);
    Flyer->MonsterKind = (monster_kind)FlyerKind;
    Flyer->MaxHp = Flyer->Hp = 100.f;
    ApplyStatus(Flyer, StatusEffect_Stunned, 1.f);
    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection;
    float Dt = Test.Input.DeltaTime;
    for(u32 Frame = 0; Frame < 40; Frame++)
    {
        UpdateMonster(Flyer, Test.World, &Test.Arena, Dt, AppState,
                      &AnimationSpeed, &AnimationType, &AnimationDirection);
        Flyer->StatusTimers[StatusEffect_Stunned] -= Dt;
    }
    Check(Flyer->Position.Z == 0.f);
    Flyer->StatusTimers[StatusEffect_Stunned] = 0.f;
    for(u32 Frame = 0; Frame < 90; Frame++)
    {
        UpdateMonster(Flyer, Test.World, &Test.Arena, Dt, AppState,
                      &AnimationSpeed, &AnimationType, &AnimationDirection);
    }
    Check(Flyer->Position.Z > FlyHeight - 5.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): running into a boulder vaults it with no jump pressed;
// running into a wall does not
internal void
TestRunningVaultsBoulders()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    SetupCollisionVolumes(AppState, &Test.Arena);
    AppState->PlayerCollision = Test.UnitVolume;
    entity_collision_volume_group *Boulder =
        MakeSimpleGroundedCollisionVolume(&Test.Arena, {13.f, 8.f, 14.f});
    AddTestEntity(&Test, EntityType_StaticObject, {400, 300, 0}, Boulder);
    AddTestEntity(&Test, EntityType_StaticObject, {400, 600, 0},
                  AppState->WallCollision);
    world_entity *Runner = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {330, 300, 0});
    world_entity *Blocked = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                            1, {330, 600, 0});
    for(u32 SlotIndex = 0; SlotIndex < 2; SlotIndex++)
    {
        AppState->Players[SlotIndex].Input.Move = V2(1.f, 0.f);
        RunPlayerFrames(&Test, SlotIndex, 150);
    }
    Check(Runner->Position.X > 430.f);
    Check(Blocked->Position.X + 15.f <= 400.f - 16.f + 0.01f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): fixes from a review: a blink into a monster shoves it no
// harder than a dash; an air dash cancels a slam's dive; a stun cuts a
// cast and drops queued clicks
internal void
TestReviewFixes()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    player_slot *Slot = &AppState->Players[0];
    world_entity *InTheWay = AddTestEntity(&Test, EntityType_Monster,
                                           {380, 300, 0}, Test.UnitVolume);
    InTheWay->MaxHp = InTheWay->Hp = 100.f;
    Slot->Input.Aim = V2(1.f, 0.f);
    Slot->Input.Pressed = PlayerButton_Blink;
    RunPlayerFrames(&Test, 0, 1);
    FinishTestCast(&Test, 0);
    Check(Length(InTheWay->Velocity.XY) <= SHOULDER_SHARE * SHOULDER_MAX_SPEED + 1.f);

    // NOTE(zoubir): slam, then dash before landing: no hit on landing
    world_entity *Near = AddTestEntity(&Test, EntityType_Monster,
                                       {Player->Position.X, 360, 0}, Test.UnitVolume);
    Near->MaxHp = Near->Hp = 100.f;
    RunPlayerFrames(&Test, 0, 60);
    Slot->Input.Pressed = PlayerButton_Jump;
    RunPlayerFrames(&Test, 0, 12);
    Slot->Input.Pressed = PlayerButton_Slam;
    RunPlayerFrames(&Test, 0, 1);
    Slot->Input.Pressed = PlayerButton_Dash;
    RunPlayerFrames(&Test, 0, 1);
    Check(Player->PendingLandArea == 0);
    RunPlayerFrames(&Test, 0, 60);
    Check(Near->Hp == 100.f);

    // NOTE(zoubir): a stun cuts a Launch's cast
    world_entity *Target = AddTestEntity(&Test, EntityType_Monster,
                                         {Player->Position.X + 70.f, Player->Position.Y, 0},
                                         Test.UnitVolume);
    Target->MaxHp = Target->Hp = 100.f;
    Slot->Input.Pressed = PlayerButton_Launch;
    RunPlayerFrames(&Test, 0, 2);
    Check(IsPlayerCasting(Player));
    ApplyStatus(Player, StatusEffect_Stunned, 1.f);
    RunPlayerFrames(&Test, 0, 30);
    Check(!IsPlayerCasting(Player));
    Check(Target->Hp == 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a predicted Launch casts (slowing the player) but neither
// hits nor asks for its effects; the server does that
internal void
TestPredictedCastLeavesHitToServer()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Target = AddTestEntity(&Test, EntityType_Monster,
                                         {370, 300, 0}, Test.UnitVolume);
    Target->MaxHp = Target->Hp = 100.f;
    player_slot *Slot = &AppState->Players[0];
    Slot->Predicted = true;
    Slot->Input.Aim = V2(1.f, 0.f);
    Slot->Input.Pressed = PlayerButton_Launch;
    AppState->Events.Count = 0;
    RunPlayerFrames(&Test, 0, 2);
    Check(IsPlayerCasting(Player));
    RunPlayerFrames(&Test, 0, 30);
    Check(!IsPlayerCasting(Player));
    Check(Target->Hp == 100.f);
    Check(CountBursts(AppState, SimBurst_LaunchColumn) == 0);
    Check(CountBursts(AppState, SimBurst_LaunchMark) == 0);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): dashing into a monster hits it once and stuns it; walking
// into it only shoulders it
internal void
TestDashStrike()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1, {300, 600, 0});
    world_entity *Dashed = AddTestEntity(&Test, EntityType_Monster,
                                         {360, 300, 0}, Test.UnitVolume);
    world_entity *Walked = AddTestEntity(&Test, EntityType_Monster,
                                         {360, 600, 0}, Test.UnitVolume);
    Dashed->MaxHp = Dashed->Hp = Walked->MaxHp = Walked->Hp = 100.f;
    AppState->Players[0].Input.Move = V2(1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Dash;
    RunPlayerFrames(&Test, 0, 10);
    AppState->Players[1].Input.Move = V2(1.f, 0.f);
    RunPlayerFrames(&Test, 1, 40);
    Check(Dashed->Hp == 100.f - DashStrikeHit.Damage);
    Check(HasStatus(Dashed, StatusEffect_Stunned));
    Check(Walked->Hp == 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): hits on monsters near the local player add up while they
// come quickly; damage-over-time ticks do not count; a pause ends it
internal void
TestHitCounter()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    AppState->LocalPlayerIndex = 0;
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {360, 300, 0}, Test.UnitVolume);
    Monster->MaxHp = Monster->Hp = 1000.f;
    hit_numbers *Fx = (hit_numbers *)calloc(1, sizeof(hit_numbers));
    float Dt = 1.f / 60.f;
    UpdateHitNumbers(Fx, AppState, Dt);
    for(u32 Hit = 0; Hit < 3; Hit++)
    {
        Monster->Hp -= 10.f;
        UpdateHitNumbers(Fx, AppState, Dt);
        for(u32 Frame = 0; Frame < 20; Frame++)
        {
            UpdateHitNumbers(Fx, AppState, Dt);
        }
    }
    Check(Fx->Combo == 3);
    Monster->Hp -= 2.f;
    UpdateHitNumbers(Fx, AppState, Dt);
    Check(Fx->Combo == 3);
    for(u32 Frame = 0; Frame < 120; Frame++)
    {
        UpdateHitNumbers(Fx, AppState, Dt);
    }
    Check(Fx->Combo == 0);
    free(Fx);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a player high in a jump takes nothing from a monster's
// area blast, a fireball or another player's shockwave; standing on the
// ground it takes all three
internal void
TestJumpClearsGroundHits()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Caster = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Jumper = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {360, 300, 0});
    Caster->SpawnShield = Jumper->SpawnShield = 0.f;
    world_entity *Brute = AddTestEntity(&Test, EntityType_Monster,
                                        {420, 300, 0}, Test.UnitVolume);
    Brute->MaxHp = Brute->Hp = 100.f;
    monster_ability Slam = {};
    Slam.Kind = MonsterAbility_Slam;
    Slam.Damage = 20.f;
    Slam.Radius = 30.f;
    Slam.Knockback = 600.f;
    for(u32 Pass = 0; Pass < 2; Pass++)
    {
        bool32 Jumping = Pass == 0;
        Jumper->MaxHp = Jumper->Hp = 1000.f;
        Jumper->Velocity = {};
        Jumper->Position.Z = Jumper->GroundZ + (Jumping ? 30.f : 0.f);
        Check(IsJumpingClear(Jumper) == Jumping);

        Check(HurtPlayersInRadius(AppState, Test.World, Brute,
                                  Jumper->Position.XY, &Slam) == (Jumping ? 0u : 1u));
        Check((Jumper->Hp < 1000.f) == !Jumping);

        float Before = Jumper->Hp;
        world_entity *FireBall = AddFireBall(AppState, Test.World, &Test.Arena,
                                             Caster, V3(330.f, 300.f, 30.f),
                                             V3(600.f, 0.f, 0.f));
        FireBallHit(AppState, Test.World, FireBall, Jumper);
        Check((Jumper->Hp < Before) == !Jumping);

        Before = Jumper->Hp;
        FireAreaAbility(AppState, Test.World, Caster,
                        &PlayerAreaAbilities[PlayerArea_Shockwave], V2(1.f, 0.f));
        Check((Jumper->Hp < Before) == !Jumping);
    }
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a blink passes through a monster in the way and lands
// beyond it; a wall still stops it
internal void
TestBlinkPassesThroughUnits()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *InTheWay = AddTestEntity(&Test, EntityType_Monster,
                                           {340, 300, 0}, Test.UnitVolume);
    InTheWay->MaxHp = InTheWay->Hp = 100.f;
    AppState->Players[0].Input.Aim = V2(1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Blink;
    RunPlayerFrames(&Test, 0, 1);
    FinishTestCast(&Test, 0);
    Check(Player->Position.X > 400.f);
    Check(!Player->Phasing);
    DestroyTestWorld(&Test);
}

#include "player_combo_tests.cpp"

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
    printf("TestBlinkLandsAtCursorThroughWalls\n");
    TestBlinkLandsAtCursorThroughWalls();
    printf("TestDashCutsBlinkWindUp\n");
    TestDashCutsBlinkWindUp();
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
    printf("TestBlinkLandingGoesThroughWalls\n");
    TestBlinkLandingGoesThroughWalls();
    printf("TestWalkSpeedDoesNotDependOnFrameRate\n");
    TestWalkSpeedDoesNotDependOnFrameRate();
    printf("TestDoubleJump\n");
    TestDoubleJump();
    printf("TestJumpOverBoulderButNotWall\n");
    TestJumpOverBoulderButNotWall();
    printf("TestPushThrowsCrowdApart\n");
    TestPushThrowsCrowdApart();
    printf("TestPushThrowsFarAndOffWalls\n");
    TestPushThrowsFarAndOffWalls();
    printf("TestLaunchThrowsUpAndStuns\n");
    TestLaunchThrowsUpAndStuns();
    printf("TestDashCutsAreaCast\n");
    TestDashCutsAreaCast();
    printf("TestStunnedPlayerCannotAct\n");
    TestStunnedPlayerCannotAct();
    printf("TestAbilitiesAskForBursts\n");
    TestAbilitiesAskForBursts();
    printf("TestImpactKillIsThePushers\n");
    TestImpactKillIsThePushers();
    printf("TestThrownWeightCarries\n");
    TestThrownWeightCarries();
    printf("TestWalkCycleFollowsSpeed\n");
    TestWalkCycleFollowsSpeed();
    printf("TestSwingInTheAirShowsSwing\n");
    TestSwingInTheAirShowsSwing();
    printf("TestSlamFromTheAir\n");
    TestSlamFromTheAir();
    printf("TestPredictedSlamLeavesHitToServer\n");
    TestPredictedSlamLeavesHitToServer();
    printf("TestSwordComboFinisher\n");
    TestSwordComboFinisher();
    printf("TestJumpPressedJustBeforeLanding\n");
    TestJumpPressedJustBeforeLanding();
    printf("TestSwordJugglesAirborneTarget\n");
    TestSwordJugglesAirborneTarget();
    printf("TestHeavyUnitsFlyLess\n");
    TestHeavyUnitsFlyLess();
    printf("TestHitStopEnds\n");
    TestHitStopEnds();
    printf("TestJumpClearsGroundHits\n");
    TestJumpClearsGroundHits();
    printf("TestStoppingFromARunSkids\n");
    TestStoppingFromARunSkids();
    printf("TestBodyPosesFollowMotion\n");
    TestBodyPosesFollowMotion();
    printf("TestMonsterKillRefundsMovement\n");
    TestMonsterKillRefundsMovement();
    printf("TestRespawnIsShielded\n");
    TestRespawnIsShielded();
    printf("TestAreaEffectsFollowTheirRows\n");
    TestAreaEffectsFollowTheirRows();
    printf("TestAirDashCarriesFarther\n");
    TestAirDashCarriesFarther();
    printf("TestStunGroundsFlyers\n");
    TestStunGroundsFlyers();
    printf("TestRunningVaultsBoulders\n");
    TestRunningVaultsBoulders();
    printf("TestReviewFixes\n");
    TestReviewFixes();
    printf("TestPredictedCastLeavesHitToServer\n");
    TestPredictedCastLeavesHitToServer();
    printf("TestDashStrike\n");
    TestDashStrike();
    printf("TestHitCounter\n");
    TestHitCounter();
    printf("TestBlinkPassesThroughUnits\n");
    TestBlinkPassesThroughUnits();
    RunPlayerComboTests();
}
