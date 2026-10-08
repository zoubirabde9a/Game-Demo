/* Tank tests (sim/dungeon/role_kits/tank.cpp), included by
   dungeon_tests.cpp after dungeon_role_tests.cpp, whose helpers they use:
   Shield Charge rushes a foe, stuns it and cancels the attack it is
   winding up, and keeps its cooldown when there is nobody to charge. */

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

internal void
RunTankTests()
{
    TestShieldChargeBreaksAWindup();
}
