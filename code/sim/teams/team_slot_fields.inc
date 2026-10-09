// A player's side in a team duel (sim/teams/teams.cpp), included inside
// struct player_slot in player.h.
// NOTE(zoubir): Team_Red or Team_Blue in a team duel, Team_None otherwise
u8 Team;
// NOTE(zoubir): a server bot plays this slot (server/bots.cpp), so it
// makes way for a human who picks its team; set from each input
// (NET_ROLE_BOT), so a replay has it too
bool32 Bot;
