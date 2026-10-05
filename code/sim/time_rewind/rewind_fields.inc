// world_entity fields for the time rewind (sim/time_rewind/). Included
// inside struct world_entity in entity.h.
    // NOTE(zoubir): who this entity is in the rewind's history: handed out
    // the first time the history records it, 0 before. A new entity in a
    // reused slot starts at 0 again, so a record only ever restores the
    // body it was taken from
    u32 RewindSerial;
    // NOTE(zoubir): a player's seconds until each rewind may be used
    // again, by rewind_kind. A rewind never restores them, or two players
    // could trade one rewind back and forth forever
#define PLAYER_REWIND_SLOTS 3
    float RewindCooldowns[PLAYER_REWIND_SLOTS];
