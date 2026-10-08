/* Duelist Tempo (role_kits/duelist.cpp): whole stacks in the slot's
   ClassMeter, 0 to DUELIST_MOST_TEMPO, so every client reads it. It is a
   buff the Duelist keeps, never spends: each stack is TEMPO_SHARE more
   damage (DuelistDealtScale), and Heartseeker hits harder by it.

   A spell gains a stack when it lands (Thrust, Lunge, Heartseeker) or
   goes off (Riposte's guard, Perfect Form) on a key other than the last
   one the Duelist used, so Thrust after Thrust gains nothing; under
   Perfect Form every key gains. A parry gains COUNTER_TEMPO. A hit the
   guard did not stop takes TEMPO_HIT_LOSS (LoseTempo, from
   DuelistParriesHit), and out of a fight Tempo fades
   (duelist/effects.cpp); Perfect Form holds it through both. */

inline u32
DuelistTempo(player_slot *Slot)
{
    u32 Result = Minimum((u32)Slot->ClassMeter, (u32)DUELIST_MOST_TEMPO);
    return Result;
}

// NOTE(zoubir): Key used now: true when it is another key than the last
// (or Perfect Form is up), so its spell gains Tempo when it lands
internal bool32
DuelistUseKey(player_slot *Slot, u32 Key)
{
    duelist_slot *Duel = &Slot->Duelist;
    bool32 Result = Duel->LastKey != Key + 1 || Duel->FormSeconds > 0.f;
    Duel->LastKey = Key + 1;
    return Result;
}

internal void
AddTempo(player_slot *Slot, u32 Amount)
{
    Slot->ClassMeter = (u8)Minimum((u32)DUELIST_MOST_TEMPO, DuelistTempo(Slot) + Amount);
}

// NOTE(zoubir): a hit landed on the Duelist that Riposte did not stop:
// TEMPO_HIT_LOSS stacks go (none under Perfect Form), and every client
// sees them break (DuelistBurst_Break, the Tempo before)
internal void
LoseTempo(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    u32 Before = DuelistTempo(Slot);
    if (Before == 0 || Slot->Duelist.FormSeconds > 0.f)
    {
        return;
    }
    Slot->ClassMeter = (u8)(Before > TEMPO_HIT_LOSS ? Before - TEMPO_HIT_LOSS : 0);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DuelistFirst, DuelistBurst_Break),
              (u8)Player->PlayerIndex, DuelistBurstSpot(ChestOf(Player), Before));
}
