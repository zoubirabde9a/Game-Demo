/* A team duel on the wire (sim/teams/, net/protocol/teams.h): whether one
   is on, each player's team and which slots are bots, for every viewer.
   Read back by client/team_requests.cpp. */

internal void
SimGameWriteTeams(app_state *AppState, net_snapshot *Out)
{
    net_teams *Teams = &Out->Teams;
    *Teams = {};
    Teams->On = IsTeamDuel(AppState) ? 1 : 0;
    for(u32 SlotIndex = 0; Teams->On && SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        Teams->Slots |= (u16)((PlayerTeam(AppState, SlotIndex) & 3) << (2 * SlotIndex));
        Teams->Bots |= (u8)((Slot->Active && Slot->Bot) ? (1 << SlotIndex) : 0);
    }
    if (AppState->VoteOpen && AppState->VoteTeams)
    {
        Out->VoteMap |= NET_VOTE_TEAMS;
    }
}
