// world_entity fields used only by player entities. Included inside
// struct world_entity in entity.h. Per-player state that is not about the
// body in the world (input, score, respawn) lives in player_slot, player.h.
    // NOTE(zoubir): seconds until the player can dash / shockwave again
    float DashCooldown;
    float ShockwaveCooldown;
    // NOTE(zoubir): seconds left on the shockwave ring effect
    float ShockwaveFlash;
