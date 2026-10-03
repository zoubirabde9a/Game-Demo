/* Blink (F): a jump through space to the cursor, at most PLAYER_AIM_REACH
   away. It is swept like any move, so it stops at the first wall or unit
   on the way (sliding along it) rather than landing inside. Keeps the
   player's speed, cancels a swing's or cast's root, and lights the dash
   streak, which clients draw along the whole jump. */

#define PLAYER_BLINK_COOLDOWN 3.f

internal void
UseBlink(app_state *AppState, world *World, memory_arena *Arena,
         world_entity *Player, player_input *Input, float DeltaTime)
{
    Player->BlinkCooldown = Maximum(0.f, Player->BlinkCooldown - DeltaTime);
    if (!WasPressed(Input, PlayerButton_Blink) || Player->BlinkCooldown > 0.f ||
        DeltaTime <= 0.f)
    {
        return;
    }
    // NOTE(zoubir): never cursor input yet: the full reach
    float Reach = Player->AimReach > 0.f ? Player->AimReach : 1.f;
    float Distance = Reach * PLAYER_AIM_REACH;
    v3 Velocity = Player->Velocity;
    Player->Velocity = V3(0.f, 0.f, 0.f);
    Player->Velocity.XY = (Distance / DeltaTime) * GetPlayerAim(Player);
    MoveEntity(Player, World, Arena, DeltaTime, AppState, V3(0.f, 0.f, 0.f),
               &Distance);
    if (!Player->IsPresent)
    {
        return;
    }
    Player->Velocity = Velocity;
    Player->ActionLock = 0.f;
    Player->BlinkCooldown = PLAYER_BLINK_COOLDOWN;
    Player->DashFlash = PLAYER_DASH_FLASH_SECONDS;
    EmitSound(&AppState->Events, AssetType_Dash, Player->Position);
}
