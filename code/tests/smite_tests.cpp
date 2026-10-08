/* Smite tests: the boss blow nobody can dodge (MonsterAbility_Smite,
   sim/monster_abilities/trigger.cpp), which lands on whoever the monster
   is after, and the feedback on monster hits: the bite's attack row and
   the bursts drawn on the bitten. Included by sim_tests.cpp after
   monster_tests.cpp, whose helpers it uses; RunSmiteTests runs them. */

// NOTE(zoubir): the kind's Smite, with every other ability held back so
// it is the one that starts
internal monster_ability *
ReadySmite(world_entity *Monster)
{
    monster_def *Def = GetMonsterDef(Monster->MonsterKind);
    monster_ability *Result = 0;
    for(u32 Index = 0; Index < Def->AbilityCount; Index++)
    {
        if (Def->Abilities[Index].Kind == MonsterAbility_Smite)
        {
            Result = &Def->Abilities[Index];
            Monster->AbilityCooldowns[Index] = 0.f;
        }
        else
        {
            Monster->AbilityCooldowns[Index] = 1000.f;
        }
    }
    return Result;
}

inline bool32
EmittedBurst(app_state *AppState, sim_burst Burst)
{
    bool32 Result = false;
    for(u32 Index = 0; Index < AppState->Events.Count; Index++)
    {
        sim_event *Event = &AppState->Events.Events[Index];
        Result |= Event->Type == SimEvent_Burst && Event->Burst == Burst;
    }
    return Result;
}

internal void
TestEveryDepthsBossHasASmite()
{
    monster_kind Bosses[] =
        {
            MonsterKind_Forgemaster,
            MonsterKind_CinderWyrm,
            MonsterKind_EmberTyrant,
        };
    for(u32 Index = 0; Index < ArrayCount(Bosses); Index++)
    {
        test_world Test = CreateTestWorld();
        world_entity *Boss = AddTestMonster(&Test, Bosses[Index], {1000, 1000, 0});
        Check(ReadySmite(Boss) != 0);
        DestroyTestWorld(&Test);
    }
}

// NOTE(zoubir): a shot or hazard sends the index of the ability that made
// it in 2 bits (server/sim_game/pack.cpp)
internal void
TestOnlyWiredSlotsMakeShotsOrHazards()
{
    for(u32 Kind = 0; Kind < MonsterKind_Count; Kind++)
    {
        monster_def *Def = GetMonsterDef((monster_kind)Kind);
        for(u32 Index = MAX_WIRED_ABILITIES; Index < Def->AbilityCount; Index++)
        {
            monster_ability *Ability = &Def->Abilities[Index];
            Check(Ability->Kind != MonsterAbility_Volley);
            Check(Ability->HazardSeconds <= 0.f);
        }
    }
}

internal void
TestSmiteLandsThroughADash()
{
    test_world Test = CreateTestWorld();
    world_entity *Kragg = AddTestMonster(&Test, MonsterKind_Forgemaster, {600, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {900, 1000, 0});
    monster_ability *Brand = ReadySmite(Kragg);

    StepMonster(&Test, Kragg, 1);
    Check(Kragg->AbilityPhase == AbilityPhase_Windup);
    Check(Kragg->AbilityPointCount == 1);
    // NOTE(zoubir): the player runs off and dashes the whole windup; the
    // mark follows them
    for(u32 Frame = 0; Frame < SecondsToFrames(Brand->Windup) && Player->Hp == 100.f; Frame++)
    {
        Player->Position.X += 2.f;
        Player->DashFlash = 1.f;
        Test.AppState->Events.Count = 0;
        StepMonster(&Test, Kragg, 1);
    }
    Check(Player->Hp <= 100.f - Brand->Damage + 0.01f);
    Check(EmittedBurst(Test.AppState, SimBurst_Smite));
    Check(HasStatus(Player, StatusEffect_Burning));
    // NOTE(zoubir): Kragg throws it from where he stands
    Check(Kragg->Position.X < 610.f);
    DestroyTestWorld(&Test);
}

internal void
TestSmiteBlinkComesDownBesideTheVictim()
{
    test_world Test = CreateTestWorld();
    world_entity *Tyrant = AddTestMonster(&Test, MonsterKind_EmberTyrant, {500, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {900, 1000, 0});
    monster_ability *Judgement = ReadySmite(Tyrant);

    StepMonster(&Test, Tyrant, 1 + SecondsToFrames(Judgement->Windup));
    Check(Player->Hp < 100.f);
    float Distance = Length(Tyrant->Position.XY - Player->Position.XY);
    Check(Distance < Judgement->Spread + 30.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): outside a dungeon a monster is after the nearest player;
// one who steps in during the windup takes the blow (a taunt does the
// same by threat)
internal void
TestSmiteFollowsWhoeverTheMonsterIsAfter()
{
    test_world Test = CreateTestWorld();
    world_entity *Wyrm = AddTestMonster(&Test, MonsterKind_CinderWyrm, {600, 1000, 0});
    world_entity *First = AddTestPlayer(&Test, {800, 1000, 0});
    world_entity *Second = AddTestPlayer(&Test, {1000, 1000, 0});
    monster_ability *Geyser = ReadySmite(Wyrm);

    StepMonster(&Test, Wyrm, 1);
    Check(Wyrm->AbilityPhase == AbilityPhase_Windup);
    Second->Position.X = 700.f;
    StepMonster(&Test, Wyrm, SecondsToFrames(Geyser->Windup));
    Check(First->Hp == 100.f);
    Check(Second->Hp <= 100.f - Geyser->Damage + 0.01f);
    DestroyTestWorld(&Test);
}

internal void
TestSmiteMissesAVictimOutOfReach()
{
    test_world Test = CreateTestWorld();
    world_entity *Kragg = AddTestMonster(&Test, MonsterKind_Forgemaster, {400, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {700, 1000, 0});
    monster_ability *Brand = ReadySmite(Kragg);

    StepMonster(&Test, Kragg, 1);
    Check(Kragg->AbilityPhase == AbilityPhase_Windup);
    Player->Position.X = 400.f + 2.f * Brand->MaxRange;
    StepMonster(&Test, Kragg, SecondsToFrames(Brand->Windup));
    Check(Player->Hp == 100.f);
    DestroyTestWorld(&Test);
}

internal void
TestBitePlaysTheAttackRowAndMarksTheBitten()
{
    test_world Test = CreateTestWorld();
    world_entity *Brute = AddTestMonster(&Test, MonsterKind_Brute, {1000, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {1030, 1000, 0});
    monster_def *Def = GetMonsterDef(MonsterKind_Brute);
    for(u32 Index = 0; Index < Def->AbilityCount; Index++)
    {
        Brute->AbilityCooldowns[Index] = 1000.f;
    }
    Brute->AttackCooldown = 0.f;
    Test.AppState->Events.Count = 0;

    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection;
    UpdateMonster(Brute, Test.World, &Test.Arena, Test.Input.DeltaTime, Test.AppState,
                  &AnimationSpeed, &AnimationType, &AnimationDirection);
    Check(Player->Hp < 100.f);
    Check(AnimationType == AnimationType_Attack);
    Check(AnimationDirection == AnimationDirection_Right);
    Check(EmittedBurst(Test.AppState, SimBurst_MonsterBite));
    for(u32 Frame = 0; Frame < SecondsToFrames(MONSTER_BITE_SECONDS); Frame++)
    {
        UpdateMonster(Brute, Test.World, &Test.Arena, Test.Input.DeltaTime, Test.AppState,
                      &AnimationSpeed, &AnimationType, &AnimationDirection);
    }
    Check(AnimationType != AnimationType_Attack);
    DestroyTestWorld(&Test);
}

internal void
TestAbilityHitsMarkThePlayer()
{
    test_world Test = CreateTestWorld();
    world_entity *Brute = AddTestMonster(&Test, MonsterKind_Brute, {1000, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {1040, 1000, 0});
    monster_ability *Slam = &GetMonsterDef(MonsterKind_Brute)->Abilities[0];
    Check(Slam->Kind == MonsterAbility_Slam);

    StepMonster(&Test, Brute, 1);
    bool32 Marked = false;
    for(u32 Frame = 0; Frame < SecondsToFrames(Slam->Windup); Frame++)
    {
        Test.AppState->Events.Count = 0;
        StepMonster(&Test, Brute, 1);
        Marked |= EmittedBurst(Test.AppState, SimBurst_MonsterHit);
    }
    Check(Player->Hp < 100.f);
    Check(Marked);
    DestroyTestWorld(&Test);
}

internal void
RunSmiteTests()
{
    printf("TestEveryDepthsBossHasASmite\n");
    TestEveryDepthsBossHasASmite();
    printf("TestOnlyWiredSlotsMakeShotsOrHazards\n");
    TestOnlyWiredSlotsMakeShotsOrHazards();
    printf("TestSmiteLandsThroughADash\n");
    TestSmiteLandsThroughADash();
    printf("TestSmiteBlinkComesDownBesideTheVictim\n");
    TestSmiteBlinkComesDownBesideTheVictim();
    printf("TestSmiteFollowsWhoeverTheMonsterIsAfter\n");
    TestSmiteFollowsWhoeverTheMonsterIsAfter();
    printf("TestSmiteMissesAVictimOutOfReach\n");
    TestSmiteMissesAVictimOutOfReach();
    printf("TestBitePlaysTheAttackRowAndMarksTheBitten\n");
    TestBitePlaysTheAttackRowAndMarksTheBitten();
    printf("TestAbilityHitsMarkThePlayer\n");
    TestAbilityHitsMarkThePlayer();
}
