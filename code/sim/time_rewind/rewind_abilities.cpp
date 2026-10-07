/* Rewind abilities: the three keys, and each rewind's way from the press
   to the end of its playback.

   Cast (REWIND_CAST_SECONDS): the caster's wind-up, a player cast like
   any other spell's (sim/player_casts.cpp): the caster walks slower and
   shows the cast pose. Whatever cuts the cast (a dash, a blink, a stun,
   death, being frozen by someone else's rewind) ends the rewind, and the
   cooldown stays spent.

   Hold (REWIND_HOLD_SECONDS): what the rewind takes is frozen: the
   caster alone, everything within the bubble around the caster, or the
   whole world. Frozen things neither act nor can be hurt or shoved;
   outside a bubble the world goes on around them. A world rewind ends
   every other rewind under way, whatever its phase.

   Playback (REWIND_PLAYBACK_SECONDS): each tick shows the history frame
   at the cursor, which runs from the hold's start back REWIND_SECONDS at
   REWIND_PLAYBACK_SPEED. The last tick lands exactly on the frame
   REWIND_SECONDS back, and what was frozen goes on from there. */

// NOTE(zoubir): the wind-up each rewind_kind casts (sim/player_casts.cpp)
inline player_spell
RewindSpell(rewind_kind Kind)
{
    player_spell Result = (player_spell)(PlayerSpell_RewindSelf + Kind);
    return Result;
}
static_assert(PlayerSpell_RewindSelf + RewindKind_Count == PlayerSpell_Count,
              "one spell per rewind, in rewind_kind order");

global_variable rewind_ability RewindAbilities[RewindKind_Count] =
{
    // NOTE(zoubir): Self (T): the caster goes back to where it was 2 s
    // ago, with the health, speed and cooldowns it had
    {PlayerButton_RewindSelf, 8.f, 0.f},
    // NOTE(zoubir): Bubble (G): everything within 160 units of the caster,
    // friend or foe, monster or fireball, goes back together
    {PlayerButton_RewindBubble, 14.f, 160.f},
    // NOTE(zoubir): World (V): everyone, everywhere
    {PlayerButton_RewindWorld, 30.f, 0.f},
};

// NOTE(zoubir): every rewind's button (the client leaves them to the
// server, client/prediction.cpp)
internal u32
PlayerRewindButtons()
{
    u32 Result = 0;
    for(u32 Index = 0; Index < RewindKind_Count; Index++)
    {
        Result |= RewindAbilities[Index].Button;
    }
    return Result;
}

// NOTE(zoubir): frozen by a rewind's hold or playback: skips its update
// (simulate.cpp) and takes no hits (entity.cpp, hit.cpp)
internal bool32
IsTimeLocked(app_state *AppState, world_entity *Entity)
{
    time_rewind *Rewind = AppState->Rewind;
    bool32 Result = Rewind && Entity->RewindSerial &&
        Entity->ID < ArrayCount(Rewind->LockedSerial) &&
        Rewind->LockedSerial[Entity->ID] == Entity->RewindSerial;
    return Result;
}

// NOTE(zoubir): can take no damage, shove or stun: frozen by a rewind, or
// anything while a world rewind holds the world (entity.cpp, hit.cpp)
internal bool32
IsRewindInvulnerable(app_state *AppState, world_entity *Entity)
{
    bool32 Result = IsTimeLocked(AppState, Entity) ||
        (AppState->Rewind && AppState->Rewind->WorldFrozen);
    return Result;
}

// NOTE(zoubir): called from UsePlayerAbilities (player_update.cpp). A
// client predicting its own player runs the wind-up (the slowdown and the
// pose) but leaves the rewind itself to the server
internal void
UseRewindAbilities(app_state *AppState, world_entity *Player,
                   player_input *Input, float DeltaTime)
{
    for(u32 Index = 0; Index < RewindKind_Count; Index++)
    {
        Player->RewindCooldowns[Index] =
            Maximum(0.f, Player->RewindCooldowns[Index] - DeltaTime);
    }
    time_rewind *Rewind = AppState->Rewind;
    bool32 Predicted = IsPredictedPlayer(AppState, Player);
    if ((!Rewind && !Predicted) || Player->PlayerIndex >= MAX_PLAYERS ||
        IsPlayerCasting(Player))
    {
        return;
    }
    rewind_cast *Cast = Predicted ? 0 : &Rewind->Casts[Player->PlayerIndex];
    if (Cast && Cast->Phase != RewindPhase_None)
    {
        return;
    }
    for(u32 Index = 0; Index < RewindKind_Count; Index++)
    {
        rewind_ability *Ability = &RewindAbilities[Index];
        if (WasPressed(Input, Ability->Button) &&
            CanUseEarly(Player->RewindCooldowns[Index]))
        {
            Player->RewindCooldowns[Index] += Ability->Cooldown *
                PlayerCooldownScale(AppState, Player, Ability->Button);
            StartPlayerCast(Player, RewindSpell((rewind_kind)Index),
                            GetPlayerAim(Player));
            if (!Cast)
            {
                break;
            }
            Cast->Kind = (rewind_kind)Index;
            Cast->Phase = RewindPhase_Cast;
            Cast->PhaseLeft = REWIND_CAST_SECONDS;
            Cast->Centre = Player->Position;
            Cast->Radius = Ability->Radius;
            Cast->AffectedCount = 0;
            EmitSound(&AppState->Events, AssetType_SfxRewind, Player->Position);
            break;
        }
    }
}

// NOTE(zoubir): the caster's wind-up is done (player_update/casts.cpp);
// UpdateRewindCast starts the hold later this tick
internal void
FinishRewindWindUp(app_state *AppState, world_entity *Player, rewind_kind Kind)
{
    time_rewind *Rewind = AppState->Rewind;
    if (Rewind && Player->PlayerIndex < MAX_PLAYERS)
    {
        rewind_cast *Cast = &Rewind->Casts[Player->PlayerIndex];
        if (Cast->Phase == RewindPhase_Cast && Cast->Kind == Kind)
        {
            Cast->PhaseLeft = 0.f;
        }
    }
}

// NOTE(zoubir): ends the rewind where it is: what it froze goes on
internal void
EndRewindCast(time_rewind *Rewind, rewind_cast *Cast)
{
    for(u32 Index = 0; Index < Cast->AffectedCount; Index++)
    {
        u32 ID = Cast->AffectedID[Index];
        if (Cast->AffectedSerial[Index] &&
            Rewind->LockedSerial[ID] == Cast->AffectedSerial[Index])
        {
            Rewind->LockedSerial[ID] = 0;
        }
    }
    if (Cast->Kind == RewindKind_World && Cast->Phase >= RewindPhase_Hold)
    {
        Rewind->WorldFrozen = false;
    }
    Cast->Phase = RewindPhase_None;
    Cast->PhaseLeft = 0.f;
    Cast->AffectedCount = 0;
}

inline void
TakeIntoRewind(time_rewind *Rewind, rewind_cast *Cast, world_entity *Entity)
{
    u32 Serial = GetRewindSerial(Rewind, Entity);
    if (Cast->AffectedCount < REWIND_MAX_AFFECTED &&
        Entity->ID < ArrayCount(Rewind->LockedSerial) &&
        !Rewind->LockedSerial[Entity->ID])
    {
        Cast->AffectedID[Cast->AffectedCount] = Entity->ID;
        Cast->AffectedSerial[Cast->AffectedCount] = Serial;
        Cast->AffectedCount++;
        Rewind->LockedSerial[Entity->ID] = Serial;
    }
}

// NOTE(zoubir): the cast is done: freeze what the rewind takes
internal void
BeginRewindHold(app_state *AppState, time_rewind *Rewind, world *World,
                rewind_cast *Cast, world_entity *Caster)
{
    Cast->Phase = RewindPhase_Hold;
    Cast->PhaseLeft = REWIND_HOLD_SECONDS;
    Cast->HoldClock = Rewind->Clock;
    Cast->Cursor = Rewind->Clock;
    Cast->Centre = Caster->Position;
    Cast->AffectedCount = 0;
    switch(Cast->Kind)
    {
        case RewindKind_Self:
        {
            TakeIntoRewind(Rewind, Cast, Caster);
        } break;

        case RewindKind_Bubble:
        {
            // NOTE(zoubir): the caster first, so it is taken (and cannot be
            // hurt) however crowded the bubble is
            TakeIntoRewind(Rewind, Cast, Caster);
            for(u32 ID = 0; ID < World->EntityCount; ID++)
            {
                world_entity *Entity = &World->Entities[ID];
                if (IsRewindRecorded(Entity) &&
                    LengthSq(Entity->Position.XY - Cast->Centre.XY) <=
                    Square(Cast->Radius))
                {
                    TakeIntoRewind(Rewind, Cast, Entity);
                }
            }
        } break;

        case RewindKind_World:
        {
            for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
            {
                rewind_cast *Other = &Rewind->Casts[SlotIndex];
                if (Other != Cast && Other->Phase != RewindPhase_None)
                {
                    EndRewindCast(Rewind, Other);
                }
            }
            Rewind->WorldFrozen = true;
        } break;

        case RewindKind_Count:
        {
            InvalidCodePath;
        } break;
    }
    EmitSound(&AppState->Events, AssetType_SfxBlink, Cast->Centre);
}

// NOTE(zoubir): one tick of one player's rewind
internal void
UpdateRewindCast(app_state *AppState, time_rewind *Rewind, world *World,
                 memory_arena *Arena, u32 SlotIndex, float DeltaTime)
{
    rewind_cast *Cast = &Rewind->Casts[SlotIndex];
    if (Cast->Phase == RewindPhase_None)
    {
        return;
    }
    player_slot *Slot = &AppState->Players[SlotIndex];
    world_entity *Caster = Slot->Active ? Slot->Entity : 0;
    switch(Cast->Phase)
    {
        // NOTE(zoubir): the caster's own cast counts the wind-up down
        // (player_update/casts.cpp); PhaseLeft follows it for the clients'
        // sigil, and FinishRewindWindUp sets it to 0 when it is done
        case RewindPhase_Cast:
        {
            bool32 Casting = Caster && Caster->IsPresent &&
                Caster->CastSpell == (u32)RewindSpell(Cast->Kind);
            if (Caster && Cast->PhaseLeft <= 0.f)
            {
                BeginRewindHold(AppState, Rewind, World, Cast, Caster);
                break;
            }
            if (!Casting || IsDeadPlayer(Caster) ||
                IsTimeLocked(AppState, Caster))
            {
                if (Casting)
                {
                    CancelPlayerCast(Caster);
                }
                EndRewindCast(Rewind, Cast);
                break;
            }
            Cast->Centre = Caster->Position;
            Cast->PhaseLeft = Caster->CastLeft;
        } break;

        case RewindPhase_Hold:
        {
            Cast->PhaseLeft -= DeltaTime;
            if (Cast->PhaseLeft <= 0.f)
            {
                Cast->Phase = RewindPhase_Playback;
                Cast->PhaseLeft = REWIND_PLAYBACK_SECONDS;
                EmitSound(&AppState->Events, AssetType_SfxRewind, Cast->Centre);
            }
        } break;

        case RewindPhase_Playback:
        {
            Cast->PhaseLeft -= DeltaTime;
            bool32 Done = Cast->PhaseLeft <= 0.f;
            float Shown = REWIND_PLAYBACK_SECONDS - Maximum(0.f, Cast->PhaseLeft);
            Cast->Cursor = Done ? Cast->HoldClock - REWIND_SECONDS :
                Cast->HoldClock - REWIND_PLAYBACK_SPEED * Shown;
            rewind_frame *Frame = FindRewindFrame(Rewind, Cast->Cursor);
            if (Frame)
            {
                if (Cast->Kind == RewindKind_World)
                {
                    RestoreRewindWorld(AppState, Rewind, World, Arena, Frame);
                }
                else
                {
                    RestoreRewindCast(AppState, Rewind, World, Arena, Cast, Frame);
                }
            }
            if (Done)
            {
                if (Cast->Kind == RewindKind_World && Frame)
                {
                    TruncateRewindHistory(Rewind, Frame);
                }
                EmitSound(&AppState->Events, AssetType_SfxBlink, Cast->Centre);
                // NOTE(zoubir): unfrozen first, so what it landed on counts
                rewind_kind Kind = Cast->Kind;
                u32 Count = Cast->AffectedCount;
                EndRewindCast(Rewind, Cast);
                for(u32 Index = 0; Index < (Kind == RewindKind_World ? World->EntityCount : Count); Index++)
                {
                    u32 ID = Kind == RewindKind_World ? Index : Cast->AffectedID[Index];
                    world_entity *Entity = &World->Entities[ID];
                    if (Kind == RewindKind_World ||
                        (Cast->AffectedSerial[Index] &&
                         Entity->RewindSerial == Cast->AffectedSerial[Index]))
                    {
                        SettleRewoundUnit(AppState, World, Arena, Entity);
                    }
                }
            }
        } break;

        case RewindPhase_None:
        case RewindPhase_Count:
        {
        } break;
    }
}

// NOTE(zoubir): every tick, after everything else has moved
internal void
UpdateRewinds(app_state *AppState, time_rewind *Rewind, world *World,
              memory_arena *Arena, float DeltaTime)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        UpdateRewindCast(AppState, Rewind, World, Arena, SlotIndex, DeltaTime);
    }
}
