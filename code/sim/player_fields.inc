// world_entity fields used only by player entities. Included inside
// struct world_entity in entity.h. Per-player state that is not about the
// body in the world (input, score, respawn) lives in player_slot, player.h.
    // NOTE(zoubir): seconds until each movement ability (dash, blink) may
    // be used again, by player_movement
#define PLAYER_MOVEMENT_SLOTS 4
    float MovementCooldowns[PLAYER_MOVEMENT_SLOTS];
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
    // NOTE(zoubir): seconds until each spawn action (sword, fireball) may
    // start again, by player_action (player.h, included after this)
#define PLAYER_ACTION_SLOTS 4
    float ActionCooldowns[PLAYER_ACTION_SLOTS];
    // NOTE(zoubir): jumps since the player last stood on something, and the
    // seconds a jump press with none left still waits to land (jump.cpp)
    u32 JumpsUsed;
    float JumpBuffer;
    // NOTE(zoubir): seconds spent pushing into something low enough to
    // vault (jump.cpp)
    float VaultPush;
    // NOTE(zoubir): area abilities (player_abilities/area_abilities.cpp):
    // seconds until each may be used again, which one is being cast (its
    // index + 1, 0 for none) and the seconds of cast left
#define PLAYER_AREA_ABILITY_SLOTS 4
    float AreaCooldowns[PLAYER_AREA_ABILITY_SLOTS];
    u32 CastingArea;
    float AreaCastLeft;
    // NOTE(zoubir): seconds left in which a respawned player cannot be hurt
    // or shoved (players.cpp)
    float SpawnShield;
    // NOTE(zoubir): the sword combo (sword.cpp): which hit of SwordCombo
    // the last swing was, and the seconds left to swing the next. A sword
    // entity carries the step it was swung with in ComboStep too
    u32 ComboStep;
    float ComboTimer;
    // NOTE(zoubir): an area ability's row + 1 that fires when the player
    // lands (a slam), 0 for none
    u32 PendingLandArea;
