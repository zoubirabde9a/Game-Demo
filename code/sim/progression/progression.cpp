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

// NOTE(zoubir): the match's duel tree rolls the first time anyone has a
// point to spend, so the panel shows it while the point waits
internal void
RollDuelSeed(app_state *AppState)
{
    AppState->DuelTicks++;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS && !AppState->DuelSeed; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Entity && TalentPointsLeft(Slot) > 0)
        {
            RollDuelTree(AppState);
        }
    }
}

// NOTE(zoubir): once a tick, before the players move: the duel tree's
// roll, the trickle of experience, the talent each player asked for,
// spent wards coming back and Mending's healing
internal void
UpdateProgression(app_state *AppState, float DeltaTime)
{
    RollDuelSeed(AppState);
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
        Slot->DuelSeed = AppState->DuelSeed;
        Slot->XpClock += DeltaTime;
        while (Slot->XpClock >= XP_CLOCK_STEP)
        {
            Slot->XpClock -= XP_CLOCK_STEP;
            AwardXp(AppState, Slot, 1);
        }
        if (Slot->Input.Learn == TALENT_LEARN_RESET)
        {
            ResetTalents(AppState, SlotIndex);
        }
        else if (Slot->Input.Learn)
        {
            LearnTalent(AppState, SlotIndex, Slot->Input.Learn - 1);
        }
        Slot->Input.Learn = 0;
        MendingHeals(Slot, DeltaTime);
        if (DuelPerkRank(Slot, DuelPerk_Ward) && !Slot->WardReady)
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
