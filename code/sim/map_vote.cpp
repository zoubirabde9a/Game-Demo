/* Map vote: any player can ask for another map (the Escape menu,
   ui/options_menu.cpp). The vote stays open MAP_VOTE_SECONDS; the one who
   asked counts as a yes, and every other player can answer yes or no.
   When more than half of the players say yes, the world moves to that map
   on the next tick (StartNextRoundMap, setup.cpp) and everyone starts
   again from level 1, without experience or talents; names and scores
   stay. When it can no longer pass, or time runs out, it closes. Alone,
   asking is enough.

   Requests come in through player_input.Vote, the same way talent points
   do: offline from the menu directly, online in spare bits of the held
   buttons (NET_VOTE_SHIFT, net/protocol.h), bots answering yes
   (server/bots.cpp). */

#define MAP_VOTE_SECONDS 30.f

// NOTE(zoubir): a map the vote may move everyone to: another duel map.
// A dungeon (sim/dungeon/) is its own mode: never voted to, and a run
// never votes its way out
inline bool32
IsVotableMap(app_state *AppState, u32 MapId)
{
    bool32 Result = MapId < MapId_Count && MapId != AppState->World.MapId &&
        !GetMapDef((map_id)MapId)->Dungeon && !IsDungeon(AppState);
    return Result;
}

// NOTE(zoubir): player_input.Vote's values: an answer, or a map asked for
// as MapVote_Ask + its map_id. Four bits on the wire (NET_VOTE_MASK)
enum map_vote_request
{
    MapVote_None,
    MapVote_Yes,
    MapVote_No,
    MapVote_Ask,
};

inline u32
MapVoteAsk(u32 MapId)
{
    u32 Result = MapVote_Ask + MapId;
    return Result;
}

// NOTE(zoubir): once a tick, after progression, before anyone moves
internal void
UpdateMapVote(app_state *AppState, float DeltaTime)
{
    u32 Players = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        u32 Request = Slot->Input.Vote;
        Slot->Input.Vote = MapVote_None;
        if (!Slot->Active)
        {
            AppState->Votes[SlotIndex] = MapVote_None;
            continue;
        }
        Players++;
        if (Request >= MapVote_Ask)
        {
            u32 MapId = Request - MapVote_Ask;
            if (!AppState->VoteOpen && !AppState->NextMapVoted &&
                IsVotableMap(AppState, MapId))
            {
                AppState->VoteOpen = true;
                AppState->VoteMap = MapId;
                AppState->VoteBy = SlotIndex;
                AppState->VoteSeconds = MAP_VOTE_SECONDS;
                ZeroArray(AppState->Votes, MAX_PLAYERS, u8);
                AppState->Votes[SlotIndex] = MapVote_Yes;
            }
        }
        else if (Request && AppState->VoteOpen)
        {
            AppState->Votes[SlotIndex] = (u8)Request;
        }
    }

    AppState->VoteYes = AppState->VoteNo = 0;
    if (!AppState->VoteOpen)
    {
        return;
    }
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        AppState->VoteYes += AppState->Votes[SlotIndex] == MapVote_Yes ? 1 : 0;
        AppState->VoteNo += AppState->Votes[SlotIndex] == MapVote_No ? 1 : 0;
    }
    AppState->VoteSeconds -= DeltaTime;
    if (2 * AppState->VoteYes > Players)
    {
        AppState->VoteOpen = false;
        AppState->NextMapVoted = true;
        AppState->NextMap = AppState->VoteMap;
        AppState->RoundBreak = 0.f;
        AppState->RoundMapDue = true;
    }
    else if (2 * AppState->VoteNo >= Players || AppState->VoteSeconds <= 0.f)
    {
        AppState->VoteOpen = false;
    }
}
