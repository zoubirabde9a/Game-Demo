/* The time rewinds under way, for a snapshot (net_rewind): every world
   rewind, and the others whose centre is within SIM_GAME_REWIND_VIEW of
   the viewer, nearest first being the order the casts happen to be in.
   Each says which of the snapshot's entities it has frozen, so the
   client draws the freeze on exactly those and leaves its own player
   where the server puts it. */

// NOTE(zoubir): farther than a screen and a half away a rewind is not sent
#define SIM_GAME_REWIND_VIEW 1200.f

internal void
SimGameWriteRewinds(server_game *Game, bool32 HasCentre, v2 Centre, net_snapshot *Out)
{
    Out->RewindCount = 0;
    time_rewind *Rewind = Game->AppState->Rewind;
    if (!Rewind)
    {
        return;
    }
    for (u32 Slot = 0; Slot < MAX_PLAYERS && Out->RewindCount < NET_MAX_SNAPSHOT_REWINDS; ++Slot)
    {
        rewind_cast *Cast = &Rewind->Casts[Slot];
        if (Cast->Phase == RewindPhase_None)
        {
            continue;
        }
        if (Cast->Kind != RewindKind_World && HasCentre &&
            LengthSq(Cast->Centre.XY - Centre) > Square(SIM_GAME_REWIND_VIEW))
        {
            continue;
        }
        net_rewind *Sent = &Out->Rewinds[Out->RewindCount++];
        *Sent = {};
        Sent->Slot = (u8)Slot;
        Sent->Kind = (u8)Cast->Kind;
        Sent->Phase = (u8)Cast->Phase;
        Sent->PhaseLeft = Maximum(0.f, Cast->PhaseLeft);
        Sent->X = Cast->Centre.X;
        Sent->Y = Cast->Centre.Y;
        Sent->Radius = Cast->Radius;
        for (u32 Index = 0; Index < Cast->AffectedCount; ++Index)
        {
            u32 ID = Cast->AffectedID[Index];
            u32 Serial = Cast->AffectedSerial[Index];
            if (!Serial || Rewind->LockedSerial[ID] != Serial)
            {
                continue;
            }
            for (u32 Entity = 0; Entity < Out->Count; ++Entity)
            {
                if (Out->Entities[Entity].Id == ID)
                {
                    Sent->Frozen[Entity / 8] |= (u8)(1u << (Entity % 8));
                }
            }
        }
    }
}
