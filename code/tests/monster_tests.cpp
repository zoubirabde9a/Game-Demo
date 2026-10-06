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

// NOTE(zoubir): like AddTestMonster, but through SpawnMonster, so it has a
// serial (summons and heals need one) and staggered cooldowns are reset
internal world_entity *
AddRegisteredMonster(test_world *Test, monster_kind Kind, v3 Position)
{
    world_entity *Monster = AddTestMonster(Test, Kind, Position);
    Monster->MonsterSerial = ++Test->AppState->Monsters->NextMonsterSerial;
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
            // NOTE(zoubir): the special row is optional
            Check(Def->FrameCounts[Row] >= 1 || Row == MonsterRow_Special);
            Check(Def->FrameCounts[Row] <= MONSTER_SHEET_COLUMNS);
            Check(Def->SecondsPerFrame[Row] > 0.f);
        }
        Check(Def->FrontArmor >= 0.f && Def->FrontArmor < 1.f);
        if (Def->FrontArmor > 0.f)
        {
            Check(Def->FrontArcDegrees > 0.f && Def->FrontArcDegrees < 360.f);
        }
        Check(Def->TurnRate >= 0.f);
        Check(Def->EnrageHpShare >= 0.f && Def->EnrageHpShare < 1.f);
        if (Def->EnrageHpShare > 0.f)
        {
            Check(Def->EnrageSpeedScale > 0.f && Def->EnrageCooldownScale > 0.f);
        }
        // NOTE(zoubir): every phase the kind can be in has an ability or
        // none of them are phase-limited
        for(u32 AbilityIndex = 0; AbilityIndex < Def->AbilityCount; AbilityIndex++)
        {
            u32 Mask = Def->Abilities[AbilityIndex].PhaseMask;
            Check((Mask & ~(PHASE_CALM | PHASE_ENRAGED)) == 0);
            if (Mask == PHASE_ENRAGED)
            {
                Check(Def->EnrageHpShare > 0.f);
            }
        }
        if (Def->DeathEffect == DeathEffect_Split)
        {
            Check(Def->SplitCount >= 1 && Def->SplitCount <= 4);
            Check(Def->SplitKind < MonsterKind_Count);
            // NOTE(zoubir): a kind splitting into itself never ends
            Check(Def->SplitKind != (monster_kind)KindIndex);
            Check(GetMonsterDef(Def->SplitKind)->DeathEffect == DeathEffect_None);
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
            if (Ability->Kind == MonsterAbility_Summon)
            {
                Check(Ability->SummonKind < MonsterKind_Count);
                Check(Ability->SummonKind != (monster_kind)KindIndex);
                Check(Ability->MaxActive >= Ability->Count && Ability->Count >= 1);
                Check(Ability->Spread > 0.f);
                continue;
            }
            if (Ability->Kind == MonsterAbility_Mend)
            {
                Check(Ability->Heal > 0.f && Ability->Radius > 0.f);
                continue;
            }
            Check(Ability->Damage > 0.f);
            Check(Ability->Radius > 0.f);
            // NOTE(zoubir): mortars and summons keep one target spot per
            // count; volleys have their own MAX_VOLLEY_SHOTS
            if (Ability->Kind == MonsterAbility_Mortar ||
                Ability->Kind == MonsterAbility_Summon)
            {
                Check(Ability->Count <= MAX_ABILITY_POINTS);
            }
            if (Ability->Status != StatusEffect_None)
            {
                Check(Ability->Status < StatusEffect_Count);
                Check(Ability->StatusSeconds > 0.f);
            }
            // NOTE(zoubir): only slams and mortar spots leave hazards
            Check(Ability->HazardSeconds == 0.f ||
                  ((Ability->Kind == MonsterAbility_Mortar ||
                    Ability->Kind == MonsterAbility_Slam) &&
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
    // NOTE(zoubir): a player burns to death in STATUS_PLAYER_BURN_SECONDS
    float Tick = Player->MaxHp / STATUS_PLAYER_BURN_SECONDS * STATUS_TICK_SECONDS;
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

inline u32
CountMonstersOfKind(world *World, monster_kind Kind)
{
    u32 Result = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        Result += Entity->IsPresent && Entity->Type == EntityType_Monster &&
            Entity->MonsterKind == Kind;
    }
    return Result;
}

internal void
TestSlimeSplitsWhenKilled()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Slime = AddTestMonster(&Test, MonsterKind_Slime, {1000, 1000, 0});
    monster_population *Population = AppState->Monsters;
    Population->Target = 0;
    monster_def *Def = GetMonsterDef(MonsterKind_Slime);

    DamageEntity(AppState, Test.World, Slime, Slime->Hp + 1.f, 0);
    Check(!Slime->IsPresent);
    // NOTE(zoubir): children appear on the population's next update
    Check(CountMonstersOfKind(Test.World, MonsterKind_Slimelet) == 0);
    UpdateMonsterPopulation(AppState, Test.World, &Test.Arena, Population,
                            Test.Input.DeltaTime);
    Check(CountMonstersOfKind(Test.World, MonsterKind_Slimelet) == Def->SplitCount);
    Check(Population->PendingDeathCount == 0);

    // NOTE(zoubir): slimelets end the chain
    for(u32 EntityIndex = 0; EntityIndex < Test.World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &Test.World->Entities[EntityIndex];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster)
        {
            DamageEntity(AppState, Test.World, Entity, Entity->Hp + 1.f, 0);
        }
    }
    UpdateMonsterPopulation(AppState, Test.World, &Test.Arena, Population,
                            Test.Input.DeltaTime);
    Check(CountLiveMonsters(Test.World) == 0);
    DestroyTestWorld(&Test);
}

internal void
TestSplitChildrenNeverOverlapWalls()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Slime = AddTestMonster(&Test, MonsterKind_Slime, {1000, 1000, 0});
    AppState->Monsters->Target = 0;
    // NOTE(zoubir): boxed in on three sides
    AddTestEntity(&Test, EntityType_StaticObject, {1040, 1000, 0}, Test.WallVolume);
    AddTestEntity(&Test, EntityType_StaticObject, {960, 1000, 0}, Test.WallVolume);
    AddTestEntity(&Test, EntityType_StaticObject, {1000, 1030, 0}, Test.WallVolume);
    DamageEntity(AppState, Test.World, Slime, Slime->Hp + 1.f, 0);
    UpdateMonsterPopulation(AppState, Test.World, &Test.Arena, AppState->Monsters,
                            Test.Input.DeltaTime);
    for(u32 EntityIndex = 0; EntityIndex < Test.World->EntityCount; EntityIndex++)
    {
        world_entity *Child = &Test.World->Entities[EntityIndex];
        if (!Child->IsPresent || Child->Type != EntityType_Monster)
        {
            continue;
        }
        for(u32 OtherIndex = 0; OtherIndex < Test.World->EntityCount; OtherIndex++)
        {
            world_entity *Other = &Test.World->Entities[OtherIndex];
            if (Other != Child && Other->IsPresent &&
                CanCollide(AppState, Child->Type, Other->Type))
            {
                Check(!EntityOverlap(Child, Other));
            }
        }
    }
    DestroyTestWorld(&Test);
}

internal void
TestBellyFlopLeavesSlowingGoo()
{
    test_world Test = CreateTestWorld();
    AddTestMonster(&Test, MonsterKind_Slime, {1000, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {1050, 1000, 0});
    monster_ability *Flop = &GetMonsterDef(MonsterKind_Slime)->Abilities[0];

    StepWorld(&Test, 1 + SecondsToFrames(Flop->Windup));
    Check(Player->Hp <= 100.f - Flop->Damage);
    Check(CountEntitiesOfType(Test.World, EntityType_MonsterHazard) == 1);
    Check(HasStatus(Player, StatusEffect_Slowed));
    DestroyTestWorld(&Test);
}

internal void
TestEliteRollRateAndVariety()
{
    random_series Series = Seed(99);
    u32 Counts[MonsterAffix_Count] = {};
    u32 Rolls = 20000;
    for(u32 Roll = 0; Roll < Rolls; Roll++)
    {
        Counts[RollEliteAffix(&Series)]++;
    }
    float EliteShare = 1.f - (float)Counts[MonsterAffix_None] / (float)Rolls;
    Check(EliteShare > ELITE_CHANCE - 0.02f && EliteShare < ELITE_CHANCE + 0.02f);
    for(u32 Affix = 1; Affix < MonsterAffix_Count; Affix++)
    {
        Check(Counts[Affix] > 0);
    }
}

internal void
TestArmoredEliteHasMoreHealth()
{
    test_world Test = CreateTestWorld();
    world_entity *Brute = AddTestMonster(&Test, MonsterKind_Brute, {1000, 1000, 0});
    float BaseHp = Brute->MaxHp;
    ApplyEliteAffix(Brute, MonsterAffix_Armored);
    Check(Brute->MaxHp == BaseHp * GetAffix(MonsterAffix_Armored)->HpScale);
    Check(Brute->Hp == Brute->MaxHp);
    Check(Brute->Tint == GetAffix(MonsterAffix_Armored)->Tint);
    Check(GetMoveSpeedScale(Brute) < 1.f);
    DestroyTestWorld(&Test);
}

internal void
TestVampiricBiteHeals()
{
    test_world Test = CreateTestWorld();
    world_entity *Brute = AddTestMonster(&Test, MonsterKind_Brute, {1000, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {1040, 1000, 0});
    ApplyEliteAffix(Brute, MonsterAffix_Vampiric);
    Brute->Hp = 20.f;
    monster_affix_def *Affix = GetAffix(MonsterAffix_Vampiric);
    float Damage = GetMonsterDef(MonsterKind_Brute)->AttackDamage * Affix->DamageScale;
    MonsterBite(Test.AppState, Test.World, Brute, Player);
    Check(Player->Hp == 100.f - Damage);
    Check(Brute->Hp == 20.f + Affix->LifeSteal * Damage);
    DestroyTestWorld(&Test);
}

internal void
TestChillingHitsSlowAndShotsCarryIt()
{
    test_world Test = CreateTestWorld();
    Test.AppState->FireBallCollision = Test.FireBallVolume;
    world_entity *Imp = AddTestMonster(&Test, MonsterKind_Imp, {600, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {800, 1000, 0});
    ApplyEliteAffix(Imp, MonsterAffix_Chilling);
    monster_ability *Fan = &GetMonsterDef(MonsterKind_Imp)->Abilities[0];
    bool32 Slowed = false;
    for(u32 Frame = 0; Frame < SecondsToFrames(Fan->Windup + Fan->Active); Frame++)
    {
        StepWorld(&Test, 1);
        Slowed |= HasStatus(Player, StatusEffect_Slowed);
    }
    Check(Slowed);
    DestroyTestWorld(&Test);
}

internal void
TestFrenziedAbilitiesRechargeFaster()
{
    test_world Test = CreateTestWorld();
    world_entity *Brute = AddTestMonster(&Test, MonsterKind_Brute, {1000, 1000, 0});
    AddTestPlayer(&Test, {1050, 1000, 0});
    ApplyEliteAffix(Brute, MonsterAffix_Frenzied);
    monster_ability *Slam = &GetMonsterDef(MonsterKind_Brute)->Abilities[0];
    StepMonster(&Test, Brute, 1);
    StepMonster(&Test, Brute, SecondsToFrames(Slam->Windup + Slam->Active +
                                              Slam->Recover));
    Check(Brute->AbilityPhase == AbilityPhase_Ready);
    float Expected = Slam->Cooldown * GetAffix(MonsterAffix_Frenzied)->CooldownScale;
    Check(Brute->AbilityCooldowns[0] <= Expected);
    Check(Brute->AbilityCooldowns[0] > Expected - 0.2f);
    DestroyTestWorld(&Test);
}

internal void
TestEliteSlimeChildrenKeepAffix()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Slime = AddTestMonster(&Test, MonsterKind_Slime, {1000, 1000, 0});
    ApplyEliteAffix(Slime, MonsterAffix_Armored);
    DamageEntity(AppState, Test.World, Slime, Slime->Hp + 1.f, 0);
    UpdateMonsterPopulation(AppState, Test.World, &Test.Arena, AppState->Monsters,
                            Test.Input.DeltaTime);
    u32 Children = 0;
    for(u32 EntityIndex = 0; EntityIndex < Test.World->EntityCount; EntityIndex++)
    {
        world_entity *Child = &Test.World->Entities[EntityIndex];
        if (Child->IsPresent && Child->MonsterKind == MonsterKind_Slimelet &&
            Child->Type == EntityType_Monster)
        {
            Children++;
            Check(Child->EliteAffix == MonsterAffix_Armored);
            Check(Child->MaxHp > GetMonsterDef(MonsterKind_Slimelet)->MaxHp);
        }
    }
    Check(Children == GetMonsterDef(MonsterKind_Slime)->SplitCount);
    DestroyTestWorld(&Test);
}

internal void
TestMonsterTableHashTracksChanges()
{
    u32 First = ComputeMonsterTableHash();
    Check(First == ComputeMonsterTableHash());
    monster_def *Def = GetMonsterDef(MonsterKind_Brute);
    float Saved = Def->MaxHp;
    Def->MaxHp += 1.f;
    Check(ComputeMonsterTableHash() != First);
    Def->MaxHp = Saved;
    Check(ComputeMonsterTableHash() == First);
}

internal void
TestShamanRaisesThrallsUpToCap()
{
    test_world Test = CreateTestWorld();
    world_entity *Shaman = AddRegisteredMonster(&Test, MonsterKind_Shaman, {600, 1000, 0});
    AddTestPlayer(&Test, {850, 1000, 0});
    monster_def *Def = GetMonsterDef(MonsterKind_Shaman);
    monster_ability *Raise = &Def->Abilities[1];

    StepWorld(&Test, 1);
    Check(Shaman->AbilityPhase == AbilityPhase_Windup);
    Check(Shaman->AbilityIndex == 1);
    Check(Shaman->AbilityPointCount == Raise->Count);
    StepWorld(&Test, SecondsToFrames(Raise->Windup));
    Check(CountMonstersOfKind(Test.World, MonsterKind_Thrall) == Raise->Count);

    // NOTE(zoubir): cast again and again; it never goes past MaxActive
    for(u32 Cast = 0; Cast < 4; Cast++)
    {
        Shaman->AbilityCooldowns[1] = 0.f;
        StepWorld(&Test, SecondsToFrames(Raise->Windup + Raise->Active +
                                         Raise->Recover) + 2);
    }
    Check(CountMonstersOfKind(Test.World, MonsterKind_Thrall) == Raise->MaxActive);
    DestroyTestWorld(&Test);
}

internal void
TestThrallsCrumbleWhenShamanDies()
{
    test_world Test = CreateTestWorld();
    world_entity *Shaman = AddRegisteredMonster(&Test, MonsterKind_Shaman, {600, 1000, 0});
    AddTestPlayer(&Test, {850, 1000, 0});
    monster_ability *Raise = &GetMonsterDef(MonsterKind_Shaman)->Abilities[1];
    StepWorld(&Test, 1 + SecondsToFrames(Raise->Windup));
    Check(CountMonstersOfKind(Test.World, MonsterKind_Thrall) == Raise->Count);

    DamageEntity(Test.AppState, Test.World, Shaman, Shaman->Hp + 1.f, 0);
    UpdateMonsterPopulation(Test.AppState, Test.World, &Test.Arena,
                            Test.AppState->Monsters, Test.Input.DeltaTime);
    Check(CountMonstersOfKind(Test.World, MonsterKind_Thrall) == 0);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a new monster in a dead summoner's slot must not adopt
// the old summoner's thralls
internal void
TestReusedSlotIsNotTheSummoner()
{
    test_world Test = CreateTestWorld();
    world_entity *Shaman = AddRegisteredMonster(&Test, MonsterKind_Shaman, {600, 1000, 0});
    u32 ShamanSlot = Shaman->ID;
    AddTestPlayer(&Test, {850, 1000, 0});
    monster_ability *Raise = &GetMonsterDef(MonsterKind_Shaman)->Abilities[1];
    StepWorld(&Test, 1 + SecondsToFrames(Raise->Windup));

    DamageEntity(Test.AppState, Test.World, Shaman, Shaman->Hp + 1.f, 0);
    world_entity *Newcomer = AddRegisteredMonster(&Test, MonsterKind_Brute, {300, 300, 0});
    Check(Newcomer->ID == ShamanSlot);
    UpdateMonsterPopulation(Test.AppState, Test.World, &Test.Arena,
                            Test.AppState->Monsters, Test.Input.DeltaTime);
    Check(CountMonstersOfKind(Test.World, MonsterKind_Thrall) == 0);
    Check(Newcomer->IsPresent);
    DestroyTestWorld(&Test);
}

internal void
TestMendHealsMostHurtAlly()
{
    test_world Test = CreateTestWorld();
    world_entity *Shaman = AddRegisteredMonster(&Test, MonsterKind_Shaman, {600, 1000, 0});
    world_entity *Scratched = AddRegisteredMonster(&Test, MonsterKind_Brute, {650, 900, 0});
    world_entity *Wounded = AddRegisteredMonster(&Test, MonsterKind_Brute, {650, 1100, 0});
    AddTestPlayer(&Test, {900, 1000, 0});
    Scratched->Hp = 0.6f * Scratched->MaxHp;
    Wounded->Hp = 0.2f * Wounded->MaxHp;
    float WoundedHp = Wounded->Hp;
    float ScratchedHp = Scratched->Hp;
    monster_ability *Mend = &GetMonsterDef(MonsterKind_Shaman)->Abilities[0];

    StepMonster(&Test, Shaman, 1);
    Check(Shaman->AbilityPhase == AbilityPhase_Windup);
    Check(Shaman->AbilityIndex == 0);
    StepMonster(&Test, Shaman, SecondsToFrames(Mend->Windup));
    Check(Wounded->Hp == WoundedHp + Mend->Heal);
    Check(Scratched->Hp == ScratchedHp);
    DestroyTestWorld(&Test);
}

internal void
TestMendWaitsForSomeoneHurt()
{
    test_world Test = CreateTestWorld();
    world_entity *Shaman = AddRegisteredMonster(&Test, MonsterKind_Shaman, {600, 1000, 0});
    AddRegisteredMonster(&Test, MonsterKind_Brute, {650, 900, 0});
    AddTestPlayer(&Test, {900, 1000, 0});
    // NOTE(zoubir): nobody hurt, so the shaman raises the dead instead
    StepMonster(&Test, Shaman, 1);
    Check(Shaman->AbilityPhase == AbilityPhase_Windup);
    Check(Shaman->AbilityIndex == 1);
    DestroyTestWorld(&Test);
}

internal void
TestShellBlocksHitsFromTheFront()
{
    test_world Test = CreateTestWorld();
    world_entity *Warden = AddTestMonster(&Test, MonsterKind_Warden, {1000, 1000, 0});
    Warden->Direction = V2(1.f, 0.f);
    world_entity *Front = AddTestPlayer(&Test, {1040, 1000, 0});
    world_entity *Behind = AddTestPlayer(&Test, {960, 1000, 0});
    monster_def *Def = GetMonsterDef(MonsterKind_Warden);

    float Start = Warden->Hp;
    DamageEntity(Test.AppState, Test.World, Warden, 10.f, Front);
    Check(Absolute(Warden->Hp - (Start - 10.f * (1.f - Def->FrontArmor))) < 0.001f);
    Check(Warden->BlockFlash > 0.f);

    Start = Warden->Hp;
    DamageEntity(Test.AppState, Test.World, Warden, 10.f, Behind);
    Check(Warden->Hp == Start - 10.f);

    // NOTE(zoubir): no source (status ticks) is never blocked
    Start = Warden->Hp;
    DamageEntity(Test.AppState, Test.World, Warden, 4.f, 0);
    Check(Warden->Hp == Start - 4.f);
    DestroyTestWorld(&Test);
}

internal void
TestWardenTurnsSlowlyTowardTarget()
{
    test_world Test = CreateTestWorld();
    world_entity *Warden = AddTestMonster(&Test, MonsterKind_Warden, {1000, 1000, 0});
    Warden->Direction = V2(1.f, 0.f);
    AddTestPlayer(&Test, {700, 1000, 0});
    monster_def *Def = GetMonsterDef(MonsterKind_Warden);

    StepMonster(&Test, Warden, 30);
    float Turned = ATan2(Warden->Direction.Y, Warden->Direction.X);
    Turned = Turned < 0.f ? -Turned : Turned;
    Check(Turned <= Def->TurnRate * 0.5f + 0.05f);
    Check(Warden->Direction.X > -0.9f);
    StepMonster(&Test, Warden, 180);
    Check(Warden->Direction.X < -0.99f);
    DestroyTestWorld(&Test);
}

internal void
TestWarlordEnragesOnceBelowHalf()
{
    test_world Test = CreateTestWorld();
    world_entity *Boss = AddRegisteredMonster(&Test, MonsterKind_Warlord, {1000, 1000, 0});
    monster_def *Def = GetMonsterDef(MonsterKind_Warlord);
    float CalmScale = GetMoveSpeedScale(Boss);
    Boss->AbilityCooldowns[0] = Boss->AbilityCooldowns[1] = 10.f;

    StepMonster(&Test, Boss, 1);
    Check(Boss->Phase == 0);
    Boss->Hp = Def->EnrageHpShare * Boss->MaxHp - 1.f;
    StepMonster(&Test, Boss, 1);
    Check(Boss->Phase == 1);
    Check(Boss->PhaseFlash > 0.f);
    Check(Boss->Tint == Def->EnrageTint);
    Check(GetMoveSpeedScale(Boss) > CalmScale);
    // NOTE(zoubir): enraging pulls every recharge in close
    Check(Boss->AbilityCooldowns[0] <= ENRAGE_COOLDOWN_CAP);

    // NOTE(zoubir): healing back up does not calm it down
    Boss->Hp = Boss->MaxHp;
    StepMonster(&Test, Boss, 1);
    Check(Boss->Phase == 1);
    DestroyTestWorld(&Test);
}

internal void
TestBroodOnlyCalledWhenEnraged()
{
    test_world Test = CreateTestWorld();
    world_entity *Boss = AddRegisteredMonster(&Test, MonsterKind_Warlord, {1000, 1000, 0});
    AddTestPlayer(&Test, {1300, 1000, 0});
    monster_def *Def = GetMonsterDef(MonsterKind_Warlord);
    // NOTE(zoubir): only the brood is off cooldown; calm, nothing starts
    Boss->AbilityCooldowns[0] = Boss->AbilityCooldowns[1] = 100.f;
    StepMonster(&Test, Boss, 1);
    Check(Boss->AbilityPhase == AbilityPhase_Ready);

    Boss->Hp = 0.4f * Boss->MaxHp;
    Boss->AbilityCooldowns[0] = Boss->AbilityCooldowns[1] = 100.f;
    StepMonster(&Test, Boss, 1);
    StepMonster(&Test, Boss, 1);
    Check(Boss->AbilityPhase == AbilityPhase_Windup);
    Check(Def->Abilities[Boss->AbilityIndex].Kind == MonsterAbility_Summon);
    DestroyTestWorld(&Test);
}

internal void
TestOnlyOneWarlordAtATime()
{
    test_world Test = CreateTestWorld();
    AddTestMonster(&Test, MonsterKind_Warlord, {1000, 1000, 0});
    random_series Series = Seed(5);
    for(u32 Pick = 0; Pick < 2000; Pick++)
    {
        Check(PickMonsterKind(&Series, Test.World) != MonsterKind_Warlord);
    }
    DestroyTestWorld(&Test);
}

internal void
TestBurrowedLurkerIsImmune()
{
    test_world Test = CreateTestWorld();
    world_entity *Lurker = AddTestMonster(&Test, MonsterKind_Lurker, {600, 1000, 0});
    AddTestPlayer(&Test, {850, 1000, 0});
    monster_ability *Tunnel = &GetMonsterDef(MonsterKind_Lurker)->Abilities[0];

    StepMonster(&Test, Lurker, 1 + SecondsToFrames(Tunnel->Windup));
    Check(Lurker->Burrowed);
    Check(Lurker->AbilityPhase == AbilityPhase_Active);
    float Hp = Lurker->Hp;
    DamageEntity(Test.AppState, Test.World, Lurker, 50.f, 0);
    Check(Lurker->Hp == Hp);
    DestroyTestWorld(&Test);
}

internal void
TestLurkerEruptsUnderStandingPlayer()
{
    test_world Test = CreateTestWorld();
    world_entity *Lurker = AddTestMonster(&Test, MonsterKind_Lurker, {600, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {850, 1000, 0});
    monster_ability *Tunnel = &GetMonsterDef(MonsterKind_Lurker)->Abilities[0];

    StepMonster(&Test, Lurker, 1 + SecondsToFrames(Tunnel->Windup + Tunnel->Active));
    Check(!Lurker->Burrowed);
    Check(Lurker->AbilityPhase == AbilityPhase_Recover);
    Check(Length(Lurker->Position.XY - Player->Position.XY) <= Tunnel->Radius);
    Check(Player->Hp == 100.f - Tunnel->Damage);
    // NOTE(zoubir): out of the ground, it can be hurt again
    float Hp = Lurker->Hp;
    DamageEntity(Test.AppState, Test.World, Lurker, 5.f, 0);
    Check(Lurker->Hp == Hp - 5.f);
    DestroyTestWorld(&Test);
}

internal void
TestLurkerLandingFollowsUntilLock()
{
    test_world Test = CreateTestWorld();
    world_entity *Lurker = AddTestMonster(&Test, MonsterKind_Lurker, {600, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {850, 1000, 0});
    monster_ability *Tunnel = &GetMonsterDef(MonsterKind_Lurker)->Abilities[0];

    StepMonster(&Test, Lurker, 1 + SecondsToFrames(Tunnel->Windup));
    Check(Lurker->Burrowed);
    // NOTE(zoubir): before the lock the landing spot follows the player
    Player->Position.Y = 1150.f;
    StepMonster(&Test, Lurker, 5);
    Check(Length(Lurker->AbilityPoints[0] - Player->Position.XY) < 1.f);
    // NOTE(zoubir): after the lock it stays put, and a player who leaves
    // the ring is safe
    StepMonster(&Test, Lurker, SecondsToFrames((1.f - BURROW_LOCK_SHARE) *
                                               Tunnel->Active));
    v2 Locked = Lurker->AbilityPoints[0];
    Player->Position.Y = 1150.f + 2.f * Tunnel->Radius;
    StepMonster(&Test, Lurker, SecondsToFrames(BURROW_LOCK_SHARE * Tunnel->Active));
    Check(!Lurker->Burrowed);
    Check(Length(Lurker->Position.XY - Locked) < 45.f);
    Check(Player->Hp == 100.f);
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
    printf("TestSlimeSplitsWhenKilled\n");
    TestSlimeSplitsWhenKilled();
    printf("TestSplitChildrenNeverOverlapWalls\n");
    TestSplitChildrenNeverOverlapWalls();
    printf("TestBellyFlopLeavesSlowingGoo\n");
    TestBellyFlopLeavesSlowingGoo();
    printf("TestEliteRollRateAndVariety\n");
    TestEliteRollRateAndVariety();
    printf("TestArmoredEliteHasMoreHealth\n");
    TestArmoredEliteHasMoreHealth();
    printf("TestVampiricBiteHeals\n");
    TestVampiricBiteHeals();
    printf("TestChillingHitsSlowAndShotsCarryIt\n");
    TestChillingHitsSlowAndShotsCarryIt();
    printf("TestFrenziedAbilitiesRechargeFaster\n");
    TestFrenziedAbilitiesRechargeFaster();
    printf("TestEliteSlimeChildrenKeepAffix\n");
    TestEliteSlimeChildrenKeepAffix();
    printf("TestMonsterTableHashTracksChanges\n");
    TestMonsterTableHashTracksChanges();
    printf("TestShamanRaisesThrallsUpToCap\n");
    TestShamanRaisesThrallsUpToCap();
    printf("TestThrallsCrumbleWhenShamanDies\n");
    TestThrallsCrumbleWhenShamanDies();
    printf("TestReusedSlotIsNotTheSummoner\n");
    TestReusedSlotIsNotTheSummoner();
    printf("TestMendHealsMostHurtAlly\n");
    TestMendHealsMostHurtAlly();
    printf("TestMendWaitsForSomeoneHurt\n");
    TestMendWaitsForSomeoneHurt();
    printf("TestShellBlocksHitsFromTheFront\n");
    TestShellBlocksHitsFromTheFront();
    printf("TestWardenTurnsSlowlyTowardTarget\n");
    TestWardenTurnsSlowlyTowardTarget();
    printf("TestWarlordEnragesOnceBelowHalf\n");
    TestWarlordEnragesOnceBelowHalf();
    printf("TestBroodOnlyCalledWhenEnraged\n");
    TestBroodOnlyCalledWhenEnraged();
    printf("TestOnlyOneWarlordAtATime\n");
    TestOnlyOneWarlordAtATime();
    printf("TestBurrowedLurkerIsImmune\n");
    TestBurrowedLurkerIsImmune();
    printf("TestLurkerEruptsUnderStandingPlayer\n");
    TestLurkerEruptsUnderStandingPlayer();
    printf("TestLurkerLandingFollowsUntilLock\n");
    TestLurkerLandingFollowsUntilLock();
}
