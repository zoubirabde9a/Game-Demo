/* Kunai tests (sim/player_abilities/kunai.cpp): the key throws one at the
   unit the input names (player_input.Target, the cursor's pick), on its
   cooldown; with no unit named, or one out of range, the key does
   nothing and spends nothing; it follows its unit as it
   walks off and even when it blinks away; a shielded player sends it
   back into the thrower; two shielded players bounce it back and forth
   for as long as their shields last. Included by sim_tests.cpp, which
   calls RunKunaiTests. */

// NOTE(zoubir): Player's cursor on Target, as the input would set it
internal void
AimKunaiAt(world_entity *Player, world_entity *Target)
{
    v2 To = Target->Position.XY - Player->Position.XY;
    Player->Aim = NormalizeOr(To, V2(1.f, 0.f));
    Player->AimReach = Minimum(1.f, Length(To) / PLAYER_AIM_REACH);
    Player->CursorTarget = Target->ID + 1;
}

// NOTE(zoubir): the one kunai in the world, 0 once it is gone
internal world_entity *
FindKunai(world *World)
{
    world_entity *Result = 0;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent && Entity->Type == EntityType_Kunai)
        {
            Result = Entity;
        }
    }
    return Result;
}

// NOTE(zoubir): flies the kunai for up to Frames ticks, counting the
// times it glanced off a shield; stops when it is gone
internal u32
FlyKunai(test_world *Test, u32 Frames)
{
    app_state *AppState = Test->AppState;
    u32 Bounces = 0;
    for(u32 Frame = 0; Frame < Frames; Frame++)
    {
        world_entity *Kunai = FindKunai(Test->World);
        if (!Kunai)
        {
            break;
        }
        AppState->Events.Count = 0;
        UpdateKunai(Kunai, Test->World, &Test->Arena, Test->Input.DeltaTime, AppState);
        for(u32 Index = 0; Index < AppState->Events.Count; Index++)
        {
            sim_event *Event = &AppState->Events.Events[Index];
            Bounces += (Event->Type == SimEvent_Burst &&
                        Event->Burst == SimBurst_KunaiReflect) ? 1 : 0;
        }
    }
    return Bounces;
}

internal void
TestKunaiKeyThrowsOnCooldown()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Target = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {450, 300, 0});
    AppState->Players[0].Input.Aim = V2(150.f / PLAYER_AIM_REACH, 0.f);
    AppState->Players[0].Input.Target = Target->ID + 1;
    AppState->Players[0].Input.Pressed = PlayerButton_Kunai;
    RunPlayerFrames(&Test, 0, 2);
    world_entity *Kunai = FindKunai(Test.World);
    Check(Kunai != 0);
    Check(Kunai && Kunai->HasOwner && Kunai->OwnerSlot == 0);
    Check(Kunai && Kunai->FollowingEntity == Target);
    Check(Kunai && Kunai->Velocity.X > PlayerStats.FireballSpeed);
    Check(Player->ActionCooldowns[PlayerAction_Kunai] > 3.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the cursor on no unit, then on one past the range: no
// kunai, and the key is still ready
internal void
TestKunaiNeedsATarget()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Far = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1,
                                        {300.f + PlayerStats.KunaiRange + 100.f, 300, 0});
    AppState->Players[0].Input.Aim = V2(1.f, 0.f);
    AppState->Players[0].Input.Pressed = PlayerButton_Kunai;
    RunPlayerFrames(&Test, 0, 30);
    Check(!FindKunai(Test.World));
    Check(Player->ActionCooldowns[PlayerAction_Kunai] == 0.f);
    AppState->Players[0].Input.Target = Far->ID + 1;
    AppState->Players[0].Input.Pressed = PlayerButton_Kunai;
    RunPlayerFrames(&Test, 0, 30);
    Check(!FindKunai(Test.World));
    Check(Player->ActionCooldowns[PlayerAction_Kunai] == 0.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the target walks down out of the throw's line; a straight
// kunai would pass over its head, a homing one turns and lands
internal void
TestKunaiFollowsItsTarget()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Thrower = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                            0, {300, 300, 0});
    world_entity *Target = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {520, 330, 0});
    Target->SpawnShield = 0.f;
    float StartHp = Target->Hp;
    AimKunaiAt(Thrower, Target);
    ThrowKunai(AppState, Test.World, &Test.Arena, Thrower, V2(1.f, 0.f), 0);
    world_entity *Kunai = FindKunai(Test.World);
    Check(Kunai && Kunai->FollowingEntity == Target);
    for(u32 Frame = 0; Frame < 60 && FindKunai(Test.World); Frame++)
    {
        Target->Position.Y += 2.f;
        FlyKunai(&Test, 1);
    }
    Check(!FindKunai(Test.World));
    Check(Target->Hp < StartHp - 0.5f * PlayerStats.KunaiDamage);

    // NOTE(zoubir): a blink 200 units aside after the throw does not lose it
    StartHp = Target->Hp;
    Target->Position = V3(520.f, 300.f, 0.f);
    AimKunaiAt(Thrower, Target);
    ThrowKunai(AppState, Test.World, &Test.Arena, Thrower, V2(1.f, 0.f), 0);
    FlyKunai(&Test, 3);
    Target->Position.Y += 200.f;
    FlyKunai(&Test, 120);
    Check(!FindKunai(Test.World));
    Check(Target->Hp < StartHp - 0.5f * PlayerStats.KunaiDamage);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the shield sends it back into the thrower, now owned by
// the shielded player
internal void
TestShieldSendsKunaiBack()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Thrower = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                            0, {300, 300, 0});
    world_entity *Shielded = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                             1, {450, 300, 0});
    Thrower->SpawnShield = 0.f;
    Shielded->SpawnShield = 2.f;
    float ThrowerHp = Thrower->Hp;
    float ShieldedHp = Shielded->Hp;
    AimKunaiAt(Thrower, Shielded);
    ThrowKunai(AppState, Test.World, &Test.Arena, Thrower, V2(1.f, 0.f), 0);
    u32 Bounces = FlyKunai(&Test, 120);
    Check(Bounces == 1);
    Check(!FindKunai(Test.World));
    Check(Shielded->Hp == ShieldedHp);
    Check(Thrower->Hp < ThrowerHp - 0.5f * PlayerStats.KunaiDamage);
    Check(Thrower->HitBySlot == 2);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): both shields up: back and forth, unhurt, and still flying
internal void
TestTwoShieldsBounceKunaiForever()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *A = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                      0, {300, 300, 0});
    world_entity *B = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                      1, {450, 300, 0});
    A->SpawnShield = B->SpawnShield = 100.f;
    float HpA = A->Hp;
    float HpB = B->Hp;
    AimKunaiAt(A, B);
    ThrowKunai(AppState, Test.World, &Test.Arena, A, V2(1.f, 0.f), 0);
    u32 Bounces = FlyKunai(&Test, 180);
    Check(Bounces >= 6);
    Check(FindKunai(Test.World) != 0);
    Check(A->Hp == HpA && B->Hp == HpB);
    DestroyTestWorld(&Test);
}

#define KUNAI_TEST(Test) printf("%s\n", #Test); Test()

internal void
RunKunaiTests()
{
    KUNAI_TEST(TestKunaiKeyThrowsOnCooldown);
    KUNAI_TEST(TestKunaiNeedsATarget);
    KUNAI_TEST(TestKunaiFollowsItsTarget);
    KUNAI_TEST(TestShieldSendsKunaiBack);
    KUNAI_TEST(TestTwoShieldsBounceKunaiForever);
}
