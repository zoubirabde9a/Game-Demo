/* Shadowblade tests (sim/dungeon/role_kits/shadowblade.cpp), included by
   dungeon_tests.cpp: the class owns its seven keys, C and V once learned;
   Twin Strike cuts twice in front and builds a point; Poisoned Shiv hits
   and poisons for the thrower; Shadowstep lands behind the foe and makes
   the next strike critical; Fan of Knives winds up, cuts every foe near
   and builds a point each; Eviscerate spends the points for damage by
   the point and refuses to cast with none; Smoke Bomb wipes the party's
   threat in it (not the tank's) and cuts what they take; Shadow Dance
   echoes strikes and hurries Shadowstep; the talents; and points fading
   out of a fight. */

// NOTE(zoubir): a Shadowblade in slot 0 aiming along +X
internal player_slot *
ReadyShadowblade(crypt_world *Crypt)
{
    app_state *AppState = Crypt->AppState;
    TickCrypt(Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Shadowblade);
    Slot->Input.Aim = V2(1.f, 0.f);
    TickCrypt(Crypt, 1);
    return Slot;
}

// NOTE(zoubir): Ticks ticks with Foe held where it is
internal void
TickHolding(crypt_world *Crypt, world_entity *Foe, u32 Ticks)
{
    v3 Spot = Foe->Position;
    for(u32 Tick = 0; Tick < Ticks; Tick++)
    {
        Foe->Position = Spot;
        Foe->Velocity = {};
        TickCrypt(Crypt, 1);
    }
}

internal void
TestShadowbladeOwnsItsKeys()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = ReadyShadowblade(&Crypt);
    Check(RoleHasKit(PlayerRole_Shadowblade));
    u32 Allowed = RunAllowedButtons(AppState, Slot, 0);
    u32 Main = PlayerButton_Launch | PlayerButton_Push | PlayerButton_Shockwave |
        PlayerButton_Cast | PlayerButton_Attack;
    Check((Allowed & Main) == Main);
    Check(!(Allowed & (PlayerButton_Slam | PlayerButton_Kunai)));
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_SmokeBomb] = 1;
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_ShadowDance] = 1;
    Allowed = RunAllowedButtons(AppState, Slot, 0);
    Check((Allowed & (PlayerButton_Slam | PlayerButton_Kunai)) == (PlayerButton_Slam | PlayerButton_Kunai));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): two cuts on the foe in front, a moment apart, one point;
// the foe behind is not cut
internal void
TestTwinStrike()
{
    crypt_world Crypt = CreateCryptWorld(1);
    player_slot *Slot = ReadyShadowblade(&Crypt);
    world_entity *Front = StrikerDummy(&Crypt, V3(45.f, 0.f, 0.f));
    world_entity *Back = StrikerDummy(&Crypt, V3(-60.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    float First = 2000.f - Front->Hp;
    Check(First > 0.f);
    Check(Slot->ClassMeter == 1);
    Check(Slot->RoleCooldowns[6] > 0.f);
    TickHolding(&Crypt, Front, 10);
    float Both = 2000.f - Front->Hp;
    Check(Both > 1.9f * First && Both < 2.1f * First);
    Check(Back->Hp == 2000.f);
    Check(Slot->ClassMeter == 1);
    // NOTE(zoubir): points stop at five
    for(u32 Strike = 0; Strike < 7; Strike++)
    {
        TickHolding(&Crypt, Front, 30);
        PressOnce(&Crypt, 0, PlayerButton_Attack);
    }
    Check(Slot->ClassMeter == SHADOWBLADE_MOST_POINTS);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the shiv hits a foe across the room and poisons it; the
// poison counts as the Shadowblade's; with no foe in reach, nothing
internal void
TestPoisonedShiv()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = ReadyShadowblade(&Crypt);
    Slot->Entity->Aim = V2(0.f, 1.f);
    Slot->Entity->AimReach = 0.9f;
    PressOnce(&Crypt, 0, PlayerButton_Cast);
    Check(Slot->RoleCooldowns[5] == 0.f && Slot->ClassMeter == 0);
    world_entity *Foe = StrikerDummy(&Crypt, V3(300.f, 0.f, 0.f));
    PressAt(&Crypt, 0, PlayerButton_Cast, Foe);
    float Hit = 2000.f - Foe->Hp;
    Check(Hit > 0.f);
    Check(HasStatus(Foe, StatusEffect_Poisoned));
    Check(Slot->ClassMeter == 1 && Slot->RoleCooldowns[5] > 0.f);
    TickHolding(&Crypt, Foe, 120);
    Check(2000.f - Foe->Hp > Hit + 4.f);
    shadowblade_poison *Poison = &AppState->Dungeon->Shadowblade.Poisons[0];
    Check(Poison->Seconds > 0.f && Poison->PerSecond == SHIV_POISON_PER_SECOND);
    TickHolding(&Crypt, Foe, (u32)(60.f * SHIV_POISON_SECONDS));
    Check(Poison->Seconds == 0.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the step lands on the far side of a foe facing the
// Shadowblade, and the next strike is twice as hard, once
internal void
TestShadowstep()
{
    crypt_world Crypt = CreateCryptWorld(1);
    player_slot *Slot = ReadyShadowblade(&Crypt);
    world_entity *Blade = Slot->Entity;
    world_entity *Foe = StrikerDummy(&Crypt, V3(220.f, 0.f, 0.f));
    world_entity *Other = StrikerDummy(&Crypt, V3(0.f, 160.f, 0.f));
    // NOTE(zoubir): a plain cut first, to compare with
    Slot->Input.Aim = V2(0.f, 1.f);
    TickCrypt(&Crypt, 1);
    MovePlayerTo(Crypt.AppState, &Crypt.AppState->World, &Crypt.Arena, Blade,
                 Other->Position - V3(0.f, 45.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    float Plain = 2000.f - Other->Hp;
    Check(Plain > 0.f);
    TickHolding(&Crypt, Foe, 40);

    Foe->Direction = DirectionTo(Blade->Position.XY - Foe->Position.XY);
    PressAt(&Crypt, 0, PlayerButton_Launch, Foe);
    Check(Slot->RoleCooldowns[0] > 0.f);
    v2 Back = Blade->Position.XY - Foe->Position.XY;
    Check(Length(Back) < SHADOWSTEP_GAP + Foe->Dimensions.X + 30.f);
    // NOTE(zoubir): on the far side from where it stood (the foe turns
    // round to face it afterwards)
    Check(Back.X > 0.f);
    Check(Slot->ClassFlags & SHADOWBLADE_FLAG_CRIT);
    Slot->Input.Aim = DirectionTo(Foe->Position.XY - Blade->Position.XY);
    TickCrypt(&Crypt, 1);
    float Before = Foe->Hp;
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    float Crit = Before - Foe->Hp;
    Check(Crit > 1.9f * SHADOWBLADE_CRIT_SCALE * 0.5f * Plain);
    Check(!(Slot->ClassFlags & SHADOWBLADE_FLAG_CRIT));
    TickHolding(&Crypt, Foe, 10);
    float Second = Before - Foe->Hp - Crit;
    Check(Second < 0.6f * Crit);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the fan winds up, then cuts every foe near, a point each
internal void
TestFanOfKnives()
{
    crypt_world Crypt = CreateCryptWorld(1);
    player_slot *Slot = ReadyShadowblade(&Crypt);
    world_entity *Near[3] = {StrikerDummy(&Crypt, V3(60.f, 0.f, 0.f)),
                             StrikerDummy(&Crypt, V3(-60.f, 30.f, 0.f)),
                             StrikerDummy(&Crypt, V3(0.f, -80.f, 0.f))};
    world_entity *Far = StrikerDummy(&Crypt, V3(400.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Push);
    Check(Slot->Entity->CastSpell == PlayerSpell_ShadowbladeA);
    Check(Slot->RoleCooldowns[1] > 0.f);
    Check(Near[0]->Hp == 2000.f);
    TickCrypt(&Crypt, (u32)(60.f * PlayerSpells[PlayerSpell_ShadowbladeA].CastTime) + 2);
    Check(Slot->Entity->CastSpell == 0);
    for(u32 Index = 0; Index < 3; Index++)
    {
        Check(Near[Index]->Hp < 2000.f);
    }
    Check(Far->Hp == 2000.f);
    Check(Slot->ClassMeter == 3);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): what Eviscerate with Points did to a fresh foe in front
internal float
EviscerateWith(crypt_world *Crypt, player_slot *Slot, u32 Points)
{
    world_entity *Foe = StrikerDummy(Crypt, V3(50.f, 0.f, 0.f));
    Slot->ClassMeter = (u8)Points;
    Slot->RoleCooldowns[4] = 0.f;
    PressAt(Crypt, 0, PlayerButton_Shockwave, Foe);
    TickHolding(Crypt, Foe, (u32)(60.f * PlayerSpells[PlayerSpell_ShadowbladeB].CastTime) + 2);
    float Result = 2000.f - Foe->Hp;
    KillEntity(Crypt->AppState, &Crypt->AppState->World, Foe, 0);
    return Result;
}

internal void
TestEviscerate()
{
    crypt_world Crypt = CreateCryptWorld(1);
    player_slot *Slot = ReadyShadowblade(&Crypt);
    // NOTE(zoubir): no points: no wind-up, no cooldown
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(Slot->Entity->CastSpell == 0 && Slot->RoleCooldowns[4] == 0.f);
    float One = EviscerateWith(&Crypt, Slot, 1);
    Check(Slot->ClassMeter == 0);
    float Five = EviscerateWith(&Crypt, Slot, 5);
    Check(Slot->ClassMeter == 0);
    float Expected = (EVISCERATE_DAMAGE + 5.f * EVISCERATE_PER_POINT) /
        (EVISCERATE_DAMAGE + EVISCERATE_PER_POINT);
    Check(One > 0.f && Five > 0.99f * Expected * One && Five < 1.01f * Expected * One);
    // NOTE(zoubir): no foe in reach when the draw ends: the points stay,
    // the key is ready again, and the burst says nothing was spent
    Slot->ClassMeter = 3;
    Slot->RoleCooldowns[4] = 0.f;
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(Slot->Entity->CastSpell == PlayerSpell_ShadowbladeB);
    TickCrypt(&Crypt, (u32)(60.f * PlayerSpells[PlayerSpell_ShadowbladeB].CastTime) + 2);
    Check(Slot->ClassMeter == 3 && Slot->RoleCooldowns[4] == 0.f);
    Check(ShadowbladeBurstVariant(ShadowbladeBurstSpot(V3(1.f, 2.f, 16.f), 4)) == 4);
    Check(ShadowbladeBurstPlace(ShadowbladeBurstSpot(V3(1.f, 2.f, 16.f), 4)).Z == 16.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the cloud takes the threat off the Shadowblade, not the
// tank, and the party in it takes less
internal void
TestSmokeBomb()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    player_slot *Slot = ReadyShadowblade(&Crypt);
    player_slot *Tank = &AppState->Players[1];
    SetPlayerRole(AppState, Tank, PlayerRole_Tank);
    MovePlayerTo(AppState, World, &Crypt.Arena, Tank->Entity, Slot->Entity->Position + V3(0.f, 50.f, 0.f));
    world_entity *Foe = StrikerDummy(&Crypt, V3(60.f, 0.f, 0.f));
    AddThreat(&Run->Threat, World, Foe, 0, 500.f);
    AddThreat(&Run->Threat, World, Foe, 1, 100.f);
    float Plain = DungeonScaleDamage(AppState, Slot->Entity, Foe, 10.f);
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    Check(Slot->RoleCooldowns[2] == 0.f);
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_SmokeBomb] = 1;
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    Check(Slot->RoleCooldowns[2] > 0.f);
    TickCrypt(&Crypt, 1);
    threat_row *Row = FindThreatRow(&Run->Threat, World, Foe, false);
    Check(Row && Row->Threat[0] == 0.f && Row->Threat[1] >= 100.f);
    Check(PickThreatTarget(AppState, &Run->Threat, Foe) == Tank->Entity);
    float Smoked = DungeonScaleDamage(AppState, Slot->Entity, Foe, 10.f);
    Check(Smoked > 0.99f * (1.f - SMOKE_SHARE) * Plain && Smoked < 1.01f * (1.f - SMOKE_SHARE) * Plain);
    // NOTE(zoubir): the second rank thickens it and slows foes in it
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_SmokeBomb] = 2;
    Slot->RoleCooldowns[2] = 0.f;
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    TickCrypt(&Crypt, 1);
    Check(HasStatus(Foe, StatusEffect_Slowed));
    float Thick = DungeonScaleDamage(AppState, Slot->Entity, Foe, 10.f);
    Check(Thick < 0.99f * Smoked);
    // NOTE(zoubir): it clears
    TickCrypt(&Crypt, (u32)(60.f * (SMOKE_SECONDS + 0.5f)));
    float After = DungeonScaleDamage(AppState, Slot->Entity, Foe, 10.f);
    Check(After > 0.99f * Plain);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Shadow Dance: half again on every strike, Shadowstep
// back at once, and the flag clients read
internal void
TestShadowDance()
{
    crypt_world Crypt = CreateCryptWorld(1);
    player_slot *Slot = ReadyShadowblade(&Crypt);
    world_entity *Foe = StrikerDummy(&Crypt, V3(45.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    float Plain = 2000.f - Foe->Hp;
    TickHolding(&Crypt, Foe, 30);
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_ShadowDance] = 1;
    PressOnce(&Crypt, 0, PlayerButton_Kunai);
    Check(Slot->RoleCooldowns[3] > 0.f);
    Check(Slot->ClassFlags & SHADOWBLADE_FLAG_DANCE);
    float Before = Foe->Hp;
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    float Danced = Before - Foe->Hp;
    Check(Danced > 0.99f * (1.f + DANCE_ECHO) * Plain && Danced < 1.01f * (1.f + DANCE_ECHO) * Plain);
    TickHolding(&Crypt, Foe, 10);
    PressAt(&Crypt, 0, PlayerButton_Launch, Foe);
    Check(Slot->RoleCooldowns[0] > 0.f && Slot->RoleCooldowns[0] <= DANCE_STEP_COOLDOWN);
    TickHolding(&Crypt, Foe, (u32)(60.f * DANCE_SECONDS));
    Check(!(Slot->ClassFlags & SHADOWBLADE_FLAG_DANCE));
    DestroyCryptWorld(&Crypt);
}

internal void
TestShadowbladeTalents()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = ReadyShadowblade(&Crypt);
    world_entity *Blade = Slot->Entity;
    world_entity *Foe = StrikerDummy(&Crypt, V3(50.f, 0.f, 0.f));
    // NOTE(zoubir): Lethality, by rank
    float Plain = ShadowbladeDealtScale(Slot, Foe);
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_Lethality] = 2;
    float Lethal = ShadowbladeDealtScale(Slot, Foe);
    Check(Lethal > Plain * (1.f + 2.f * LETHALITY_SHARE) - 0.001f &&
          Lethal < Plain * (1.f + 2.f * LETHALITY_SHARE) + 0.001f);
    // NOTE(zoubir): Opportunist, only from behind
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_Opportunist] = 1;
    Foe->Direction = V2(-1.f, 0.f);
    Check(ShadowbladeDealtScale(Slot, Foe) == Lethal);
    Foe->Direction = V2(1.f, 0.f);
    float Behind = ShadowbladeDealtScale(Slot, Foe);
    Check(Behind > Lethal * (1.f + OPPORTUNIST_SHARE) - 0.001f &&
          Behind < Lethal * (1.f + OPPORTUNIST_SHARE) + 0.001f);
    // NOTE(zoubir): Venom: a stronger, longer poison
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_Venom] = 1;
    Check(CastShadowbladeKey(AppState, &AppState->World, &Crypt.Arena, Slot, Blade, 5));
    shadowblade_poison *Poison = &AppState->Dungeon->Shadowblade.Poisons[0];
    Check(Poison->Seconds == SHIV_POISON_SECONDS + VENOM_SECONDS);
    Check(Poison->PerSecond == SHIV_POISON_PER_SECOND * (1.f + VENOM_SHARE));
    // NOTE(zoubir): Relentless: a killing Eviscerate gives points back and
    // Shadowstep with them
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_Relentless] = 1;
    Foe->Hp = 5.f;
    Slot->ClassMeter = 5;
    Slot->RoleCooldowns[0] = 5.f;
    Slot->Input.Target = (u32)(Foe - AppState->World.Entities) + 1;
    FinishShadowbladeCast(AppState, Slot, Blade, PlayerSpell_ShadowbladeB);
    Check(Foe->Hp <= 0.f || !Foe->IsPresent);
    Check(Slot->ClassMeter == RELENTLESS_POINTS);
    Check(Slot->RoleCooldowns[0] == 0.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): out of a fight the points last a moment, then go one by
// one, and the flag says so
internal void
TestComboPointsFade()
{
    crypt_world Crypt = CreateCryptWorld(1);
    player_slot *Slot = ReadyShadowblade(&Crypt);
    Slot->ClassMeter = 4;
    Slot->Shadowblade.IdleSeconds = 0.f;
    TickCrypt(&Crypt, (u32)(60.f * (SHADOWBLADE_IDLE_SECONDS - 1.f)));
    Check(Slot->ClassMeter == 4);
    TickCrypt(&Crypt, 90);
    Check(Slot->ClassFlags & SHADOWBLADE_FLAG_FADING);
    TickCrypt(&Crypt, (u32)(60.f * 5.f * SHADOWBLADE_FADE_SECONDS));
    Check(Slot->ClassMeter == 0);
    Check(!(Slot->ClassFlags & SHADOWBLADE_FLAG_FADING));
    DestroyCryptWorld(&Crypt);
}

internal void
RunShadowbladeTests()
{
    TestShadowbladeOwnsItsKeys();
    TestTwinStrike();
    TestPoisonedShiv();
    TestShadowstep();
    TestFanOfKnives();
    TestEviscerate();
    TestSmokeBomb();
    TestShadowDance();
    TestShadowbladeTalents();
    TestComboPointsFade();
}
