/* Round break, under the duel rules (GameRules.RoundBreaks), on a round
   map: one with no monsters (map_def.MonsterPopulation 0). There a dead
   player stays out until one player or nobody is left standing; that
   ends the round. Every player gains a level, and everyone gets
   ROUND_BREAK_SECONDS to spend the point in the talent tree, which opens
   on its own (ui/round_break_view.cpp). While it lasts nobody can be hurt
   (every living player holds the respawn shield, IsDodging in entity.cpp)
   and the only key that works is jump, so players can walk around and
   open the talent panel but not fight. The tick after the break runs
   out, everyone starts again at their spawn on the same map, keeping
   their level, talents and score (StartNextRoundMap, setup.cpp), unless
   a map vote passed meanwhile (map_vote.cpp). On a map with monsters
   each player respawns on their own, as under the classic rules. Online
   the server sends the time left in every snapshot
   (net_snapshot.RoundBreak), so prediction blocks the same keys and the
   client draws the countdown (ui/round_break_view.cpp).

   A break that a death started opens with the final blow: for
   FINAL_BLOW_SECONDS the whole world runs in slow motion (RoundTimeScale,
   which SimulateTick applies), nobody can move or press a key, and the
   client plays the death as a cinematic (client/final_blow.cpp). The
   break counts those seconds on top of its own, so RoundBreak starts at
   ROUND_BREAK_SECONDS + FINAL_BLOW_SECONDS and the client tells the two
   apart from the one number every snapshot already carries. */

#define ROUND_BREAK_SECONDS 10.f
// NOTE(zoubir): the final blow's slow motion: the world runs at
// FINAL_BLOW_SLOWEST of its speed, easing back to full over the last
// FINAL_BLOW_EASE_OUT seconds
#define FINAL_BLOW_SECONDS 3.f
#define FINAL_BLOW_SLOWEST 0.2f
#define FINAL_BLOW_EASE_OUT 0.6f

// NOTE(zoubir): a dead player on a round map waits this long, which no
// round lasts; the next round's start brings them back (setup.cpp)
#define ROUND_WAIT_SECONDS 1000000.f

// NOTE(zoubir): rounds are played on maps without monsters; a dungeon
// (sim/dungeon/) has its own rules for the dead
inline bool32
IsRoundMap(app_state *AppState)
{
    map_def *Map = GetMapDef((map_id)AppState->World.MapId);
    bool32 Result = GameRules.RoundBreaks && Map->MonsterPopulation == 0 &&
        !Map->Dungeon;
    return Result;
}

// NOTE(zoubir): seconds of the final blow's slow motion left, 0 when
// none plays
inline float
FinalBlowLeft(app_state *AppState)
{
    float Result = Maximum(0.f, AppState->RoundBreak - ROUND_BREAK_SECONDS);
    return Result;
}

// NOTE(zoubir): seconds of the break proper left, not counting the
// final blow; what the countdowns show
inline float
RoundBreakLeft(app_state *AppState)
{
    float Result = Minimum(AppState->RoundBreak, ROUND_BREAK_SECONDS);
    return Result;
}

// NOTE(zoubir): how fast the world runs this tick, 1 outside the final blow
inline float
RoundTimeScale(app_state *AppState)
{
    float Left = FinalBlowLeft(AppState);
    float Slow = Minimum(1.f, Left / FINAL_BLOW_EASE_OUT);
    float Result = 1.f - (1.f - FINAL_BLOW_SLOWEST) * Slow;
    return Result;
}

// NOTE(zoubir): the players still standing, and whether anyone is out
internal u32
CountStandingPlayers(app_state *AppState, bool32 *AnyoneOut)
{
    u32 Result = 0;
    *AnyoneOut = false;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Entity)
        {
            if (IsDeadPlayer(Slot->Entity))
            {
                *AnyoneOut = true;
            }
            else
            {
                Result++;
            }
        }
    }
    return Result;
}

// NOTE(zoubir): the round is over: a level for everyone, and the dead
// wait for the break to end. FinalBlow is true when a death ended it,
// not a player leaving: the break then opens with the slow motion
internal void
BeginRoundBreak(app_state *AppState, bool32 FinalBlow)
{
    AppState->RoundBreak = ROUND_BREAK_SECONDS + (FinalBlow ? FINAL_BLOW_SECONDS : 0.f);
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
            if (IsDeadPlayer(Slot->Entity))
            {
                Slot->RespawnTimer = AppState->RoundBreak;
            }
        }
    }
}

// NOTE(zoubir): from KillEntity when a player dies. On a round map the
// victim is out until the round ends, which this death may do; a death
// during the break (a pit still kills) waits for the same break to end
internal void
StartRoundBreak(app_state *AppState, player_slot *Victim)
{
    if (!IsRoundMap(AppState))
    {
        return;
    }
    if (AppState->RoundBreak > 0.f)
    {
        Victim->RespawnTimer = Maximum(Victim->RespawnTimer, AppState->RoundBreak);
        return;
    }
    Victim->RespawnTimer = ROUND_WAIT_SECONDS;
    bool32 AnyoneOut;
    if (CountStandingPlayers(AppState, &AnyoneOut) <= 1)
    {
        BeginRoundBreak(AppState, true);
    }
}

// NOTE(zoubir): the keys a player may press now; everything but jump is
// held back during a break, and jump too while the final blow plays
inline u32
RoundBreakButtons(app_state *AppState)
{
    u32 Result = FinalBlowLeft(AppState) > 0.f ? 0u :
        AppState->RoundBreak > 0.f ? (u32)PlayerButton_Jump : ~0u;
    return Result;
}

// NOTE(zoubir): once a tick, before anyone moves
internal void
UpdateRoundBreak(app_state *AppState, float DeltaTime)
{
    if (AppState->RoundBreak <= 0.f)
    {
        // NOTE(zoubir): the last players standing but one left the game
        bool32 AnyoneOut;
        if (IsRoundMap(AppState) &&
            CountStandingPlayers(AppState, &AnyoneOut) <= 1 && AnyoneOut)
        {
            BeginRoundBreak(AppState, false);
        }
        return;
    }
    AppState->RoundBreak = Maximum(0.f, AppState->RoundBreak - DeltaTime);
    if (AppState->RoundBreak <= 0.f)
    {
        AppState->RoundMapDue = true;
    }
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Entity && !IsDeadPlayer(Slot->Entity))
        {
            Slot->Entity->SpawnShield = Maximum(Slot->Entity->SpawnShield,
                                                AppState->RoundBreak);
        }
        // NOTE(zoubir): the dead come back as the break ends; their own
        // countdown runs on the slowed clock (simulate.cpp), the break's
        // on the real one
        else if (Slot->Active && Slot->Entity)
        {
            Slot->RespawnTimer = AppState->RoundBreak;
        }
    }
}
