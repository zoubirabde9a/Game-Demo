/* Team duels: the duel played in two teams, Red and Blue, picked in the
   map vote's Mode row (map_vote.cpp, MapVote_Teams) like a game mode.
   AppState->TeamDuel says one is on (team_fields.inc); each player's side
   is player_slot.Team (team_slot_fields.inc).

   - Everyone is on a team. A player without one (just joined, or the
     team duel just started) goes to the smaller team, then the one with
     fewer kills, then Red, and appears on that team's side of the map.
   - A player asks for the other team through player_input.Team (online,
     bits 5-6 of the role byte, net_input.Role). Teams never end up more
     than one player apart; a full team still takes a human when one of
     its players is a server bot, which swaps over to make way. When
     players leave, bots move to even the teams out again.
   - Changing sides mid-fight takes the player out like a death that
     nobody scores: on a round map until the round ends, elsewhere for the
     usual respawn wait, then back on the new side. Dead, or during the
     break between rounds, the change is free.
   - Teammates cannot hurt, shove or stun each other (IsFriendlyFire,
     sim/dungeon/dungeon.cpp), and on a round map the round ends when one
     team is left standing (round_break.cpp).

   Bots leave their teammates alone (BotFindTarget, server/bots.cpp). The
   team panel (ui/teams/) shows both teams and switches sides. */

enum team_id
{
    Team_None,
    Team_Red,
    Team_Blue,
    Team_Count
};

// NOTE(zoubir): on an endless map each team's spawns are an arc of the
// ring around the clearing at the origin, Red to the west and Blue to
// the east, TEAM_SPAWN_SPREAD radians apart
#define TEAM_SPAWN_RADIUS 120.f
#define TEAM_SPAWN_SPREAD 0.45f
// NOTE(zoubir): a team with more players than a map has spawns for it
// steps this far further out for each extra lap
#define TEAM_SPAWN_LAP 48.f

inline bool32
IsTeamDuel(app_state *AppState)
{
    bool32 Result = AppState->TeamDuel &&
        !GetMapDef((map_id)AppState->World.MapId)->Dungeon;
    return Result;
}

inline u32
OtherTeam(u32 Team)
{
    u32 Result = Team == Team_Red ? Team_Blue : Team_Red;
    return Result;
}

// NOTE(zoubir): the team of the player in SlotIndex, Team_None outside a
// team duel
inline u32
PlayerTeam(app_state *AppState, u32 SlotIndex)
{
    u32 Result = Team_None;
    if (IsTeamDuel(AppState) && SlotIndex < MAX_PLAYERS &&
        AppState->Players[SlotIndex].Active)
    {
        Result = AppState->Players[SlotIndex].Team;
    }
    return Result;
}

// NOTE(zoubir): two different players on the same team
inline bool32
AreTeammates(app_state *AppState, u32 A, u32 B)
{
    u32 Team = PlayerTeam(AppState, A);
    bool32 Result = A != B && Team != Team_None && Team == PlayerTeam(AppState, B);
    return Result;
}

// NOTE(zoubir): the players on Team, not counting ExceptSlot
internal u32
CountTeam(app_state *AppState, u32 Team, u32 ExceptSlot = MAX_PLAYERS)
{
    u32 Result = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (SlotIndex != ExceptSlot && PlayerTeam(AppState, SlotIndex) == Team)
        {
            Result++;
        }
    }
    return Result;
}

// NOTE(zoubir): the team's score: the kills of the players on it now
internal u32
TeamKills(app_state *AppState, u32 Team)
{
    u32 Result = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (PlayerTeam(AppState, SlotIndex) == Team)
        {
            Result += AppState->Players[SlotIndex].Kills;
        }
    }
    return Result;
}

// NOTE(zoubir): where SlotIndex goes when it has no team: the team with
// fewer players (not counting it), then the one with fewer kills, then Red
internal u32
SmallerTeam(app_state *AppState, u32 SlotIndex)
{
    u32 Red = CountTeam(AppState, Team_Red, SlotIndex);
    u32 Blue = CountTeam(AppState, Team_Blue, SlotIndex);
    u32 Result = Team_Red;
    if (Blue < Red || (Blue == Red && TeamKills(AppState, Team_Blue) < TeamKills(AppState, Team_Red)))
    {
        Result = Team_Blue;
    }
    return Result;
}

enum team_refusal
{
    TeamRefusal_None,
    TeamRefusal_Already,
    // NOTE(zoubir): the team would have two players more than the other
    TeamRefusal_Uneven,
};

// NOTE(zoubir): whether SlotIndex may move to Team. A team may end at most
// one player bigger than the other; a full one still takes a human when
// it has a bot, which then swaps sides (*MakesWay, MAX_PLAYERS for none)
internal team_refusal
CanJoinTeam(app_state *AppState, u32 SlotIndex, u32 Team, u32 *MakesWay)
{
    *MakesWay = MAX_PLAYERS;
    if (PlayerTeam(AppState, SlotIndex) == Team)
    {
        return TeamRefusal_Already;
    }
    u32 Joined = CountTeam(AppState, Team, SlotIndex) + 1;
    u32 Left = CountTeam(AppState, OtherTeam(Team), SlotIndex);
    if (Joined <= Left + 1)
    {
        return TeamRefusal_None;
    }
    if (!AppState->Players[SlotIndex].Bot)
    {
        for(u32 Step = 0; Step < MAX_PLAYERS; Step++)
        {
            u32 Other = MAX_PLAYERS - 1 - Step;
            if (Other != SlotIndex && AppState->Players[Other].Bot &&
                PlayerTeam(AppState, Other) == Team)
            {
                *MakesWay = Other;
                return TeamRefusal_None;
            }
        }
    }
    return TeamRefusal_Uneven;
}

// NOTE(zoubir): where the Rank-th player of Team (in slot order) appears:
// the left half of the map's spawns for Red, the right half for Blue, or
// an arc of the ring on an endless map
internal v3
TeamSpawnPosition(world *World, u32 Team, u32 Rank)
{
    map_def *Map = GetMapDef((map_id)World->MapId);
    float TileSize = World->TileWidth ? (float)World->TileWidth : (float)ARENA_TILE_SIZE;
    float Side = Team == Team_Red ? -1.f : 1.f;
    v3 Result = V3(0.5f * TileSize, 0.5f * TileSize, 0.f);
    if (Map->Kind == MapKind_Infinite)
    {
        // NOTE(zoubir): the middle of the arc first, then either side of it
        float Step = (float)((Rank + 1) / 2) * ((Rank & 1) ? 1.f : -1.f);
        float Angle = (Team == Team_Red ? Pi32 : 0.f) + TEAM_SPAWN_SPREAD * Step;
        Result.X += TEAM_SPAWN_RADIUS * Cos(Angle);
        Result.Y += TEAM_SPAWN_RADIUS * Sin(Angle);
    }
    else if (Map->SpawnCount >= 2)
    {
        // NOTE(zoubir): the spawns from left to right (top to bottom on a
        // column); an odd one in the middle belongs to neither team
        u32 Order[MAX_MAP_SPAWNS];
        for(u32 Spawn = 0; Spawn < Map->SpawnCount; Spawn++)
        {
            u32 Insert = Spawn;
            while (Insert > 0 &&
                   (Map->SpawnX[Order[Insert - 1]] > Map->SpawnX[Spawn] ||
                    (Map->SpawnX[Order[Insert - 1]] == Map->SpawnX[Spawn] &&
                     Map->SpawnY[Order[Insert - 1]] > Map->SpawnY[Spawn])))
            {
                Order[Insert] = Order[Insert - 1];
                Insert--;
            }
            Order[Insert] = Spawn;
        }
        u32 Half = Map->SpawnCount / 2;
        u32 Index = Rank % Half;
        u32 Spawn = Team == Team_Red ? Order[Index] : Order[Map->SpawnCount - 1 - Index];
        float Lap = (float)(Rank / Half);
        Result.X = ((float)Map->SpawnX[Spawn] + 0.5f) * TileSize + Side * TEAM_SPAWN_LAP * Lap;
        Result.Y = ((float)Map->SpawnY[Spawn] + 0.5f) * TileSize;
    }
    else
    {
        Result = PlayerSpawnPosition(World, Team == Team_Red ? 0 : 1);
        Result.X += Side * TEAM_SPAWN_LAP * (float)(Rank + 1);
    }
    return Result;
}

// NOTE(zoubir): every player's spawn on its team's side, ranked in slot
// order within the team
internal void
PlaceTeamSpawns(app_state *AppState)
{
    u32 Ranks[Team_Count] = {};
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        u32 Team = PlayerTeam(AppState, SlotIndex);
        if (Team != Team_None)
        {
            Slot->SpawnPosition = TeamSpawnPosition(&AppState->World, Team, Ranks[Team]++);
        }
    }
}

// NOTE(zoubir): a living player that just appeared goes to its spawn
internal void
MovePlayerToSpawn(app_state *AppState, memory_arena *Arena, player_slot *Slot)
{
    world_entity *Player = Slot->Entity;
    if (!Player || !Player->IsPresent || IsDeadPlayer(Player))
    {
        return;
    }
    v3 OldPosition = Player->Position;
    Player->Position = FindFreePlayerSpot(AppState, &AppState->World,
                                          Slot->SpawnPosition, Player);
    Player->Velocity = {};
    CheckAndChangeEntityChunk(AppState, &AppState->World, Arena, OldPosition, Player);
}

// NOTE(zoubir): a player changing sides mid-fight leaves it, like a death
// nobody scores: it waits out a respawn (on a round map, the round) and
// comes back on its new side. Dead, or during a break, nothing happens
internal void
SitOutForTeamChange(app_state *AppState, player_slot *Slot)
{
    world_entity *Player = Slot->Entity;
    if (!Player || !Player->IsPresent || IsDeadPlayer(Player) || AppState->RoundBreak > 0.f)
    {
        return;
    }
    EmitBurst(&AppState->Events, SimBurst_Spawn, (u8)Player->PlayerIndex, Player->Position);
    CancelPlayerCast(Player);
    Slot->DelayedInputCount = 0;
    Player->Hp = 0.f;
    Player->Velocity = {};
    Slot->RespawnTimer = RespawnSeconds(Slot);
    StartRoundBreak(AppState, Slot);
}

// NOTE(zoubir): SlotIndex onto Team. A player without a team was just
// placed and goes straight to its new spawn; one changing sides sits out
internal void
MoveToTeam(app_state *AppState, memory_arena *Arena, u32 SlotIndex, u32 Team)
{
    player_slot *Slot = &AppState->Players[SlotIndex];
    bool32 Placing = Slot->Team == Team_None;
    Slot->Team = (u8)Team;
    PlaceTeamSpawns(AppState);
    if (Placing)
    {
        MovePlayerToSpawn(AppState, Arena, Slot);
    }
    else
    {
        SitOutForTeamChange(AppState, Slot);
    }
}

// NOTE(zoubir): players left and the teams are two or more apart: a bot
// of the bigger team crosses over, if it has one
internal void
EvenTeamsWithBots(app_state *AppState, memory_arena *Arena)
{
    for(u32 Pass = 0; Pass < MAX_PLAYERS; Pass++)
    {
        u32 Red = CountTeam(AppState, Team_Red);
        u32 Blue = CountTeam(AppState, Team_Blue);
        if (Red <= Blue + 1 && Blue <= Red + 1)
        {
            return;
        }
        u32 Bigger = Red > Blue ? Team_Red : Team_Blue;
        u32 Mover = MAX_PLAYERS;
        for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
        {
            if (AppState->Players[SlotIndex].Bot && PlayerTeam(AppState, SlotIndex) == Bigger)
            {
                Mover = SlotIndex;
            }
        }
        if (Mover == MAX_PLAYERS)
        {
            return;
        }
        MoveToTeam(AppState, Arena, Mover, OtherTeam(Bigger));
    }
}

// NOTE(zoubir): once a tick, after the map vote: places players without a
// team and takes the changes asked for. Outside a team duel nobody has one
internal void
UpdateTeams(app_state *AppState, memory_arena *Arena)
{
    bool32 Teams = IsTeamDuel(AppState);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        u32 Request = Slot->Input.Team;
        Slot->Input.Team = 0;
        if (!Teams || !Slot->Active)
        {
            Slot->Team = Team_None;
            continue;
        }
        if (Slot->Team == Team_None)
        {
            MoveToTeam(AppState, Arena, SlotIndex, SmallerTeam(AppState, SlotIndex));
            continue;
        }
        u32 MakesWay;
        if ((Request != Team_Red && Request != Team_Blue) ||
            CanJoinTeam(AppState, SlotIndex, Request, &MakesWay) != TeamRefusal_None)
        {
            continue;
        }
        if (MakesWay < MAX_PLAYERS)
        {
            MoveToTeam(AppState, Arena, MakesWay, Slot->Team);
        }
        MoveToTeam(AppState, Arena, SlotIndex, Request);
    }
    if (Teams)
    {
        EvenTeamsWithBots(AppState, Arena);
    }
}

// NOTE(zoubir): from StartNextRoundMap (setup.cpp), once every player is
// back in the new world: a team for whoever has none, and everyone at
// their team's spawn
internal void
StartTeamRound(app_state *AppState, memory_arena *Arena)
{
    if (!IsTeamDuel(AppState))
    {
        for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
        {
            AppState->Players[SlotIndex].Team = Team_None;
        }
        return;
    }
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Team == Team_None)
        {
            Slot->Team = (u8)SmallerTeam(AppState, SlotIndex);
        }
    }
    PlaceTeamSpawns(AppState);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (AppState->Players[SlotIndex].Active)
        {
            MovePlayerToSpawn(AppState, Arena, &AppState->Players[SlotIndex]);
        }
    }
}
