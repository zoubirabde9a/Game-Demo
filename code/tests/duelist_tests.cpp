/* Duelist tests (sim/dungeon/role_kits/duelist.cpp), included by
   dungeon_tests.cpp: the class owns its five keys, V once learned, and X
   and C do nothing; Tempo comes only from a key other than the last
   (Thrust after Thrust gains nothing), a hit Riposte did not stop takes
   two, and it fades out of a fight; Riposte cancels a hit whole (damage
   and shove), counters once, takes the hits after the first without a
   second counter, and a guard that parries nothing keeps its whole
   cooldown; a client predicting its own Duelist neither guards nor
   parries; Heartseeker locks its foe at the press, scales with Tempo
   without spending it and bites harder on a hurt foe (sooner with
   Precision). Perfect Form and the other talents with code are
   duelist/talents.cpp. */

// NOTE(zoubir): a big brute Offset from the Duelist in slot 0, stunned
// for good so it neither walks nor hits back
internal world_entity *
DuelistDummy(crypt_world *Crypt, v3 Offset)
{
    app_state *AppState = Crypt->AppState;
    world_entity *Duelist = AppState->Players[0].Entity;
    world_entity *Result = SpawnMonster(AppState, &AppState->World, &Crypt->Arena,
                                        Duelist->Position + Offset, MonsterKind_Brute);
    Result->MaxHp = Result->Hp = 2000.f;
    ApplyStatus(Result, StatusEffect_Stunned, 1000.f);
    return Result;
}

// NOTE(zoubir): a crypt with one Duelist facing +X, its spawn shield gone
// so hits land
internal crypt_world
DuelistCrypt()
{
    crypt_world Result = CreateCryptWorld(1);
    TickCrypt(&Result, 1);
    player_slot *Slot = &Result.AppState->Players[0];
    SetPlayerRole(Result.AppState, Slot, PlayerRole_Duelist);
    GrantClassSpells(Slot);
    Slot->Input.Aim = V2(1.f, 0.f);
    Slot->Entity->Aim = V2(1.f, 0.f);
    Slot->Entity->SpawnShield = 0.f;
    TickCrypt(&Result, 1);
    return Result;
}

// NOTE(zoubir): a monster's blow of Damage from By on the Duelist, as
// HitPlayerWith lands every monster hit, shoving it along +Y
internal void
HitDuelist(crypt_world *Crypt, world_entity *By, float Damage)
{
    app_state *AppState = Crypt->AppState;
    HitPlayerWith(AppState, &AppState->World, AppState->Players[0].Entity, By, Damage,
                  V2(0.f, 200.f), 0.f, StatusEffect_None, 0.f, SimBurst_MonsterHit);
}

// NOTE(zoubir): Ticks ticks, then the cooldown of Key cleared
internal void
DuelistReady(crypt_world *Crypt, u32 Key)
{
    TickCrypt(Crypt, 2);
    Crypt->AppState->Players[0].RoleCooldowns[Key] = 0.f;
}

internal void
TestDuelistOwnsItsKeys()
{
    crypt_world Crypt = DuelistCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    Check(RoleHasKit(PlayerRole_Duelist));
    // NOTE(zoubir): the two base spells only, before any point
    ResetRoleTalents(Slot);
    u32 Allowed = RunAllowedButtons(AppState, Slot, 0);
    u32 Main = PlayerButton_Shockwave | PlayerButton_Attack;
    Check((Allowed & Main) == Main);
    Check((Allowed & PlayerButton_Launch) &&
          !(Allowed & (PlayerButton_Push | PlayerButton_Slam | PlayerButton_Kunai | PlayerButton_Cast)));
    GrantClassSpells(Slot);
    SetClassTalentRank(Slot, DuelistTalent_Footwork, 2);
    SetClassTalentRank(Slot, DuelistTalent_PerfectForm, 1);
    Allowed = RunAllowedButtons(AppState, Slot, 0);
    Check(Allowed & PlayerButton_Kunai);
    Check((Allowed & PlayerButton_Slam) && !(Allowed & PlayerButton_Cast));
    Check(DuelistKeyWindsUp(4) && !DuelistKeyWindsUp(1) && !DuelistKeyWindsUp(6));
    Check(RoleSpellOnButton(AppState, Slot->Entity, PlayerButton_Attack) == &DuelistSpells[6]);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Tempo for a spell that lands on another key than the
// last; Thrust after Thrust gains nothing, nor a Thrust at nothing
internal void
TestTempoWeave()
{
    crypt_world Crypt = DuelistCrypt();
    player_slot *Slot = &Crypt.AppState->Players[0];
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Slot->ClassMeter == 0);
    world_entity *Foe = DuelistDummy(&Crypt, V3(60.f, 0.f, 0.f));
    DuelistReady(&Crypt, 6);
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Foe->Hp < 2000.f);
    Check(Slot->ClassMeter == 0);
    DuelistReady(&Crypt, 0);
    PressAt(&Crypt, 0, PlayerButton_Launch, Foe);
    Check(Slot->ClassMeter == 1 && Slot->RoleCooldowns[0] > 0.f);
    DuelistReady(&Crypt, 6);
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Slot->ClassMeter == 2);
    DuelistReady(&Crypt, 6);
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Slot->ClassMeter == 2);
    DuelistReady(&Crypt, 1);
    PressOnce(&Crypt, 0, PlayerButton_Push);
    Check(Slot->ClassMeter == 3);
    DuelistReady(&Crypt, 6);
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Slot->ClassMeter == 4);
    // NOTE(zoubir): Tempo stops at five, and every stack is more damage
    Slot->ClassMeter = 9;
    AddTempo(Slot, 1);
    Check(Slot->ClassMeter == DUELIST_MOST_TEMPO);
    Slot->ClassMeter = 0;
    float Plain = DuelistDealtScale(Slot, Foe);
    Slot->ClassMeter = 5;
    float Full = DuelistDealtScale(Slot, Foe);
    Check(Full > Plain * (1.f + 5.f * TEMPO_SHARE) - 0.001f && Full < Plain * (1.f + 5.f * TEMPO_SHARE) + 0.001f);
    TickCrypt(&Crypt, 2);
    Check(Slot->ClassFlags & DUELIST_FLAG_TEMPO);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): a hit Riposte did not stop takes two stacks, and every
// client sees them break; with none there is nothing to lose
internal void
TestTempoLostToHits()
{
    crypt_world Crypt = DuelistCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Foe = DuelistDummy(&Crypt, V3(60.f, 0.f, 0.f));
    Slot->ClassMeter = 5;
    float Hp = Slot->Entity->Hp;
    AppState->Events.Count = 0;
    HitDuelist(&Crypt, Foe, 10.f);
    Check(Slot->Entity->Hp < Hp);
    Check(Slot->ClassMeter == 3);
    bool32 Broke = false;
    for(u32 Index = 0; Index < AppState->Events.Count; Index++)
    {
        sim_event *Event = &AppState->Events.Events[Index];
        Broke |= Event->Type == SimEvent_Burst &&
            Event->Burst == ClassBurst(SimBurst_DuelistFirst, DuelistBurst_Break) &&
            DuelistBurstVariant(Event->Position) == 5;
    }
    Check(Broke);
    HitDuelist(&Crypt, Foe, 10.f);
    Check(Slot->ClassMeter == 1);
    HitDuelist(&Crypt, Foe, 10.f);
    Check(Slot->ClassMeter == 0);
    HitDuelist(&Crypt, Foe, 10.f);
    Check(Slot->ClassMeter == 0);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): out of a fight Tempo holds a few seconds after the last
// hit dealt, then goes a stack a second, and the flag says so
internal void
TestTempoFades()
{
    crypt_world Crypt = DuelistCrypt();
    player_slot *Slot = &Crypt.AppState->Players[0];
    Check(!Crypt.AppState->Dungeon->FightingRoom);
    Slot->ClassMeter = 4;
    Slot->Duelist.IdleSeconds = 0.f;
    TickCrypt(&Crypt, (u32)(60.f * (DUELIST_IDLE_SECONDS - 1.f)));
    Check(Slot->ClassMeter == 4);
    TickCrypt(&Crypt, 90);
    Check(Slot->ClassFlags & DUELIST_FLAG_FADING);
    Check(Slot->ClassMeter == 4 || Slot->ClassMeter == 3);
    TickCrypt(&Crypt, (u32)(60.f * 5.f * DUELIST_FADE_SECONDS));
    Check(Slot->ClassMeter == 0);
    Check(!(Slot->ClassFlags & DUELIST_FLAG_FADING));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): on guard a blow does nothing, neither damage nor shove;
// the first is parried (Tempo, Riposte back soon) and the attacker
// countered once on the next tick; the next blow in the same guard is
// cancelled too without a second counter; once the guard is down a blow
// lands and takes Tempo
internal void
TestRiposte()
{
    crypt_world Crypt = DuelistCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Player = Slot->Entity;
    world_entity *Foe = DuelistDummy(&Crypt, V3(80.f, 0.f, 0.f));
    world_entity *Near = DuelistDummy(&Crypt, V3(-50.f, 0.f, 0.f));
    Slot->ClassMeter = 1;
    PressOnce(&Crypt, 0, PlayerButton_Push);
    Check(Slot->ClassFlags & DUELIST_FLAG_GUARD);
    Check(Slot->ClassMeter == 2);
    Check(Slot->RoleCooldowns[1] > RIPOSTE_COOLDOWN - 0.5f);
    float Hp = Player->Hp;
    v2 Speed = Player->Velocity.XY;
    HitDuelist(&Crypt, Foe, 30.f);
    Check(Player->Hp == Hp);
    Check(LengthSq(Player->Velocity.XY - Speed) < 0.01f);
    Check(Slot->ClassMeter == 2 + COUNTER_TEMPO);
    Check(Slot->RoleCooldowns[1] <= RIPOSTE_READY_SECONDS);
    Check(Slot->Duelist.CounterDue && Foe->Hp == 2000.f);
    // NOTE(zoubir): the counter goes to the attacker, not the nearer foe
    Foe->StatusTimers[StatusEffect_Stunned] = 0.f;
    TickCrypt(&Crypt, 1);
    Check(Slot->ClassFlags & DUELIST_FLAG_PARRIED);
    float Countered = 2000.f - Foe->Hp;
    Check(Countered > COUNTER_DAMAGE);
    Check(Near->Hp == 2000.f);
    Check(Foe->StatusTimers[StatusEffect_Stunned] > COUNTER_STUN - 0.1f);
    // NOTE(zoubir): a second blow in the guard: cancelled, no counter
    HitDuelist(&Crypt, Foe, 30.f);
    TickCrypt(&Crypt, 1);
    Check(Player->Hp == Hp);
    Check(2000.f - Foe->Hp == Countered);
    Check(Slot->ClassMeter == 2 + COUNTER_TEMPO);
    // NOTE(zoubir): damage that is not a blow (a burn) is cancelled too
    Check(DuelistTakenScale(Slot, Player) == 0.f);
    TickCrypt(&Crypt, (u32)(60.f * RIPOSTE_GUARD_SECONDS));
    Check(!(Slot->ClassFlags & (DUELIST_FLAG_GUARD | DUELIST_FLAG_PARRIED)));
    HitDuelist(&Crypt, Foe, 30.f);
    Check(Player->Hp < Hp);
    Check(Slot->ClassMeter == COUNTER_TEMPO);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): a guard that parries nothing keeps the whole cooldown
internal void
TestRiposteWhiff()
{
    crypt_world Crypt = DuelistCrypt();
    player_slot *Slot = &Crypt.AppState->Players[0];
    PressOnce(&Crypt, 0, PlayerButton_Push);
    Check(Slot->Duelist.GuardSeconds > 0.f);
    TickCrypt(&Crypt, (u32)(60.f * RIPOSTE_GUARD_SECONDS) + 2);
    Check(Slot->Duelist.GuardSeconds == 0.f);
    Check(!(Slot->ClassFlags & DUELIST_FLAG_GUARD));
    Check(Slot->RoleCooldowns[1] > RIPOSTE_COOLDOWN - RIPOSTE_GUARD_SECONDS - 0.2f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): online, a client predicting its own Duelist starts the
// Heartseeker's wind-up but strikes nothing, raises no guard, and never
// parries or loses Tempo: a blow on it is the server's to count
internal void
TestPredictedDuelist()
{
    crypt_world Crypt = DuelistCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Player = Slot->Entity;
    world_entity *Foe = DuelistDummy(&Crypt, V3(60.f, 0.f, 0.f));
    Slot->ClassMeter = 3;
    Slot->Predicted = true;
    PressOnce(&Crypt, 0, PlayerButton_Push);
    Check(Slot->Duelist.GuardSeconds == 0.f);
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Foe->Hp == 2000.f);
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(Player->CastSpell == PlayerSpell_DuelistB);
    TickCrypt(&Crypt, 40);
    Check(Foe->Hp == 2000.f);
    Slot->Duelist.GuardSeconds = 1.f;
    Check(!DuelistParriesHit(AppState, Player, Foe, 10.f));
    Check(Slot->ClassMeter == 3 && !Slot->Duelist.Parried);
    Slot->Predicted = false;
    Slot->Duelist.GuardSeconds = 0.f;
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): what Heartseeker with Tempo did to a fresh foe in front,
// pressed on a repeat key so it gains nothing
internal float
HeartseekerWith(crypt_world *Crypt, u32 Tempo, float HealthShare = 1.f)
{
    player_slot *Slot = &Crypt->AppState->Players[0];
    world_entity *Foe = DuelistDummy(Crypt, V3(60.f, 0.f, 0.f));
    Foe->Hp = HealthShare * Foe->MaxHp;
    float Before = Foe->Hp;
    Slot->ClassMeter = (u8)Tempo;
    Slot->Duelist.LastKey = 4 + 1;
    Slot->RoleCooldowns[4] = 0.f;
    PressAt(Crypt, 0, PlayerButton_Shockwave, Foe);
    Check(Slot->Entity->CastSpell == PlayerSpell_DuelistB);
    TickCrypt(Crypt, (u32)(60.f * PlayerSpells[PlayerSpell_DuelistB].CastTime) + 2);
    Check(Slot->ClassMeter == Tempo);
    float Result = Before - Foe->Hp;
    KillEntity(Crypt->AppState, &Crypt->AppState->World, Foe, 0);
    return Result;
}

internal void
TestHeartseeker()
{
    crypt_world Crypt = DuelistCrypt();
    player_slot *Slot = &Crypt.AppState->Players[0];
    // NOTE(zoubir): nobody in reach: no wind-up, no cooldown
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(Slot->Entity->CastSpell == 0 && Slot->RoleCooldowns[4] == 0.f);
    float None = HeartseekerWith(&Crypt, 0);
    float Five = HeartseekerWith(&Crypt, 5);
    float Expected = (HEARTSEEKER_DAMAGE + 5.f * HEARTSEEKER_PER_TEMPO) * (1.f + 5.f * TEMPO_SHARE) /
        HEARTSEEKER_DAMAGE;
    Check(None > 0.f && Five > 0.99f * Expected * None && Five < 1.01f * Expected * None);
    // NOTE(zoubir): a hurt foe takes half again, below 30% only
    float Hurt = HeartseekerWith(&Crypt, 0, 0.25f);
    Check(Hurt > 0.99f * HEARTSEEKER_LOW_SCALE * None && Hurt < 1.01f * HEARTSEEKER_LOW_SCALE * None);
    float Bruised = HeartseekerWith(&Crypt, 0, 0.4f);
    Check(Bruised > 0.99f * None && Bruised < 1.01f * None);
    // NOTE(zoubir): Precision: from 45%
    SetClassTalentRank(Slot, DuelistTalent_Precision, 1);
    float Precise = HeartseekerWith(&Crypt, 0, 0.4f);
    Check(Precise > 0.99f * HEARTSEEKER_LOW_SCALE * None);
    SetClassTalentRank(Slot, DuelistTalent_Precision, 0);
    // NOTE(zoubir): a new key gains a stack when it lands
    world_entity *Foe = DuelistDummy(&Crypt, V3(60.f, 0.f, 0.f));
    Slot->ClassMeter = 2;
    Slot->Duelist.LastKey = 6 + 1;
    Slot->RoleCooldowns[4] = 0.f;
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
    u32 WindUp = (u32)(60.f * PlayerSpells[PlayerSpell_DuelistB].CastTime) + 2;
    TickCrypt(&Crypt, WindUp);
    Check(Slot->ClassMeter == 3);
    Check(Slot->RoleCooldowns[4] > HEARTSEEKER_COOLDOWN - 1.f);
    // NOTE(zoubir): the foe pressed on is held: it backs off past the
    // reach during the wind-up and is still struck, not the one in front
    world_entity *Other = DuelistDummy(&Crypt, V3(50.f, 30.f, 0.f));
    Slot->RoleCooldowns[4] = 0.f;
    float Before = Foe->Hp;
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
    Check(Slot->Entity->CastSpell == PlayerSpell_DuelistB);
    Foe->Position.X = Slot->Entity->Position.X + HEARTSEEKER_REACH + 40.f;
    Foe->Position.Y = Slot->Entity->Position.Y;
    TickCrypt(&Crypt, WindUp);
    Check(Foe->Hp < Before && Other->Hp == 2000.f);
    // NOTE(zoubir): it gets clean away: the rapier pierces the air and
    // the key is ready again
    Slot->RoleCooldowns[4] = 0.f;
    Foe->Position.X = Slot->Entity->Position.X + 60.f;
    KillEntity(Crypt.AppState, &Crypt.AppState->World, Other, 0);
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
    Foe->Position.X = Slot->Entity->Position.X + 600.f;
    Before = Foe->Hp;
    TickCrypt(&Crypt, WindUp);
    Check(Foe->Hp == Before && Slot->RoleCooldowns[4] == 0.f);
    Check(DuelistBurstVariant(DuelistBurstSpot(V3(1.f, 2.f, 16.f), 21)) == 21);
    Check(DuelistBurstPlace(DuelistBurstSpot(V3(1.f, 2.f, 16.f), 21)).Z == 16.f);
    DestroyCryptWorld(&Crypt);
}

#include "duelist/talents.cpp"

// NOTE(zoubir): Feint hastes the Duelist; the next blow in its time
// misses whole and gives a Tempo, without a counter; the one after lands;
// a Feint left to run out dodges nothing
internal void
TestFeint()
{
    crypt_world Crypt = DuelistCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Player = Slot->Entity;
    world_entity *Foe = DuelistDummy(&Crypt, V3(80.f, 0.f, 0.f));
    Slot->ClassMeter = 1;
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    Check(Slot->RoleCooldowns[2] > FEINT_COOLDOWN - 0.5f);
    Check(Slot->ClassFlags & DUELIST_FLAG_FEINT);
    Check(HasStatus(Player, StatusEffect_Hasted));
    u32 Tempo = Slot->ClassMeter;
    float Hp = Player->Hp;
    HitDuelist(&Crypt, Foe, 30.f);
    Check(Player->Hp == Hp);
    Check(Slot->ClassMeter == Tempo + FEINT_TEMPO);
    Check(!Slot->Duelist.CounterDue);
    TickCrypt(&Crypt, 1);
    Check(!(Slot->ClassFlags & DUELIST_FLAG_FEINT));
    HitDuelist(&Crypt, Foe, 30.f);
    Check(Player->Hp < Hp);
    // NOTE(zoubir): run out, it dodges nothing
    DuelistReady(&Crypt, 2);
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    TickCrypt(&Crypt, (u32)(60.f * FEINT_SECONDS) + 2);
    Hp = Player->Hp;
    HitDuelist(&Crypt, Foe, 30.f);
    Check(Player->Hp < Hp);
    DestroyCryptWorld(&Crypt);
}

internal void
RunDuelistTests()
{
    TestDuelistOwnsItsKeys();
    TestTempoWeave();
    TestTempoLostToHits();
    TestTempoFades();
    TestRiposte();
    TestRiposteWhiff();
    TestFeint();
    TestPredictedDuelist();
    TestHeartseeker();
    TestPerfectForm();
    TestLungeAndFootwork();
    TestBait();
    TestCrescendo();
    TestFlurryAndMasterstroke();
}
