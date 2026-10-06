/* Progression tests (sim/progression/): experience from player kills,
   monster kills and time; the level curve; the talent tree's rules (a
   point per level, tiers that open with points spent in their branch,
   ranks that stop at their most); talents unlocking abilities and levels
   shortening cooldowns; Frost Nova, Gravity Well and the ward. Most run
   under the duel rules the game plays and put the classic ones back.
   Included by sim_tests.cpp, which calls RunProgressionTests. */

// NOTE(zoubir): Xp for Slot as if earned, so its level follows
internal void
GiveTestXp(test_world *Test, u32 SlotIndex, u32 Xp)
{
    AwardXp(Test->AppState, &Test->AppState->Players[SlotIndex], Xp);
}

internal void
TestLevelCurve()
{
    Check(XpToReach(1) == 0);
    Check(XpToReach(2) == 80);
    Check(XpToReach(3) == 180);
    Check(XpToReach(PLAYER_MAX_LEVEL) == 4940);
    Check(LevelForXp(0) == 1);
    Check(LevelForXp(79) == 1);
    Check(LevelForXp(80) == 2);
    Check(LevelForXp(4939) == 19);
    Check(LevelForXp(4940) == PLAYER_MAX_LEVEL);
    Check(LevelForXp(100000) == PLAYER_MAX_LEVEL);
    Check(PlayerKillXp(1, 1) == 100);
    Check(PlayerKillXp(3, 5) == 130);
    Check(PlayerKillXp(5, 3) == 70);
    Check(PlayerKillXp(1, 20) == XP_PLAYER_KILL_MAX);
    Check(PlayerKillXp(20, 1) == XP_PLAYER_KILL_MIN);
    Check(LevelProgress(80) == 0.f);
    Check(LevelProgress(130) == 0.5f);
}

// NOTE(zoubir): a player kill is worth a level at the start, a monster a
// token, and a second of play two points, alive or dead
internal void
TestKillsAndTimeGiveXp()
{
    // NOTE(zoubir): without the round break's level for everyone
    // (round_break.cpp), which would hide what a kill pays
    GameRules = DuelRules;
    GameRules.RoundBreaks = false;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Killer = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Victim = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {360, 300, 0});
    player_slot *KillerSlot = &AppState->Players[0];
    Check(KillerSlot->Level == 1 && KillerSlot->Xp == 0);
    Victim->SpawnShield = 0.f;
    DamageEntity(AppState, Test.World, Victim, Victim->MaxHp, Killer);
    Check(Victim->Hp <= 0.f);
    Check(KillerSlot->Xp == XP_PLAYER_KILL);
    Check(KillerSlot->Level == 2);
    Check(TalentPointsLeft(KillerSlot) == 1);
    Check(AppState->Players[1].Xp == 0);

    world_entity *Monster = AddTestMonster(&Test, (monster_kind)0, {600, 600, 0});
    DamageEntity(AppState, Test.World, Monster, Monster->Hp + 1.f, Killer);
    Check(KillerSlot->Xp == XP_PLAYER_KILL + XP_MONSTER_KILL);

    // NOTE(zoubir): ten seconds of ticks, the victim still dead for most
    // of them
    u32 Before = AppState->Players[1].Xp;
    for(u32 Tick = 0; Tick < 600; Tick++)
    {
        UpdateProgression(AppState, Test.Input.DeltaTime);
    }
    u32 Gained = AppState->Players[1].Xp - Before;
    Check(Gained >= 19 && Gained <= 21);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

// NOTE(zoubir): a point per level; tier 1 opens at 2 points in its branch,
// tier 3 at 6; a base ability takes two more levels, a passive its ranks
internal void
TestTalentTreeRules()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    player_slot *Slot = &AppState->Players[0];
    Check(TalentLevel(Slot, Talent_Fireball) == 1);
    Check(TalentLevel(Slot, Talent_Shockwave) == 0);
    Check(TalentMaxRanks(Talent_Fireball) == 2);
    Check(TalentMaxRanks(Talent_Shockwave) == 3);
    Check(CanLearnTalent(Slot, Talent_Fireball) == TalentRefusal_NoPoints);
    Check(!LearnTalent(AppState, 0, Talent_Fireball));

    GiveTestXp(&Test, 0, XpToReach(10));
    Check(Slot->Level == 10 && TalentPointsLeft(Slot) == 9);
    Check(CanLearnTalent(Slot, Talent_Shockwave) == TalentRefusal_TierLocked);
    Check(LearnTalent(AppState, 0, Talent_Fireball));
    Check(CanLearnTalent(Slot, Talent_Shockwave) == TalentRefusal_TierLocked);
    Check(LearnTalent(AppState, 0, Talent_SwiftFlames));
    Check(CanLearnTalent(Slot, Talent_Shockwave) == TalentRefusal_None);
    // NOTE(zoubir): two points in Fire open nothing in Motion
    Check(CanLearnTalent(Slot, Talent_Sword) == TalentRefusal_TierLocked);
    Check(LearnTalent(AppState, 0, Talent_Fireball));
    Check(TalentLevel(Slot, Talent_Fireball) == 3);
    Check(CanLearnTalent(Slot, Talent_Fireball) == TalentRefusal_MaxRank);
    Check(!LearnTalent(AppState, 0, Talent_Fireball));
    Check(LearnTalent(AppState, 0, Talent_SwiftFlames));
    Check(!LearnTalent(AppState, 0, Talent_SwiftFlames));
    Check(TalentPointsSpent(Slot, TalentBranch_Fire) == 4);
    Check(TalentPointsLeft(Slot) == 5);
    Check(CanLearnTalent(Slot, Talent_FrostNova) == TalentRefusal_None);
    Check(CanLearnTalent(Slot, Talent_Pyre) == TalentRefusal_TierLocked);

    // NOTE(zoubir): a request through the input is spent once, on the tick
    Slot->Input.Learn = Talent_Launch + 1;
    UpdateProgression(AppState, Test.Input.DeltaTime);
    Check(TalentLevel(Slot, Talent_Launch) == 2);
    Check(Slot->Input.Learn == 0);
    UpdateProgression(AppState, Test.Input.DeltaTime);
    Check(TalentLevel(Slot, Talent_Launch) == 2);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

// NOTE(zoubir): Shockwave does nothing in a duel until the tree unlocks
// it; then its key casts it
internal void
TestTalentUnlocksAbility()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Aim = V2(1.f, 0.f);
    Slot->Input.Pressed = PlayerButton_Shockwave;
    RunPlayerFrames(&Test, 0, 1);
    Check(Player->AreaCooldowns[PlayerArea_Shockwave] == 0.f);
    Check((PlayerAllowedButtons(Slot) & PlayerButton_Shockwave) == 0);

    GiveTestXp(&Test, 0, XpToReach(4));
    Check(LearnTalent(AppState, 0, Talent_Fireball));
    Check(LearnTalent(AppState, 0, Talent_SwiftFlames));
    Check(LearnTalent(AppState, 0, Talent_Shockwave));
    Check(PlayerAllowedButtons(Slot) & PlayerButton_Shockwave);
    Slot->Input.Pressed = PlayerButton_Shockwave;
    RunPlayerFrames(&Test, 0, 1);
    Check(Player->AreaCooldowns[PlayerArea_Shockwave] > 0.f);
    Check(Player->CastSpell == PlayerSpell_Shockwave);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

// NOTE(zoubir): each level after the first takes 15% off: 6 s, 5.1, 4.2
internal void
TestAbilityLevelsShortenCooldowns()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Aim = V2(1.f, 0.f);
    float Full = 0.f;
    u32 FireballCooldown = PlayerMove_Count + PLAYER_AREA_ABILITY_COUNT +
        RewindKind_Count + PlayerAction_FireBall;
    PlayerCooldown(AppState, Player, FireballCooldown, &Full);
    Check(Absolute(Full - 6.f) < 0.001f);

    GiveTestXp(&Test, 0, XpToReach(3));
    Check(LearnTalent(AppState, 0, Talent_Fireball));
    Slot->Input.Pressed = PlayerButton_Cast;
    RunPlayerFrames(&Test, 0, 1);
    Check(Absolute(Player->ActionCooldowns[PlayerAction_FireBall] - 5.1f) < 0.05f);
    PlayerCooldown(AppState, Player, FireballCooldown, &Full);
    Check(Absolute(Full - 5.1f) < 0.001f);

    Check(LearnTalent(AppState, 0, Talent_Shield));
    Slot->Input.Pressed = PlayerButton_Shield;
    RunPlayerFrames(&Test, 0, 1);
    Check(Absolute(Player->MovementCooldowns[PlayerMove_Shield] - 6.f * 0.85f) < 0.05f);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

// NOTE(zoubir): Frost Nova freezes and slows who stands near; Gravity
// Well pulls toward its centre from either side
internal void
TestFrostNovaAndGravityWell()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Caster = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {600, 600, 0});
    world_entity *Near = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                         1, {690, 600, 0});
    world_entity *Far = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                        2, {600, 900, 0});
    Near->SpawnShield = 0.f;
    Far->SpawnShield = 0.f;
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Aim = V2(1.f, 0.f);
    Slot->Input.Pressed = PlayerButton_FrostNova;
    RunPlayerFrames(&Test, 0, 1);
    Check(Caster->CastSpell == PlayerSpell_FrostNova);
    FinishTestCast(&Test, 0);
    Check(HasStatus(Near, StatusEffect_Stunned));
    Check(HasStatus(Near, StatusEffect_Slowed));
    Check(!HasStatus(Far, StatusEffect_Stunned));
    Check(Near->Hp == Near->MaxHp);

    // NOTE(zoubir): a target past the well's centre is pulled back toward
    // the caster, one short of it out toward it
    Near->StatusTimers[StatusEffect_Stunned] = 0.f;
    Near->Velocity = {};
    Near->Position = V3(Caster->Position.X + 240.f, Caster->Position.Y, 0.f);
    Far->Position = V3(Caster->Position.X + 110.f, Caster->Position.Y, 0.f);
    Far->Velocity = {};
    Slot->Input.Pressed = PlayerButton_GravityWell;
    RunPlayerFrames(&Test, 0, 1);
    Check(Caster->CastSpell == PlayerSpell_GravityWell);
    FinishTestCast(&Test, 0);
    Check(Near->Velocity.X < -100.f);
    Check(Far->Velocity.X > 100.f);
    Check(HasStatus(Near, StatusEffect_Stunned));
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a ward takes one hit whole, then the next ones hurt; it
// comes back after its rank's seconds
internal void
TestWardTakesOneHit()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Caster = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Target = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {360, 300, 0});
    Target->SpawnShield = 0.f;
    player_slot *Slot = &AppState->Players[1];
    GiveTestXp(&Test, 1, XpToReach(2));
    Check(LearnTalent(AppState, 1, Talent_Ward));
    Check(Slot->WardReady);
    world_entity *FireBall = AddFireBall(AppState, Test.World, &Test.Arena, Caster,
                                         V3(330.f, 300.f, 30.f), V3(600.f, 0.f, 0.f));
    FireBallHit(AppState, Test.World, FireBall, Target);
    Check(Target->Hp == Target->MaxHp);
    Check(!Slot->WardReady);
    Check(Length(Target->Velocity.XY) == 0.f);

    for(u32 Tick = 0; Tick < (u32)(17.9f / Test.Input.DeltaTime); Tick++)
    {
        UpdateProgression(AppState, Test.Input.DeltaTime);
    }
    Check(!Slot->WardReady);
    for(u32 Tick = 0; Tick < 12; Tick++)
    {
        UpdateProgression(AppState, Test.Input.DeltaTime);
    }
    Check(Slot->WardReady);
    FireBallHit(AppState, Test.World, FireBall, Target);
    Check(Target->Hp == Target->MaxHp);
    FireBallHit(AppState, Test.World, FireBall, Target);
    Check(Target->Hp < Target->MaxHp && Target->Hp > 0.f);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

// NOTE(zoubir): Second Wind halves the wait and lengthens the shield;
// Pyre makes the fireball ready on a kill
internal void
TestKillAndDeathTalents()
{
    // NOTE(zoubir): without the round break, whose wait would hide the
    // shorter respawn (round_break.cpp)
    GameRules = DuelRules;
    GameRules.RoundBreaks = false;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Killer = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Victim = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {360, 300, 0});
    player_slot *KillerSlot = &AppState->Players[0];
    player_slot *VictimSlot = &AppState->Players[1];
    KillerSlot->Ranks[Talent_Pyre] = 1;
    VictimSlot->Ranks[Talent_SecondWind] = 1;
    Killer->ActionCooldowns[PlayerAction_FireBall] = 5.f;
    Victim->SpawnShield = 0.f;
    DamageEntity(AppState, Test.World, Victim, Victim->MaxHp, Killer);
    Check(Killer->ActionCooldowns[PlayerAction_FireBall] == 0.f);
    Check(Absolute(VictimSlot->RespawnTimer - 0.5f * PLAYER_RESPAWN_SECONDS) < 0.001f);
    for(u32 Tick = 0; Tick < 120 && IsDeadPlayer(Victim); Tick++)
    {
        UpdateDeadPlayer(VictimSlot, Test.World, &Test.Arena, AppState,
                         Test.Input.DeltaTime);
    }
    Check(!IsDeadPlayer(Victim));
    Check(Absolute(Victim->SpawnShield - TALENT_SECOND_WIND_SHIELD) < 0.001f);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

// NOTE(zoubir): a level bought past the first lengthens Frost Nova's
// freeze and slow, shoves harder with Push and speeds the fireball; the
// first level adds nothing
internal void
TestAbilityLevelPerks()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    player_slot *Slot = &AppState->Players[0];
    hit *Nova = &PlayerAreaAbilities[PlayerArea_FrostNova].Hit;
    hit Base = AbilityLevelHit(AppState, Player, PlayerButton_FrostNova, Nova);
    Check(Base.StunSeconds == Nova->StunSeconds && Base.StatusSeconds == Nova->StatusSeconds);
    Slot->Ranks[Talent_FrostNova] = 2;
    hit Top = AbilityLevelHit(AppState, Player, PlayerButton_FrostNova, Nova);
    Check(Absolute(Top.StunSeconds - (Nova->StunSeconds + 0.4f)) < 0.001f);
    Check(Absolute(Top.StatusSeconds - (Nova->StatusSeconds + 1.5f)) < 0.001f);

    hit *Push = &PlayerAreaAbilities[PlayerArea_Push].Hit;
    Slot->Ranks[Talent_Push] = 1;
    Check(Absolute(AbilityLevelHit(AppState, Player, PlayerButton_Push, Push).Shove -
                   1.15f * Push->Shove) < 0.01f);

    Check(FireballSpeedScale(AppState, Player) == 1.f);
    Slot->Ranks[Talent_Fireball] = 1;
    Slot->Ranks[Talent_SwiftFlames] = 2;
    Check(Absolute(FireballSpeedScale(AppState, Player) - 1.08f * 1.3f) < 0.001f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the reset gives every point back and locks what the
// points unlocked
internal void
TestTalentReset()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    player_slot *Slot = &AppState->Players[0];
    GiveTestXp(&Test, 0, XpToReach(6));
    Check(LearnTalent(AppState, 0, Talent_Fireball));
    Check(LearnTalent(AppState, 0, Talent_SwiftFlames));
    Check(LearnTalent(AppState, 0, Talent_Shockwave));
    Check(LearnTalent(AppState, 0, Talent_Ward));
    Check(TalentPointsLeft(Slot) == 1);
    Check(PlayerAllowedButtons(Slot) & PlayerButton_Shockwave);
    Slot->Input.Learn = TALENT_LEARN_RESET;
    UpdateProgression(AppState, Test.Input.DeltaTime);
    Check(TalentPointsSpent(Slot) == 0);
    Check(TalentPointsLeft(Slot) == 5);
    Check((PlayerAllowedButtons(Slot) & PlayerButton_Shockwave) == 0);
    Check(!Slot->WardReady);
    Check(TalentLevel(Slot, Talent_Fireball) == 1);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

#define PROGRESSION_TEST(Test) printf("%s\n", #Test); Test()

internal void
RunProgressionTests()
{
    PROGRESSION_TEST(TestLevelCurve);
    PROGRESSION_TEST(TestKillsAndTimeGiveXp);
    PROGRESSION_TEST(TestTalentTreeRules);
    PROGRESSION_TEST(TestTalentUnlocksAbility);
    PROGRESSION_TEST(TestAbilityLevelsShortenCooldowns);
    PROGRESSION_TEST(TestFrostNovaAndGravityWell);
    PROGRESSION_TEST(TestWardTakesOneHit);
    PROGRESSION_TEST(TestKillAndDeathTalents);
    PROGRESSION_TEST(TestAbilityLevelPerks);
    PROGRESSION_TEST(TestTalentReset);
}
