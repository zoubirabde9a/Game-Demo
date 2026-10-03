/* Sword (right click): a swing toward the aim that hits each thing in its
   slice once (UpdateSword, update.cpp; reach and width in entity.h). Roots the player for
   PLAYER_SWING_LOCK, then they can walk out of the swing. */

#define PLAYER_SWING_LOCK 0.08f
// NOTE(zoubir): one swing per swing animation (6 frames of 0.03 s,
// animations.cpp). The animation alone paced swings, but in the air the
// jump state replaces the swing state, so every click swung at once
#define PLAYER_SWING_INTERVAL 0.18f

inline bool32
CanStartSwing(world_entity *Player)
{
    bool32 Result = Player->State != EntityState_Attacking &&
        Player->SwingCooldown <= 0.f;
    return Result;
}

// NOTE(zoubir): a sword hitbox toward Dir (the aim); the player lunges a
// little that way
internal void
StartSwordSwing(app_state *AppState, world *World, memory_arena *Arena,
                world_entity *Player, v2 Dir, player_tick *Tick)
{
    Player->State = EntityState_Attacking;
    Player->ActionLock = PLAYER_SWING_LOCK;
    Player->SwingCooldown = PLAYER_SWING_INTERVAL;
    Player->CastingDirection = Dir;
    Player->AnimationState.SlotIndex = 0;
    Tick->Acceleration *= 0.6f;
    Tick->DDPlayer.XY = Dir;

    v3 SwordPosition = Player->Position +
        V3(SWORD_OFFSET * Dir.X, SWORD_OFFSET * Dir.Y, 0.f);
    world_entity *Sword = AddSword(AppState, World, Arena, SwordPosition,
                                   Player, DominantFacing(Dir));
    Sword->CastingDirection = Dir;
    EmitSound(&AppState->Events, AssetType_Dash, Player->Position);
}

