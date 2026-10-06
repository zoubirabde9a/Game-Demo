// The break between rounds (sim/round_break.cpp), included inside
// app_state: seconds left of it, 0 while a round is being played.
// Online the client sets it from each snapshot.
float RoundBreak;
// Set as a break runs out: the next tick starts the new round on the next
// map (StartNextRoundMap, sim/setup.cpp).
bool32 RoundMapDue;
