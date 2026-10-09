/* What the Duelist's spells leave behind (role_kits/duelist.cpp), once a
   tick from UpdateDuelistEffects: Perfect Form's second Thrust,
   Masterstroke's second strike, the counter a parry owes, the guard and
   Perfect Form running out, and Tempo fading out of a fight. Each
   Duelist's ClassFlags are set here from its state, so clients draw what
   the server has. */

// NOTE(zoubir): developer builds, offline: GAME_DUELIST=full gives a
// Duelist every talent and full Tempo, kept full; "full guard" keeps it
// on guard and "full form" keeps Perfect Form up, so a scripted screenshot
// (misc\screenshot.bat) can show the looks
internal void
ApplyDeveloperDuelist(player_slot *Slot)
{
#if APP_DEV
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
    char *Value = getenv("GAME_DUELIST");
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    if (Value && Value[0] == 'f')
    {
        GrantWholeClassTree(Slot);
        Slot->ClassMeter = DUELIST_MOST_TEMPO;
        if (strstr(Value, "guard"))
        {
            Slot->Duelist.GuardSeconds = Maximum(Slot->Duelist.GuardSeconds, 0.5f);
        }
        if (strstr(Value, "form"))
        {
            Slot->Duelist.FormSeconds = Maximum(Slot->Duelist.FormSeconds, 1.f);
        }
    }
#endif
}

// NOTE(zoubir): one Duelist's timers, its fading Tempo and its flags
internal void
UpdateDuelistSlot(app_state *AppState, dungeon_run *Run, player_slot *Slot, float DeltaTime)
{
    duelist_slot *Duel = &Slot->Duelist;
    world_entity *Player = Slot->Entity;
    bool32 Alive = Player && Player->IsPresent && !IsDeadPlayer(Player);
    ApplyDeveloperDuelist(Slot);
    if (Duel->EchoDelay > 0.f)
    {
        Duel->EchoDelay -= DeltaTime;
        if (Duel->EchoDelay <= 0.f && Alive)
        {
            ThrustAlong(AppState, Slot, Player, Duel->EchoDirection);
        }
    }
    if (Duel->MasterDelay > 0.f)
    {
        Duel->MasterDelay -= DeltaTime;
        if (Duel->MasterDelay <= 0.f && Alive)
        {
            MasterstrokeAgain(AppState, Slot, Player);
        }
    }
    if (Duel->CounterDue)
    {
        if (Alive)
        {
            StrikeCounter(AppState, Slot, Player);
        }
        Duel->CounterDue = false;
    }
    Duel->FeintSeconds = Alive ? Maximum(0.f, Duel->FeintSeconds - DeltaTime) : 0.f;
    Duel->GuardSeconds = Maximum(0.f, Duel->GuardSeconds - DeltaTime);
    if (Duel->GuardSeconds <= 0.f || !Alive)
    {
        Duel->GuardSeconds = 0.f;
        Duel->Parried = false;
    }
    Duel->FormSeconds = Maximum(0.f, Duel->FormSeconds - DeltaTime);
    Duel->IdleSeconds += DeltaTime;
    // NOTE(zoubir): out of a fight a moment after the last hit dealt, the
    // stacks go one by one; in a fight, or under Perfect Form, they hold
    bool32 Fading = Slot->ClassMeter > 0 && !Run->FightingRoom && Duel->FormSeconds <= 0.f &&
        Duel->IdleSeconds > DUELIST_IDLE_SECONDS;
    if (Fading)
    {
        Duel->FadeTimer += DeltaTime;
        if (Duel->FadeTimer >= DUELIST_FADE_SECONDS)
        {
            Duel->FadeTimer -= DUELIST_FADE_SECONDS;
            Slot->ClassMeter--;
        }
    }
    else
    {
        Duel->FadeTimer = 0.f;
    }
    SetDuelistFlags(Slot, Fading && Slot->ClassMeter > 0);
}

// NOTE(zoubir): once a tick, from UpdateClassEffects: what the class's
// spells left behind, for every player of the class. A client predicting
// its own Duelist leaves all of it to the server, which sends the Tempo
// and the flags back
internal void
UpdateDuelistEffects(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Role == PlayerRole_Duelist && Slot->Entity && !Slot->Predicted)
        {
            UpdateDuelistSlot(AppState, Run, Slot, DeltaTime);
        }
    }
}
