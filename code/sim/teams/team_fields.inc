// Team duels (sim/teams/teams.cpp), included inside app_state. Online the
// client sets TeamDuel from each snapshot (client/replicas.cpp).
// NOTE(zoubir): the duel is played in two teams; never true in a dungeon
bool32 TeamDuel;
// NOTE(zoubir): set with NextMapVoted (map_vote.cpp): whether the next
// round is a team duel
bool32 NextTeamDuel;
// NOTE(zoubir): the open vote's map is to be played in teams
bool32 VoteTeams;
