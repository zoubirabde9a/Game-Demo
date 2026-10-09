/* What the Shadowblade's spells leave behind (role_kits/shadowblade.cpp), once a
   tick from UpdateShadowbladeEffects: Twin Strike's second cut, the
   critical strike and Shadow Dance running out, combo points fading out
   of a fight, and poison ticking on what its daggers cut. Each
   Shadowblade's ClassFlags are set here from its state, so
   clients draw what the server has. */

// NOTE(zoubir): each poison row bites its monster every BLADE_POISON_TICK
// for its Shadowblade, and goes when the monster or the time does
internal void
UpdatePoisons(app_state *AppState, shadowblade_run *Run, float DeltaTime)
{
    world *World = &AppState->World;
    for(u32 Index = 0; Index < SHADOWBLADE_POISONS; Index++)
    {
        shadowblade_poison *Poison = &Run->Poisons[Index];
        if (Poison->Seconds <= 0.f)
        {
            continue;
        }
        world_entity *Monster = FindMonsterBySerial(World, Poison->Slot, Poison->Serial);
        world_entity *Caster = Poison->By < MAX_PLAYERS ? AppState->Players[Poison->By].Entity : 0;
        if (!Monster || Monster->Hp <= 0.f || !Caster)
        {
            *Poison = {};
            continue;
        }
        Poison->Seconds = Maximum(0.f, Poison->Seconds - DeltaTime);
        Poison->TickTimer += DeltaTime;
        if (Poison->TickTimer >= BLADE_POISON_TICK)
        {
            Poison->TickTimer -= BLADE_POISON_TICK;
            DamageEntity(AppState, World, Monster, Poison->PerSecond * BLADE_POISON_TICK, Caster);
        }
    }
}

// NOTE(zoubir): one Shadowblade's timers, its fading points and its flags
internal void
UpdateShadowbladeSlot(app_state *AppState, dungeon_run *Run, player_slot *Slot, float DeltaTime)
{
    shadowblade_slot *Blade = &Slot->Shadowblade;
    world_entity *Player = Slot->Entity;
    bool32 Alive = Player && Player->IsPresent && !IsDeadPlayer(Player);
    if (Blade->CutDelay > 0.f)
    {
        Blade->CutDelay -= DeltaTime;
        if (Blade->CutDelay <= 0.f && Alive)
        {
            SecondTwinCut(AppState, Slot, Player);
        }
    }
    Blade->CritSeconds = Maximum(0.f, Blade->CritSeconds - DeltaTime);
    Blade->DanceSeconds = Maximum(0.f, Blade->DanceSeconds - DeltaTime);
    Blade->GuardSeconds = Maximum(0.f, Blade->GuardSeconds - DeltaTime);
    Blade->IdleSeconds += DeltaTime;
    // NOTE(zoubir): out of a fight a moment after the last strike, or a
    // long while without one in it, the points go one by one
    bool32 Fading = Slot->ClassMeter > 0 &&
        (Run->FightingRoom ? Blade->IdleSeconds > 3.f * SHADOWBLADE_IDLE_SECONDS :
         Blade->IdleSeconds > SHADOWBLADE_IDLE_SECONDS);
    if (Fading)
    {
        Blade->FadeTimer += DeltaTime;
        if (Blade->FadeTimer >= SHADOWBLADE_FADE_SECONDS)
        {
            Blade->FadeTimer -= SHADOWBLADE_FADE_SECONDS;
            Slot->ClassMeter--;
        }
    }
    else
    {
        Blade->FadeTimer = 0.f;
    }
    SetShadowbladeFlags(Slot, Fading);
}

// NOTE(zoubir): once a tick, from UpdateRoleEffects: what the class's
// spells left behind, for every player of the class
internal void
UpdateShadowbladeEffects(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    shadowblade_run *Blades = &Run->Shadowblade;
    UpdatePoisons(AppState, Blades, DeltaTime);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Role == PlayerRole_Shadowblade)
        {
            UpdateShadowbladeSlot(AppState, Run, Slot, DeltaTime);
        }
    }
}
