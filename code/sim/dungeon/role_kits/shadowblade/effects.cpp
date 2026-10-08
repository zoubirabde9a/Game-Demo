/* What the Shadowblade's spells leave behind (role_kits/shadowblade.cpp), once a
   tick from UpdateShadowbladeEffects: Twin Strike's second cut, the
   critical strike and Shadow Dance running out, combo points fading out
   of a fight, poison ticking on what a Shiv struck, and Smoke Bomb's
   clouds. Each Shadowblade's ClassFlags are set here from its state, so
   clients draw what the server has. */

// NOTE(zoubir): a cloud's party members drop off the threat of the
// monsters in it (a tank's threat is its job and stays), and take less
// while they stand in it, through the rally a tank's slam gives
// (DungeonScaleDamage reads it)
internal void
UpdateSmoke(app_state *AppState, dungeon_run *Run, shadowblade_smoke *Smoke, float DeltaTime)
{
    world *World = &AppState->World;
    Smoke->Seconds = Maximum(0.f, Smoke->Seconds - DeltaTime);
    u32 Inside[MAX_PLAYERS];
    u32 InsideCount = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Ally = LivingPlayerInSlot(AppState, SlotIndex);
        if (!Ally || Length(Ally->Position.XY - Smoke->Position.XY) > Smoke->Radius)
        {
            continue;
        }
        player_slot *AllySlot = &AppState->Players[SlotIndex];
        // NOTE(zoubir): a short rally, renewed every tick inside; a tank's
        // longer one keeps its length and takes the better share
        if (AllySlot->RallySeconds <= 0.25f)
        {
            AllySlot->RallyShare = Smoke->Share;
        }
        else
        {
            AllySlot->RallyShare = Maximum(AllySlot->RallyShare, Smoke->Share);
        }
        AllySlot->RallySeconds = Maximum(AllySlot->RallySeconds, 0.2f);
        if (RoleKindOf(AllySlot->Role) != RoleKind_Tank)
        {
            Inside[InsideCount++] = SlotIndex;
        }
    }
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            Length(Monster->Position.XY - Smoke->Position.XY) > Smoke->Radius)
        {
            continue;
        }
        threat_row *Row = FindThreatRow(&Run->Threat, World, Monster, false);
        for(u32 Index = 0; Row && Index < InsideCount; Index++)
        {
            Row->Threat[Inside[Index]] = 0.f;
        }
        if (Smoke->Slows)
        {
            ApplyStatus(Monster, StatusEffect_Slowed, 0.3f);
        }
    }
}

// NOTE(zoubir): each poison row bites its monster every SHIV_POISON_TICK
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
        if (Poison->TickTimer >= SHIV_POISON_TICK)
        {
            Poison->TickTimer -= SHIV_POISON_TICK;
            DamageEntity(AppState, World, Monster, Poison->PerSecond * SHIV_POISON_TICK, Caster);
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
    for(u32 Index = 0; Index < SHADOWBLADE_SMOKES; Index++)
    {
        if (Blades->Smokes[Index].Seconds > 0.f)
        {
            UpdateSmoke(AppState, Run, &Blades->Smokes[Index], DeltaTime);
        }
    }
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
