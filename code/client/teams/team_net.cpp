/* Teams on the client (sim/teams/): each team's name and colours for
   every screen and the world, the snapshot's team block applied to the
   slots (net/protocol/teams.h), and a team picked in the team panel
   (ui/teams/team_panel.cpp) on its way to the simulation.

   Offline the pick goes in player_input.Team at the start of the next
   frame (app.cpp), the way a map vote does. Online it rides in the top
   bits of the role byte (net_input.Role), held and let go by the role
   request (client/dungeon/role_requests.cpp), so a lost packet loses
   nothing. The panel shows "Switching" until the snapshot says the
   player is on the team asked for, or TEAM_ASK_SECONDS pass. */

#define TEAM_ASK_SECONDS 2.f

// NOTE(zoubir): the team's colour for text and fills; Strong is the
// brighter one for names and rings
global_variable u32 TeamColors[Team_Count] =
{
    UI_RGBA(238, 238, 232, 255),
    UI_RGBA(240, 92, 78, 255),
    UI_RGBA(82, 160, 255, 255),
};
global_variable u32 TeamStrongColors[Team_Count] =
{
    UI_RGBA(238, 238, 232, 255),
    UI_RGBA(255, 140, 124, 255),
    UI_RGBA(140, 196, 255, 255),
};
global_variable char *TeamNames[Team_Count] = {"No team", "Red", "Blue"};

struct team_ui
{
    // NOTE(zoubir): a team picked, not yet handed to the simulation (offline)
    u32 Request;
    // NOTE(zoubir): the team asked for last and the seconds the panel
    // waits for it, 0 once it came or gave up
    u32 Asked;
    float AskedLeft;
    bool32 PanelOpen;
    // NOTE(zoubir): ui/teams/team_watch.cpp: the local player's team last
    // frame, whether a team duel was on, and the rounds each team won
    u32 SeenTeam;
    bool32 SeenTeamDuel;
    bool32 TeamDuelAnnounced;
    u32 RoundWins[Team_Count];
    bool32 FinalBlowSeen;
    u32 RoundMapId;
};

internal team_ui *
GetTeamUi(app_state *AppState)
{
    if (!AppState->TeamUi)
    {
        AppState->TeamUi = AllocateStruct(&AppState->MemoryArena, team_ui);
        ZeroSize(AppState->TeamUi, sizeof(team_ui));
    }
    return AppState->TeamUi;
}

inline u32
TeamColor(u32 Team)
{
    u32 Result = TeamColors[Team < Team_Count ? Team : Team_None];
    return Result;
}

inline u32
TeamStrongColor(u32 Team)
{
    u32 Result = TeamStrongColors[Team < Team_Count ? Team : Team_None];
    return Result;
}

inline char *
TeamName(u32 Team)
{
    char *Result = TeamNames[Team < Team_Count ? Team : Team_None];
    return Result;
}

inline u32
LocalTeam(app_state *AppState)
{
    u32 Result = PlayerTeam(AppState, AppState->LocalPlayerIndex);
    return Result;
}

// NOTE(zoubir): the colour a player's name is written in: its team's in a
// team duel, else Otherwise
inline u32
PlayerNameColor(app_state *AppState, u32 SlotIndex, u32 Otherwise)
{
    u32 Team = PlayerTeam(AppState, SlotIndex);
    u32 Result = Team != Team_None ? TeamStrongColor(Team) : Otherwise;
    return Result;
}

// NOTE(zoubir): the ring under another player (draw_entities.cpp): its
// team's colour in a team duel, a teammate's a little fainter than a foe's
inline u32
TeamMarkerColor(app_state *AppState, world_entity *Entity, u32 Otherwise)
{
    u32 Result = Otherwise;
    u32 Team = Entity->Type == EntityType_Player ?
        PlayerTeam(AppState, Entity->PlayerIndex) : (u32)Team_None;
    if (Team != Team_None)
    {
        bool32 Ally = Team == LocalTeam(AppState);
        Result = WithAlpha(TeamColor(Team), Ally ? 0.55f : 0.7f);
    }
    return Result;
}

// NOTE(zoubir): from SyncReplicas, every snapshot: the team duel, each
// slot's team and which slots are bots
internal void
ApplySnapshotTeams(app_state *AppState, net_snapshot *Snapshot)
{
    net_teams *Teams = &Snapshot->Teams;
    AppState->TeamDuel = Teams->On != 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        Slot->Team = (u8)((Teams->Slots >> (2 * SlotIndex)) & 3);
        Slot->Bot = (Teams->Bots >> SlotIndex) & 1;
    }
    AppState->VoteTeams = false;
    if (Snapshot->VoteMap != NET_NO_VOTE)
    {
        AppState->VoteTeams = (Snapshot->VoteMap & NET_VOTE_TEAMS) != 0;
        AppState->VoteMap = Snapshot->VoteMap & ~NET_VOTE_TEAMS;
    }
}

// NOTE(zoubir): the team panel's pick. Online it goes out in the role
// byte; offline the next frame hands it to the simulation
internal void
RequestTeam(app_state *AppState, u32 Team)
{
    team_ui *Ui = GetTeamUi(AppState);
    Ui->Request = Team;
    Ui->Asked = Team;
    Ui->AskedLeft = TEAM_ASK_SECONDS;
    AppState->RoleRequest = (Team & NET_ROLE_TEAM_MASK) << NET_ROLE_TEAM_SHIFT;
}

// NOTE(zoubir): the team picked since the last frame, 0 for none; the
// caller hands it to the simulation when offline
internal u32
TakeTeamRequest(app_state *AppState)
{
    team_ui *Ui = GetTeamUi(AppState);
    u32 Result = Ui->Request;
    Ui->Request = 0;
    return Result;
}

// NOTE(zoubir): the team the panel waits to see the player on, Team_None
// when it waits for nothing
internal u32
PendingTeam(app_state *AppState, float DeltaTime)
{
    team_ui *Ui = GetTeamUi(AppState);
    Ui->AskedLeft = Maximum(0.f, Ui->AskedLeft - DeltaTime);
    if (Ui->AskedLeft <= 0.f || LocalTeam(AppState) == Ui->Asked)
    {
        Ui->Asked = Team_None;
        Ui->AskedLeft = 0.f;
    }
    return Ui->Asked;
}
