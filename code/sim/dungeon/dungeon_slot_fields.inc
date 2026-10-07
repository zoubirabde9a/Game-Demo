// player_slot fields for the dungeon (sim/dungeon/), included inside
// struct player_slot in player.h.
    // NOTE(zoubir): the player's player_role (sim/dungeon/roles.cpp)
    u8 Role;
    // NOTE(zoubir): seconds until each role key may cast again, the
    // seconds left of the tank's Shield Wall, and the damage a healer's
    // Ward still absorbs (sim/dungeon/role_abilities.cpp)
#define ROLE_KEYS 3
    float RoleCooldowns[ROLE_KEYS];
    float ShieldWallSeconds;
    float WardAbsorb;
