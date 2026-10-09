// The break between rounds (sim/round_break.cpp), included inside
// app_state: seconds left of it, 0 while a round is being played.
// Online the client sets it from each snapshot.
float RoundBreak;
// Set as a break runs out: the next tick starts the new round on the next
// map (StartNextRoundMap, sim/setup.cpp).
bool32 RoundMapDue;
// The duel tree's roll for this match (sim/progression/talents/duel_perks.cpp),
// 0 until the first point of the match is there to spend, and the ticks
// played, which the roll is made from so a replay rolls the same.
u32 DuelSeed;
u32 DuelTicks;
