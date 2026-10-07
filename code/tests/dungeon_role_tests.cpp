/* Dungeon role tests (sim/dungeon/role_abilities.cpp, role_talents.cpp):
   each role's keys, who an ally spell lands on, Shield Slam, Inferno,
   the role branch of the talent tree, and how a bigger party makes the
   fights harder and the roles keep up. Included by dungeon_tests.cpp,
   whose crypt world helpers they use. */

// NOTE(zoubir): A within a tick's rest between fights of B
// (REST_SHARE_PER_SECOND, role_abilities.cpp)
inline bool32
NearHp(float A, float B)
{
    bool32 Result = A > B - 0.5f && A < B + 0.5f;
    return Result;
}

// NOTE(zoubir): Button pressed for one tick by SlotIndex with Target
// (player_input.Target: entity index + 1, 0 for none) picked
internal void
PressAt(crypt_world *Crypt, u32 SlotIndex, u32 Button, world_entity *Target)
{
    player_slot *Slot = &Crypt->AppState->Players[SlotIndex];
    Slot->Input.Target = Target ? (u32)(Target - Crypt->AppState->World.Entities) + 1 : 0;
    PressOnce(Crypt, SlotIndex, Button);
    Slot->Input.Target = 0;
}

// NOTE(zoubir): the tank's taunt and shield slam; the healer's bolt,
// ward and sanctuary; the damage role's inferno
internal void
TestRoleKeys()
{
    crypt_world Crypt = CreateCryptWorld(3);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Tank);
    SetPlayerRole(AppState, &AppState->Players[1], PlayerRole_Healer);
    world_entity *Tank = AppState->Players[0].Entity;
    world_entity *Healer = AppState->Players[1].Entity;
    world_entity *Striker = AppState->Players[2].Entity;

    world_entity *Monster = SpawnMonster(AppState, World, &Crypt.Arena,
                                         Tank->Position + V3(120.f, 0.f, 0.f),
                                         MonsterKind_Brute);
    DamageEntity(AppState, World, Monster, 5.f, Striker);
    Check(FindMonsterTarget(AppState, World, Monster, 0) == Striker);
    PressOnce(&Crypt, 0, PlayerButton_Launch);
    Check(FindMonsterTarget(AppState, World, Monster, 0) == Tank);
    Check(AppState->Players[0].RoleCooldowns[0] > 0.f);
    Check(Tank->CastSpell == 0);
    // NOTE(zoubir): the cooldown list (what the HUD and the snapshot read)
    // gives the taunt's, not Launch's
    for(u32 Index = 0; Index < PLAYER_COOLDOWN_COUNT; Index++)
    {
        if (PlayerCooldownButton(Index) == PlayerButton_Launch)
        {
            float Full = 0.f;
            float *Left = PlayerCooldown(AppState, Tank, Index, &Full);
            Check(Left == &AppState->Players[0].RoleCooldowns[0]);
            Check(Full == TAUNT_COOLDOWN);
        }
    }
    // NOTE(zoubir): the damage role owns A and E (Detonate), and keeps
    // the game's V
    Check(RoleSpellOnButton(AppState, Striker, PlayerButton_Launch) != 0);
    Check(RoleSpellOnButton(AppState, Striker, PlayerButton_Shield) != 0);
    Check(RoleSpellOnButton(AppState, Striker, PlayerButton_Kunai) == 0);

    PressOnce(&Crypt, 0, PlayerButton_Shield);
    Check(AppState->Players[0].ShieldWallSeconds > 0.f);
    float Before = Tank->Hp;
    DamageEntity(AppState, World, Tank, 10.f, Monster);
    Check(Before - Tank->Hp > 2.7f && Before - Tank->Hp < 2.9f);
    // NOTE(zoubir): the slam rallied whoever stood near; not wanted below
    AppState->Players[1].RallySeconds = 0.f;
    AppState->Players[2].RallySeconds = 0.f;

    Striker->Hp = Striker->MaxHp - 50.f;
    PressOnce(&Crypt, 1, PlayerButton_Kunai);
    Check(NearHp(Striker->Hp, Striker->MaxHp - 50.f + MENDING_BOLT_HEAL));
    PressOnce(&Crypt, 1, PlayerButton_Shield);
    Check(AppState->Players[2].WardAbsorb == WARD_ABSORB);
    Before = Striker->Hp;
    DamageEntity(AppState, World, Striker, 20.f, Monster);
    Check(NearHp(Striker->Hp, Before));
    Check(AppState->Players[2].WardAbsorb == WARD_ABSORB - 20.f);
    RemoveEntity(World, Monster);

    // NOTE(zoubir): a sanctuary at the healer's own feet
    Healer->Hp = Healer->MaxHp - 30.f;
    Healer->AimReach = 0.f;
    PressOnce(&Crypt, 1, PlayerButton_Launch);
    TickCrypt(&Crypt, 120);
    Check(Healer->Hp > Healer->MaxHp - 15.f);
    Check(Run->Sanctuaries[0].Seconds > 0.f);
    Check(Run->Sanctuaries[0].Radius == SANCTUARY_RADIUS);

    PressOnce(&Crypt, 2, PlayerButton_Launch);
    Check(Striker->CastSpell == 0);
    Check(AppState->Players[2].RoleCooldowns[0] > 0.f);
    Check(Run->Infernos[0].Delay > 0.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Mending Bolt and Ward land on the player the client
// picked, the healer included; with nobody picked, on the most hurt
internal void
TestAllySpellsGoWherePicked()
{
    crypt_world Crypt = CreateCryptWorld(3);
    app_state *AppState = Crypt.AppState;
    TickCrypt(&Crypt, 1);
    SetPlayerRole(AppState, &AppState->Players[1], PlayerRole_Healer);
    world_entity *A = AppState->Players[0].Entity;
    world_entity *Healer = AppState->Players[1].Entity;
    world_entity *C = AppState->Players[2].Entity;
    A->Hp = A->MaxHp - 60.f;
    C->Hp = C->MaxHp - 40.f;
    Healer->Hp = Healer->MaxHp - 20.f;

    // NOTE(zoubir): C picked, though A is hurt worse
    float *BoltWait = &AppState->Players[1].RoleCooldowns[2];
    PressAt(&Crypt, 1, PlayerButton_Kunai, C);
    Check(NearHp(C->Hp, C->MaxHp - 40.f + MENDING_BOLT_HEAL));
    Check(NearHp(A->Hp, A->MaxHp - 60.f));
    *BoltWait = 0.f;

    // NOTE(zoubir): the healer's own frame picked
    PressAt(&Crypt, 1, PlayerButton_Kunai, Healer);
    Check(Healer->Hp == Healer->MaxHp);
    *BoltWait = 0.f;

    // NOTE(zoubir): nobody picked: the most hurt
    float HurtBefore = A->Hp;
    PressAt(&Crypt, 1, PlayerButton_Kunai, 0);
    Check(NearHp(A->Hp, HurtBefore + MENDING_BOLT_HEAL));

    PressAt(&Crypt, 1, PlayerButton_Shield, C);
    Check(AppState->Players[2].WardAbsorb == WARD_ABSORB);
    // NOTE(zoubir): A shares the ward only if it stands near C
    bool32 ANear = Length(A->Position.XY - C->Position.XY) <= WARD_SPLASH_RADIUS;
    Check(AppState->Players[0].WardAbsorb == (ANear ? WARD_SPLASH_SHARE * WARD_ABSORB : 0.f));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Shield Slam stuns and shoves the monsters near the tank,
// pulls them onto it, and rallies the allies near it
internal void
TestShieldSlamStunsAndRallies()
{
    crypt_world Crypt = CreateCryptWorld(3);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    TickCrypt(&Crypt, 1);
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Tank);
    world_entity *Tank = AppState->Players[0].Entity;
    world_entity *Near = AppState->Players[1].Entity;
    world_entity *Far = AppState->Players[2].Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Near, Tank->Position + V3(40.f, 0.f, 0.f));
    MovePlayerTo(AppState, World, &Crypt.Arena, Far, Tank->Position + V3(0.f, 300.f, 0.f));
    world_entity *Monster = SpawnMonster(AppState, World, &Crypt.Arena,
                                         Tank->Position + V3(-60.f, 0.f, 0.f),
                                         MonsterKind_Brute);
    float Hp = Monster->Hp;
    DamageEntity(AppState, World, Monster, 1.f, Far);
    PressOnce(&Crypt, 0, PlayerButton_Shield);
    Check(Monster->Hp < Hp - 1.f);
    Check(HasStatus(Monster, StatusEffect_Stunned));
    Check(FindMonsterTarget(AppState, World, Monster, 0) == Tank);
    Check(AppState->Players[1].RallySeconds > 0.f);
    // NOTE(zoubir): the far one only if the map put it within reach
    bool32 FarInReach = Length(Far->Position.XY - Tank->Position.XY) <= RALLY_RADIUS;
    Check((AppState->Players[2].RallySeconds > 0.f) == FarInReach);
    float Before = Near->Hp;
    DamageEntity(AppState, World, Near, 20.f, Monster);
    Check(NearHp(Before - Near->Hp, 15.f));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Inferno marks the ground, strikes when the meteor lands,
// then burns what stands there
internal void
TestInfernoFallsThenBurns()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    world_entity *Striker = AppState->Players[0].Entity;
    world_entity *Monster = SpawnMonster(AppState, World, &Crypt.Arena,
                                         Striker->Position + V3(150.f, 0.f, 0.f),
                                         MonsterKind_Brute);
    Monster->MaxHp = Monster->Hp = 1000.f;
    Striker->Aim = V2(1.f, 0.f);
    Striker->AimReach = 150.f / PLAYER_AIM_REACH;
    PressOnce(&Crypt, 0, PlayerButton_Launch);
    Check(Run->Infernos[0].Delay > 0.f);
    // NOTE(zoubir): the monster is held in place so it stays in the fire
    v3 Spot = Monster->Position;
    Check(Monster->Hp == 1000.f);
    for(u32 Tick = 0; Tick < (u32)(60.f * INFERNO_DELAY) + 2; Tick++)
    {
        Monster->Position = Spot;
        Monster->Velocity = {};
        TickCrypt(&Crypt, 1);
    }
    Check(Run->Infernos[0].Delay == 0.f);
    float AfterBlast = Monster->Hp;
    Check(AfterBlast <= 1000.f - INFERNO_DAMAGE);
    for(u32 Tick = 0; Tick < 120; Tick++)
    {
        Monster->Position = Spot;
        Monster->Velocity = {};
        TickCrypt(&Crypt, 1);
    }
    Check(Monster->Hp < AfterBlast - INFERNO_BURN_PER_SECOND);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the role branch takes points only in a run, its ranks go
// back when the role changes, and its talents change the numbers
internal void
TestRoleTalents()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Tank);
    Slot->Level = 6;
    Check(LearnTalent(AppState, 0, Talent_RoleFirst + TankTalent_IronSkin));
    Check(LearnTalent(AppState, 0, Talent_RoleFirst + TankTalent_IronSkin));
    Check(!LearnTalent(AppState, 0, Talent_RoleFirst + TankTalent_IronSkin));
    Check(ShownTalentDef(Slot, Talent_RoleFirst + TankTalent_IronSkin) ==
          &RoleTalentDefs[PlayerRole_Tank][TankTalent_IronSkin]);
    world_entity *Tank = Slot->Entity;
    float Before = Tank->Hp;
    DamageEntity(AppState, World, Tank, 100.f, 0);
    float Taken = Before - Tank->Hp;
    Check(Taken > 0.7f * 100.f * (1.f - 2.f * IRON_SKIN_SHARE) - 0.1f &&
          Taken < 0.7f * 100.f * (1.f - 2.f * IRON_SKIN_SHARE) + 0.1f);

    // NOTE(zoubir): another role: the branch's points come back
    u32 Left = TalentPointsLeft(Slot);
    SetPlayerRole(AppState, Slot, PlayerRole_Damage);
    Check(Slot->Ranks[Talent_RoleFirst + TankTalent_IronSkin] == 0);
    Check(TalentPointsLeft(Slot) == Left + 2);
    Check(LearnTalent(AppState, 0, Talent_RoleFirst + StrikerTalent_Kindling));
    Check(RoleSpellCooldown(Slot, 0) == INFERNO_COOLDOWN - KINDLING_COOLDOWN);

    // NOTE(zoubir): outside a run nobody can buy one
    dungeon_run *Run = AppState->Dungeon;
    AppState->Dungeon = 0;
    Check(!LearnTalent(AppState, 0, Talent_RoleFirst + StrikerTalent_Kindling));
    AppState->Dungeon = Run;
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): each player past the first makes the fight harder than
// the one before did; the tank takes only the root of the extra damage,
// heals grow by that root
internal void
TestPartyScalesExponentially()
{
    Check(PartyHealthScale(1) == 1.f && PartyDamageScale(1) == 1.f);
    for(u32 Players = 2; Players < MAX_PLAYERS; Players++)
    {
        float Step = PartyHealthScale(Players) - PartyHealthScale(Players - 1);
        float Next = PartyHealthScale(Players + 1) - PartyHealthScale(Players);
        Check(Next > Step);
        Check(PartyDamageScale(Players + 1) > PartyDamageScale(Players));
    }
    Check(PartyHealthScale(8) > 13.f && PartyHealthScale(8) < 14.f);
    Check(PartyDamageScale(8) > 2.2f && PartyDamageScale(8) < 2.22f);

    crypt_world Crypt = CreateCryptWorld(3);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Tank);
    world_entity *Tank = AppState->Players[0].Entity;
    world_entity *Striker = AppState->Players[2].Entity;
    world_entity *Monster = SpawnMonster(AppState, World, &Crypt.Arena,
                                         Tank->Position + V3(200.f, 0.f, 0.f),
                                         MonsterKind_Brute);
    Run->FightingRoom = 2;
    Run->PartyDamage = 4.f;
    float Before = Striker->Hp;
    DamageEntity(AppState, World, Striker, 10.f, Monster);
    Check(NearHp(Before - Striker->Hp, 40.f));
    Before = Tank->Hp;
    DamageEntity(AppState, World, Tank, 10.f, Monster);
    Check(NearHp(Before - Tank->Hp, 10.f * 0.7f * 2.f));
    Check(NearHp(HealPlayer(AppState, 1, Striker, 10.f), 20.f));
    Run->FightingRoom = 0;
    Run->PartyDamage = 0.f;
    RemoveEntity(World, Monster);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): with nobody hurt and nobody picked, Ward and Mending Bolt
// go to an ally, not the healer; Ward shields the allies near the warded
// one; only a healer alone wards themselves
internal void
TestHealerFavoursAllies()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    TickCrypt(&Crypt, 1);
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Healer);
    world_entity *Healer = AppState->Players[0].Entity;
    world_entity *Ally = AppState->Players[1].Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Ally, Healer->Position + V3(60.f, 0.f, 0.f));
    PressAt(&Crypt, 0, PlayerButton_Shield, 0);
    Check(AppState->Players[1].WardAbsorb == WARD_ABSORB);
    Check(AppState->Players[0].WardAbsorb == WARD_SPLASH_SHARE * WARD_ABSORB);
    DestroyCryptWorld(&Crypt);

    crypt_world Solo = CreateCryptWorld(1);
    TickCrypt(&Solo, 1);
    SetPlayerRole(Solo.AppState, &Solo.AppState->Players[0], PlayerRole_Healer);
    PressAt(&Solo, 0, PlayerButton_Shield, 0);
    Check(Solo.AppState->Players[0].WardAbsorb == WARD_ABSORB);
    DestroyCryptWorld(&Solo);
}

// NOTE(zoubir): a taunt raises Shield Wall; Shield Slam heals the tank
// for each monster it strikes
internal void
TestTankSustain()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    TickCrypt(&Crypt, 1);
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Tank);
    player_slot *Slot = &AppState->Players[0];
    world_entity *Tank = Slot->Entity;
    PressOnce(&Crypt, 0, PlayerButton_Launch);
    Check(Slot->ShieldWallSeconds > TAUNT_WALL_SECONDS - 0.1f);
    Slot->ShieldWallSeconds = 0.f;

    v3 Spots[3] = {V3(-60.f, 0.f, 0.f), V3(60.f, 0.f, 0.f), V3(0.f, 60.f, 0.f)};
    world_entity *Monsters[3];
    for(u32 Index = 0; Index < 3; Index++)
    {
        Monsters[Index] = SpawnMonster(AppState, World, &Crypt.Arena,
                                       Tank->Position + Spots[Index], MonsterKind_Brute);
    }
    u32 Near = 0;
    for(u32 Index = 0; Index < 3; Index++)
    {
        Near += Length(Monsters[Index]->Position.XY - Tank->Position.XY) <=
            SHIELD_SLAM_RADIUS ? 1 : 0;
    }
    Check(Near > 0);
    Tank->Hp = Tank->MaxHp - 100.f;
    PressOnce(&Crypt, 0, PlayerButton_Shield);
    float Healed = Tank->Hp - (Tank->MaxHp - 100.f);
    Check(NearHp(Healed, SHIELD_SLAM_HEAL_SHARE * (float)Near * Tank->MaxHp));
    for(u32 Index = 0; Index < 3; Index++)
    {
        RemoveEntity(World, Monsters[Index]);
    }
    DestroyCryptWorld(&Crypt);
}

internal void
RunDungeonRoleTests()
{
    TestRoleKeys();
    TestAllySpellsGoWherePicked();
    TestShieldSlamStunsAndRallies();
    TestInfernoFallsThenBurns();
    TestRoleTalents();
    TestPartyScalesExponentially();
    TestHealerFavoursAllies();
    TestTankSustain();
}
