/* The map and a team duel in a snapshot (protocol.cpp,
   protocol/teams.h): one byte in a free-for-all, the map id with
   NET_MAP_TEAMS set and three more in a team duel. */

// NOTE(zoubir): false for a map id that does not fit under NET_MAP_TEAMS
internal bool32
NetSerializeMapAndTeams(net_stream *S, u8 *MapId, net_teams *Teams)
{
    if (S->Writing && *MapId >= NET_MAP_TEAMS)
    {
        return false;
    }
    u8 MapByte = (u8)(*MapId | (Teams->On ? NET_MAP_TEAMS : 0));
    NetU8(S, &MapByte);
    *MapId = (u8)(MapByte & ~NET_MAP_TEAMS);
    Teams->On = (MapByte & NET_MAP_TEAMS) ? 1 : 0;
    if (Teams->On)
    {
        NetU16(S, &Teams->Slots);
        NetU8(S, &Teams->Bots);
    }
    else
    {
        Teams->Slots = 0;
        Teams->Bots = 0;
    }
    return true;
}
