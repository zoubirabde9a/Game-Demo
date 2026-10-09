// A team duel on the wire (sim/teams/teams.cpp), in every snapshot. On
// is 1 while a team duel is played, sent as the top bit of the map byte
// (NET_MAP_TEAMS), which costs a free-for-all nothing; only then do
// Slots, each slot's team_id in 2 bits (slot 0 lowest), and Bots, bit N
// set when slot N is a server bot, follow it. The fullest snapshot is
// exactly NET_MAX_PACKET_SIZE with them. The other way, a player asks for a team in bits
// NET_ROLE_TEAM_SHIFT and up of net_input.Role, held and let go like the
// role picked there. A server bot sets NET_ROLE_BOT in every input it
// makes, so a replay knows the bots too; the server clears it in inputs
// that come over the network. A map vote for a team duel sets
// NET_VOTE_TEAMS in net_snapshot.VoteMap.
#define NET_ROLE_TEAM_SHIFT 5
#define NET_ROLE_TEAM_MASK 3u
#define NET_ROLE_BOT 0x80u
#define NET_VOTE_TEAMS 0x40u
#define NET_MAP_TEAMS 0x80u

struct net_teams
{
    u8 On;
    u16 Slots;
    u8 Bots;
};
