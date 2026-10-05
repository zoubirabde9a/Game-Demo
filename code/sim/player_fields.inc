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
    // NOTE(zoubir): seconds left of a shove's stagger (hit.cpp): the keys
    // do not walk and only the carry drag slows the body, so a small shove
    // slides instead of being braked away. A movement ability ends it
    float Stagger;
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
    // seconds until each may be used again
#define PLAYER_AREA_ABILITY_SLOTS 6
    float AreaCooldowns[PLAYER_AREA_ABILITY_SLOTS];
    // NOTE(zoubir): the spell winding up (player_spell, player_casts.cpp),
    // PlayerSpell_None for none, and the seconds of it left
    u32 CastSpell;
    float CastLeft;
    // NOTE(zoubir): seconds left in which a respawned player cannot be hurt
    // or shoved (players.cpp)
    float SpawnShield;
    // NOTE(zoubir): set only during a blink's sweep: units do not stop it
    // (move.cpp CanSweepAgainst)
    bool32 Phasing;
    // NOTE(zoubir): the sword combo (sword.cpp): which cut of SwordCuts
    // the last swing was, and the seconds left to swing the next. A sword
    // entity carries the step it was swung with in ComboStep too
    u32 ComboStep;
    float ComboTimer;
    // NOTE(zoubir): an area ability's row + 1 that fires when the player
    // lands (a slam), 0 for none
    u32 PendingLandArea;
    // NOTE(zoubir): the combo trail (player_abilities/combos.cpp): the
    // player's last moves (combo_move), newest first, and the seconds since
    // each started
#define PLAYER_COMBO_TRAIL 4
    u32 ComboTrail[PLAYER_COMBO_TRAIL];
    float ComboTrailAge[PLAYER_COMBO_TRAIL];
    // NOTE(zoubir): set by the long jump combo until the player lands: the
    // air drag is cut so the dash's speed carries (movement.cpp)
    bool32 LongJump;
    // NOTE(zoubir): on a sword entity, the slice it hits (update.cpp
    // IsInSwordSlice), from its cut (sword.cpp SwordCuts)
    float SwordReach;
    float SwordHalfAngle;
