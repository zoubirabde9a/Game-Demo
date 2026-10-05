/* Rewind abilities: the three keys, and each rewind's way from the press
   to the end of its playback.

   Cast (REWIND_CAST_SECONDS): the caster winds up and may still walk.
   Being stunned, killed or frozen by someone else's rewind ends it, and
   the cooldown stays spent.

   Hold (REWIND_HOLD_SECONDS): what the rewind takes is frozen: the
   caster alone, everything within the bubble around the caster, or the
   whole world. Frozen things neither act nor can be hurt or shoved;
   outside a bubble the world goes on around them. A world rewind ends
   every other rewind under way, whatever its phase.

   Playback (REWIND_PLAYBACK_SECONDS): each tick shows the history frame
   at the cursor, which runs from the hold's start back REWIND_SECONDS at
   REWIND_PLAYBACK_SPEED. The last tick lands exactly on the frame
   REWIND_SECONDS back, and what was frozen goes on from there. */

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

// NOTE(zoubir): called from UsePlayerAbilities (player_update.cpp). Only
// the authority starts a rewind; a client predicting its own player
// leaves it to the server
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
    if (!Rewind || IsPredictedPlayer(AppState, Player) ||
        Player->PlayerIndex >= MAX_PLAYERS)
    {
        return;
    }
    rewind_cast *Cast = &Rewind->Casts[Player->PlayerIndex];
    if (Cast->Phase != RewindPhase_None)
    {
        return;
    }
    for(u32 Index = 0; Index < RewindKind_Count; Index++)
    {
        rewind_ability *Ability = &RewindAbilities[Index];
        if (WasPressed(Input, Ability->Button) &&
            CanUseEarly(Player->RewindCooldowns[Index]))
        {
            Player->RewindCooldowns[Index] += Ability->Cooldown;
            Cast->Kind = (rewind_kind)Index;
            Cast->Phase = RewindPhase_Cast;
            Cast->PhaseLeft = REWIND_CAST_SECONDS;
            Cast->Centre = Player->Position;
            Cast->Radius = Ability->Radius;
            Cast->AffectedCount = 0;
            EmitSound(&AppState->Events, AssetType_FireCast, Player->Position);
            break;
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
    EmitSound(&AppState->Events, AssetType_Dash, Cast->Centre);
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
    Cast->PhaseLeft -= DeltaTime;
    switch(Cast->Phase)
    {
        case RewindPhase_Cast:
        {
            if (!Caster || !Caster->IsPresent || IsDeadPlayer(Caster) ||
                HasStatus(Caster, StatusEffect_Stunned) ||
                IsTimeLocked(AppState, Caster))
            {
                EndRewindCast(Rewind, Cast);
                break;
            }
            Cast->Centre = Caster->Position;
            if (Cast->PhaseLeft <= 0.f)
            {
                BeginRewindHold(AppState, Rewind, World, Cast, Caster);
            }
        } break;

        case RewindPhase_Hold:
        {
            if (Cast->PhaseLeft <= 0.f)
            {
                Cast->Phase = RewindPhase_Playback;
                Cast->PhaseLeft = REWIND_PLAYBACK_SECONDS;
                EmitSound(&AppState->Events, AssetType_FireCast, Cast->Centre);
            }
        } break;

        case RewindPhase_Playback:
        {
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
                EmitSound(&AppState->Events, AssetType_Dash, Cast->Centre);
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
