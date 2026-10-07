// player_slot fields for the dungeon (sim/dungeon/), included inside
// struct player_slot in player.h.
    // NOTE(zoubir): the player's player_role (sim/dungeon/roles.cpp)
    u8 Role;
    // NOTE(zoubir): seconds until each role key may cast again, the
    // seconds left of the tank's Shield Wall, and the damage a healer's
    // Ward still absorbs and absorbed when cast, for the ring that empties
    // (sim/dungeon/role_abilities.cpp)
#define ROLE_KEYS 3
    float RoleCooldowns[ROLE_KEYS];
    float ShieldWallSeconds;
    float WardAbsorb;
    float WardFull;
    // NOTE(zoubir): a tank's Shield Slam rallied this player: seconds left
    // and the share of damage it takes off; a healer's Renewal: seconds
    // left, health a second, and the healer's slot
    float RallySeconds;
    float RallyShare;
    float RenewSeconds;
    float RenewPerSecond;
    u8 RenewBy;
    // NOTE(zoubir): how many monsters of the fight are after this player
    // (CountAggro), for the party frames
    u8 Aggro;
    // NOTE(zoubir): while downed, the seconds a healer has stood over the
    // body (sim/dungeon/revive.cpp)
    float ReviveSeconds;
