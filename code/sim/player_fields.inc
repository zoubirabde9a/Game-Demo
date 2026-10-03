// world_entity fields used only by player entities. Included inside
// struct world_entity in entity.h. Per-player state that is not about the
// body in the world (input, score, respawn) lives in player_slot, player.h.
    // NOTE(zoubir): seconds until the player can dash / shockwave again
    float DashCooldown;
    float ShockwaveCooldown;
    // NOTE(zoubir): seconds left on the shockwave ring effect
    float ShockwaveFlash;
    // NOTE(zoubir): unit vector toward the cursor; the body faces it and
    // sword and fireball fly along it, whichever way the player walks
    v2 Aim;
    // NOTE(zoubir): how far toward PLAYER_AIM_REACH the cursor is, 0..1
    float AimReach;
    // NOTE(zoubir): seconds left in which the player cannot walk, set when
    // a swing or cast starts; the rest of the animation can be walked out of
    float ActionLock;
    // NOTE(zoubir): seconds left on the dash streak clients draw
    float DashFlash;
    float BlinkCooldown;
    // NOTE(zoubir): seconds until the next fireball may leave the hand
    float FireBallCooldown;
    // NOTE(zoubir): seconds until the next sword swing may start
    float SwingCooldown;
