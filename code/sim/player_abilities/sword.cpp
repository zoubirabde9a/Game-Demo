/* Sword (right click): a short-lived hitbox toward the aim, hitting each
   thing once (UpdateSword, update.cpp). Roots the player for
   PLAYER_SWING_LOCK, then they can walk out of the swing. */

#define PLAYER_SWING_LOCK 0.08f

// NOTE(zoubir): a sword hitbox toward Dir (the aim); the player lunges a
// little that way
internal void
StartSwordSwing(app_state *AppState, world *World, memory_arena *Arena,
                world_entity *Player, v2 Dir, player_tick *Tick)
{
    Player->State = EntityState_Attacking;
    Player->ActionLock = PLAYER_SWING_LOCK;
    Player->CastingDirection = Dir;
    Player->AnimationState.SlotIndex = 0;
    Tick->Acceleration *= 0.6f;
    Tick->DDPlayer.XY = Dir;

    v3 SwordPosition = Player->Position + V3(16.f * Dir.X, 16.f * Dir.Y, 0.f);
    AddSword(AppState, World, Arena, SwordPosition, Player,
             DominantFacing(Dir));
    EmitSound(&AppState->Events, AssetType_Dash, Player->Position);
}

