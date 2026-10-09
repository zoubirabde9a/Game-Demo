/* Frost Mage tests (sim/dungeon/role_kits/frostmage.cpp), included by
   dungeon_tests.cpp: the class owns its keys, C and V once learned, and
   the right click does nothing; Frostbolt landing when its bolt arrives,
   chilling and growing an Icicle; Shatter on a rooted or stunned foe;
   Glacial Spike spending the Icicles and freezing on five; Blizzard
   striking and chilling inside its circle only; Frost Nova rooting what is
   near; Ice Barrier taking hits; Frozen Orb rolling and growing Icicles;
   each talent with code of its own; and Icicles melting between fights.
   The dummies and the tick are the Ranger tests' (ranger_tests.cpp). */

// NOTE(zoubir): a Frost Mage in slot 0 of a fresh crypt, aiming along +X
internal crypt_world
CreateFrostMageWorld(u32 Players = 1)
{
    crypt_world Crypt = CreateCryptWorld(Players);
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &Crypt.AppState->Players[0];
    SetPlayerRole(Crypt.AppState, Slot, PlayerRole_FrostMage);
    GrantClassSpells(Slot);
    Slot->Entity->Aim = V2(1.f, 0.f);
    Slot->Entity->AimReach = 0.6f;
    return Crypt;
}

// NOTE(zoubir): ticks for a bolt to fly Distance, and one to spare
inline u32
FrostFlightTicks(float Distance, float Speed)
{
    u32 Result = (u32)(60.f * Distance / Speed) + 3;
    return Result;
}

#define FROST_DEALT (GetRoleDef(PlayerRole_FrostMage)->DamageDealt)
#define FROST_SPIKE_TICKS ((u32)(60.f * 1.25f) + 4)

internal void
TestFrostMageKeys()
{
    crypt_world Crypt = CreateFrostMageWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    Check(RoleHasKit(PlayerRole_FrostMage));
    Check(IsDamageRole(PlayerRole_FrostMage) && RoleKindOf(PlayerRole_FrostMage) == RoleKind_Ranged);
    // NOTE(zoubir): the two base spells only, before any point
    ResetRoleTalents(Slot);
    u32 Allowed = RunAllowedButtons(AppState, Slot, PLAYER_ALL_BUTTONS);
    Check(Allowed == (DUNGEON_SHARED_BUTTONS | PlayerButton_Push | PlayerButton_Shockwave));
    Check(!(Allowed & PlayerButton_Attack));
    SetClassTalentRank(Slot, FrostMageTalent_IceBarrier, 1);
    SetClassTalentRank(Slot, FrostMageTalent_FrozenOrb, 1);
    Allowed = RunAllowedButtons(AppState, Slot, 0);
    Check((Allowed & PlayerButton_Slam) && (Allowed & PlayerButton_Kunai));
    Check(FrostMageKeyWindsUp(1) && !FrostMageKeyWindsUp(0) && !FrostMageKeyWindsUp(5));
    Check(PlayerSpells[PlayerSpell_FrostMageA].CastTime == 1.25f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): X looses a bolt; the hit lands when it gets there, chills
// and grows an Icicle, which clients see
internal void
TestFrostboltLandsAndChills()
{
    crypt_world Crypt = CreateFrostMageWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    ranger_dummies Dummies = {};
    world_entity *Foe = RangerDummy(&Crypt, &Dummies, V3(300.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Cast);
    Check(Slot->RoleCooldowns[5] > 0.f);
    Check(Foe->Hp == 2000.f);
    RangerTick(&Crypt, &Dummies, FrostFlightTicks(300.f, FROSTBOLT_SPEED));
    float Dealt = 2000.f - Foe->Hp;
    float Expected = FROSTBOLT_DAMAGE * FROST_DEALT;
    Check(Dealt > 0.99f * Expected && Dealt < 1.01f * Expected);
    Check(HasStatus(Foe, StatusEffect_Slowed));
    Check(Slot->FrostMage.Icicles == 1);
    RangerTick(&Crypt, &Dummies, 1);
    Check(Slot->ClassMeter == 1);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): a rooted or stunned foe takes 40% more from the mage
internal void
TestShatter()
{
    crypt_world Crypt = CreateFrostMageWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    ranger_dummies Dummies = {};
    world_entity *Foe = RangerDummy(&Crypt, &Dummies, V3(250.f, 0.f, 0.f));
    world_entity *Other = RangerDummy(&Crypt, &Dummies, V3(0.f, 250.f, 0.f));
    Check(FrostMageDealtScale(Slot, Foe) == 1.f);
    ApplyStatus(Foe, StatusEffect_Rooted, 5.f);
    ApplyStatus(Other, StatusEffect_Stunned, 5.f);
    Check(FrostMageDealtScale(Slot, Foe) > 1.f + FROST_SHATTER_SHARE - 0.001f);
    Check(FrostMageDealtScale(Slot, Other) > 1.f + FROST_SHATTER_SHARE - 0.001f);
    Slot->Input.Target = (u32)(Foe - AppState->World.Entities) + 1;
    PressOnce(&Crypt, 0, PlayerButton_Cast);
    RangerTick(&Crypt, &Dummies, FrostFlightTicks(250.f, FROSTBOLT_SPEED));
    float Dealt = 2000.f - Foe->Hp;
    float Expected = FROSTBOLT_DAMAGE * FROST_DEALT * (1.f + FROST_SHATTER_SHARE);
    Check(Dealt > 0.99f * Expected && Dealt < 1.01f * Expected);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): R casts, then a spike spends every Icicle; five freeze
internal void
TestGlacialSpike()
{
    for(u32 Icicles = 2; Icicles <= FROSTMAGE_ICICLES_MOST; Icicles += 3)
    {
        crypt_world Crypt = CreateFrostMageWorld();
        app_state *AppState = Crypt.AppState;
        player_slot *Slot = &AppState->Players[0];
        ranger_dummies Dummies = {};
        world_entity *Foe = RangerDummy(&Crypt, &Dummies, V3(280.f, 0.f, 0.f));
        Slot->FrostMage.Icicles = Icicles;
        Slot->FrostMage.IcicleHold = FROSTMAGE_ICICLE_HOLD;
        PressOnce(&Crypt, 0, PlayerButton_Push);
        Check(Slot->Entity->CastSpell == PlayerSpell_FrostMageA);
        Check(Foe->Hp == 2000.f && Slot->FrostMage.Icicles == Icicles);
        if (Icicles == FROSTMAGE_ICICLES_MOST)
        {
            RangerTick(&Crypt, &Dummies, 1);
            Check(Slot->ClassFlags & FROSTMAGE_FLAG_FULL);
        }
        RangerTick(&Crypt, &Dummies, FROST_SPIKE_TICKS + FrostFlightTicks(280.f, GLACIAL_SPIKE_SPEED));
        Check(Slot->FrostMage.Icicles == 0);
        float Dealt = 2000.f - Foe->Hp;
        float Expected = (GLACIAL_SPIKE_DAMAGE + GLACIAL_SPIKE_PER_ICICLE * (float)Icicles) * FROST_DEALT;
        Check(Dealt > 0.99f * Expected && Dealt < 1.01f * Expected);
        Check(HasStatus(Foe, StatusEffect_Stunned) == (Icicles == FROSTMAGE_ICICLES_MOST));
        DestroyCryptWorld(&Crypt);
    }
}

// NOTE(zoubir): Blizzard strikes and chills inside its circle for 3 s,
// and nothing outside it
internal void
TestBlizzard()
{
    crypt_world Crypt = CreateFrostMageWorld();
    app_state *AppState = Crypt.AppState;
    world_entity *Mage = AppState->Players[0].Entity;
    ranger_dummies Dummies = {};
    v2 Point = AimPoint(Mage);
    world_entity *Inside = RangerDummy(&Crypt, &Dummies, V3(Point.X - Mage->Position.X, 0.f, 0.f));
    world_entity *Outside = RangerDummy(&Crypt, &Dummies,
                                        V3(Point.X - Mage->Position.X, 3.f * BLIZZARD_RADIUS, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Launch);
    RangerTick(&Crypt, &Dummies, 20);
    Check(Inside->Hp < 2000.f && HasStatus(Inside, StatusEffect_Slowed));
    RangerTick(&Crypt, &Dummies, (u32)(60.f * BLIZZARD_SECONDS) + 10);
    float Dealt = 2000.f - Inside->Hp;
    float Ticks = BLIZZARD_SECONDS / BLIZZARD_TICK;
    float Expected = Ticks * BLIZZARD_TICK_DAMAGE * FROST_DEALT;
    Check(Dealt > 0.95f * Expected && Dealt < 1.05f * Expected);
    Check(Outside->Hp == 2000.f && !HasStatus(Outside, StatusEffect_Slowed));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): W roots the foes round the mage, not the far ones
internal void
TestFrostNova()
{
    crypt_world Crypt = CreateFrostMageWorld();
    app_state *AppState = Crypt.AppState;
    ranger_dummies Dummies = {};
    world_entity *Near = RangerDummy(&Crypt, &Dummies, V3(90.f, 40.f, 0.f));
    world_entity *Far = RangerDummy(&Crypt, &Dummies, V3(-320.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(HasStatus(Near, StatusEffect_Rooted) && Near->Hp < 2000.f);
    Check(!HasStatus(Far, StatusEffect_Rooted) && Far->Hp == 2000.f);
    Check(Near->StatusTimers[StatusEffect_Rooted] > FROST_NOVA_ROOT - 0.1f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): C raises a shield that takes hits; rank 2 a bigger one
internal void
TestIceBarrier()
{
    for(u32 Rank = 1; Rank <= 2; Rank++)
    {
        crypt_world Crypt = CreateFrostMageWorld();
        app_state *AppState = Crypt.AppState;
        player_slot *Slot = &AppState->Players[0];
        SetClassTalentRank(Slot, FrostMageTalent_IceBarrier, Rank);
        PressOnce(&Crypt, 0, PlayerButton_Slam);
        float Absorb = Rank == 2 ? ICE_BARRIER_ABSORB_2 : ICE_BARRIER_ABSORB;
        Check(Slot->FireguardAbsorb == Absorb);
        TickCrypt(&Crypt, 1);
        Check(Slot->ClassFlags & FROSTMAGE_FLAG_BARRIER);
        float Before = Slot->Entity->Hp;
        float Left = DungeonScaleDamage(AppState, Slot->Entity, 0, 20.f);
        Check(Left == 0.f && Slot->Entity->Hp == Before);
        Check(Slot->FireguardAbsorb > Absorb - 20.5f && Slot->FireguardAbsorb < Absorb - 19.5f);
        TickCrypt(&Crypt, (u32)(60.f * ICE_BARRIER_SECONDS) + 2);
        Check(Slot->FireguardAbsorb == 0.f && !(Slot->ClassFlags & FROSTMAGE_FLAG_BARRIER));
        DestroyCryptWorld(&Crypt);
    }
}

// NOTE(zoubir): V rolls an orb down the aim that strikes, chills and
// grows Icicles, then is gone
internal void
TestFrozenOrb()
{
    crypt_world Crypt = CreateFrostMageWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    SetClassTalentRank(Slot, FrostMageTalent_FrozenOrb, 1);
    ranger_dummies Dummies = {};
    world_entity *Foe = RangerDummy(&Crypt, &Dummies, V3(160.f, 0.f, 0.f));
    world_entity *Aside = RangerDummy(&Crypt, &Dummies, V3(160.f, 300.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Kunai);
    RangerTick(&Crypt, &Dummies, 2);
    Check(Slot->ClassFlags & FROSTMAGE_FLAG_ORB);
    RangerTick(&Crypt, &Dummies, (u32)(60.f * FROZEN_ORB_SECONDS) + 4);
    Check(Foe->Hp < 2000.f && Aside->Hp == 2000.f);
    Check(Slot->FrostMage.Icicles >= 2);
    Check(!(Slot->ClassFlags & FROSTMAGE_FLAG_ORB));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the talents with code of their own
internal void
TestFrostMageTalents()
{
    // NOTE(zoubir): Frostbite raises everything the mage deals
    {
        crypt_world Crypt = CreateFrostMageWorld();
        player_slot *Slot = &Crypt.AppState->Players[0];
        SetClassTalentRank(Slot, FrostMageTalent_Frostbite, 2);
        Check(FrostMageDealtScale(Slot, 0) > 1.f + 2.f * FROSTBITE_SHARE - 0.001f);
        DestroyCryptWorld(&Crypt);
    }
    // NOTE(zoubir): Permafrost: the chill outlasts the plain one
    for(u32 Rank = 0; Rank <= 1; Rank++)
    {
        crypt_world Crypt = CreateFrostMageWorld();
        player_slot *Slot = &Crypt.AppState->Players[0];
        SetClassTalentRank(Slot, FrostMageTalent_Permafrost, Rank);
        ranger_dummies Dummies = {};
        world_entity *Foe = RangerDummy(&Crypt, &Dummies, V3(200.f, 0.f, 0.f));
        PressOnce(&Crypt, 0, PlayerButton_Cast);
        RangerTick(&Crypt, &Dummies, FrostFlightTicks(200.f, FROSTBOLT_SPEED));
        float Chill = Foe->StatusTimers[StatusEffect_Slowed];
        float Expected = FROSTBOLT_CHILL_SECONDS + (Rank ? PERMAFROST_SECONDS : 0.f);
        Check(Chill > Expected - 0.15f && Chill <= Expected);
        DestroyCryptWorld(&Crypt);
    }
    // NOTE(zoubir): Splitting Ice: half the spike on to the next foe
    for(u32 Rank = 0; Rank <= 1; Rank++)
    {
        crypt_world Crypt = CreateFrostMageWorld();
        player_slot *Slot = &Crypt.AppState->Players[0];
        SetClassTalentRank(Slot, FrostMageTalent_SplittingIce, Rank);
        ranger_dummies Dummies = {};
        world_entity *Foe = RangerDummy(&Crypt, &Dummies, V3(200.f, 0.f, 0.f));
        world_entity *Next = RangerDummy(&Crypt, &Dummies, V3(200.f, 100.f, 0.f));
        Slot->Input.Target = (u32)(Foe - Crypt.AppState->World.Entities) + 1;
        PressOnce(&Crypt, 0, PlayerButton_Push);
        RangerTick(&Crypt, &Dummies, FROST_SPIKE_TICKS + 30);
        float Spike = 2000.f - Foe->Hp;
        float Split = 2000.f - Next->Hp;
        Check(Spike > 0.f);
        Check(Rank ? (Split > 0.49f * Spike && Split < 0.51f * Spike) : Split == 0.f);
        DestroyCryptWorld(&Crypt);
    }
    // NOTE(zoubir): Fingers of Frost: the fourth bolt shatters and grows
    // two, and the bolt before it says so to clients
    {
        crypt_world Crypt = CreateFrostMageWorld();
        app_state *AppState = Crypt.AppState;
        player_slot *Slot = &AppState->Players[0];
        SetClassTalentRank(Slot, FrostMageTalent_FingersOfFrost, 1);
        ranger_dummies Dummies = {};
        world_entity *Foe = RangerDummy(&Crypt, &Dummies, V3(200.f, 0.f, 0.f));
        u32 Flight = FrostFlightTicks(200.f, FROSTBOLT_SPEED) + (u32)(60.f * FROSTBOLT_COOLDOWN);
        for(u32 Bolt = 0; Bolt < 3; Bolt++)
        {
            PressOnce(&Crypt, 0, PlayerButton_Cast);
            RangerTick(&Crypt, &Dummies, Flight);
        }
        Check(Slot->FrostMage.Icicles == 3 && (Slot->ClassFlags & FROSTMAGE_FLAG_FINGERS));
        float Before = Foe->Hp;
        PressOnce(&Crypt, 0, PlayerButton_Cast);
        RangerTick(&Crypt, &Dummies, Flight);
        Check(Slot->FrostMage.Icicles == 5);
        float Dealt = Before - Foe->Hp;
        float Expected = FROSTBOLT_DAMAGE * FROST_DEALT * (1.f + FROST_SHATTER_SHARE);
        Check(Dealt > 0.99f * Expected && Dealt < 1.01f * Expected);
        Check(!(Slot->ClassFlags & FROSTMAGE_FLAG_FINGERS));
        DestroyCryptWorld(&Crypt);
    }
    // NOTE(zoubir): Deep Freeze holds the nova longer, a rank at a time
    {
        crypt_world Crypt = CreateFrostMageWorld();
        player_slot *Slot = &Crypt.AppState->Players[0];
        SetClassTalentRank(Slot, FrostMageTalent_DeepFreeze, 4);
        ranger_dummies Dummies = {};
        world_entity *Near = RangerDummy(&Crypt, &Dummies, V3(80.f, 0.f, 0.f));
        PressOnce(&Crypt, 0, PlayerButton_Shockwave);
        Check(Near->StatusTimers[StatusEffect_Rooted] > FROST_NOVA_ROOT + 4.f * DEEP_FREEZE_SECONDS - 0.1f);
        DestroyCryptWorld(&Crypt);
    }
    // NOTE(zoubir): Absolute Zero: a five-Icicle spike freezes the foes
    // near its own, and only near it
    for(u32 Rank = 0; Rank <= 1; Rank++)
    {
        crypt_world Crypt = CreateFrostMageWorld();
        player_slot *Slot = &Crypt.AppState->Players[0];
        SetClassTalentRank(Slot, FrostMageTalent_AbsoluteZero, Rank);
        ranger_dummies Dummies = {};
        world_entity *Foe = RangerDummy(&Crypt, &Dummies, V3(220.f, 0.f, 0.f));
        world_entity *Near = RangerDummy(&Crypt, &Dummies, V3(220.f, 90.f, 0.f));
        world_entity *Far = RangerDummy(&Crypt, &Dummies, V3(220.f, -300.f, 0.f));
        Slot->Input.Target = (u32)(Foe - Crypt.AppState->World.Entities) + 1;
        Slot->FrostMage.Icicles = FROSTMAGE_ICICLES_MOST;
        Slot->FrostMage.IcicleHold = FROSTMAGE_ICICLE_HOLD;
        PressOnce(&Crypt, 0, PlayerButton_Push);
        RangerTick(&Crypt, &Dummies, FROST_SPIKE_TICKS + FrostFlightTicks(220.f, GLACIAL_SPIKE_SPEED));
        Check(HasStatus(Foe, StatusEffect_Stunned));
        Check(HasStatus(Near, StatusEffect_Stunned) == (Rank != 0));
        Check(!HasStatus(Far, StatusEffect_Stunned));
        DestroyCryptWorld(&Crypt);
    }
}

// NOTE(zoubir): between fights Icicles melt one a second after a hold,
// and a player who leaves the class leaves them
internal void
TestIciclesMelt()
{
    crypt_world Crypt = CreateFrostMageWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    Check(!AppState->Dungeon->FightingRoom);
    Slot->FrostMage.Icicles = 4;
    Slot->FrostMage.IcicleHold = 0.f;
    TickCrypt(&Crypt, 125);
    Check(Slot->FrostMage.Icicles == 2 && Slot->ClassMeter == 2);
    SetPlayerRole(AppState, Slot, PlayerRole_Tank);
    GrantClassSpells(Slot);
    TickCrypt(&Crypt, 1);
    Check(Slot->FrostMage.Icicles == 0 && Slot->ClassMeter == 0);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): with Deep Freeze a Blizzard's end freezes what is still in
// it, 0.4 s a rank
internal void
TestDeepFreezeBlizzard()
{
    crypt_world Crypt = CreateFrostMageWorld();
    app_state *AppState = Crypt.AppState;
    world_entity *Mage = AppState->Players[0].Entity;
    SetClassTalentRank(&AppState->Players[0], FrostMageTalent_DeepFreeze, 2);
    GrantClassSpells(&AppState->Players[0]);
    ranger_dummies Dummies = {};
    v2 Point = AimPoint(Mage);
    world_entity *Inside = RangerDummy(&Crypt, &Dummies, V3(Point.X - Mage->Position.X, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Launch);
    RangerTick(&Crypt, &Dummies, 20);
    Check(!HasStatus(Inside, StatusEffect_Rooted));
    RangerTick(&Crypt, &Dummies, (u32)(60.f * BLIZZARD_SECONDS) - 18);
    Check(HasStatus(Inside, StatusEffect_Rooted));
    Check(Inside->StatusTimers[StatusEffect_Rooted] <= 2.f * DEEP_FREEZE_BLIZZARD_SECONDS);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Cone of Cold strikes and chills the foes in front, not the
// one behind, and grows an Icicle for each struck
internal void
TestConeOfCold()
{
    crypt_world Crypt = CreateFrostMageWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    ranger_dummies Dummies = {};
    world_entity *Front = RangerDummy(&Crypt, &Dummies, V3(120.f, 0.f, 0.f));
    world_entity *Side = RangerDummy(&Crypt, &Dummies, V3(110.f, 60.f, 0.f));
    world_entity *Behind = RangerDummy(&Crypt, &Dummies, V3(-120.f, 0.f, 0.f));
    Slot->FrostMage.Icicles = 0;
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Slot->RoleCooldowns[6] > 0.f);
    Check(Front->Hp < 2000.f && Side->Hp < 2000.f && Behind->Hp == 2000.f);
    Check(HasStatus(Front, StatusEffect_Slowed) && !HasStatus(Behind, StatusEffect_Slowed));
    Check(Slot->FrostMage.Icicles == 2);
    DestroyCryptWorld(&Crypt);
}

internal void
RunFrostMageTests()
{
    TestFrostMageKeys();
    TestFrostboltLandsAndChills();
    TestShatter();
    TestConeOfCold();
    TestGlacialSpike();
    TestBlizzard();
    TestDeepFreezeBlizzard();
    TestFrostNova();
    TestIceBarrier();
    TestFrozenOrb();
    TestFrostMageTalents();
    TestIciclesMelt();
}
