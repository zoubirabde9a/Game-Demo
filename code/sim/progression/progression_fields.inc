// player_slot fields for experience, levels and talents
// (sim/progression/). Included inside struct player_slot in player.h. They
// live on the slot, not the entity, so a time rewind never takes back
// experience or a talent.
    // NOTE(zoubir): experience earned since joining, and the level it
    // makes (LevelForXp); one talent point per level after the first
    u32 Xp;
    u32 Level;
    // NOTE(zoubir): the level it had when the dungeon run began; the run
    // lets it climb one past this for each room cleared (LevelCap)
    u32 RunStartLevel;
    // NOTE(zoubir): seconds of play not yet paid out as experience
    float XpClock;
    // NOTE(zoubir): points spent in each talent, by talent_id. A base
    // ability's first level is free and not counted here
#define TALENT_SLOTS 45
    u8 Ranks[TALENT_SLOTS];
    // NOTE(zoubir): the Ward talent: a charge that takes the next hit
    // whole, and the seconds until a spent one comes back
    bool32 WardReady;
    float WardRecharge;
    // NOTE(zoubir): the match's duel tree (talents/duel_perks.cpp), a copy
    // of app_state's DuelSeed the simulation keeps here so whatever holds
    // a slot reads the tree; a client gets it in its snapshots
    u32 DuelSeed;
    // NOTE(zoubir): Second Wind went off this round
    bool32 SecondWindSpent;
    // NOTE(zoubir): the dungeon character, set aside while the party
    // plays a duel and given back when it goes into a dungeon again
    // (StartNextRoundMap, setup.cpp)
    u32 DungeonXp;
    u8 DungeonRanks[TALENT_SLOTS];
