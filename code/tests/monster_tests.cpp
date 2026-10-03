/* Monster tests: every kind's definition and sprite sheet, and each
   ability kind's windup, hit and dodge rules. Included by sim_tests.cpp,
   which calls RunMonsterTests. */

internal world_entity *
AddTestMonster(test_world *Test, monster_kind Kind, v3 Position)
{
    app_state *AppState = Test->AppState;
    if (!AppState->Monsters)
    {
        AppState->Monsters = CreateMonsterPopulation(&Test->Arena, 0, 11);
    }
    AppState->PlayerCollision = Test->UnitVolume;
    AppState->BatCollision = Test->UnitVolume;
    world_entity *Monster = AddMonster(AppState, Test->World, &Test->Arena,
                                       Position, Kind);
    // NOTE(zoubir): keep the plain bite out of the way so only the
    // ability under test deals damage
    Monster->AttackCooldown = 1000.f;
    return Monster;
}

internal world_entity *
AddTestPlayer(test_world *Test, v3 Position)
{
    world_entity *Player = AddTestEntity(Test, EntityType_Player, Position,
                                         Test->UnitVolume);
    Player->MaxHp = Player->Hp = 100.f;
    return Player;
}

internal void
StepMonster(test_world *Test, world_entity *Monster, u32 Frames)
{
    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection;
    for(u32 Frame = 0; Frame < Frames && Monster->IsPresent; Frame++)
    {
        Monster->AttackCooldown = 1000.f;
        UpdateMonster(Monster, Test->World, &Test->Arena, Test->Input.DeltaTime,
                      Test->AppState, &AnimationSpeed, &AnimationType,
                      &AnimationDirection);
    }
}

inline u32
SecondsToFrames(float Seconds)
{
    u32 Result = (u32)(Seconds * 60.f) + 2;
    return Result;
}

internal void
TestMonsterDefsAreValid()
{
    for(u32 KindIndex = 0; KindIndex < MonsterKind_Count; KindIndex++)
    {
        monster_def *Def = GetMonsterDef((monster_kind)KindIndex);
        Check(Def->Name != 0);
        Check(Def->MaxHp > 0.f);
        Check(Def->FrameSize >= 16 && Def->FrameSize <= 128);
        Check(Def->AggroRange > Def->StopRange);
        Check(Def->AbilityCount <= MAX_MONSTER_ABILITIES);
        for(u32 Row = 0; Row < MonsterRow_Count; Row++)
        {
            Check(Def->FrameCounts[Row] >= 1);
            Check(Def->FrameCounts[Row] <= MONSTER_SHEET_COLUMNS);
            Check(Def->SecondsPerFrame[Row] > 0.f);
        }
        for(u32 AbilityIndex = 0;
            AbilityIndex < Def->AbilityCount;
            AbilityIndex++)
        {
            monster_ability *Ability = &Def->Abilities[AbilityIndex];
            Check(Ability->Kind > MonsterAbility_None &&
                  Ability->Kind < MonsterAbility_Count);
            Check(Ability->Name != 0);
            // NOTE(zoubir): no windup means no telegraph, which is unfair
            Check(Ability->Windup >= 0.3f);
            Check(Ability->Cooldown > Ability->Windup);
            Check(Ability->MaxRange >= Ability->MinRange);
            Check(Ability->Damage > 0.f);
            Check(Ability->Radius > 0.f);
            Check(Ability->Count <= MAX_ABILITY_POINTS);
            if (Ability->Status != StatusEffect_None)
            {
                Check(Ability->Status < StatusEffect_Count);
                Check(Ability->StatusSeconds > 0.f);
            }
            // NOTE(zoubir): only mortar spots leave hazards so far
            Check(Ability->HazardSeconds == 0.f ||
                  (Ability->Kind == MonsterAbility_Mortar &&
                   Ability->HazardStyle < HazardStyle_Count &&
                   Ability->Status != StatusEffect_None));
            if (Ability->Kind == MonsterAbility_Volley)
            {
                Check(Ability->Count >= 1 && Ability->Count <= MAX_VOLLEY_SHOTS);
                Check(Ability->Speed > 0.f && Ability->Active > 0.f);
                Check(Ability->ShotStyle < ShotStyle_Count);
            }
            if (Ability->Kind == MonsterAbility_Charge)
            {
                Check(Ability->Speed > 0.f && Ability->Active > 0.f);
            }
        }
    }
}

// NOTE(zoubir): every frame a row uses has something drawn in it, and the
// unused cells stay empty
internal void
TestMonsterSheetsHaveEveryFrame()
{
    for(u32 KindIndex = 0; KindIndex < MonsterKind_Count; KindIndex++)
    {
        monster_def *Def = GetMonsterDef((monster_kind)KindIndex);
        u32 Width = MonsterSheetWidth(Def);
        u32 Height = MonsterSheetHeight(Def);
        u32 *Pixels = (u32 *)calloc(Width * Height, sizeof(u32));
        BuildMonsterSheet((monster_kind)KindIndex, Pixels);
        for(u32 Row = 0; Row < MonsterRow_Count; Row++)
        {
            for(u32 Column = 0; Column < MONSTER_SHEET_COLUMNS; Column++)
            {
                u32 Drawn = 0;
                for(u32 Y = 0; Y < Def->FrameSize; Y++)
                {
                    for(u32 X = 0; X < Def->FrameSize; X++)
                    {
                        u32 Pixel = Pixels[(Row * Def->FrameSize + Y) * Width +
                                           Column * Def->FrameSize + X];
                        Drawn += Pixel != ART_CLEAR;
                    }
                }
                bool32 Used = Column < Def->FrameCounts[Row];
                // NOTE(zoubir): a dissolving frame still keeps some pixels
                Check(Used ? Drawn > 40 : Drawn == 0);
            }
        }
        free(Pixels);
    }
}

internal void
TestSlamHitsOnlyAfterWindup()
{
    test_world Test = CreateTestWorld();
    world_entity *Brute = AddTestMonster(&Test, MonsterKind_Brute, {1000, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {1050, 1000, 0});
    monster_ability *Slam = &GetMonsterDef(MonsterKind_Brute)->Abilities[0];

    StepMonster(&Test, Brute, 1);
    Check(Brute->AbilityPhase == AbilityPhase_Windup);
    StepMonster(&Test, Brute, SecondsToFrames(Slam->Windup) - 4);
    Check(Player->Hp == 100.f);
    StepMonster(&Test, Brute, 4);
    Check(Player->Hp == 100.f - Slam->Damage);
    Check(Player->Velocity.X > 0.f);
    DestroyTestWorld(&Test);
}

internal void
TestSlamMissesPlayerWhoLeft()
{
    test_world Test = CreateTestWorld();
    world_entity *Brute = AddTestMonster(&Test, MonsterKind_Brute, {1000, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {1050, 1000, 0});
    monster_ability *Slam = &GetMonsterDef(MonsterKind_Brute)->Abilities[0];

    StepMonster(&Test, Brute, 1);
    Check(Brute->AbilityPhase == AbilityPhase_Windup);
    Player->Position.X = 1000.f + Slam->Radius + 20.f;
    StepMonster(&Test, Brute, SecondsToFrames(Slam->Windup + Slam->Active));
    Check(Player->Hp == 100.f);
    DestroyTestWorld(&Test);
}

internal void
TestChargeHitsPlayerInLine()
{
    test_world Test = CreateTestWorld();
    world_entity *Ravager = AddTestMonster(&Test, MonsterKind_Ravager, {600, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {800, 1000, 0});
    monster_ability *Rush = &GetMonsterDef(MonsterKind_Ravager)->Abilities[0];

    StepMonster(&Test, Ravager, 1);
    Check(Ravager->AbilityPhase == AbilityPhase_Windup);
    Check(Ravager->AbilityAim.X > 0.99f);
    StepMonster(&Test, Ravager, SecondsToFrames(Rush->Windup + Rush->Active));
    Check(Player->Hp == 100.f - Rush->Damage);
    Check(Ravager->AbilityPhase == AbilityPhase_Recover);
    DestroyTestWorld(&Test);
}

internal void
TestChargeIntoWallStuns()
{
    test_world Test = CreateTestWorld();
    world_entity *Ravager = AddTestMonster(&Test, MonsterKind_Ravager, {600, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {800, 1000, 0});
    monster_ability *Rush = &GetMonsterDef(MonsterKind_Ravager)->Abilities[0];

    StepMonster(&Test, Ravager, 1);
    Check(Ravager->AbilityPhase == AbilityPhase_Windup);
    // NOTE(zoubir): the player ducks behind a wall during the windup
    AddTestEntity(&Test, EntityType_StaticObject, {700, 1000, 0}, Test.WallVolume);
    bool32 Stunned = false;
    for(u32 Frame = 0;
        Frame < SecondsToFrames(Rush->Windup + Rush->Active);
        Frame++)
    {
        StepMonster(&Test, Ravager, 1);
        if (Ravager->AbilityPhase == AbilityPhase_Recover &&
            Ravager->AbilityTimer > Rush->Recover)
        {
            Stunned = true;
            break;
        }
    }
    Check(Stunned);
    Check(Player->Hp == 100.f);
    Check(Ravager->Position.X < 700.f);
    DestroyTestWorld(&Test);
}

internal void
TestMortarLandsOnStandingTarget()
{
    test_world Test = CreateTestWorld();
    world_entity *Toad = AddTestMonster(&Test, MonsterKind_Toad, {600, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {800, 1000, 0});
    monster_ability *Barrage = &GetMonsterDef(MonsterKind_Toad)->Abilities[0];

    StepMonster(&Test, Toad, 1);
    Check(Toad->AbilityPhase == AbilityPhase_Windup);
    Check(Toad->AbilityPointCount == Barrage->Count);
    Check(Length(Toad->AbilityPoints[0] - Player->Position.XY) < 1.f);
    StepMonster(&Test, Toad, SecondsToFrames(Barrage->Windup));
    Check(Player->Hp <= 100.f - Barrage->Damage);
    DestroyTestWorld(&Test);
}

internal void
TestBlinkLandsBehindTarget()
{
    test_world Test = CreateTestWorld();
    world_entity *Shade = AddTestMonster(&Test, MonsterKind_Shade, {600, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {800, 1000, 0});
    monster_ability *Step = &GetMonsterDef(MonsterKind_Shade)->Abilities[0];

    StepMonster(&Test, Shade, 1);
    Check(Shade->AbilityPhase == AbilityPhase_Windup);
    Check(Shade->Position.X < 700.f);
    StepMonster(&Test, Shade, SecondsToFrames(Step->Windup));
    Check(Shade->Position.X > Player->Position.X);
    Check(Player->Hp == 100.f - Step->Damage);
    // NOTE(zoubir): it faces back toward the player it went past
    Check(Shade->AbilityAim.X < 0.f);
    DestroyTestWorld(&Test);
}

internal void
TestAbilityGoesBackOnCooldown()
{
    test_world Test = CreateTestWorld();
    world_entity *Brute = AddTestMonster(&Test, MonsterKind_Brute, {1000, 1000, 0});
    AddTestPlayer(&Test, {1050, 1000, 0});
    monster_ability *Slam = &GetMonsterDef(MonsterKind_Brute)->Abilities[0];

    StepMonster(&Test, Brute, 1);
    StepMonster(&Test, Brute, SecondsToFrames(Slam->Windup + Slam->Active +
                                              Slam->Recover));
    Check(Brute->AbilityPhase == AbilityPhase_Ready);
    Check(Brute->AbilityCooldowns[0] > Slam->Cooldown - 0.2f);
    DestroyTestWorld(&Test);
}

inline u32
CountEntitiesOfType(world *World, entity_type Type)
{
    u32 Result = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        Result += Entity->IsPresent && Entity->Type == Type;
    }
    return Result;
}

// NOTE(zoubir): shots are entities, so they need the simulation tick to fly
internal void
StepWorld(test_world *Test, u32 Frames)
{
    for(u32 Frame = 0; Frame < Frames; Frame++)
    {
        world *World = Test->World;
        u32 EntityCount = World->EntityCount;
        for(u32 EntityIndex = 0; EntityIndex < EntityCount; EntityIndex++)
        {
            world_entity *Entity = &World->Entities[EntityIndex];
            if (!Entity->IsPresent)
            {
                continue;
            }
            if (Entity->Type == EntityType_Monster)
            {
                StepMonster(Test, Entity, 1);
            }
            else if (Entity->Type == EntityType_MonsterShot)
            {
                UpdateMonsterShot(Entity, World, &Test->Arena,
                                  Test->Input.DeltaTime, Test->AppState);
            }
            else if (Entity->Type == EntityType_MonsterHazard)
            {
                UpdateMonsterHazard(Entity, World, Test->AppState,
                                    Test->Input.DeltaTime);
            }
        }
        UpdateStatusEffects(Test->AppState, World, Test->Input.DeltaTime);
    }
}

internal void
TestVolleyFansShotsAndHitsPlayerInLane()
{
    test_world Test = CreateTestWorld();
    Test.AppState->FireBallCollision = Test.FireBallVolume;
    world_entity *Imp = AddTestMonster(&Test, MonsterKind_Imp, {600, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {800, 1000, 0});
    monster_ability *Fan = &GetMonsterDef(MonsterKind_Imp)->Abilities[0];

    StepWorld(&Test, 1);
    Check(Imp->AbilityPhase == AbilityPhase_Windup);
    Check(CountEntitiesOfType(Test.World, EntityType_MonsterShot) == 0);
    StepWorld(&Test, SecondsToFrames(Fan->Windup) - 1);
    Check(CountEntitiesOfType(Test.World, EntityType_MonsterShot) == Fan->Count);
    // NOTE(zoubir): only the middle lane points at the player
    StepWorld(&Test, SecondsToFrames(Fan->Active));
    // NOTE(zoubir): the ember also sets the player burning
    float BurnLimit = STATUS_BURN_DPS * (Fan->StatusSeconds + STATUS_TICK_SECONDS);
    Check(Player->Hp <= 100.f - Fan->Damage);
    Check(Player->Hp >= 100.f - Fan->Damage - BurnLimit);
    Check(CountEntitiesOfType(Test.World, EntityType_MonsterShot) == 0);
    DestroyTestWorld(&Test);
}

internal void
TestWallBlocksVolley()
{
    test_world Test = CreateTestWorld();
    Test.AppState->FireBallCollision = Test.FireBallVolume;
    world_entity *Imp = AddTestMonster(&Test, MonsterKind_Imp, {600, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {800, 1000, 0});
    monster_ability *Fan = &GetMonsterDef(MonsterKind_Imp)->Abilities[0];
    AddTestEntity(&Test, EntityType_StaticObject, {700, 1000, 0}, Test.WallVolume);

    StepWorld(&Test, 1 + SecondsToFrames(Fan->Windup + Fan->Active));
    Check(Imp->IsPresent);
    Check(Player->Hp == 100.f);
    DestroyTestWorld(&Test);
}

internal void
TestShotsDoNotHurtMonsters()
{
    test_world Test = CreateTestWorld();
    Test.AppState->FireBallCollision = Test.FireBallVolume;
    AddTestMonster(&Test, MonsterKind_Imp, {600, 1000, 0});
    world_entity *Brute = AddTestMonster(&Test, MonsterKind_Brute, {700, 1000, 0});
    AddTestPlayer(&Test, {900, 1000, 0});
    float BruteHp = Brute->Hp;
    StepWorld(&Test, 120);
    Check(Brute->Hp == BruteHp);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the blink crosses from chunk 0 into chunk 1 (512 units
// wide); a player walking into the new spot must bump into the shade
internal void
TestBlinkMovesMonsterToNewChunk()
{
    test_world Test = CreateTestWorld();
    world_entity *Shade = AddTestMonster(&Test, MonsterKind_Shade, {440, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {520, 1000, 0});
    monster_ability *Step = &GetMonsterDef(MonsterKind_Shade)->Abilities[0];

    StepMonster(&Test, Shade, 1 + SecondsToFrames(Step->Windup));
    Check(Shade->Position.X > 512.f);
    world_entity *Walker = AddTestPlayer(&Test, {Shade->Position.X + 80.f, 1000, 0});
    Walk(&Test, Walker, {-1, 0}, 40);
    Check(Walker->Position.X >= Shade->Position.X + 29.f);
    DestroyTestWorld(&Test);
}

internal void
TestStatusRefreshesInsteadOfStacking()
{
    world_entity Entity = {};
    ApplyStatus(&Entity, StatusEffect_Burning, 2.f);
    ApplyStatus(&Entity, StatusEffect_Burning, 1.f);
    Check(Entity.StatusTimers[StatusEffect_Burning] == 2.f);
    ApplyStatus(&Entity, StatusEffect_Burning, 3.f);
    Check(Entity.StatusTimers[StatusEffect_Burning] == 3.f);
    ApplyStatus(&Entity, StatusEffect_None, 3.f);
    Check(GetMoveSpeedScale(&Entity) == 1.f);
    ApplyStatus(&Entity, StatusEffect_Slowed, 1.f);
    Check(GetMoveSpeedScale(&Entity) == STATUS_SLOW_SCALE);
}

internal void
TestBurnDamagesInTicksThenStops()
{
    test_world Test = CreateTestWorld();
    world_entity *Player = AddTestPlayer(&Test, {800, 1000, 0});
    ApplyStatus(Player, StatusEffect_Burning, 2.f);
    StepWorld(&Test, 20);
    // NOTE(zoubir): no tick before half a second
    Check(Player->Hp == 100.f);
    StepWorld(&Test, 180);
    float Tick = STATUS_BURN_DPS * STATUS_TICK_SECONDS;
    Check(Player->Hp <= 100.f - 3.f * Tick);
    Check(Player->Hp >= 100.f - 4.f * Tick);
    Check(!HasStatus(Player, StatusEffect_Burning));
    float AfterBurn = Player->Hp;
    StepWorld(&Test, 60);
    Check(Player->Hp == AfterBurn);
    DestroyTestWorld(&Test);
}

internal void
TestSlowedMonsterCoversLessGround()
{
    test_world Test = CreateTestWorld();
    world_entity *Normal = AddTestMonster(&Test, MonsterKind_Brute, {400, 600, 0});
    world_entity *Slowed = AddTestMonster(&Test, MonsterKind_Brute, {400, 1400, 0});
    // NOTE(zoubir): monsters chase players through the player slots
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {650, 600, 0});
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 1, {650, 1400, 0});
    ApplyStatus(Slowed, StatusEffect_Slowed, 10.f);
    StepMonster(&Test, Normal, 90);
    StepMonster(&Test, Slowed, 90);
    float NormalMoved = Normal->Position.X - 400.f;
    float SlowedMoved = Slowed->Position.X - 400.f;
    Check(NormalMoved > 20.f);
    Check(SlowedMoved < 0.7f * NormalMoved);
    DestroyTestWorld(&Test);
}

internal void
TestBileShellsLeavePoisonPuddles()
{
    test_world Test = CreateTestWorld();
    AddTestMonster(&Test, MonsterKind_Toad, {600, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {800, 1000, 0});
    monster_ability *Barrage = &GetMonsterDef(MonsterKind_Toad)->Abilities[0];

    StepWorld(&Test, 1 + SecondsToFrames(Barrage->Windup));
    Check(CountEntitiesOfType(Test.World, EntityType_MonsterHazard) == Barrage->Count);
    Check(HasStatus(Player, StatusEffect_Poisoned));
    StepWorld(&Test, SecondsToFrames(Barrage->HazardSeconds));
    Check(CountEntitiesOfType(Test.World, EntityType_MonsterHazard) == 0);
    DestroyTestWorld(&Test);
}

internal void
TestWebSlowsOnlyWhileStandingInIt()
{
    test_world Test = CreateTestWorld();
    world_entity *Spider = AddTestMonster(&Test, MonsterKind_Spider, {600, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {800, 1000, 0});
    monster_ability *Snare = &GetMonsterDef(MonsterKind_Spider)->Abilities[0];

    StepWorld(&Test, 1 + SecondsToFrames(Snare->Windup));
    Check(CountEntitiesOfType(Test.World, EntityType_MonsterHazard) == 1);
    // NOTE(zoubir): keep the spider from spitting while we watch the web
    Spider->AbilityCooldowns[1] = 1000.f;
    StepWorld(&Test, SecondsToFrames(2.f * Snare->StatusSeconds));
    Check(HasStatus(Player, StatusEffect_Slowed));
    Player->Position.Y += 200.f;
    StepWorld(&Test, SecondsToFrames(Snare->StatusSeconds));
    Check(!HasStatus(Player, StatusEffect_Slowed));
    DestroyTestWorld(&Test);
}

internal void
TestEmbersSetPlayerOnFire()
{
    test_world Test = CreateTestWorld();
    Test.AppState->FireBallCollision = Test.FireBallVolume;
    AddTestMonster(&Test, MonsterKind_Imp, {600, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {800, 1000, 0});
    monster_ability *Fan = &GetMonsterDef(MonsterKind_Imp)->Abilities[0];
    bool32 Burned = false;
    for(u32 Frame = 0; Frame < SecondsToFrames(Fan->Windup + Fan->Active); Frame++)
    {
        StepWorld(&Test, 1);
        Burned |= HasStatus(Player, StatusEffect_Burning);
    }
    Check(Burned);
    DestroyTestWorld(&Test);
}

internal void
RunMonsterTests()
{
    printf("TestMonsterDefsAreValid\n");
    TestMonsterDefsAreValid();
    printf("TestMonsterSheetsHaveEveryFrame\n");
    TestMonsterSheetsHaveEveryFrame();
    printf("TestSlamHitsOnlyAfterWindup\n");
    TestSlamHitsOnlyAfterWindup();
    printf("TestSlamMissesPlayerWhoLeft\n");
    TestSlamMissesPlayerWhoLeft();
    printf("TestChargeHitsPlayerInLine\n");
    TestChargeHitsPlayerInLine();
    printf("TestChargeIntoWallStuns\n");
    TestChargeIntoWallStuns();
    printf("TestMortarLandsOnStandingTarget\n");
    TestMortarLandsOnStandingTarget();
    printf("TestBlinkLandsBehindTarget\n");
    TestBlinkLandsBehindTarget();
    printf("TestAbilityGoesBackOnCooldown\n");
    TestAbilityGoesBackOnCooldown();
    printf("TestVolleyFansShotsAndHitsPlayerInLane\n");
    TestVolleyFansShotsAndHitsPlayerInLane();
    printf("TestWallBlocksVolley\n");
    TestWallBlocksVolley();
    printf("TestShotsDoNotHurtMonsters\n");
    TestShotsDoNotHurtMonsters();
    printf("TestBlinkMovesMonsterToNewChunk\n");
    TestBlinkMovesMonsterToNewChunk();
    printf("TestStatusRefreshesInsteadOfStacking\n");
    TestStatusRefreshesInsteadOfStacking();
    printf("TestBurnDamagesInTicksThenStops\n");
    TestBurnDamagesInTicksThenStops();
    printf("TestSlowedMonsterCoversLessGround\n");
    TestSlowedMonsterCoversLessGround();
    printf("TestBileShellsLeavePoisonPuddles\n");
    TestBileShellsLeavePoisonPuddles();
    printf("TestWebSlowsOnlyWhileStandingInIt\n");
    TestWebSlowsOnlyWhileStandingInIt();
    printf("TestEmbersSetPlayerOnFire\n");
    TestEmbersSetPlayerOnFire();
}
