/* Duel tree tests (sim/progression/talents/duel_perks.cpp): every seed
   fills each branch's wild slots from that branch's pools, never twice
   the same minor in a branch, and every perk turns up in some tree; the
   tree rolls once a point is there to spend and holds for the match; and
   the keystones and minors do what they say: Second Wind, Thorns, Pyre,
   Searing, Overcharge, Thick Skin, Momentum, Steadfast and Mending.
   Included by sim_tests.cpp, which calls RunDuelTreeTests. */

internal void
TestDuelTreeRollsFromEachBranch()
{
    u32 Seen[DuelPerk_Count] = {};
    for(u32 Seed = 1; Seed < 2000; Seed++)
    {
        for(u32 Branch = 0; Branch < TALENT_GAME_BRANCHES; Branch++)
        {
            u32 Minors[4] = {};
            u32 MinorCount = 0;
            for(u32 Index = 0; Index < ArrayCount(DuelWildSlots[0]); Index++)
            {
                u32 Talent = DuelWildSlots[Branch][Index];
                if (Talent == Talent_Count)
                {
                    continue;
                }
                Check(IsDuelWildTalent(Talent));
                Check(TalentDefs[Talent].Branch == Branch);
                u32 Perk = DuelPerkAt(Seed, Talent);
                Check(Perk > DuelPerk_None && Perk < DuelPerk_Count);
                Check(DuelPerkDefs[Perk].Branch == Branch);
                Check((bool32)DuelPerkDefs[Perk].Keystone == (TalentDefs[Talent].MaxLevel == 1));
                Seen[Perk]++;
                if (!DuelPerkDefs[Perk].Keystone)
                {
                    for(u32 Other = 0; Other < MinorCount; Other++)
                    {
                        Check(Minors[Other] != Perk);
                    }
                    Minors[MinorCount++] = Perk;
                }
            }
        }
    }
    for(u32 Perk = 1; Perk < DuelPerk_Count; Perk++)
    {
        Check(Seen[Perk] > 0);
    }
    // NOTE(zoubir): an ability is never wild, and every slot has a shape
    Check(!IsDuelWildTalent(Talent_Fireball) && !IsDuelWildTalent(Talent_Shield));
    Check(DuelPerkAt(5, Talent_Fireball) == DuelPerk_None);
}

// NOTE(zoubir): no tree before the first point; then one, the same for
// everyone and for the rest of the match
internal void
TestDuelTreeRollsAtTheFirstPoint()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1, {400, 300, 0});
    for(u32 Tick = 0; Tick < 30; Tick++)
    {
        UpdateProgression(AppState, Test.Input.DeltaTime);
    }
    Check(AppState->DuelSeed == 0);
    GiveTestXp(&Test, 0, XpToReach(2));
    UpdateProgression(AppState, Test.Input.DeltaTime);
    u32 Seed = AppState->DuelSeed;
    Check(Seed != 0);
    Check(AppState->Players[0].DuelSeed == Seed && AppState->Players[1].DuelSeed == Seed);
    GiveTestXp(&Test, 1, XpToReach(4));
    for(u32 Tick = 0; Tick < 30; Tick++)
    {
        UpdateProgression(AppState, Test.Input.DeltaTime);
    }
    Check(AppState->DuelSeed == Seed);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

// NOTE(zoubir): two players under the duel rules, the target's shields off
internal void
AddDuelPair(test_world *Test, world_entity **Hitter, world_entity **Target)
{
    *Hitter = AddPlayerToSlot(Test->AppState, Test->World, &Test->Arena, 0, {300, 300, 0});
    *Target = AddPlayerToSlot(Test->AppState, Test->World, &Test->Arena, 1, {360, 300, 0});
    (*Hitter)->SpawnShield = 0.f;
    (*Target)->SpawnShield = 0.f;
}

internal void
TestSecondWindAndThorns()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Hitter;
    world_entity *Target;
    AddDuelPair(&Test, &Hitter, &Target);
    UseDuelTree(AppState, DuelPerk_SecondWind, Talent_GuardKeystone);
    player_slot *Slot = &AppState->Players[1];
    Slot->Ranks[Talent_GuardKeystone] = 1;
    DamageEntity(AppState, Test.World, Target, 500.f, Hitter);
    Check(!IsDeadPlayer(Target) && Target->Hp == 1.f);
    Check(Target->SpawnShield >= DUEL_SECOND_WIND_SHIELD - 0.001f && Slot->SecondWindSpent);
    Target->SpawnShield = 0.f;
    DamageEntity(AppState, Test.World, Target, 500.f, Hitter);
    Check(IsDeadPlayer(Target));
    DestroyTestWorld(&Test);

    Test = CreateTestWorld();
    AppState = Test.AppState;
    AddDuelPair(&Test, &Hitter, &Target);
    UseDuelTree(AppState, DuelPerk_Thorns, Talent_GuardKeystone);
    AppState->Players[1].Ranks[Talent_GuardKeystone] = 1;
    float Before = Hitter->Hp;
    DamageEntity(AppState, Test.World, Target, 20.f, Hitter);
    Check(Absolute(Hitter->Hp - (Before - DUEL_THORNS_SHARE * 20.f)) < 0.01f);
    // NOTE(zoubir): both with Thorns trade one answer, not forever
    AppState->Players[0].Ranks[Talent_GuardKeystone] = 1;
    Before = Hitter->Hp;
    float TargetBefore = Target->Hp;
    DamageEntity(AppState, Test.World, Target, 20.f, Hitter);
    Check(Absolute(Hitter->Hp - (Before - DUEL_THORNS_SHARE * 20.f)) < 0.01f);
    Check(Absolute(Target->Hp - (TargetBefore - 20.f)) < 0.01f);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

// NOTE(zoubir): what a fireball lands with its levels and the Fire perks
internal void
TestFireballPerks()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Hitter;
    world_entity *Target;
    AddDuelPair(&Test, &Hitter, &Target);
    player_slot *Slot = &AppState->Players[0];
    hit Plain = FireballHitFor(AppState, Hitter, &FireBallHitRow);
    Check(Plain.Damage == PlayerStats.FireballDamage);
    Slot->Ranks[Talent_Fireball] = 2;
    UseDuelTree(AppState, DuelPerk_Searing, Talent_FireWildA);
    Slot->Ranks[Talent_FireWildA] = 2;
    hit Hot = FireballHitFor(AppState, Hitter, &FireBallHitRow);
    float Want = PlayerStats.FireballDamage *
        (1.f + 2.f * TalentDefs[Talent_Fireball].PowerPerLevel + 2.f * DUEL_SEARING_DAMAGE);
    Check(Absolute(Hot.Damage - Want) < 0.01f);

    UseDuelTree(AppState, DuelPerk_Pyre, Talent_FireKeystone);
    ZeroArray(Slot->Ranks, TALENT_SLOTS, u8);
    Slot->Ranks[Talent_FireKeystone] = 1;
    hit Big = FireballHitFor(AppState, Hitter, &FireBallHitRow);
    Check(Absolute(Big.Damage - PlayerStats.FireballDamage * (1.f + DUEL_PYRE_DAMAGE)) < 0.01f);
    Check(Absolute(PlayerCooldownScale(AppState, Hitter, PlayerButton_Cast) -
                   (1.f + DUEL_PYRE_COOLDOWN)) < 0.001f);

    UseDuelTree(AppState, DuelPerk_Scorch, Talent_FireWildA);
    ZeroArray(Slot->Ranks, TALENT_SLOTS, u8);
    Slot->Ranks[Talent_FireWildA] = 1;
    hit Slow = FireballHitFor(AppState, Hitter, &FireBallHitRow);
    Check(Slow.Status == StatusEffect_Slowed && Slow.StatusSeconds == DUEL_SCORCH_SLOW);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

// NOTE(zoubir): Overcharge deals and takes more, Thick Skin takes less
internal void
TestDamagePerks()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Hitter;
    world_entity *Target;
    AddDuelPair(&Test, &Hitter, &Target);
    UseDuelTree(AppState, DuelPerk_Overcharge, Talent_FireKeystone);
    AppState->Players[0].Ranks[Talent_FireKeystone] = 1;
    Check(Absolute(DuelScaleDamage(AppState, Target, Hitter, 10.f) -
                   10.f * (1.f + DUEL_OVERCHARGE_DAMAGE)) < 0.001f);
    Check(Absolute(DuelScaleDamage(AppState, Hitter, Target, 10.f) -
                   10.f * (1.f + DUEL_OVERCHARGE_TAKEN)) < 0.001f);
    ZeroArray(AppState->Players[0].Ranks, TALENT_SLOTS, u8);
    UseDuelTree(AppState, DuelPerk_ThickSkin, Talent_GuardWildA);
    AppState->Players[1].Ranks[Talent_GuardWildA] = 2;
    Check(Absolute(DuelScaleDamage(AppState, Target, Hitter, 10.f) -
                   10.f * (1.f - 2.f * DUEL_THICK_SKIN_SHARE)) < 0.001f);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

// NOTE(zoubir): Momentum takes cooldown off on a hit; Steadfast shortens
// a stun; Mending heals
internal void
TestMotionAndGuardPerks()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Hitter;
    world_entity *Target;
    AddDuelPair(&Test, &Hitter, &Target);
    UseDuelTree(AppState, DuelPerk_Momentum, Talent_MotionKeystone);
    AppState->Players[0].Ranks[Talent_MotionKeystone] = 1;
    Hitter->MovementCooldowns[PlayerMove_Blink] = 2.f;
    Hitter->ActionCooldowns[PlayerAction_Kunai] = 2.f;
    DamageEntity(AppState, Test.World, Target, 5.f, Hitter);
    Check(Absolute(Hitter->MovementCooldowns[PlayerMove_Blink] - (2.f - DUEL_MOMENTUM_REFUND)) < 0.001f);
    Check(Absolute(Hitter->ActionCooldowns[PlayerAction_Kunai] - (2.f - DUEL_MOMENTUM_REFUND)) < 0.001f);

    UseDuelTree(AppState, DuelPerk_Steadfast, Talent_GuardWildA);
    AppState->Players[1].Ranks[Talent_GuardWildA] = 2;
    Check(Absolute(DuelStunSeconds(AppState, Target, 1.f) - (1.f - 2.f * DUEL_STEADFAST_SHARE)) < 0.001f);
    Check(DuelStunSeconds(AppState, Hitter, 1.f) == 1.f);

    UseDuelTree(AppState, DuelPerk_Mending, Talent_GuardWildA);
    Target->Hp = 30.f;
    for(u32 Tick = 0; Tick < 60; Tick++)
    {
        UpdateProgression(AppState, 1.f / 60.f);
    }
    Check(Absolute(Target->Hp - (30.f + 2.f * DUEL_MENDING_PER_SECOND)) < 0.05f);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

#define DUEL_TREE_TEST(Test) printf("%s\n", #Test); Test()

internal void
RunDuelTreeTests()
{
    DUEL_TREE_TEST(TestDuelTreeRollsFromEachBranch);
    DUEL_TREE_TEST(TestDuelTreeRollsAtTheFirstPoint);
    DUEL_TREE_TEST(TestSecondWindAndThorns);
    DUEL_TREE_TEST(TestFireballPerks);
    DUEL_TREE_TEST(TestDamagePerks);
    DUEL_TREE_TEST(TestMotionAndGuardPerks);
}
