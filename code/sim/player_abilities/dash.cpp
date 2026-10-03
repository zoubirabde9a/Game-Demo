/* Dash (Alt): a burst of speed the way the keys point, or toward the aim
   when standing; ground drag eases it back to a walk. Cancels a swing's
   or cast's root. While DashFlash lasts the player cannot be hurt or
   shoved (IsDodging, entity.cpp), and clients draw the streak from it. */

// NOTE(zoubir): ground drag brings this back to a walk in about a quarter
// second, about 65 units travelled
#define PLAYER_DASH_SPEED 650.f
#define PLAYER_DASH_FLASH_SECONDS 0.15f

internal void
UseDash(app_state *AppState, world_entity *Player, player_input *Input,
        float DeltaTime)
{
    Player->DashCooldown = Maximum(0.f, Player->DashCooldown - DeltaTime);
    Player->DashFlash = Maximum(0.f, Player->DashFlash - DeltaTime);
    if (WasPressed(Input, PlayerButton_Dash) &&
        Player->DashCooldown <= 0.f)
    {
        // NOTE(zoubir): used to scale one tick's push, so standing still
        // spent the cooldown and went nowhere
        v2 Dir = GetPlayerAim(Player);
        float HeldSquared = LengthSq(Input->Move);
        if (HeldSquared > 0.0001f)
        {
            Dir = Input->Move * (1.f / SquareRoot(HeldSquared));
        }
        Player->Velocity.XY = PLAYER_DASH_SPEED * Dir;
        Player->ActionLock = 0.f;
        Player->DashCooldown = PLAYER_DASH_COOLDOWN;
        Player->DashFlash = PLAYER_DASH_FLASH_SECONDS;
        EmitSound(&AppState->Events, AssetType_Dash, Player->Position);
    }
}
