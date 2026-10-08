// The dungeon run (sim/dungeon/dungeon.cpp), included inside app_state:
// made in the world's arena when the world is built for a dungeon map,
// 0 on every other map.
struct dungeon_run *Dungeon;
// Rooms with monsters the party has cleared since the run began, counted
// across every level of the dungeon (UpdateFightEnd, fight_end.cpp), as the run itself
// is built again for each level. It caps how high a player can level
// there (LevelCap, sim/progression/experience.cpp).
u32 DungeonRoomsCleared;
