/* Rift ability tests: the Aurora Rift's new ways to dodge
   (sim/monster_abilities/waves_beams_shards.cpp). A frost wave hits who
   stands on the ground once per ring and passes under a jump; a beam
   hits who it sweeps across and stops at walls; a Rimeglass Sentinel
   bursts into shards when it dies. Included by sim_tests.cpp after
   smite_tests.cpp, whose helpers it uses; RunRiftAbilityTests runs them. */

// NOTE(zoubir): the kind's first ability of Kind, with every other one
// held back so it is the one that starts
internal monster_ability *
ReadyAbilityOfKind(world_entity *Monster, monster_ability_kind Kind)
{
    monster_def *Def = GetMonsterDef(Monster->MonsterKind);
    monster_ability *Result = 0;
    for(u32 Index = 0; Index < Def->AbilityCount; Index++)
    {
        if (!Result && Def->Abilities[Index].Kind == Kind)
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

// NOTE(zoubir): steps the monster, holding Jumper in the air, high enough
// that what strikes the ground passes under
internal void
StepMonsterOverJumper(test_world *Test, world_entity *Monster, world_entity *Jumper,
                      u32 Frames)
{
    for(u32 Frame = 0; Frame < Frames; Frame++)
    {
        Jumper->GroundZ = 0.f;
        Jumper->Position.Z = 30.f;
        StepMonster(Test, Monster, 1);
    }
}

internal void
TestWavePassesUnderAJump()
{
    test_world Test = CreateTestWorld();
    world_entity *Yeti = AddTestMonster(&Test, MonsterKind_Yeti, {600, 1000, 0});
    world_entity *Grounded = AddTestPlayer(&Test, {720, 1000, 0});
    world_entity *Jumper = AddTestPlayer(&Test, {600, 1150, 0});
    monster_ability *Pound = ReadyAbilityOfKind(Yeti, MonsterAbility_Wave);
    Check(Pound != 0);

    StepMonsterOverJumper(&Test, Yeti, Jumper, 1);
    Check(Yeti->AbilityPhase == AbilityPhase_Windup);
    StepMonsterOverJumper(&Test, Yeti, Jumper,
                          SecondsToFrames(Pound->Windup) + SecondsToFrames(Pound->Active));
    Check(Grounded->Hp <= 100.f - Pound->Damage + 0.01f);
    Check(Grounded->Hp > 100.f - 2.f * Pound->Damage);
    Check(Jumper->Hp == 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): each of Grondmaw's three rings hits a player who stays
// on the ground once, and only once, however long it takes to pass
internal void
TestEveryRingHitsOnce()
{
    test_world Test = CreateTestWorld();
    world_entity *Grondmaw = AddTestMonster(&Test, MonsterKind_Grondmaw, {500, 1000, 0});
    world_entity *Player = AddTestPlayer(&Test, {800, 1000, 0});
    world_entity *Far = AddTestPlayer(&Test, {500 + 700, 1000, 0});
    monster_ability *Roar = ReadyAbilityOfKind(Grondmaw, MonsterAbility_Wave);
    Check(Roar->Count == 3);
    // NOTE(zoubir): the rings finish rolling before the Active part ends
    Check(Roar->Speed * Roar->Active >= Roar->Radius + (Roar->Count - 1) * Roar->Spread);

    // NOTE(zoubir): the player stands still; a hit's shove is not played
    // out here, so its velocity is cleared each frame
    for(u32 Frame = 0; Frame < 1 + SecondsToFrames(Roar->Windup) +
            SecondsToFrames(Roar->Active); Frame++)
    {
        Player->Velocity = {};
        StepMonster(&Test, Grondmaw, 1);
    }
    float Taken = 100.f - Player->Hp;
    Check(Taken > 3.f * Roar->Damage - 0.1f && Taken < 3.f * Roar->Damage + 0.1f);
    // NOTE(zoubir): past the wave's reach nothing lands
    Check(Far->Hp == 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a player running in toward a ring crosses it as surely as
// one it rolls over; the yeti pounds with them in reach, and they run in
// once the ring is out
internal void
TestRunningIntoARingIsAHit()
{
    test_world Test = CreateTestWorld();
    world_entity *Yeti = AddTestMonster(&Test, MonsterKind_Yeti, {600, 1000, 0});
    world_entity *Runner = AddTestPlayer(&Test, {760, 1000, 0});
    monster_ability *Pound = ReadyAbilityOfKind(Yeti, MonsterAbility_Wave);
    StepMonster(&Test, Yeti, 1 + SecondsToFrames(Pound->Windup));
    Check(Yeti->AbilityPhase == AbilityPhase_Active);
    Runner->Position.X = 600.f + 0.9f * Pound->Radius;
    float Step = 400.f / 60.f;
    for(u32 Frame = 0; Frame < SecondsToFrames(Pound->Active); Frame++)
    {
        Runner->Velocity.XY = V2(-400.f, 0.f);
        Runner->Position.X = Maximum(610.f, Runner->Position.X - Step);
        StepMonster(&Test, Yeti, 1);
    }
    Check(Runner->Hp <= 100.f - Pound->Damage + 0.01f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): an Aurora Wisp's ray sweeps across its target; a player
// off to the side of the sweep is never touched, a jump saves nobody
internal void
TestBeamSweepsAcrossItsTarget()
{
    test_world Test = CreateTestWorld();
    world_entity *Wisp = AddTestMonster(&Test, MonsterKind_Wisp, {600, 1000, 0});
    world_entity *Target = AddTestPlayer(&Test, {800, 1000, 0});
    world_entity *Aside = AddTestPlayer(&Test, {600, 1260, 0});
    monster_ability *Ray = ReadyAbilityOfKind(Wisp, MonsterAbility_Beam);
    Check(Ray != 0);

    StepMonsterOverJumper(&Test, Wisp, Target, 1);
    Check(Wisp->AbilityPhase == AbilityPhase_Windup);
    // NOTE(zoubir): the beam starts half its arc off the target
    float Off = acosf(DotProduct(Wisp->AbilityAim, V2(1.f, 0.f))) * (180.f / Pi32);
    Check(Off > 0.5f * Ray->Spread - 1.f && Off < 0.5f * Ray->Spread + 1.f);
    StepMonsterOverJumper(&Test, Wisp, Target,
                          SecondsToFrames(Ray->Windup) + SecondsToFrames(Ray->Active));
    Check(Target->Hp < 100.f);
    Check(Aside->Hp == 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a slain sentinel throws its shards all the way round, and
// a living one never throws them
internal void
TestSentinelShattersWhenItDies()
{
    test_world Test = CreateTestWorld();
    world_entity *Sentinel = AddRegisteredMonster(&Test, MonsterKind_Sentinel, {800, 1000, 0});
    AddTestPlayer(&Test, {860, 1000, 0});
    monster_def *Def = GetMonsterDef(MonsterKind_Sentinel);
    StepMonster(&Test, Sentinel, 600);
    u32 Shots = 0;
    for(u32 Index = 0; Index < Test.World->EntityCount; Index++)
    {
        Shots += Test.World->Entities[Index].IsPresent &&
            Test.World->Entities[Index].Type == EntityType_MonsterShot;
    }
    Check(Shots == 0);

    DamageEntity(Test.AppState, Test.World, Sentinel, 10000.f, 0);
    Check(!Sentinel->IsPresent);
    RunPendingMonsterDeaths(Test.AppState, Test.World, &Test.Arena, Test.AppState->Monsters);
    for(u32 Index = 0; Index < Test.World->EntityCount; Index++)
    {
        world_entity *Shot = &Test.World->Entities[Index];
        if (Shot->IsPresent && Shot->Type == EntityType_MonsterShot)
        {
            Shots++;
            Check(Shot->MonsterKind == MonsterKind_Sentinel);
            Check(Shot->AbilityIndex == Def->ShatterAbility);
        }
    }
    Check(Shots == Def->Abilities[Def->ShatterAbility].Count);
    DestroyTestWorld(&Test);
}

internal void
RunRiftAbilityTests()
{
    TestWavePassesUnderAJump();
    TestEveryRingHitsOnce();
    TestRunningIntoARingIsAHit();
    TestBeamSweepsAcrossItsTarget();
    TestSentinelShattersWhenItDies();
}
