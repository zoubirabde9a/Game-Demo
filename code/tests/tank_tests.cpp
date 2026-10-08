/* Tank tests (sim/dungeon/role_kits/tank.cpp), included by
   dungeon_tests.cpp after dungeon_role_tests.cpp, whose helpers they use:
   Shield Charge rushes a foe, stuns it and cancels the attack it is
   winding up, and keeps its cooldown when there is nobody to charge.
   Juggernaut shortens its cooldown and lengthens its stun; Unbroken
   catches the tank's killing blow once a fight. */

// NOTE(zoubir): a Brute winding up its Ground Slam (monsters/brute.cpp)
// gets charged: the tank lands beside it, the Brute is stunned, hurt and
// after the tank, and its slam never goes off
internal void
TestShieldChargeBreaksAWindup()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    TickCrypt(&Crypt, 1);
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Tank);
    world_entity *Tank = AppState->Players[0].Entity;
    world_entity *Striker = AppState->Players[1].Entity;
    Check(Tank->MaxHp == GetRoleDef(PlayerRole_Tank)->MaxHp);
    Check(RoleSpellOnButton(AppState, Tank, PlayerButton_Cast) != 0);

    // NOTE(zoubir): nobody in reach: the key does nothing and keeps its cooldown
    PressOnce(&Crypt, 0, PlayerButton_Cast);
    Check(AppState->Players[0].RoleCooldowns[5] == 0.f);

    world_entity *Brute = SpawnMonster(AppState, World, &Crypt.Arena,
                                       Tank->Position + V3(250.f, 0.f, 0.f), MonsterKind_Brute);
    Brute->MaxHp = Brute->Hp = 1000.f;
    DamageEntity(AppState, World, Brute, 1.f, Striker);
    Check(FindMonsterTarget(AppState, World, Brute, 0) == Striker);
    Brute->AbilityIndex = 0;
    SetMonsterPhase(Brute, AbilityPhase_Windup, 0.75f);
    float Hp = Brute->Hp;
    float Before = Length(Brute->Position.XY - Tank->Position.XY);
    PressAt(&Crypt, 0, PlayerButton_Cast, Brute);
    Check(AppState->Players[0].RoleCooldowns[5] > 0.f);
    Check(Length(Brute->Position.XY - Tank->Position.XY) < Before - 100.f);
    Check(Brute->AbilityPhase == AbilityPhase_Recover);
    Check(HasStatus(Brute, StatusEffect_Stunned));
    Check(Brute->Hp < Hp);
    Check(FindMonsterTarget(AppState, World, Brute, 0) == Tank);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): four ranks of Juggernaut: Shield Charge comes back 4 s
// sooner and its stun lasts a second longer
internal void
TestJuggernautSpeedsTheCharge()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Tank);
    world_entity *Tank = Slot->Entity;
    float Plain = RoleSpellCooldown(Slot, 5);
    Slot->Ranks[Talent_RoleFirst + TankTalent_Juggernaut] = 4;
    float Faster = RoleSpellCooldown(Slot, 5);
    Check(Faster > Plain - 4.f * JUGGERNAUT_COOLDOWN - 0.01f &&
          Faster < Plain - 4.f * JUGGERNAUT_COOLDOWN + 0.01f);
    // NOTE(zoubir): only Shield Charge's key
    Check(RoleSpellCooldown(Slot, 4) == RoleSpells[PlayerRole_Tank][4].Cooldown);

    world_entity *Brute = SpawnMonster(AppState, World, &Crypt.Arena,
                                       Tank->Position + V3(250.f, 0.f, 0.f), MonsterKind_Brute);
    Brute->MaxHp = Brute->Hp = 1000.f;
    PressAt(&Crypt, 0, PlayerButton_Cast, Brute);
    float Stun = SHIELD_CHARGE_STUN + 4.f * JUGGERNAUT_STUN;
    Check(Slot->RoleCooldowns[5] > Faster - 0.1f && Slot->RoleCooldowns[5] <= Faster);
    Check(Brute->StatusTimers[StatusEffect_Stunned] > Stun - 0.1f &&
          Brute->StatusTimers[StatusEffect_Stunned] <= Stun);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): with Unbroken, the first killing blow of a fight leaves the
// tank at a tenth of its health behind Shield Wall; the second downs it.
// Out of the fight it is ready again, and without the talent nothing
// holds the tank up
internal void
TestUnbrokenCatchesTheKillingBlow()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Tank);
    world_entity *Tank = Slot->Entity;
    world_entity *Brute = SpawnMonster(AppState, World, &Crypt.Arena,
                                       Tank->Position + V3(200.f, 0.f, 0.f), MonsterKind_Brute);
    Slot->Ranks[Talent_RoleFirst + TankTalent_Unbroken] = 1;

    // NOTE(zoubir): no fight on: the talent waits for one
    Tank->Hp = 20.f;
    Check(DungeonScaleDamage(AppState, Tank, Brute, 1000.f) >= 20.f);
    Check(!(Slot->ClassFlags & TANK_FLAG_UNBROKEN_SPENT));

    Run->FightingRoom = 2;
    Run->PartyDamage = 1.f;
    Tank->Hp = 0.5f * Tank->MaxHp;
    Slot->ShieldWallSeconds = 0.f;
    DamageEntity(AppState, World, Tank, 100000.f, Brute);
    Check(!IsDeadPlayer(Tank));
    Check(NearHp(Tank->Hp, UNBROKEN_HEALTH_SHARE * Tank->MaxHp));
    Check(Slot->ShieldWallSeconds == UNBROKEN_WALL_SECONDS);
    Check(Slot->ClassFlags & TANK_FLAG_UNBROKEN_SPENT);
    // NOTE(zoubir): a blow it can take goes through as before
    float Hp = Tank->Hp;
    DamageEntity(AppState, World, Tank, 1.f, Brute);
    Check(Tank->Hp < Hp && Tank->Hp > 0.f);
    // NOTE(zoubir): once a fight
    Check(DungeonScaleDamage(AppState, Tank, Brute, 100000.f) >= Tank->Hp);

    // NOTE(zoubir): the fight over, the next one has it again
    Run->FightingRoom = 0;
    TickCrypt(&Crypt, 1);
    Check(!(Slot->ClassFlags & TANK_FLAG_UNBROKEN_SPENT));
    Run->FightingRoom = 2;
    Tank->Hp = 0.5f * Tank->MaxHp;
    Check(DungeonScaleDamage(AppState, Tank, Brute, 100000.f) < Tank->Hp);

    // NOTE(zoubir): without the talent the blow downs the tank
    Slot->Ranks[Talent_RoleFirst + TankTalent_Unbroken] = 0;
    Slot->ClassFlags = 0;
    Check(DungeonScaleDamage(AppState, Tank, Brute, 100000.f) >= Tank->Hp);
    Run->FightingRoom = 0;
    Run->PartyDamage = 0.f;
    RemoveEntity(World, Brute);
    DestroyCryptWorld(&Crypt);
}

internal void
RunTankTests()
{
    TestShieldChargeBreaksAWindup();
    TestJuggernautSpeedsTheCharge();
    TestUnbrokenCatchesTheKillingBlow();
}
