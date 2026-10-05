// player_slot fields for experience, levels and talents
// (sim/progression/). Included inside struct player_slot in player.h. They
// live on the slot, not the entity, so a time rewind never takes back
// experience or a talent.
    // NOTE(zoubir): experience earned since joining, and the level it
    // makes (LevelForXp); one talent point per level after the first
    u32 Xp;
    u32 Level;
    // NOTE(zoubir): seconds of play not yet paid out as experience
    float XpClock;
    // NOTE(zoubir): points spent in each talent, by talent_id. A base
    // ability's first level is free and not counted here
#define TALENT_SLOTS 24
    u8 Ranks[TALENT_SLOTS];
    // NOTE(zoubir): the Ward talent: a charge that takes the next hit
    // whole, and the seconds until a spent one comes back
    bool32 WardReady;
    float WardRecharge;
