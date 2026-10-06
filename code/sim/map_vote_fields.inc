// The map vote (sim/map_vote.cpp), included inside app_state. Online the
// client sets these from each snapshot (client/replicas.cpp), so the
// screens read them the same way offline and online.
// NOTE(zoubir): a vote is open while VoteOpen; VoteMap is the map asked
// for, VoteBy the slot that asked, VoteSeconds what is left of it
bool32 VoteOpen;
u32 VoteMap;
u32 VoteBy;
float VoteSeconds;
u32 VoteYes;
u32 VoteNo;
// NOTE(zoubir): each slot's answer, MapVote_Yes or MapVote_No, 0 for none
u8 Votes[MAX_PLAYERS];
// NOTE(zoubir): set as a vote passes: the next round is played on
// NextMap, with every level, experience and talent back to the start
bool32 NextMapVoted;
u32 NextMap;
