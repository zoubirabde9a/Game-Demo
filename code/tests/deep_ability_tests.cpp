/* Deep ability tests: the cone, lanes, gaze and share kinds of the rift's
   and the deep's newer monsters
   (sim/monster_abilities/cones_lanes_gazes_shares.cpp). A breath hits who
   stands in front, jumping or not, and spares who stands beside or
   behind; lanes hit who stands on a strip and spare the gaps; a gaze hits
   only who is still moving; a shared blow lands on the player farthest
   off and is split between everyone in its circle, which holds still at
   the end. Included by sim_tests.cpp
   after starless_ability_tests.cpp, whose helpers it uses;
   RunDeepAbilityTests runs them. */

internal void
TestConeSparesTheFlanks()
{
    test_world Test = CreateTestWorld();
    world_entity *Stag = AddTestMonster(&Test, MonsterKind_Stag, {600, 1000, 0});
    world_entity *Target = AddTestPlayer(&Test, {680, 1000, 0});
    world_entity *Ahead = AddTestPlayer(&Test, {790, 1030, 0});
    world_entity *Jumper = AddTestPlayer(&Test, {740, 980, 0});
    world_entity *Beside = AddTestPlayer(&Test, {600, 1140, 0});
    world_entity *Behind = AddTestPlayer(&Test, {480, 1000, 0});
    monster_ability *Bellow = ReadyAbilityOfKind(Stag, MonsterAbility_Cone);
    Check(Bellow != 0);

    StepMonsterOverJumper(&Test, Stag, Jumper, 1);
    Check(Stag->AbilityPhase == AbilityPhase_Windup);
    Check(Stag->AbilityAim.X > 0.99f);
    StepMonsterOverJumper(&Test, Stag, Jumper, SecondsToFrames(Bellow->Windup) + 1);
    Check(Target->Hp <= 100.f - Bellow->Damage + 0.01f);
    Check(Ahead->Hp <= 100.f - Bellow->Damage + 0.01f);
    Check(Jumper->Hp <= 100.f - Bellow->Damage + 0.01f);
    Check(Beside->Hp == 100.f && Behind->Hp == 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): three strips 90 apart, the middle one on the target; the
// gap between two strips and the ground past their end are safe, a jump
// is not
internal void
TestLanesSpareTheGaps()
{
    test_world Test = CreateTestWorld();
    world_entity *Harrier = AddTestMonster(&Test, MonsterKind_Harrier, {600, 1000, 0});
    world_entity *Target = AddTestPlayer(&Test, {800, 1000, 0});
    monster_ability *Rake = ReadyAbilityOfKind(Harrier, MonsterAbility_Lanes);
    Check(Rake != 0 && Rake->Count == 3 && Rake->Spread > 2.f * Rake->Radius + 20.f);
    float Gap = 0.5f * Rake->Spread;
    world_entity *InGap = AddTestPlayer(&Test, {800, 1000 + Gap, 0});
    world_entity *OnSide = AddTestPlayer(&Test, {900, 1000 + Rake->Spread, 0});
    world_entity *Past = AddTestPlayer(&Test, {600 + Rake->Speed + 60.f, 1000, 0});
    world_entity *Jumper = AddTestPlayer(&Test, {900, 1000 - Rake->Spread, 0});

    StepMonsterOverJumper(&Test, Harrier, Jumper, 1);
    Check(Harrier->AbilityPhase == AbilityPhase_Windup);
    Check(Harrier->AbilityPointCount == 3);
    Check(IsOnLane(Harrier, Rake, Target->Position.XY));
    Check(!IsOnLane(Harrier, Rake, InGap->Position.XY));
    StepMonsterOverJumper(&Test, Harrier, Jumper, SecondsToFrames(Rake->Windup) + 1);
    Check(Target->Hp <= 100.f - Rake->Damage + 0.01f);
    Check(OnSide->Hp <= 100.f - Rake->Damage + 0.01f);
    Check(Jumper->Hp <= 100.f - Rake->Damage + 0.01f);
    Check(InGap->Hp == 100.f && Past->Hp == 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): when the eye opens only a player still moving is hit; a
// jump on the spot counts as standing still
internal void
TestGazeHitsOnlyTheMoving()
{
    test_world Test = CreateTestWorld();
    world_entity *Watcher = AddTestMonster(&Test, MonsterKind_Watcher, {600, 1000, 0});
    world_entity *Still = AddTestPlayer(&Test, {700, 1100, 0});
    world_entity *Moving = AddTestPlayer(&Test, {800, 1000, 0});
    world_entity *Jumper = AddTestPlayer(&Test, {600, 1200, 0});
    world_entity *FarAway = AddTestPlayer(&Test, {1100, 1000, 0});
    monster_ability *Stare = ReadyAbilityOfKind(Watcher, MonsterAbility_Gaze);
    Check(Stare != 0 && Stare->Radius < 500.f);

    StepMonsterOverJumper(&Test, Watcher, Jumper, 1);
    Check(Watcher->AbilityPhase == AbilityPhase_Windup);
    for(u32 Frame = 0; Frame < SecondsToFrames(Stare->Windup) + 1; Frame++)
    {
        Moving->Velocity = V3(3.f * Stare->Speed, 0.f, 0.f);
        FarAway->Velocity = V3(3.f * Stare->Speed, 0.f, 0.f);
        Still->Velocity = V3(0.5f * Stare->Speed, 0.f, 0.f);
        StepMonsterOverJumper(&Test, Watcher, Jumper, 1);
    }
    Check(Moving->Hp <= 100.f - Stare->Damage + 0.01f);
    Check(Still->Hp == 100.f && Jumper->Hp == 100.f && FarAway->Hp == 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the circle goes on the player in reach farthest from the
// monster and follows them; two players in it take half each
internal void
TestShareSplitsBetweenThoseInIt()
{
    test_world Test = CreateTestWorld();
    world_entity *Mammoth = AddTestMonster(&Test, MonsterKind_Mammoth, {600, 1000, 0});
    world_entity *Target = AddTestPlayer(&Test, {700, 1000, 0});
    world_entity *Buddy = AddTestPlayer(&Test, {760, 1020, 0});
    world_entity *Apart = AddTestPlayer(&Test, {700, 1520, 0});
    monster_ability *Stomp = ReadyAbilityOfKind(Mammoth, MonsterAbility_Share);
    Check(Stomp != 0);

    StepMonster(&Test, Mammoth, 1);
    Check(Mammoth->AbilityPhase == AbilityPhase_Windup);
    Check(Length(Mammoth->AbilityPoints[0] - Buddy->Position.XY) < 1.f);
    Buddy->Position.X += 20.f;
    StepMonster(&Test, Mammoth, 2);
    Check(Length(Mammoth->AbilityPoints[0] - Buddy->Position.XY) < 1.f);
    StepMonster(&Test, Mammoth, SecondsToFrames(Stomp->Windup) + 1);
    float Half = 0.5f * Stomp->Damage;
    Check(Target->Hp <= 100.f - Half + 0.01f && Target->Hp >= 100.f - Half - 0.01f);
    Check(Buddy->Hp <= 100.f - Half + 0.01f && Buddy->Hp >= 100.f - Half - 0.01f);
    Check(Apart->Hp == 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): alone in the circle a player takes all of it; once the
// circle holds still, stepping off it takes nothing
internal void
TestShareAloneTakesAll()
{
    test_world Test = CreateTestWorld();
    world_entity *Mammoth = AddTestMonster(&Test, MonsterKind_Mammoth, {600, 1000, 0});
    world_entity *Alone = AddTestPlayer(&Test, {700, 1000, 0});
    monster_ability *Stomp = ReadyAbilityOfKind(Mammoth, MonsterAbility_Share);
    StepMonster(&Test, Mammoth, SecondsToFrames(Stomp->Windup) + 2);
    Check(Alone->Hp <= 100.f - Stomp->Damage + 0.01f);
    DestroyTestWorld(&Test);

    Test = CreateTestWorld();
    Mammoth = AddTestMonster(&Test, MonsterKind_Mammoth, {600, 1000, 0});
    world_entity *Leaver = AddTestPlayer(&Test, {700, 1000, 0});
    Stomp = ReadyAbilityOfKind(Mammoth, MonsterAbility_Share);
    StepMonster(&Test, Mammoth, 1);
    StepMonster(&Test, Mammoth, SecondsToFrames((1.f - 0.5f * SHARE_LOCK_SHARE) * Stomp->Windup));
    Check(Mammoth->AbilityPhase == AbilityPhase_Windup);
    v2 Spot = Mammoth->AbilityPoints[0];
    Leaver->Position.Y += Stomp->Radius + 40.f;
    StepMonster(&Test, Mammoth, 2);
    Check(Length(Mammoth->AbilityPoints[0] - Spot) < 0.01f);
    StepMonster(&Test, Mammoth, SecondsToFrames(Stomp->Windup));
    Check(Leaver->Hp == 100.f);
    DestroyTestWorld(&Test);
}

internal void
RunDeepAbilityTests()
{
    TestConeSparesTheFlanks();
    TestLanesSpareTheGaps();
    TestGazeHitsOnlyTheMoving();
    TestShareSplitsBetweenThoseInIt();
    TestShareAloneTakesAll();
}
