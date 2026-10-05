/* Progression: experience, levels and talents. A player earns
   experience by killing players, a little by killing monsters, and a
   steady trickle for being in the match (experience.cpp); every level
   after the first is a point to spend in the talent tree, which unlocks
   abilities, raises their levels and adds passives (talents.cpp).

   It all lives on the player_slot (progression_fields.inc), so a time
   rewind never takes it back. Only the simulation that owns the world
   runs it: the server online, the local game offline. A client learns
   its own ranks and experience from snapshots (client/replicas/apply.cpp),
   and everyone's level from the scores. */

#include "talents.cpp"
#include "experience.cpp"

// NOTE(zoubir): XP_PER_SECOND paid out in whole points
#define XP_CLOCK_STEP (1.f / (float)XP_PER_SECOND)

// NOTE(zoubir): once a tick, before the players move: the trickle of
// experience, the talent each player asked for, and spent wards coming
// back
internal void
UpdateProgression(app_state *AppState, float DeltaTime)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (!Slot->Active || !Slot->Entity)
        {
            continue;
        }
        if (Slot->Level == 0)
        {
            Slot->Level = LevelForXp(Slot->Xp);
        }
        Slot->XpClock += DeltaTime;
        while (Slot->XpClock >= XP_CLOCK_STEP)
        {
            Slot->XpClock -= XP_CLOCK_STEP;
            AwardXp(AppState, Slot, 1);
        }
        if (Slot->Input.Learn)
        {
            LearnTalent(AppState, SlotIndex, Slot->Input.Learn - 1);
            Slot->Input.Learn = 0;
        }
        if (Slot->Ranks[Talent_Ward] && !Slot->WardReady)
        {
            Slot->WardRecharge -= DeltaTime;
            if (Slot->WardRecharge <= 0.f)
            {
                Slot->WardRecharge = 0.f;
                Slot->WardReady = true;
            }
        }
    }
}
