/* Shield bubble: a pale blue sphere of light around every player who
   cannot be hurt right now, after Shield (E) or a respawn (both set
   SpawnShield), so friend and foe both see that hits will do nothing. A
   soft glow fills it and a thin ring outlines it, both breathing a
   little; it swells in over its first moments. Online, replicas carry it
   as the PLAYER_FLASH_SHIELD flag (server/sim_game/pack.cpp). It
   replaces the sprite's old flicker. */

#define SHIELD_BUBBLE_SIZE 70.f
#define SHIELD_BUBBLE_RGB 0x00FFD890 // blue-white, as 0x00BBGGRR
#define SHIELD_BUBBLE_GROW_RATE 10.f

internal void
DrawShieldBubbles(render_context *RenderContext, app_state *AppState,
                  float *Grow, v3 CameraOffset, float DeltaTime)
{
    float Clock = GetFxClock(AppState);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = AppState->Players[SlotIndex].Entity;
        bool32 Up = Player && Player->IsPresent && !IsDeadPlayer(Player) &&
            (Player->SpawnShield > 0.f ||
             (Player->AbilityIndex & PLAYER_FLASH_SHIELD));
        if (!Up)
        {
            Grow[SlotIndex] = 0.f;
            continue;
        }
        // NOTE(zoubir): eases from 0 toward 1 as the shield comes up
        Grow[SlotIndex] += (1.f - Grow[SlotIndex]) *
            (1.f - expf(-SHIELD_BUBBLE_GROW_RATE * DeltaTime));
        float Breath = 0.5f + 0.5f * sinf(6.f * Clock + 1.7f * SlotIndex);
        float Size = SHIELD_BUBBLE_SIZE * (0.6f + 0.4f * Grow[SlotIndex]) *
            (0.97f + 0.06f * Breath);
        v2 Chest = Player->Position.XY - CameraOffset.XY;
        Chest.Y -= Player->Position.Z + 22.f;
        float X = Chest.X - 0.5f * Size;
        float Y = Chest.Y - 0.5f * Size;
        u32 GlowAlpha = (u32)(70.f + 40.f * Breath);
        u32 RingAlpha = (u32)(150.f + 80.f * Breath);
        DrawShaderQuad(RenderContext, Shader_Glow, X, Y, Size, Size,
                       (GlowAlpha << 24) | SHIELD_BUBBLE_RGB, RenderBlend_Additive);
        DrawShaderQuad(RenderContext, Shader_Ring, X, Y, Size, Size,
                       (RingAlpha << 24) | SHIELD_BUBBLE_RGB, RenderBlend_Additive);
    }
}
