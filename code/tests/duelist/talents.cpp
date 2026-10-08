/* Duelist talent tests (tests/duelist_tests.cpp, included before its
   list): Perfect Form, and the talents with code: Footwork, Bait,
   Crescendo, Flurry and Masterstroke. Precision is in TestHeartseeker. */

// NOTE(zoubir): Perfect Form: the flag, Thrust striking twice, every key
// building Tempo (Thrust after Thrust too), and no Tempo lost to a blow
internal void
TestPerfectForm()
{
    crypt_world Crypt = DuelistCrypt();
    player_slot *Slot = &Crypt.AppState->Players[0];
    world_entity *Foe = DuelistDummy(&Crypt, V3(60.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    float Plain = 2000.f - Foe->Hp;
    Check(Plain > 0.f);
    Slot->Ranks[Talent_RoleFirst + DuelistTalent_PerfectForm] = 1;
    PressOnce(&Crypt, 0, PlayerButton_Kunai);
    Check(Slot->ClassFlags & DUELIST_FLAG_FORM);
    Check(Slot->RoleCooldowns[3] > FORM_COOLDOWN - 1.f);
    Slot->ClassMeter = 0;
    float Before = Foe->Hp;
    DuelistReady(&Crypt, 6);
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    TickCrypt(&Crypt, (u32)(60.f * FORM_ECHO_DELAY) + 2);
    float Twice = Before - Foe->Hp;
    // NOTE(zoubir): the second Thrust struck with the stack the first gave
    Check(Twice > 1.9f * Plain && Twice < 2.2f * Plain);
    Check(Slot->ClassMeter == 1);
    DuelistReady(&Crypt, 6);
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Slot->ClassMeter == 2);
    HitDuelist(&Crypt, Foe, 10.f);
    Check(Slot->ClassMeter == 2);
    TickCrypt(&Crypt, (u32)(60.f * FORM_SECONDS));
    Check(!(Slot->ClassFlags & DUELIST_FLAG_FORM));
    HitDuelist(&Crypt, Foe, 10.f);
    Check(Slot->ClassMeter == 0);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Lunge lands in front of the foe and strikes it; with no
// foe in range it costs nothing; Footwork shortens its cooldown by rank
// and leaves the Duelist Hasted
internal void
TestLungeAndFootwork()
{
    crypt_world Crypt = DuelistCrypt();
    player_slot *Slot = &Crypt.AppState->Players[0];
    world_entity *Player = Slot->Entity;
    PressOnce(&Crypt, 0, PlayerButton_Launch);
    Check(Slot->RoleCooldowns[0] == 0.f);
    world_entity *Foe = DuelistDummy(&Crypt, V3(250.f, 0.f, 0.f));
    PressAt(&Crypt, 0, PlayerButton_Launch, Foe);
    Check(Foe->Hp < 2000.f);
    Check(Length(Foe->Position.XY - Player->Position.XY) < LUNGE_GAP + Foe->Dimensions.X + 20.f);
    Check(Slot->RoleCooldowns[0] > LUNGE_COOLDOWN - 0.5f);
    Check(!HasStatus(Player, StatusEffect_Hasted));
    Slot->Ranks[Talent_RoleFirst + DuelistTalent_Footwork] = 2;
    float Short = LUNGE_COOLDOWN - 2.f * FOOTWORK_COOLDOWN;
    Check(RoleSpellCooldown(Slot, 0) > Short - 0.01f && RoleSpellCooldown(Slot, 0) < Short + 0.01f);
    MovePlayerTo(Crypt.AppState, &Crypt.AppState->World, &Crypt.Arena, Player,
                 Foe->Position - V3(200.f, 0.f, 0.f));
    DuelistReady(&Crypt, 0);
    PressAt(&Crypt, 0, PlayerButton_Launch, Foe);
    Check(HasStatus(Player, StatusEffect_Hasted));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Bait: a longer guard, and a counter heals
internal void
TestBait()
{
    crypt_world Crypt = DuelistCrypt();
    player_slot *Slot = &Crypt.AppState->Players[0];
    world_entity *Player = Slot->Entity;
    world_entity *Foe = DuelistDummy(&Crypt, V3(60.f, 0.f, 0.f));
    Slot->Ranks[Talent_RoleFirst + DuelistTalent_Bait] = 1;
    PressOnce(&Crypt, 0, PlayerButton_Push);
    TickCrypt(&Crypt, (u32)(60.f * RIPOSTE_GUARD_SECONDS) + 2);
    Check(Slot->ClassFlags & DUELIST_FLAG_GUARD);
    Player->Hp = 0.5f * Player->MaxHp;
    HitDuelist(&Crypt, Foe, 30.f);
    TickCrypt(&Crypt, 1);
    Check(Foe->Hp < 2000.f);
    Check(Player->Hp > 0.5f * Player->MaxHp + 0.9f * BAIT_HEAL_SHARE * Player->MaxHp);
    TickCrypt(&Crypt, (u32)(60.f * (BAIT_GUARD_SECONDS - RIPOSTE_GUARD_SECONDS)));
    Check(!(Slot->ClassFlags & DUELIST_FLAG_GUARD));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Crescendo: a full-Tempo Heartseeker also cuts the foe
// beside its target and readies Lunge; short of full it does neither
internal void
TestCrescendo()
{
    crypt_world Crypt = DuelistCrypt();
    player_slot *Slot = &Crypt.AppState->Players[0];
    world_entity *Foe = DuelistDummy(&Crypt, V3(60.f, 0.f, 0.f));
    world_entity *Side = DuelistDummy(&Crypt, V3(40.f, 55.f, 0.f));
    world_entity *Behind = DuelistDummy(&Crypt, V3(-70.f, 0.f, 0.f));
    Slot->Ranks[Talent_RoleFirst + DuelistTalent_Crescendo] = 1;
    u32 WindUp = (u32)(60.f * PlayerSpells[PlayerSpell_DuelistB].CastTime) + 2;
    for(u32 Tempo = 4; Tempo <= 5; Tempo++)
    {
        Slot->ClassMeter = (u8)Tempo;
        Slot->RoleCooldowns[0] = 5.f;
        Slot->RoleCooldowns[4] = 0.f;
        Slot->Duelist.LastKey = 4 + 1;
        PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
        TickCrypt(&Crypt, WindUp);
        Check(Foe->Hp < 2000.f);
        if (Tempo == 4)
        {
            Check(Side->Hp == 2000.f && Slot->RoleCooldowns[0] > 4.f);
        }
        else
        {
            Check(Side->Hp < 2000.f && Slot->RoleCooldowns[0] == 0.f);
        }
    }
    Check(Behind->Hp == 2000.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Flurry: Thrust FLURRY_SHARE harder a rank; Masterstroke: a
// full-Tempo Heartseeker strikes again for 60%, and a kill readies it
internal void
TestFlurryAndMasterstroke()
{
    crypt_world Crypt = DuelistCrypt();
    player_slot *Slot = &Crypt.AppState->Players[0];
    world_entity *Foe = DuelistDummy(&Crypt, V3(60.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    float Plain = 2000.f - Foe->Hp;
    Slot->Ranks[Talent_RoleFirst + DuelistTalent_Flurry] = 4;
    DuelistReady(&Crypt, 6);
    Slot->ClassMeter = 0;
    float Before = Foe->Hp;
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    float Flurried = Before - Foe->Hp;
    float Scale = 1.f + 4.f * FLURRY_SHARE;
    Check(Flurried > 0.99f * Scale * Plain && Flurried < 1.01f * Scale * Plain);

    float Single = HeartseekerWith(&Crypt, 5);
    Slot->Ranks[Talent_RoleFirst + DuelistTalent_Masterstroke] = 1;
    float Double = HeartseekerWith(&Crypt, 5);
    Check(Double < 1.01f * Single);
    // NOTE(zoubir): the second strike lands a moment later
    world_entity *Target = DuelistDummy(&Crypt, V3(60.f, 0.f, 0.f));
    Slot->ClassMeter = 5;
    Slot->RoleCooldowns[4] = 0.f;
    Slot->Duelist.LastKey = 4 + 1;
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Target);
    TickCrypt(&Crypt, (u32)(60.f * (PlayerSpells[PlayerSpell_DuelistB].CastTime + MASTERSTROKE_DELAY)) + 4);
    float Both = 2000.f - Target->Hp;
    Check(Both > 0.99f * (1.f + MASTERSTROKE_SHARE) * Single &&
          Both < 1.01f * (1.f + MASTERSTROKE_SHARE) * Single);
    Check(Slot->RoleCooldowns[4] > HEARTSEEKER_COOLDOWN - 1.f);
    // NOTE(zoubir): a Heartseeker that kills is ready again
    Target->Hp = 5.f;
    Slot->ClassMeter = 2;
    Slot->RoleCooldowns[4] = 0.f;
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Target);
    TickCrypt(&Crypt, (u32)(60.f * PlayerSpells[PlayerSpell_DuelistB].CastTime) + 2);
    Check(Target->Hp <= 0.f || !Target->IsPresent);
    Check(Slot->RoleCooldowns[4] == 0.f);
    DestroyCryptWorld(&Crypt);
}
