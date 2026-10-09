/* Map vote: any player can ask for another map (the Escape menu,
   ui/options_menu.cpp), of either game mode: a duel map, or a dungeon
   (sim/dungeon/), the co-op mode. Asking for a map of the other mode is
   how the players change mode. A duel is played free for all or in two
   teams (sim/teams/); MapVote_Teams asks for the other kind, on the same
   map, or on the first duel map from a dungeon, and a vote for another
   duel map keeps the kind. The vote stays open MAP_VOTE_SECONDS; the one who
   asked counts as a yes, and every other player can answer yes or no.
   When more than half of the players say yes, the world moves to that map
   on the next tick (StartNextRoundMap, setup.cpp). From one dungeon map
   to another everyone keeps their level and talents and a new run
   starts; a change of mode starts everyone again from level 1, keeping
   the dungeon character aside for the next dungeon. Names and scores
   stay. When it can no longer pass, or time runs out, it closes. Alone,
   asking is enough.

   Requests come in through player_input.Vote, the same way talent points
   do: offline from the menu directly, online in spare bits of the held
   buttons (NET_VOTE_SHIFT, net/protocol.h), bots answering yes
   (server/bots.cpp). */

#define MAP_VOTE_SECONDS 30.f

// NOTE(zoubir): whether MapId is played as a dungeon run, not a duel
inline bool32
IsDungeonMap(u32 MapId)
{
    bool32 Result = MapId < MapId_Count && GetMapDef((map_id)MapId)->Dungeon;
    return Result;
}

// NOTE(zoubir): a map the vote may move everyone to: any map but the one
// being played. The world is built again for it (StartNextRoundMap), which
// starts a dungeon run on a dungeon map and ends one anywhere else
inline bool32
IsVotableMap(app_state *AppState, u32 MapId)
{
    bool32 Result = MapId < MapId_Count && MapId != AppState->World.MapId;
    return Result;
}

// NOTE(zoubir): the map a vote for the other mode asks for: the first map
// of that mode, MapId_Count when there is none
internal u32
FirstMapOfMode(bool32 Dungeon)
{
    u32 Result = MapId_Count;
    for(u32 MapId = MapId_Count; MapId-- > 0;)
    {
        if (IsDungeonMap(MapId) == (Dungeon != 0))
        {
            Result = MapId;
        }
    }
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
    // NOTE(zoubir): the other kind of duel: teams from a free-for-all or
    // a dungeon, a free-for-all from teams. The top of the four bits,
    // above every MapVote_Ask + map_id
    MapVote_Teams = 15,
};
static_assert(MapVote_Ask + MapId_Count <= MapVote_Teams, "a map asked for fits under MapVote_Teams");

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
            bool32 Votable = IsVotableMap(AppState, MapId);
            bool32 Teams = AppState->TeamDuel && !IsDungeonMap(MapId);
            if (Request == MapVote_Teams)
            {
                bool32 InDungeon = IsDungeonMap(AppState->World.MapId);
                MapId = InDungeon ? FirstMapOfMode(false) : AppState->World.MapId;
                Votable = MapId < MapId_Count;
                Teams = InDungeon || !AppState->TeamDuel;
            }
            if (!AppState->VoteOpen && !AppState->NextMapVoted && Votable)
            {
                AppState->VoteOpen = true;
                AppState->VoteMap = MapId;
                AppState->VoteTeams = Teams;
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
        AppState->NextTeamDuel = AppState->VoteTeams;
        AppState->RoundBreak = 0.f;
        AppState->RoundMapDue = true;
    }
    else if (2 * AppState->VoteNo >= Players || AppState->VoteSeconds <= 0.f)
    {
        AppState->VoteOpen = false;
    }
}
