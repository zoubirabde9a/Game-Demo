/* Round break, under the duel rules (GameRules.RoundBreaks): a player's
   death ends the round. Every player gains a
   level, and everyone gets ROUND_BREAK_SECONDS to spend the point in the
   talent tree, which opens on its own (ui/round_break_view.cpp). While
   it lasts nobody can be hurt (every living player holds the respawn
   shield, IsDodging in entity.cpp) and the only key that works is jump,
   so players can walk around and open the talent panel but not fight.
   The dead respawn as it ends. Online the server sends the time left in
   every snapshot (net_snapshot.RoundBreak), so prediction blocks the
   same keys and the client draws the countdown (ui/round_break_view.cpp). */

#define ROUND_BREAK_SECONDS 10.f

// NOTE(zoubir): from KillEntity when a player dies; a death during the
// break (a pit still kills) waits for the same break to end
internal void
StartRoundBreak(app_state *AppState, player_slot *Victim)
{
    if (!GameRules.RoundBreaks)
    {
        return;
    }
    if (AppState->RoundBreak <= 0.f)
    {
        AppState->RoundBreak = ROUND_BREAK_SECONDS;
        for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
        {
            player_slot *Slot = &AppState->Players[SlotIndex];
            if (!Slot->Active)
            {
                continue;
            }
            if (Slot->Level < PLAYER_MAX_LEVEL)
            {
                AwardXp(AppState, Slot, XpToReach(Slot->Level + 1) - Slot->Xp);
            }
            // NOTE(zoubir): a spell winding up would go off into the break
            if (Slot->Entity)
            {
                CancelPlayerCast(Slot->Entity);
                Slot->DelayedInputCount = 0;
            }
        }
    }
    Victim->RespawnTimer = Maximum(Victim->RespawnTimer, AppState->RoundBreak);
}

// NOTE(zoubir): the keys a player may press now; everything but jump is
// held back during a break
inline u32
RoundBreakButtons(app_state *AppState)
{
    u32 Result = AppState->RoundBreak > 0.f ? (u32)PlayerButton_Jump : ~0u;
    return Result;
}

// NOTE(zoubir): once a tick, before anyone moves
internal void
UpdateRoundBreak(app_state *AppState, float DeltaTime)
{
    if (AppState->RoundBreak <= 0.f)
    {
        return;
    }
    AppState->RoundBreak = Maximum(0.f, AppState->RoundBreak - DeltaTime);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Entity && !IsDeadPlayer(Slot->Entity))
        {
            Slot->Entity->SpawnShield = Maximum(Slot->Entity->SpawnShield,
                                                AppState->RoundBreak);
        }
    }
}
