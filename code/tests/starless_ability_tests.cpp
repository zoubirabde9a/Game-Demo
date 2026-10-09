/* Starless ability tests: the Starless Deep's new kinds
   (sim/monster_abilities/wells_brands_mirrors.cpp). A gravity well drags
   who stands in reach toward it and collapses only on who it caught; a
   void brand follows the farthest player and bursts on them and those
   near them; an eclipse spares only who stands in a light. Mirrors need a
   dungeon run and are tested with the level (starless_tests.cpp).
   Included by sim_tests.cpp after rift_ability_tests.cpp, whose helpers
   it uses; RunStarlessAbilityTests runs them. */

internal void
TestWellDragsAndCollapses()
{
    test_world Test = CreateTestWorld();
    world_entity *Collapsar = AddTestMonster(&Test, MonsterKind_Collapsar, {600, 1000, 0});
    world_entity *Target = AddTestPlayer(&Test, {800, 1000, 0});
    world_entity *Caught = AddTestPlayer(&Test, {900, 1000, 0});
    world_entity *Far = AddTestPlayer(&Test, {800, 1400, 0});
    monster_ability *Well = ReadyAbilityOfKind(Collapsar, MonsterAbility_Pull);
    Check(Well != 0 && Well->InnerRadius < 100.f && Well->Radius > 100.f);

    StepMonster(&Test, Collapsar, 1);
    Check(Collapsar->AbilityPhase == AbilityPhase_Windup);
    // NOTE(zoubir): the well opens under the target
    Check(Length(Collapsar->AbilityPoints[0] - Target->Position.XY) < 1.f);
    StepMonster(&Test, Collapsar, SecondsToFrames(Well->Windup) + 10);
    Check(Collapsar->AbilityPhase == AbilityPhase_Active);
    Check(Caught->Velocity.X < -10.f && Absolute(Caught->Velocity.Y) < 1.f);
    Check(Far->Velocity.X == 0.f && Far->Velocity.Y == 0.f);
    Check(Target->Hp == 100.f && Caught->Hp == 100.f);

    StepMonster(&Test, Collapsar, SecondsToFrames(Well->Active));
    Check(Target->Hp <= 100.f - Well->Damage + 0.01f);
    // NOTE(zoubir): still outside the core when it collapsed
    Check(Caught->Hp == 100.f && Far->Hp == 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a seer brands the farthest player in reach, the brand
// follows them, and its burst hits them and whoever stands near them
internal void
TestBrandBurstsOnThoseNearIt()
{
    test_world Test = CreateTestWorld();
    world_entity *Seer = AddTestMonster(&Test, MonsterKind_Seer, {600, 1000, 0});
    world_entity *Near = AddTestPlayer(&Test, {650, 1000, 0});
    world_entity *Branded = AddTestPlayer(&Test, {900, 1000, 0});
    world_entity *Beside = AddTestPlayer(&Test, {840, 1060, 0});
    world_entity *Apart = AddTestPlayer(&Test, {700, 1250, 0});
    monster_ability *Brand = ReadyAbilityOfKind(Seer, MonsterAbility_Brand);
    Check(Brand != 0);

    StepMonster(&Test, Seer, 1);
    Check(Seer->AbilityPhase == AbilityPhase_Windup);
    Check(Length(Seer->AbilityPoints[0] - Branded->Position.XY) < 1.f);
    // NOTE(zoubir): the brand goes where they go, and Beside follows
    Branded->Position.X += 40.f;
    Beside->Position.X += 40.f;
    StepMonster(&Test, Seer, 2);
    Check(Length(Seer->AbilityPoints[0] - Branded->Position.XY) < 1.f);

    StepMonster(&Test, Seer, SecondsToFrames(Brand->Windup));
    Check(Branded->Hp <= 100.f - Brand->Damage + 0.01f);
    Check(Beside->Hp <= 100.f - Brand->Damage + 0.01f);
    Check(Near->Hp == 100.f && Apart->Hp == 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): when the dark falls only a player standing in a light is
// spared; a jump does not help the one outside
internal void
TestEclipseSparesTheLit()
{
    test_world Test = CreateTestWorld();
    world_entity *Nyxara = AddTestMonster(&Test, MonsterKind_Nyxara, {1000, 1000, 0});
    world_entity *Lit = AddTestPlayer(&Test, {1100, 1000, 0});
    world_entity *Dark = AddTestPlayer(&Test, {1000, 1100, 0});
    monster_ability *Eclipse = ReadyAbilityOfKind(Nyxara, MonsterAbility_Eclipse);
    Check(Eclipse != 0 && Eclipse->Count >= 2);

    StepMonsterOverJumper(&Test, Nyxara, Dark, 1);
    Check(Nyxara->AbilityPhase == AbilityPhase_Windup);
    Check(Nyxara->AbilityPointCount == Eclipse->Count);
    for(u32 Light = 0; Light < Nyxara->AbilityPointCount; Light++)
    {
        float Distance = Length(Nyxara->AbilityPoints[Light] - Nyxara->Position.XY);
        Check(Distance >= ECLIPSE_MIN_SHARE * Eclipse->Spread - 0.5f &&
              Distance <= Eclipse->Spread + 0.5f);
    }
    Lit->Position.XY = Nyxara->AbilityPoints[0];
    Dark->Position.XY = Nyxara->Position.XY + V2(0.f, 30.f);
    Check(!IsInEclipseLight(Nyxara, Eclipse, Dark->Position.XY));
    StepMonsterOverJumper(&Test, Nyxara, Dark, SecondsToFrames(Eclipse->Windup));
    Check(Lit->Hp == 100.f);
    Check(Dark->Hp <= 100.f - Eclipse->Damage + 0.01f);
    DestroyTestWorld(&Test);
}

internal void
RunStarlessAbilityTests()
{
    TestWellDragsAndCollapses();
    TestBrandBurstsOnThoseNearIt();
    TestEclipseSparesTheLit();
}
