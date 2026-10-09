/* Druid tests (sim/dungeon/role_kits/druid.cpp), included by
   dungeon_tests.cpp: the class owns its keys, C and V once learned, and
   sits in the healer's column; Wrath landing when its bolt arrives and
   growing Bloom; Moonfire's burn; Starfire's cast and falling star, and
   Eclipse on a burning foe; Rejuvenation spending Bloom at once and
   healing over time, longer with Verdancy, spread by Wild Growth;
   Regrowth spending Bloom, more with Overgrowth; heals counting on the
   meter; Entangling Roots holding the foes inside only, longer at rank
   2; Tranquility healing the allies near through its channel; Symbiosis
   healing from damage; Bloom fading between fights; and a Druid reviving
   a downed ally. Heals are checked against the party's sustain scale
   (PartySustainScale), which every heal is multiplied by. */

// NOTE(zoubir): a Druid in slot 0 of a fresh crypt, aiming along +X
internal crypt_world
CreateDruidWorld(u32 Players = 1)
{
    crypt_world Crypt = CreateCryptWorld(Players);
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &Crypt.AppState->Players[0];
    SetPlayerRole(Crypt.AppState, Slot, PlayerRole_Druid);
    GrantClassSpells(Slot);
    Slot->Entity->Aim = V2(1.f, 0.f);
    Slot->Entity->AimReach = 0.6f;
    return Crypt;
}

// NOTE(zoubir): a still monster Offset from the Druid, with health to
// spare, stunned so it stays where it is and hits nobody
internal world_entity *
DruidDummy(crypt_world *Crypt, v3 Offset, bool32 Stunned = true)
{
    app_state *AppState = Crypt->AppState;
    world_entity *Druid = AppState->Players[0].Entity;
    world_entity *Result = SpawnMonster(AppState, &AppState->World, &Crypt->Arena,
                                        Druid->Position + Offset, MonsterKind_Brute);
    Result->MaxHp = Result->Hp = 2000.f;
    if (Stunned)
    {
        ApplyStatus(Result, StatusEffect_Stunned, 100.f);
    }
    return Result;
}

// NOTE(zoubir): Ticks of the world with the Druid kept alive
internal void
DruidTick(crypt_world *Crypt, u32 Ticks)
{
    for(u32 Tick = 0; Tick < Ticks; Tick++)
    {
        world_entity *Druid = Crypt->AppState->Players[0].Entity;
        Druid->Hp = Druid->MaxHp;
        TickCrypt(Crypt, 1);
    }
}

inline bool32
DruidNear(float Value, float Expected)
{
    bool32 Result = Value > 0.99f * Expected - 0.01f && Value < 1.01f * Expected + 0.01f;
    return Result;
}

internal void
TestDruidKeys()
{
    crypt_world Crypt = CreateDruidWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    Check(RoleHasKit(PlayerRole_Druid));
    Check(RoleKindOf(PlayerRole_Druid) == RoleKind_Healer && !IsDamageRole(PlayerRole_Druid));
    // NOTE(zoubir): the two base spells only, before any point
    ResetRoleTalents(Slot);
    u32 Allowed = RunAllowedButtons(AppState, Slot, PLAYER_ALL_BUTTONS);
    Check(Allowed == ((DUNGEON_SHARED_BUTTONS & ~(u32)PlayerButton_Cast) | PlayerButton_Shockwave |
                      PlayerButton_Attack));
    SetClassTalentRank(Slot, DruidTalent_Starfire, 1);
    SetClassTalentRank(Slot, DruidTalent_Tranquility, 1);
    Allowed = RunAllowedButtons(AppState, Slot, 0);
    Check((Allowed & PlayerButton_Push) && (Allowed & PlayerButton_Kunai) && !(Allowed & PlayerButton_Slam));
    Check(DruidKeyWindsUp(1) && DruidKeyWindsUp(3) && !DruidKeyWindsUp(0) && !DruidKeyWindsUp(6));
    for(u32 Variant = 0; Variant < 10; Variant++)
    {
        v3 Spot = DruidBurstSpot(V3(10.f, 20.f, -40.f), Variant);
        Check(DruidBurstVariant(Spot) == Variant);
        Check(Absolute(DruidBurstPlace(Spot).Z + 40.f) < 0.01f);
    }
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the right click looses Wrath; it lands when its bolt gets
// there and grows a Bloom, which clients see; with no foe it casts nothing
internal void
TestWrathGrowsBloom()
{
    crypt_world Crypt = CreateDruidWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Slot->RoleCooldowns[6] == 0.f);
    world_entity *Foe = DruidDummy(&Crypt, V3(300.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Slot->RoleCooldowns[6] > 0.f);
    Check(Foe->Hp == 2000.f);
    DruidTick(&Crypt, (u32)(60.f * 300.f / DRUID_BOLT_SPEED) + 2);
    float Expected = WRATH_DAMAGE * GetRoleDef(PlayerRole_Druid)->DamageDealt;
    Check(DruidNear(2000.f - Foe->Hp, Expected));
    Check(Slot->Druid.Bloom == 1 && Slot->ClassMeter == 1);
    // NOTE(zoubir): Bloom holds at five, and says so
    for(u32 Cast = 0; Cast < 6; Cast++)
    {
        Slot->RoleCooldowns[6] = 0.f;
        PressOnce(&Crypt, 0, PlayerButton_Attack);
        DruidTick(&Crypt, 30);
    }
    Check(Slot->Druid.Bloom == DRUID_BLOOM_MOST);
    Check(Slot->ClassFlags & DRUID_FLAG_BLOOM_FULL);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): X burns a foe: a little at once, then each second
internal void
TestMoonfireBurns()
{
    crypt_world Crypt = CreateDruidWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Foe = DruidDummy(&Crypt, V3(250.f, 0.f, 0.f));
    float Dealt = GetRoleDef(PlayerRole_Druid)->DamageDealt;
    PressOnce(&Crypt, 0, PlayerButton_Cast);
    Check(DruidNear(2000.f - Foe->Hp, MOONFIRE_DAMAGE * Dealt));
    Check(IsDruidMoonfired(AppState, 0, Foe));
    DruidTick(&Crypt, 4 * 60 + 2);
    Check(DruidNear(2000.f - Foe->Hp, (MOONFIRE_DAMAGE + 4.f * MOONFIRE_TICK_DAMAGE) * Dealt));
    Check(Slot->ClassFlags & DRUID_FLAG_MOONFIRE);
    Check(Slot->Druid.Bloom == 0);
    DruidTick(&Crypt, (u32)(60.f * MOONFIRE_SECONDS));
    Check(!IsDruidMoonfired(AppState, 0, Foe));
    Check(!(Slot->ClassFlags & DRUID_FLAG_MOONFIRE));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): R casts, then the star falls and grows two Bloom; with
// Eclipse a burning foe takes more from it
internal void
TestStarfire()
{
    crypt_world Crypt = CreateDruidWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Foe = DruidDummy(&Crypt, V3(250.f, 0.f, 0.f));
    float Dealt = GetRoleDef(PlayerRole_Druid)->DamageDealt;
    PressOnce(&Crypt, 0, PlayerButton_Push);
    Check(Slot->Entity->CastSpell == PlayerSpell_DruidA);
    DruidTick(&Crypt, (u32)(60.f * 1.5f) - 2);
    Check(Foe->Hp == 2000.f);
    DruidTick(&Crypt, (u32)(60.f * STARFIRE_FALL) + 4);
    Check(DruidNear(2000.f - Foe->Hp, STARFIRE_DAMAGE * Dealt));
    Check(Slot->Druid.Bloom == STARFIRE_BLOOM);

    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Rejuvenation spends the Bloom at once, heals the rest
// over its time, and counts on the meter in a fight; Verdancy makes it
// stronger and longer; Wild Growth spreads it to the most hurt near
internal void
TestRejuvenation()
{
    crypt_world Crypt = CreateDruidWorld(4);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Druid = Slot->Entity;
    world_entity *Ally = AppState->Players[1].Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Ally, Druid->Position + V3(100.f, 0.f, 0.f));
    float Sustain = PartySustainScale(Run);
    Ally->Hp = 10.f;
    Ally->MaxHp = 500.f;
    Slot->Druid.Bloom = 3;
    Slot->Input.Target = (u32)(Ally - World->Entities) + 1;
    Run->FightingRoom = 2;
    Check(CastDruidKey(AppState, World, &Crypt.Arena, Slot, Druid, 0));
    Check(DruidNear(Ally->Hp, 10.f + 3.f * REJUVENATION_PER_BLOOM * Sustain));
    Check(Slot->Druid.Bloom == 0);
    float Before = Ally->Hp;
    for(u32 Tick = 0; Tick < (u32)(60.f * REJUVENATION_SECONDS) + 30; Tick++)
    {
        UpdateDruidRejuvenations(AppState, &Run->Druid, 1.f / 60.f);
    }
    Check(DruidNear(Ally->Hp - Before, REJUVENATION_PER_SECOND * REJUVENATION_SECONDS * Sustain));
    Check(DruidNear(Slot->MeterHealing, Ally->Hp - 10.f));
    Run->FightingRoom = 0;

    // NOTE(zoubir): Verdancy
    SetClassTalentRank(Slot, DruidTalent_Verdancy, 1);
    Ally->Hp = 10.f;
    Check(CastDruidKey(AppState, World, &Crypt.Arena, Slot, Druid, 0));
    for(u32 Tick = 0; Tick < (u32)(60.f * (REJUVENATION_SECONDS + VERDANCY_SECONDS)) + 30; Tick++)
    {
        UpdateDruidRejuvenations(AppState, &Run->Druid, 1.f / 60.f);
    }
    Check(DruidNear(Ally->Hp - 10.f, REJUVENATION_PER_SECOND * (1.f + VERDANCY_SHARE) *
                    (REJUVENATION_SECONDS + VERDANCY_SECONDS) * Sustain));

    // NOTE(zoubir): Wild Growth: the two most hurt near the target too,
    // not the one far off
    SetClassTalentRank(Slot, DruidTalent_WildGrowth, 1);
    world_entity *Near = AppState->Players[2].Entity;
    world_entity *Far = AppState->Players[3].Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Near, Ally->Position + V3(0.f, 60.f, 0.f));
    MovePlayerTo(AppState, World, &Crypt.Arena, Far, Ally->Position + V3(0.f, -1000.f, 0.f));
    Near->Hp = 0.5f * Near->MaxHp;
    Far->Hp = 0.2f * Far->MaxHp;
    Ally->Hp = 10.f;
    Check(CastDruidKey(AppState, World, &Crypt.Arena, Slot, Druid, 0));
    u32 OnNear = 0;
    u32 OnFar = 0;
    for(u32 Index = 0; Index < DRUID_MAX_REJUVENATIONS; Index++)
    {
        druid_rejuvenation *Rejuv = &Run->Druid.Rejuvenations[Index];
        OnNear += (Rejuv->Seconds > 0.f && Rejuv->Ally == Near->PlayerIndex) ? 1 : 0;
        OnFar += (Rejuv->Seconds > 0.f && Rejuv->Ally == Far->PlayerIndex) ? 1 : 0;
    }
    Check(OnNear == 1 && OnFar == 0);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Regrowth heals at once for its base and the Bloom spent,
// more with Overgrowth; Symbiosis heals from Wrath's damage
internal void
TestRegrowthAndSymbiosis()
{
    crypt_world Crypt = CreateDruidWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Druid = Slot->Entity;
    world_entity *Ally = AppState->Players[1].Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Ally, Druid->Position + V3(0.f, 100.f, 0.f));
    float Sustain = PartySustainScale(Run);
    Ally->MaxHp = 500.f;
    Ally->Hp = 10.f;
    Slot->Druid.Bloom = 5;
    SetClassTalentRank(Slot, DruidTalent_Overgrowth, 2);
    Check(CastDruidKey(AppState, World, &Crypt.Arena, Slot, Druid, 4));
    float Expected = (REGROWTH_HEAL + 5.f * REGROWTH_PER_BLOOM) * (1.f + 2.f * OVERGROWTH_SHARE) * Sustain;
    Check(DruidNear(Ally->Hp - 10.f, Expected));
    Check(Slot->Druid.Bloom == 0);

    // NOTE(zoubir): Symbiosis
    SetClassTalentRank(Slot, DruidTalent_Symbiosis, 1);
    world_entity *Foe = DruidDummy(&Crypt, V3(200.f, 0.f, 0.f));
    Ally->Hp = 10.f;
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    DruidTick(&Crypt, 20);
    float Dealt = 2000.f - Foe->Hp;
    Check(Dealt > 0.f);
    // NOTE(zoubir): the rest between fights heals too; Symbiosis at least
    Check(Ally->Hp - 10.f >= SYMBIOSIS_SHARE * Dealt * Sustain - 0.01f);
    DestroyCryptWorld(&Crypt);
}

internal void
TestTranquility()
{
    crypt_world Crypt = CreateDruidWorld(3);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Druid = Slot->Entity;
    world_entity *Near = AppState->Players[1].Entity;
    world_entity *Far = AppState->Players[2].Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Near, Druid->Position + V3(0.f, 120.f, 0.f));
    MovePlayerTo(AppState, World, &Crypt.Arena, Far, Druid->Position + V3(0.f, -600.f, 0.f));
    SetClassTalentRank(Slot, DruidTalent_Tranquility, 1);
    // NOTE(zoubir): a fight, so the rest between fights heals nobody
    AppState->Dungeon->FightingRoom = 2;
    Near->MaxHp = Far->MaxHp = 1000.f;
    Near->Hp = Far->Hp = 100.f;
    Check(CastDruidKey(AppState, World, &Crypt.Arena, Slot, Druid, 3));
    Check(Druid->CastSpell == PlayerSpell_DruidB);
    float Pulses = 0.f;
    float Sustain = PartySustainScale(AppState->Dungeon);
    for(u32 Tick = 0; Tick < 200 && Druid->CastSpell == PlayerSpell_DruidB; Tick++)
    {
        float Was = Near->Hp;
        Druid->CastLeft -= 1.f / 60.f;
        UpdateDruidSlot(AppState, AppState->Dungeon, Slot, 1.f / 60.f);
        if (Druid->CastLeft <= 0.f)
        {
            FinishDruidCast(AppState, Slot, Druid, PlayerSpell_DruidB);
            Druid->CastSpell = PlayerSpell_None;
        }
        Pulses += Near->Hp > Was ? 1.f : 0.f;
    }
    Check(Pulses == 6.f);
    Check(DruidNear(Near->Hp - 100.f, 6.f * TRANQUILITY_HEAL * Sustain));
    Check(Far->Hp == 100.f);
    AppState->Dungeon->FightingRoom = 0;
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Bloom holds a while after a fight, then fades one a second
internal void
TestBloomFades()
{
    crypt_world Crypt = CreateDruidWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    AddDruidBloom(AppState, Slot, 3);
    DruidTick(&Crypt, (u32)(60.f * DRUID_BLOOM_HOLD) - 10);
    Check(Slot->Druid.Bloom == 3);
    DruidTick(&Crypt, 10 + 60 + 5);
    Check(Slot->Druid.Bloom == 2);
    DruidTick(&Crypt, 3 * 60);
    Check(Slot->Druid.Bloom == 0 && Slot->ClassMeter == 0);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): a Druid standing over a downed ally brings them back, as
// the Mender does
internal void
TestDruidRevives()
{
    crypt_world Crypt = CreateCryptWorld(3);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    Run->RoomStates[1] = RoomState_Cleared;
    SetPlayerRole(AppState, &AppState->Players[1], PlayerRole_Druid);
    GrantClassSpells(&AppState->Players[1]);
    world_entity *Down = AppState->Players[0].Entity;
    world_entity *Druid = AppState->Players[1].Entity;
    world_entity *Other = AppState->Players[2].Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Down, Run->RoomEntry[2]);
    TickCrypt(&Crypt, 1);
    Check(Run->FightingRoom == 2);
    for(u32 Index = 1; Index < Run->FoeCount; Index++)
    {
        world_entity *Foe = FindMonsterBySerial(World, Run->FoeSlots[Index], Run->FoeSerials[Index]);
        if (Foe)
        {
            RemoveEntity(World, Foe);
        }
    }
    world_entity *Last = FindMonsterBySerial(World, Run->FoeSlots[0], Run->FoeSerials[0]);
    Check(Last != 0);
    ApplyStatus(Last, StatusEffect_Stunned, 100.f);
    v3 Lying = Down->Position;
    KillEntity(AppState, World, Down, 0);
    MovePlayerTo(AppState, World, &Crypt.Arena, Other, Lying + V3(0.f, 40.f, 0.f));
    MovePlayerTo(AppState, World, &Crypt.Arena, Druid, Lying + V3(30.f, 0.f, 0.f));
    TickCrypt(&Crypt, (u32)(3.2f * 60.f));
    Check(!IsDeadPlayer(Down));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Eclipse: with Starfire alone, the star leaves a Moonfire on
// its foe; then Wrath on that foe lands 25% harder
internal void
TestEclipse()
{
    crypt_world Crypt = CreateDruidWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    ResetRoleTalents(Slot);
    SetClassTalentRank(Slot, DruidTalent_Starfire, 1);
    SetClassTalentRank(Slot, DruidTalent_Eclipse, 1);
    Check(!RoleSpellLearned(Slot, 5));
    world_entity *Foe = DruidDummy(&Crypt, V3(250.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Push);
    DruidTick(&Crypt, (u32)(60.f * (1.5f + STARFIRE_FALL)) + 4);
    Check(IsDruidMoonfired(AppState, 0, Foe));
    // NOTE(zoubir): the burn holds its bite, so only the bolt counts
    FindDruidMoonfire(AppState, 0, Foe)->TickTimer = 100.f;
    float Before = Foe->Hp;
    Slot->RoleCooldowns[6] = 0.f;
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    DruidTick(&Crypt, (u32)(60.f * 250.f / DRUID_BOLT_SPEED) + 2);
    float Expected = WRATH_DAMAGE * GetRoleDef(PlayerRole_Druid)->DamageDealt * (1.f + ECLIPSE_SHARE);
    Check(DruidNear(Before - Foe->Hp, Expected));
    DestroyCryptWorld(&Crypt);
}

internal void
RunDruidTests()
{
    TestDruidKeys();
    TestWrathGrowsBloom();
    TestMoonfireBurns();
    TestStarfire();
    TestEclipse();
    TestRejuvenation();
    TestRegrowthAndSymbiosis();
    TestTranquility();
    TestBloomFades();
    TestDruidRevives();
}
