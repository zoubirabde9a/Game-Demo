/* Class kit tests (sim/dungeon/role_abilities.cpp, role_kits/), included
   by dungeon_tests.cpp: in a run a class casts only its own spells and
   the shared fireball, shield and blink; its C and V spells wait for
   their talent; the Giant Fireball winds up, flies and blows up a pack,
   toward the spot it was cast at; a client predicting its own striker
   shows the Meteor and Giant Fireball wind-ups but fires neither; the tree's V spells (Combustion, Last Stand, Radiance) do what
   they say; and the tank's and healer's W attacks land. */

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
    u32 Allowed = RunAllowedButtons(AppState, Slot, PLAYER_ALL_BUTTONS);
    Check(Allowed == (DUNGEON_SHARED_BUTTONS | PlayerButton_Launch | PlayerButton_Push));
    Check(!(Allowed & (PlayerButton_Attack | PlayerButton_Dash | PlayerButton_Shockwave |
                       PlayerButton_RewindSelf | PlayerButton_Kunai | PlayerButton_Slam)));
    Slot->Ranks[Talent_RoleFirst + StrikerTalent_Detonate] = 1;
    Check(RunAllowedButtons(AppState, Slot, 0) & PlayerButton_Slam);
    Slot->Ranks[Talent_RoleFirst + StrikerTalent_Combustion] = 1;
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
    Slot->Ranks[Talent_RoleFirst + HealerTalent_Sanctuary] = 1;
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
    SetPlayerRole(AppState, Tank, PlayerRole_Tank);
    SetPlayerRole(AppState, Healer, PlayerRole_Healer);
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
    Check(!(RunAllowedButtons(AppState, Tank, 0) & PlayerButton_Shockwave));
    SetPlayerRole(AppState, Tank, PlayerRole_Tank);
    SetPlayerRole(AppState, Healer, PlayerRole_Healer);
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

internal void
RunClassKitTests()
{
    TestRunKeepsTheClassKeys();
    TestGiantFireballBlowsUp();
    TestGiantFireballFliesToTheCastSpot();
    TestPredictedStrikerWindsUp();
    TestTreeFinishers();
    TestAttackSpells();
}
