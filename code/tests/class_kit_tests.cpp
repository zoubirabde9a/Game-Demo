/* Class kit tests (sim/dungeon/role_abilities.cpp, role_kits/), included
   by dungeon_tests.cpp: in a run a class casts only its own spells and
   the shared fireball, shield and blink; its C and V spells wait for
   their talent; the Giant Fireball winds up, flies and blows up a pack,
   toward the spot it was cast at; a client predicting its own striker
   shows the Meteor and Giant Fireball wind-ups but fires neither; the tree's V spells (Combustion, Last Stand, Radiance) do what
   they say; the tank's and healer's W attacks land; and their right-click
   basic attacks, Shield Bash and Smite Bolt, hit a little; and the
   Mender's Steadfast Ward and Guardian Angel do what they say. */

// NOTE(zoubir): the run lets through the shared keys and the class's
// learned spells, nothing else
internal void
TestRunKeepsTheClassKeys()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Damage);
    // NOTE(zoubir): the two base spells only, before any point
    ResetRoleTalents(Slot);
    u32 Allowed = RunAllowedButtons(AppState, Slot, PLAYER_ALL_BUTTONS);
    Check(Allowed == (DUNGEON_SHARED_BUTTONS | PlayerButton_Push | PlayerButton_Launch));
    Check(!(Allowed & (PlayerButton_Attack | PlayerButton_Dash | PlayerButton_Shockwave |
                       PlayerButton_RewindSelf | PlayerButton_Kunai)));
    SetClassTalentRank(Slot, StrikerTalent_Combustion, 1);
    Check(RunAllowedButtons(AppState, Slot, 0) & PlayerButton_Kunai);

    // NOTE(zoubir): outside a run the game's own rules stand
    dungeon_run *Run = AppState->Dungeon;
    AppState->Dungeon = 0;
    Check(RunAllowedButtons(AppState, Slot, PlayerButton_Attack) == PlayerButton_Attack);
    AppState->Dungeon = Run;

    // NOTE(zoubir): a tree spell's key does nothing until its point
    SetPlayerRole(AppState, Slot, PlayerRole_Healer);
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    Check(Slot->RoleCooldowns[2] == 0.f && Run->Sanctuaries[0].Seconds <= 0.f);
    SetClassTalentRank(Slot, HealerTalent_Sanctuary, 1);
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    Check(Slot->RoleCooldowns[2] > 0.f && Run->Sanctuaries[0].Seconds > 0.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): R winds up 1.5 s, then a slow ball flies along the aim
// and blows up on the monster, hitting and marking it
internal void
TestGiantFireballBlowsUp()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Damage);
    GrantClassSpells(Slot);
    world_entity *Striker = Slot->Entity;
    world_entity *Monster = SpawnMonster(AppState, World, &Crypt.Arena,
                                         Striker->Position + V3(200.f, 0.f, 0.f),
                                         MonsterKind_Brute);
    Monster->MaxHp = Monster->Hp = 2000.f;
    v3 Spot = Monster->Position;
    Striker->Aim = V2(1.f, 0.f);
    PressOnce(&Crypt, 0, PlayerButton_Push);
    Check(Striker->CastSpell == PlayerSpell_GiantFireball);
    Check(Slot->RoleCooldowns[1] > 0.f);
    Check(Run->GiantFireballs[0].Distance == 0.f);
    u32 CastTicks = (u32)(60.f * PlayerSpells[PlayerSpell_GiantFireball].CastTime) + 1;
    for(u32 Tick = 0; Tick < CastTicks; Tick++)
    {
        Monster->Position = Spot;
        Monster->Velocity = {};
        TickCrypt(&Crypt, 1);
    }
    Check(Striker->CastSpell == 0);
    Check(Run->GiantFireballs[0].Distance > 0.f);
    for(u32 Tick = 0; Tick < 120 && Run->GiantFireballs[0].Distance > 0.f; Tick++)
    {
        Monster->Position = Spot;
        Monster->Velocity = {};
        TickCrypt(&Crypt, 1);
    }
    Check(Run->GiantFireballs[0].Distance == 0.f);
    Check(Monster->Hp <= 2000.f - GIANT_FIREBALL_DAMAGE);
    foe_mark *Mark = FindFoeMark(Run, World, Monster);
    Check(Mark && Mark->Stacks >= 1);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the striker casts at a spot, then walks off and turns
// the cursor away and throws a fireball mid-cast: the Giant Fireball
// still flies toward the spot
internal void
TestGiantFireballFliesToTheCastSpot()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Damage);
    GrantClassSpells(Slot);
    world_entity *Striker = Slot->Entity;
    // NOTE(zoubir): the aim reaches the body a tick before the press
    Slot->Input.Aim = V2(1.f, 0.f);
    TickCrypt(&Crypt, 1);
    PressOnce(&Crypt, 0, PlayerButton_Push);
    Check(Striker->CastSpell == PlayerSpell_GiantFireball);
    v2 Spot = Striker->Position.XY + V2(PLAYER_AIM_REACH, 0.f);
    Slot->Input.Aim = V2(0.f, 1.f);
    Slot->Input.Move = V2(0.f, 1.f);
    PressOnce(&Crypt, 0, PlayerButton_Cast);
    giant_fireball *Ball = &Run->GiantFireballs[0];
    for(u32 Tick = 0; Tick < 120 && Ball->Distance <= 0.f; Tick++)
    {
        TickCrypt(&Crypt, 1);
    }
    Slot->Input.Move = {};
    Check(Ball->Distance > 0.f);
    Check(LengthSq(Striker->Position.XY + V2(PLAYER_AIM_REACH, 0.f) - Spot) > Square(40.f));
    v2 Flying = DirectionTo(Ball->Velocity);
    v2 ToSpot = DirectionTo(Spot - Ball->Position.XY);
    Check(DotProduct(Flying, ToSpot) > 0.999f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): online, the client predicting its own striker starts
// both wind-ups the moment they are pressed, so the cast pose and bar
// show, but leaves the meteor and the ball to the server
internal void
TestPredictedStrikerWindsUp()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Damage);
    GrantClassSpells(Slot);
    world_entity *Striker = Slot->Entity;
    Slot->Input.Aim = V2(1.f, 0.f);
    Slot->Predicted = true;
    u32 Keys[2] = {PlayerButton_Launch, PlayerButton_Push};
    player_spell Spells[2] = {PlayerSpell_Meteor, PlayerSpell_GiantFireball};
    for(u32 Key = 0; Key < 2; Key++)
    {
        PressOnce(&Crypt, 0, Keys[Key]);
        Check(Striker->CastSpell == (u32)Spells[Key]);
        Check(Slot->RoleCooldowns[Key] > 0.f);
        TickCrypt(&Crypt, 120);
        Check(Striker->CastSpell == 0);
    }
    Check(Run->GiantFireballs[0].Distance <= 0.f);
    for(u32 Index = 0; Index < MAX_INFERNOS; Index++)
    {
        Check(Run->Infernos[Index].Delay <= 0.f && Run->Infernos[Index].Seconds <= 0.f);
    }
    Slot->Predicted = false;
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the V spells: Combustion raises the striker's damage,
// Last Stand heals the tank behind Shield Wall, Radiance heals and wards
// everyone round the healer
internal void
TestTreeFinishers()
{
    crypt_world Crypt = CreateCryptWorld(3);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    TickCrypt(&Crypt, 1);
    player_slot *Striker = &AppState->Players[0];
    player_slot *Tank = &AppState->Players[1];
    player_slot *Healer = &AppState->Players[2];
    SetPlayerRole(AppState, Striker, PlayerRole_Damage);
    GrantClassSpells(Striker);
    SetPlayerRole(AppState, Tank, PlayerRole_Tank);
    GrantClassSpells(Tank);
    SetPlayerRole(AppState, Healer, PlayerRole_Healer);
    GrantClassSpells(Healer);
    world_entity *Monster = SpawnMonster(AppState, World, &Crypt.Arena,
                                         Striker->Entity->Position + V3(300.f, 0.f, 0.f),
                                         MonsterKind_Brute);
    Monster->MaxHp = Monster->Hp = 2000.f;
    float Plain = DungeonScaleDamage(AppState, Monster, Striker->Entity, 10.f);
    Check(CastStrikerKey(AppState, Striker, Striker->Entity, 3));
    float Burning = DungeonScaleDamage(AppState, Monster, Striker->Entity, 10.f);
    Check(Burning > (1.f + COMBUSTION_SHARE) * Plain - 0.01f &&
          Burning < (1.f + COMBUSTION_SHARE) * Plain + 0.01f);

    world_entity *Body = Tank->Entity;
    Body->Hp = 0.5f * Body->MaxHp;
    Check(CastTankKey(AppState, World, &Crypt.Arena, Tank, Body, 3));
    Check(NearHp(Body->Hp, (0.5f + LAST_STAND_HEAL_SHARE) * Body->MaxHp));
    Check(Tank->ShieldWallSeconds == LAST_STAND_SECONDS);

    // NOTE(zoubir): the striker beside the healer is healed and warded,
    // the tank far off is not
    world_entity *Mender = Healer->Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Striker->Entity,
                 Mender->Position + V3(60.f, 0.f, 0.f));
    MovePlayerTo(AppState, World, &Crypt.Arena, Body, Mender->Position + V3(0.f, 400.f, 0.f));
    bool32 TankNear = Length(Body->Position.XY - Mender->Position.XY) <= RADIANCE_RADIUS;
    Striker->Entity->Hp = Striker->Entity->MaxHp - 50.f;
    Striker->WardAbsorb = Tank->WardAbsorb = 0.f;
    Check(CastHealerKey(AppState, Healer, Mender, 3));
    Check(NearHp(Striker->Entity->Hp, Striker->Entity->MaxHp - 50.f + RADIANCE_HEAL));
    Check(Striker->WardAbsorb == RADIANCE_WARD);
    Check((Tank->WardAbsorb > 0.f) == TankNear);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the W spells: Shield Throw hits a foe and bounces to the
// next two, sundering each, never to a fourth; Holy Fire hurts a foe and
// heals the most hurt ally; the striker has nothing on W
internal void
TestAttackSpells()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    player_slot *Tank = &AppState->Players[0];
    player_slot *Healer = &AppState->Players[1];
    SetPlayerRole(AppState, Tank, PlayerRole_Damage);
    // NOTE(zoubir): the striker's W is Detonate, from its tree
    ResetRoleTalents(Tank);
    Check(!(RunAllowedButtons(AppState, Tank, 0) & PlayerButton_Shockwave));
    SetPlayerRole(AppState, Tank, PlayerRole_Tank);
    GrantClassSpells(Tank);
    SetPlayerRole(AppState, Healer, PlayerRole_Healer);
    GrantClassSpells(Healer);
    Check(RunAllowedButtons(AppState, Tank, 0) & PlayerButton_Shockwave);
    Check(RunAllowedButtons(AppState, Healer, 0) & PlayerButton_Shockwave);
    world_entity *Body = Tank->Entity;
    world_entity *Mender = Healer->Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Mender, Body->Position + V3(0.f, 60.f, 0.f));
    world_entity *Foes[4];
    for(u32 Index = 0; Index < 4; Index++)
    {
        Foes[Index] = SpawnMonster(AppState, World, &Crypt.Arena,
                                   Body->Position + V3(120.f + 70.f * (float)Index, 0.f, 0.f),
                                   MonsterKind_Brute);
        Foes[Index]->MaxHp = Foes[Index]->Hp = 2000.f;
    }
    // NOTE(zoubir): the first one picked under the cursor
    Tank->Input.Target = (u32)(Foes[0] - World->Entities) + 1;
    Check(CastTankKey(AppState, World, &Crypt.Arena, Tank, Body, 4));
    for(u32 Index = 0; Index < 3; Index++)
    {
        foe_mark *Mark = FindFoeMark(Run, World, Foes[Index]);
        Check(Foes[Index]->Hp < 2000.f);
        Check(Mark && Mark->SunderSeconds > 0.f);
    }
    Check(Foes[3]->Hp == 2000.f);
    Check(2000.f - Foes[0]->Hp > 2000.f - Foes[2]->Hp);

    float FoesHp = 0.f;
    for(u32 Index = 0; Index < 4; Index++)
    {
        FoesHp += Foes[Index]->Hp;
    }
    Body->Hp = Body->MaxHp - 80.f;
    Mender->Hp = Mender->MaxHp;
    Check(CastHealerKey(AppState, Healer, Mender, 4));
    float FoesAfter = 0.f;
    for(u32 Index = 0; Index < 4; Index++)
    {
        FoesAfter += Foes[Index]->Hp;
    }
    Check(FoesAfter < FoesHp);
    Check(Body->Hp > Body->MaxHp - 80.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the right click: Shield Bash strikes what is in front
// of the tank, not behind, makes threat and ticks the bash count the
// shield's punch is drawn from; Smite Bolt hurts a foe and heals the
// most hurt ally
internal void
TestBasicAttacks()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    TickCrypt(&Crypt, 1);
    player_slot *Tank = &AppState->Players[0];
    player_slot *Healer = &AppState->Players[1];
    SetPlayerRole(AppState, Tank, PlayerRole_Tank);
    GrantClassSpells(Tank);
    SetPlayerRole(AppState, Healer, PlayerRole_Healer);
    GrantClassSpells(Healer);
    Check(RunAllowedButtons(AppState, Tank, 0) & PlayerButton_Attack);
    Check(RunAllowedButtons(AppState, Healer, 0) & PlayerButton_Attack);
    world_entity *Body = Tank->Entity;
    world_entity *Mender = Healer->Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Mender, Body->Position + V3(0.f, 200.f, 0.f));
    world_entity *Front = SpawnMonster(AppState, World, &Crypt.Arena,
                                       Body->Position + V3(50.f, 0.f, 0.f), MonsterKind_Brute);
    world_entity *Back = SpawnMonster(AppState, World, &Crypt.Arena,
                                      Body->Position + V3(-50.f, 0.f, 0.f), MonsterKind_Brute);
    Front->MaxHp = Front->Hp = 2000.f;
    Back->MaxHp = Back->Hp = 2000.f;
    Body->Aim = V2(1.f, 0.f);
    u8 CountBefore = (u8)(Tank->ClassFlags & TANK_FLAG_BASH_COUNT);
    CastTankKey(AppState, World, &Crypt.Arena, Tank, Body, 6);
    Check(Front->Hp < 2000.f);
    Check(Back->Hp == 2000.f);
    threat_row *Row = FindThreatRow(&AppState->Dungeon->Threat, World, Front, false);
    Check(Row && Row->Threat[0] > 0.f);
    Check((u8)(Tank->ClassFlags & TANK_FLAG_BASH_COUNT) != CountBefore);
    // NOTE(zoubir): a little damage, well under a damage class's
    Check(2000.f - Front->Hp < 10.f);

    float FrontBefore = Front->Hp;
    Body->Hp = Body->MaxHp - 50.f;
    Healer->Input.Target = (u32)(Front - World->Entities) + 1;
    Check(CastHealerKey(AppState, Healer, Mender, 6));
    Check(Front->Hp < FrontBefore);
    Check(Body->Hp > Body->MaxHp - 50.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the Mender's later talents: Steadfast Ward's ranks make
// Ward absorb more and come back sooner; Guardian Angel keeps a blow from
// killing an ally, heals and wards them, once per ally per fight
internal void
TestMenderLaterTalents()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    player_slot *Striker = &AppState->Players[0];
    player_slot *Healer = &AppState->Players[1];
    SetPlayerRole(AppState, Striker, PlayerRole_Damage);
    GrantClassSpells(Striker);
    SetPlayerRole(AppState, Healer, PlayerRole_Healer);
    GrantClassSpells(Healer);
    world_entity *Ally = Striker->Entity;
    world_entity *Mender = Healer->Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Ally, Mender->Position + V3(60.f, 0.f, 0.f));
    float Sustain = PartySustainScale(Run);

    float Plain = RoleSpellCooldown(Healer, 1);
    SetClassTalentRank(Healer, HealerTalent_SteadfastWard, 4);
    Check(NearHp(RoleSpellCooldown(Healer, 1), Plain - 4.f * STEADFAST_WARD_SECONDS));
    Healer->Input.Target = (u32)(Ally - World->Entities) + 1;
    Striker->WardAbsorb = 0.f;
    Check(CastHealerKey(AppState, Healer, Mender, 1));
    Check(NearHp(Striker->WardAbsorb, (WARD_ABSORB + 4.f * STEADFAST_WARD_ABSORB) * Sustain));

    // NOTE(zoubir): without the capstone a blow lands whole
    Run->FightingRoom = 2;
    Striker->WardAbsorb = 0.f;
    Ally->Hp = 0.5f * Ally->MaxHp;
    Check(GuardianAngelSave(AppState, Ally, Striker, 1000.f) == 1000.f);
    Check(Ally->Hp == 0.5f * Ally->MaxHp && !Striker->AngelSpent);

    // NOTE(zoubir): with it a killing blow leaves the ally standing,
    // healed and warded
    SetClassTalentRank(Healer, HealerTalent_GuardianAngel, 1);
    DamageEntity(AppState, World, Ally, 100000.f, 0);
    Check(!IsDeadPlayer(Ally) && Striker->AngelSpent);
    Check(NearHp(Ally->Hp, 1.f + GUARDIAN_ANGEL_HEAL_SHARE * Ally->MaxHp * Sustain));
    Check(NearHp(Striker->WardAbsorb, GUARDIAN_ANGEL_WARD * Sustain));

    // NOTE(zoubir): a blow that leaves the ally over the share is not
    // caught, nor a second one the same fight
    Striker->AngelSpent = 0;
    Ally->Hp = Ally->MaxHp;
    Check(GuardianAngelSave(AppState, Ally, Striker, 0.5f * Ally->MaxHp) == 0.5f * Ally->MaxHp);
    Check(!Striker->AngelSpent);
    Striker->AngelSpent = 1;
    Ally->Hp = 0.5f * Ally->MaxHp;
    Check(GuardianAngelSave(AppState, Ally, Striker, Ally->MaxHp) == Ally->MaxHp);
    Check(Ally->Hp == 0.5f * Ally->MaxHp);

    // NOTE(zoubir): the next fight it catches them again
    Run->FightingRoom = 0;
    UpdateGuardianAngels(AppState, Run);
    Check(!Striker->AngelSpent);
    Run->FightingRoom = 2;
    Striker->WardAbsorb = 0.f;
    float Hit = 0.3f * Ally->MaxHp;
    float Left = GuardianAngelSave(AppState, Ally, Striker, Hit);
    Check(Left == Hit && Striker->AngelSpent);
    Check(NearHp(Ally->Hp - Left, (0.2f + GUARDIAN_ANGEL_HEAL_SHARE * Sustain) * Ally->MaxHp));
    Check(Striker->WardAbsorb > 0.f);
    Run->FightingRoom = 0;
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Atonement: the heal the healer's light gives the most hurt
// ally grows 20% a rank
internal void
TestAtonement()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    TickCrypt(&Crypt, 1);
    player_slot *Healer = &AppState->Players[0];
    SetPlayerRole(AppState, Healer, PlayerRole_Healer);
    world_entity *Ally = AppState->Players[1].Entity;
    float Healed[2];
    for(u32 Ranks = 0; Ranks < 2; Ranks++)
    {
        SetClassTalentRank(Healer, HealerTalent_Atonement, 2 * Ranks);
        Ally->Hp = 10.f;
        OnHealerShot(AppState, Healer->Entity, 10.f);
        Healed[Ranks] = Ally->Hp - 10.f;
    }
    Check(Healed[0] > 0.f);
    Check(Healed[1] > 0.99f * (1.f + 2.f * ATONEMENT_SHARE) * Healed[0] &&
          Healed[1] < 1.01f * (1.f + 2.f * ATONEMENT_SHARE) * Healed[0]);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the Mender's later spells: Prayer of Healing heals the
// most hurt allies near, Dawnbreak strikes the foes on its line and heals
// through Smite, Purify clears a held ally and wards it
internal void
TestMenderNewLight()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Healer);
    GrantClassSpells(Slot);
    world_entity *Healer = Slot->Entity;
    world_entity *Ally = AppState->Players[1].Entity;
    v3 Old = Ally->Position;
    Ally->Position = Healer->Position + V3(80.f, 60.f, 0.f);
    CheckAndChangeEntityChunk(AppState, World, &Crypt.Arena, Old, Ally);
    Ally->Hp = 0.4f * Ally->MaxHp;
    float Before = Ally->Hp;
    PressOnce(&Crypt, 0, PlayerButton_FrostNova);
    Check(Slot->RoleCooldowns[7] > 0.f && Ally->Hp > Before);
    Healer->Aim = V2(1.f, 0.f);
    world_entity *Foe = SpawnMonster(AppState, World, &Crypt.Arena,
                                     Healer->Position + V3(150.f, 0.f, 0.f), MonsterKind_Brute);
    Foe->MaxHp = Foe->Hp = 1000.f;
    Before = Ally->Hp;
    PressOnce(&Crypt, 0, PlayerButton_Cast);
    Check(Slot->RoleCooldowns[5] > 0.f && Foe->Hp < 1000.f && Ally->Hp > Before);
    ApplyStatus(Ally, StatusEffect_Rooted, 5.f);
    AppState->Players[1].WardAbsorb = 0.f;
    Slot->Input.Target = (u32)(Ally - World->Entities) + 1;
    PressOnce(&Crypt, 0, PlayerButton_GravityWell);
    Check(Slot->RoleCooldowns[8] > 0.f && !HasStatus(Ally, StatusEffect_Rooted));
    Check(AppState->Players[1].WardAbsorb >= PURIFY_WARD - 0.01f);
    DestroyCryptWorld(&Crypt);
}

internal void
RunClassKitTests()
{
    TestRunKeepsTheClassKeys();
    TestGiantFireballBlowsUp();
    TestGiantFireballFliesToTheCastSpot();
    TestPredictedStrikerWindsUp();
    TestTreeFinishers();
    TestAttackSpells();
    TestBasicAttacks();
    TestMenderLaterTalents();
    TestAtonement();
    TestMenderNewLight();
}
