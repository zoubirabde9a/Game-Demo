/* Stormcaller tests, the second half (tests/stormcaller_tests.cpp, which
   has the helpers): Capacitor and Stormbringer; the overload's nova and
   its cost, the cooldown floor while grounded, and Live Wire; Eye of the
   Storm holding Charge at the top and striking; Lightning Dash; Charge
   draining; and a client predicting its own Stormcaller. */

// NOTE(zoubir): Capacitor: a Thunderclap of 70+ gives CAPACITOR_CHARGE back
// and readies Chain Lightning; under 70 it gives nothing. Stormbringer:
// a Thunderclap of 70+ calls bolts on the three nearest other foes within
// STORMBRINGER_RADIUS, none on a fourth
internal void
TestCapacitorAndStormbringer()
{
    crypt_world Crypt = CreateStormWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    SetClassTalentRank(Slot, StormcallerTalent_Capacitor, 1);
    storm_dummies Dummies = {};
    world_entity *Foe = StormDummy(&Crypt, &Dummies, V3(300.f, 0.f, 0.f));
    SetStormCharge(Slot, 50.f);
    Slot->RoleCooldowns[0] = 5.f;
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
    StormTick(&Crypt, &Dummies, STORM_CLAP_TICKS);
    Check(Slot->Stormcaller.Charge == 0.f && Slot->RoleCooldowns[0] > 4.f);
    SetStormCharge(Slot, 75.f);
    Slot->RoleCooldowns[4] = 0.f;
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
    StormTick(&Crypt, &Dummies, STORM_CLAP_TICKS);
    Check(Slot->Stormcaller.Charge == CAPACITOR_CHARGE && Slot->RoleCooldowns[0] == 0.f);
    DestroyCryptWorld(&Crypt);

    Crypt = CreateStormWorld();
    AppState = Crypt.AppState;
    Slot = &AppState->Players[0];
    SetClassTalentRank(Slot, StormcallerTalent_Stormbringer, 1);
    Dummies = {};
    Foe = StormDummy(&Crypt, &Dummies, V3(150.f, 40.f, 0.f));
    world_entity *Others[4];
    Others[0] = StormDummy(&Crypt, &Dummies, V3(270.f, 40.f, 0.f));
    Others[1] = StormDummy(&Crypt, &Dummies, V3(150.f, 190.f, 0.f));
    Others[2] = StormDummy(&Crypt, &Dummies, V3(0.f, -60.f, 0.f));
    Others[3] = StormDummy(&Crypt, &Dummies, V3(330.f, 150.f, 0.f));
    SetStormCharge(Slot, 70.f);
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
    StormTick(&Crypt, &Dummies, STORM_CLAP_TICKS);
    float Clap = StormDealt(THUNDERCLAP_DAMAGE + 70.f * THUNDERCLAP_PER_CHARGE);
    for(u32 Index = 0; Index < 3; Index++)
    {
        Check(StormNear(2000.f - Others[Index]->Hp, STORMBRINGER_SHARE * Clap));
    }
    Check(Others[3]->Hp == 2000.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Charge reaching the top overloads: the nova hurts the foes
// near, Charge is gone, a tenth of the health is lost (never below 1)
// and every key waits at least OVERLOAD_GROUNDED while grounded. Live
// Wire: no cost, no wait, a wider and harder nova
internal void
TestOverload()
{
    for(u32 LiveWire = 0; LiveWire < 2; LiveWire++)
    {
        crypt_world Crypt = CreateStormWorld();
        app_state *AppState = Crypt.AppState;
        player_slot *Slot = &AppState->Players[0];
        world_entity *Player = Slot->Entity;
        SetClassTalentRank(Slot, StormcallerTalent_LiveWire, (u8)LiveWire);
        storm_dummies Dummies = {};
        world_entity *Target = StormDummy(&Crypt, &Dummies, V3(330.f, 150.f, 0.f));
        world_entity *Close = StormDummy(&Crypt, &Dummies, V3(-60.f, 40.f, 0.f));
        world_entity *Wide = StormDummy(&Crypt, &Dummies, V3(0.f, 170.f, 0.f));
        StormTick(&Crypt, &Dummies, 1);
        SetStormCharge(Slot, STORM_CHARGE_MOST - 3.f);
        Slot->RoleCooldowns[4] = 5.f;
        Player->Hp = Player->MaxHp;
        PressAt(&Crypt, 0, PlayerButton_Cast, Target);
        Check(Target->Hp < 2000.f);
        Check(Slot->Stormcaller.Charge == 0.f);
        float Scale = LiveWire ? LIVE_WIRE_SCALE : 1.f;
        Check(StormNear(2000.f - Close->Hp, StormDealt(OVERLOAD_DAMAGE * Scale)));
        Check((Wide->Hp < 2000.f) == (LiveWire != 0));
        if (LiveWire)
        {
            Check(Player->Hp == Player->MaxHp);
            Check(Slot->RoleCooldowns[1] == 0.f && Slot->RoleCooldowns[5] < OVERLOAD_GROUNDED - 0.5f);
            Check(!(Slot->ClassFlags & STORMCALLER_FLAG_GROUNDED));
        }
        else
        {
            Check(StormNear(Player->MaxHp - Player->Hp, OVERLOAD_HEALTH_SHARE * Player->MaxHp));
            Check(Slot->ClassFlags & STORMCALLER_FLAG_GROUNDED);
            // NOTE(zoubir): the floor holds every key, the Spark's own too,
            // and leaves a longer cooldown alone
            TickCrypt(&Crypt, 1);
            for(u32 Key = 0; Key < 4; Key++)
            {
                Check(Slot->RoleCooldowns[Key] > OVERLOAD_GROUNDED - 0.1f);
            }
            Check(Slot->RoleCooldowns[5] > OVERLOAD_GROUNDED - 0.1f);
            Check(Slot->RoleCooldowns[4] > 4.9f);
            Check(StormcallerBurstVariant(StormcallerBurstSpot(V3(0.f, 0.f, 0.f),
                                                               (u32)OVERLOAD_RADIUS)) == (u32)OVERLOAD_RADIUS);
            // NOTE(zoubir): an overload never downs its caster
            StormTick(&Crypt, &Dummies, (u32)(60.f * OVERLOAD_GROUNDED) + 2);
            Check(!(Slot->ClassFlags & STORMCALLER_FLAG_GROUNDED));
            Player->Hp = 5.f;
            SetStormCharge(Slot, STORM_CHARGE_MOST - 1.f);
            Slot->RoleCooldowns[5] = 0.f;
            Slot->Input.Pressed = PlayerButton_Cast;
            Slot->Input.Target = (u32)(Target - AppState->World.Entities) + 1;
            TickCrypt(&Crypt, 1);
            Slot->Input.Pressed = 0;
            Check(Slot->Stormcaller.Charge == 0.f);
            Check(Player->Hp >= 1.f && Player->Hp < 5.f && !IsDeadPlayer(Player));
        }
        DestroyCryptWorld(&Crypt);
    }
}

// NOTE(zoubir): Eye of the Storm: Charge holds at the top without
// overloading and counts as Supercharged, and bolts strike the foes
internal void
TestEyeOfTheStorm()
{
    crypt_world Crypt = CreateStormWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    SetClassTalentRank(Slot, StormcallerTalent_EyeOfTheStorm, 1);
    storm_dummies Dummies = {};
    world_entity *Foe = StormDummy(&Crypt, &Dummies, V3(300.f, 0.f, 0.f));
    SetStormCharge(Slot, 20.f);
    PressOnce(&Crypt, 0, PlayerButton_Kunai);
    Check(Slot->RoleCooldowns[3] > EYE_COOLDOWN - 1.f);
    StormTick(&Crypt, &Dummies, 1);
    Check(Slot->ClassFlags & STORMCALLER_FLAG_EYE);
    Check(Slot->ClassFlags & STORMCALLER_FLAG_SUPERCHARGED);
    SetStormCharge(Slot, STORM_CHARGE_MOST - 2.f);
    float Hp = Slot->Entity->Hp;
    PressAt(&Crypt, 0, PlayerButton_Cast, Foe);
    Check(Slot->Stormcaller.Charge == STORM_CHARGE_MOST);
    Check(!(Slot->ClassFlags & STORMCALLER_FLAG_GROUNDED) && Slot->Entity->Hp >= Hp);
    // NOTE(zoubir): the Charge does not drain under the Eye either
    Slot->Stormcaller.IdleSeconds = 2.f * STORM_IDLE_SECONDS;
    float Before = Foe->Hp;
    StormTick(&Crypt, &Dummies, (u32)(60.f * EYE_SECONDS));
    Check(Slot->ClassMeter == (u8)STORM_CHARGE_MOST);
    float Bolts = (Before - Foe->Hp) / StormDealt(EYE_BOLT_DAMAGE);
    Check(Bolts > EYE_SECONDS / EYE_BOLT_SECONDS - 2.5f && Bolts < EYE_SECONDS / EYE_BOLT_SECONDS + 0.5f);
    StormTick(&Crypt, &Dummies, 30);
    Check(!(Slot->ClassFlags & STORMCALLER_FLAG_EYE));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): C dashes along the aim, shocking and slowing the foe it
// passes and building Charge; a foe off its line is left alone; rank 2
// brings it back sooner; a wall stops it short
internal void
TestLightningDash()
{
    crypt_world Crypt = CreateStormWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Player = Slot->Entity;
    SetClassTalentRank(Slot, StormcallerTalent_LightningDash, 1);
    Check(RoleSpellCooldown(Slot, 2) == LIGHTNING_DASH_COOLDOWN);
    storm_dummies Dummies = {};
    world_entity *Crossed = StormDummy(&Crypt, &Dummies, V3(140.f, 10.f, 0.f));
    world_entity *Aside = StormDummy(&Crypt, &Dummies, V3(140.f, 160.f, 0.f));
    v3 Start = Player->Position;
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    Check(Slot->RoleCooldowns[2] > 0.f);
    Check(Player->Position.X > Start.X + LIGHTNING_DASH_LENGTH - 40.f);
    Check(Player->Position.X < Start.X + LIGHTNING_DASH_LENGTH + 10.f);
    Check(StormNear(2000.f - Crossed->Hp, StormDealt(LIGHTNING_DASH_DAMAGE)));
    Check(HasStatus(Crossed, StatusEffect_Slowed));
    Check(Aside->Hp == 2000.f);
    Check(Slot->Stormcaller.Charge == LIGHTNING_DASH_CHARGE);
    SetClassTalentRank(Slot, StormcallerTalent_LightningDash, 2);
    Check(RoleSpellCooldown(Slot, 2) == LIGHTNING_DASH_RANK2_COOLDOWN);

    // NOTE(zoubir): back toward the wall the run starts by, and on past
    // it: the dash stops short every time and never leaves the map
    Player->Aim = V2(-1.f, 0.f);
    for(u32 Dash = 0; Dash < 4; Dash++)
    {
        Slot->RoleCooldowns[2] = 0.f;
        PressOnce(&Crypt, 0, PlayerButton_Slam);
        TickCrypt(&Crypt, 2);
    }
    Check(Player->Position.X > 0.f);
    Check(Player->Position.X > Start.X - 4.f * LIGHTNING_DASH_LENGTH + 100.f);
    Check(!IsBlinkSpotBlocked(AppState, &AppState->World, Player->Position.XY, Player->Position.Z));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Charge holds STORM_IDLE_SECONDS after the last hit, then
// drains; a player who leaves the class loses it
internal void
TestChargeDrains()
{
    crypt_world Crypt = CreateStormWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    SetStormCharge(Slot, 60.f);
    TickCrypt(&Crypt, (u32)(60.f * STORM_IDLE_SECONDS) - 6);
    Check(Slot->Stormcaller.Charge == 60.f && Slot->ClassMeter == 60);
    TickCrypt(&Crypt, 66);
    Check(Slot->Stormcaller.Charge < 60.f - 0.9f * STORM_CHARGE_DRAIN);
    Check(Slot->ClassMeter == (u8)(Slot->Stormcaller.Charge + 0.5f));
    SetPlayerRole(AppState, Slot, PlayerRole_Tank);
    GrantClassSpells(Slot);
    TickCrypt(&Crypt, 1);
    Check(Slot->Stormcaller.Charge == 0.f && Slot->ClassMeter == 0);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): online, the client predicting its own Stormcaller starts
// the wind-ups from the Charge the server sent, deals nothing, leaves
// the Charge alone, and casts none of the rest
internal void
TestPredictedStormcaller()
{
    crypt_world Crypt = CreateStormWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Player = Slot->Entity;
    storm_dummies Dummies = {};
    world_entity *Foe = StormDummy(&Crypt, &Dummies, V3(200.f, 0.f, 0.f));
    Slot->Predicted = true;
    Slot->ClassMeter = 10;
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
    Check(Player->CastSpell == 0);
    Slot->ClassMeter = 50;
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
    Check(Player->CastSpell == PlayerSpell_StormcallerB);
    Slot->ClassMeter = 50;
    StormTick(&Crypt, &Dummies, STORM_CLAP_TICKS);
    PressAt(&Crypt, 0, PlayerButton_Launch, Foe);
    Check(Player->CastSpell == PlayerSpell_StormcallerA);
    StormTick(&Crypt, &Dummies, STORM_CHAIN_TICKS);
    PressAt(&Crypt, 0, PlayerButton_Cast, Foe);
    PressOnce(&Crypt, 0, PlayerButton_Push);
    Check(Foe->Hp == 2000.f);
    Check(Slot->Stormcaller.Charge == 0.f);
    Slot->Predicted = false;
    DestroyCryptWorld(&Crypt);
}
