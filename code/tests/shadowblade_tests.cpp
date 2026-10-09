/* Shadowblade tests (sim/dungeon/role_kits/shadowblade.cpp), included by
   dungeon_tests.cpp: the class owns its six keys, V once learned, and C
   does nothing; Twin Strike cuts twice in front, builds a point,
   takes a press a moment early and poisons for the Shadowblade;
   Shadowstep lands behind the foe and makes the next strike critical;
   Fan of Knives winds up, cuts every foe near and builds a point each;
   Eviscerate spends the points for damage by the point, refuses to
   cast with none or with no foe in reach, and strikes the foe it was
   pressed on; Deadly Throw spends them from afar, poisons and slows; Shadow Dance echoes strikes and hurries Shadowstep;
   the talents, Knife Storm's wider, harder fan and Kidney Shot's stun
   among them; and points fading out of a fight. */

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
    u32 Main = PlayerButton_Launch | PlayerButton_Push | PlayerButton_Shockwave | PlayerButton_Attack |
        PlayerButton_Cast;
    Check((Allowed & Main) == Main);
    Check(!(Allowed & (PlayerButton_Slam | PlayerButton_Kunai)));
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_Envenom] = 2;
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_ShadowDance] = 1;
    Allowed = RunAllowedButtons(AppState, Slot, 0);
    Check(Allowed & PlayerButton_Kunai);
    Check(!(Allowed & PlayerButton_Slam));
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

// NOTE(zoubir): a Twin Strike clicked a moment before it is ready still
// cuts, the moment it was early added to the next cooldown; one clicked
// well before is dropped (ROLE_EARLY_PRESS_SECONDS, role_abilities.cpp)
internal void
TestTwinStrikeEarlyPress()
{
    crypt_world Crypt = CreateCryptWorld(1);
    player_slot *Slot = ReadyShadowblade(&Crypt);
    world_entity *Front = StrikerDummy(&Crypt, V3(45.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Slot->ClassMeter == 1);
    // NOTE(zoubir): far too early: nothing
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Slot->ClassMeter == 1);
    while(Slot->RoleCooldowns[6] > 0.6f * ROLE_EARLY_PRESS_SECONDS)
    {
        TickHolding(&Crypt, Front, 1);
    }
    float Left = Slot->RoleCooldowns[6];
    Check(Left > 0.f);
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Slot->ClassMeter == 2);
    Check(Slot->RoleCooldowns[6] > TWIN_STRIKE_COOLDOWN);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Twin Strike poisons what it cuts, as the Shadowblade's
// poison, which bites on after the cuts and runs out
internal void
TestTwinStrikePoisons()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    ReadyShadowblade(&Crypt);
    world_entity *Foe = StrikerDummy(&Crypt, V3(45.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(HasStatus(Foe, StatusEffect_Poisoned));
    TickHolding(&Crypt, Foe, 10);
    float Cuts = 2000.f - Foe->Hp;
    TickHolding(&Crypt, Foe, 120);
    Check(2000.f - Foe->Hp > Cuts + 4.f);
    shadowblade_poison *Poison = &AppState->Dungeon->Shadowblade.Poisons[0];
    Check(Poison->Seconds > 0.f && Poison->PerSecond == BLADE_POISON_PER_SECOND && Poison->By == 0);
    TickHolding(&Crypt, Foe, (u32)(60.f * BLADE_POISON_SECONDS));
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
    Check(Slot->RoleCooldowns[4] > EVISCERATE_COOLDOWN - 1.f);
    u32 WindUp = (u32)(60.f * PlayerSpells[PlayerSpell_ShadowbladeB].CastTime) + 2;
    // NOTE(zoubir): points but nobody in reach: no wind-up, no cooldown,
    // the points kept
    Slot->ClassMeter = 3;
    Slot->RoleCooldowns[4] = 0.f;
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(Slot->Entity->CastSpell == 0 && Slot->RoleCooldowns[4] == 0.f && Slot->ClassMeter == 3);
    // NOTE(zoubir): the foe pressed on is held: it backs off past the
    // reach during the wind-up and is still struck
    world_entity *Foe = StrikerDummy(&Crypt, V3(50.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(Slot->Entity->CastSpell == PlayerSpell_ShadowbladeB);
    Foe->Position.X = Slot->Entity->Position.X + EVISCERATE_REACH + 40.f;
    TickHolding(&Crypt, Foe, WindUp);
    Check(Foe->Hp < 2000.f && Slot->ClassMeter == 0);
    // NOTE(zoubir): it gets clean away during the wind-up: the points
    // stay, the key is ready again, and the burst says nothing was spent
    Slot->ClassMeter = 3;
    Slot->RoleCooldowns[4] = 0.f;
    Foe->Position.X = Slot->Entity->Position.X + 50.f;
    TickHolding(&Crypt, Foe, 1);
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(Slot->Entity->CastSpell == PlayerSpell_ShadowbladeB);
    Foe->Position.X = Slot->Entity->Position.X + 600.f;
    TickHolding(&Crypt, Foe, WindUp);
    Check(Slot->ClassMeter == 3 && Slot->RoleCooldowns[4] == 0.f);
    Check(ShadowbladeBurstVariant(ShadowbladeBurstSpot(V3(1.f, 2.f, 16.f), 4)) == 4);
    Check(ShadowbladeBurstPlace(ShadowbladeBurstSpot(V3(1.f, 2.f, 16.f), 4)).Z == 16.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): what Deadly Throw with Points did, at once, to a fresh foe
// Distance away
internal float
DeadlyThrowWith(crypt_world *Crypt, player_slot *Slot, u32 Points, float Distance)
{
    world_entity *Foe = StrikerDummy(Crypt, V3(Distance, 0.f, 0.f));
    Slot->ClassMeter = (u8)Points;
    Slot->RoleCooldowns[5] = 0.f;
    PressAt(Crypt, 0, PlayerButton_Cast, Foe);
    float Result = 2000.f - Foe->Hp;
    if (Result > 0.f)
    {
        Check(Foe->StatusTimers[StatusEffect_Poisoned] > 0.f);
        Check(Foe->StatusTimers[StatusEffect_Slowed] > 0.f);
    }
    KillEntity(Crypt->AppState, &Crypt->AppState->World, Foe, 0);
    return Result;
}

// NOTE(zoubir): Deadly Throw: no wind-up, from far off, every point
// spent, harder by the point; with no points or nobody in range it does
// not cast and keeps its cooldown
internal void
TestDeadlyThrow()
{
    crypt_world Crypt = CreateCryptWorld(1);
    player_slot *Slot = ReadyShadowblade(&Crypt);
    Check(DeadlyThrowWith(&Crypt, Slot, 0, 250.f) == 0.f);
    Check(Slot->RoleCooldowns[5] == 0.f);
    float One = DeadlyThrowWith(&Crypt, Slot, 1, 250.f);
    Check(One > 0.f && Slot->ClassMeter == 0 && Slot->Entity->CastSpell == 0);
    Check(Slot->RoleCooldowns[5] > DEADLY_THROW_COOLDOWN - 1.f);
    float Five = DeadlyThrowWith(&Crypt, Slot, 5, 250.f);
    Check(Five > 2.5f * One && Slot->ClassMeter == 0);
    Check(DeadlyThrowWith(&Crypt, Slot, 3, DEADLY_THROW_RANGE + 120.f) == 0.f);
    Check(Slot->ClassMeter == 3 && Slot->RoleCooldowns[5] == 0.f);
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
    // NOTE(zoubir): Venom: a stronger, longer poison; Envenom stronger
    // again by rank
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_Venom] = 1;
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_Envenom] = 2;
    Check(CastShadowbladeKey(AppState, &AppState->World, &Crypt.Arena, Slot, Blade, 6));
    shadowblade_poison *Poison = &AppState->Dungeon->Shadowblade.Poisons[0];
    Check(Poison->Seconds == BLADE_POISON_SECONDS + VENOM_SECONDS);
    float Bite = BLADE_POISON_PER_SECOND * (1.f + VENOM_SHARE) * (1.f + 2.f * ENVENOM_SHARE);
    Check(Poison->PerSecond > Bite - 0.001f && Poison->PerSecond < Bite + 0.001f);
    // NOTE(zoubir): Envenom: Fan of Knives poisons too
    world_entity *Side = StrikerDummy(&Crypt, V3(-60.f, 0.f, 0.f));
    FinishShadowbladeCast(AppState, Slot, Blade, PlayerSpell_ShadowbladeA);
    Check(HasStatus(Side, StatusEffect_Poisoned));
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

// NOTE(zoubir): Shadowstep wipes the Shadowblade's threat and guards it
// for SHADOWSTEP_GUARD_SECONDS
internal void
TestShadowstepGuard()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = ReadyShadowblade(&Crypt);
    world_entity *Blade = Slot->Entity;
    world_entity *Foe = StrikerDummy(&Crypt, V3(220.f, 0.f, 0.f));
    AddThreat(&AppState->Dungeon->Threat, &AppState->World, Foe, Blade->PlayerIndex, 500.f);
    float Plain = DungeonScaleDamage(AppState, Blade, Foe, 100.f);
    PressAt(&Crypt, 0, PlayerButton_Launch, Foe);
    threat_row *Row = FindThreatRow(&AppState->Dungeon->Threat, &AppState->World, Foe, false);
    Check(!Row || Row->Threat[Blade->PlayerIndex] == 0.f);
    float Guarded = DungeonScaleDamage(AppState, Blade, Foe, 100.f);
    Check(Guarded < (SHADOWSTEP_GUARD_SCALE + 0.01f) * Plain);
    TickCrypt(&Crypt, (u32)(60.f * SHADOWSTEP_GUARD_SECONDS) + 2);
    Check(DungeonScaleDamage(AppState, Blade, Foe, 100.f) > 0.9f * Plain);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Knife Storm at full rank: the fan reaches a foe past its
// plain radius and cuts KNIFE_STORM_DAMAGE_SHARE harder a rank
internal void
TestKnifeStorm()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = ReadyShadowblade(&Crypt);
    world_entity *Blade = Slot->Entity;
    world_entity *Near = StrikerDummy(&Crypt, V3(-60.f, 0.f, 0.f));
    world_entity *Edge = StrikerDummy(&Crypt, V3(170.f, 0.f, 0.f));
    Edge->Position = Blade->Position;
    Edge->Position.X += FAN_OF_KNIVES_RADIUS + 0.5f * Edge->Dimensions.X + 25.f;
    Check(RoomAtPosition(&AppState->World, Edge->Position.XY) ==
          RoomAtPosition(&AppState->World, Blade->Position.XY));
    FinishShadowbladeCast(AppState, Slot, Blade, PlayerSpell_ShadowbladeA);
    float Plain = 2000.f - Near->Hp;
    Check(Plain > 0.f && Edge->Hp == 2000.f);
    Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_KnifeStorm] = 4;
    float Wide = FAN_OF_KNIVES_RADIUS * (1.f + 4.f * KNIFE_STORM_RADIUS_SHARE);
    Check(RoleSpellRadius(Slot, 1) > Wide - 0.01f && RoleSpellRadius(Slot, 1) < Wide + 0.01f);
    float Before = Near->Hp;
    FinishShadowbladeCast(AppState, Slot, Blade, PlayerSpell_ShadowbladeA);
    float Storm = Before - Near->Hp;
    float Scale = 1.f + 4.f * KNIFE_STORM_DAMAGE_SHARE;
    Check(Storm > 0.99f * Scale * Plain && Storm < 1.01f * Scale * Plain);
    Check(Edge->Hp < 2000.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Kidney Shot: a five-point Eviscerate stuns the foe it
// leaves alive; four points do not, nor five without the talent
internal void
TestKidneyShot()
{
    crypt_world Crypt = CreateCryptWorld(1);
    player_slot *Slot = ReadyShadowblade(&Crypt);
    u32 WindUp = (u32)(60.f * PlayerSpells[PlayerSpell_ShadowbladeB].CastTime) + 2;
    for(u32 Case = 0; Case < 3; Case++)
    {
        u32 Points = Case == 1 ? 4 : SHADOWBLADE_MOST_POINTS;
        Slot->Ranks[Talent_RoleFirst + ShadowbladeTalent_KidneyShot] = Case == 0 ? 0 : 1;
        world_entity *Foe = StrikerDummy(&Crypt, V3(50.f, 0.f, 0.f));
        Slot->ClassMeter = (u8)Points;
        Slot->RoleCooldowns[4] = 0.f;
        PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
        TickHolding(&Crypt, Foe, WindUp);
        Check(Foe->Hp < 2000.f && Slot->ClassMeter == 0);
        bool32 Stunned = HasStatus(Foe, StatusEffect_Stunned);
        Check(Case == 2 ? Stunned : !Stunned);
        if (Case == 2)
        {
            Check(Foe->StatusTimers[StatusEffect_Stunned] > KIDNEY_SHOT_SECONDS - 0.1f);
        }
        KillEntity(Crypt.AppState, &Crypt.AppState->World, Foe, 0);
    }
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
    TestTwinStrikeEarlyPress();
    TestTwinStrikePoisons();
    TestShadowstep();
    TestShadowstepGuard();
    TestFanOfKnives();
    TestEviscerate();
    TestDeadlyThrow();
    TestShadowDance();
    TestShadowbladeTalents();
    TestKnifeStorm();
    TestKidneyShot();
    TestComboPointsFade();
}
