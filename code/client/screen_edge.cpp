/* Screen edge: a soft dark vignette over the edges of the world, so the
   eye stays on the middle where the player is, and a red tint over it
   when the player is hurt. A hit flashes it (strength from the share of
   health lost), and below SCREEN_EDGE_LOW_HEALTH of full health it
   pulses, faster the closer the player is to dying. Drawn in window
   pixels under the HUD (screen_pass.inc). The shader is
   build/shaders/fx/screen_edge.frag; when it fails to build this draws
   nothing rather than a solid sprite quad. */

#define SCREEN_EDGE_LOW_HEALTH 0.35f
// NOTE(zoubir): a hit's flash fades over this many seconds
#define SCREEN_EDGE_HURT_SECONDS 0.45f

internal void
DrawScreenEdge(render_context *RenderContext, app_state *AppState,
               u32 Width, u32 Height, float DeltaTime)
{
    render_program *Program = &RenderContext->Programs[Shader_ScreenEdge];
    if (Program->ID == RenderContext->TextureProgram.ID)
    {
        return;
    }

    float Tint = 0.f;
    world_entity *Player = GetLocalPlayer(AppState);
    if (Player && Player->MaxHp > 0.f)
    {
        float Health = Maximum(0.f, Player->Hp) / Player->MaxHp;
        float Last = AppState->ScreenEdgeLastHp;
        // NOTE(zoubir): a respawn or the first frame raises health; only a
        // drop while alive is a hit
        if (Player->Hp < Last && Player->Hp > 0.f)
        {
            float Lost = (Last - Player->Hp) / Player->MaxHp;
            AppState->ScreenEdgeHurt = Maximum(AppState->ScreenEdgeHurt,
                                               Minimum(1.f, 0.35f + 2.f * Lost));
        }
        AppState->ScreenEdgeLastHp = Player->Hp;

        if (Player->Hp > 0.f && Health < SCREEN_EDGE_LOW_HEALTH)
        {
            float Danger = 1.f - Health / SCREEN_EDGE_LOW_HEALTH;
            float Rate = 2.5f + 4.f * Danger;
            float Beat = 0.5f + 0.5f * sinf(Rate * AppState->ScreenEdgeClock);
            Tint = (0.12f + 0.25f * Danger) * (0.6f + 0.4f * Beat);
        }
    }
    AppState->ScreenEdgeClock += DeltaTime;
    AppState->ScreenEdgeHurt = Maximum(0.f, AppState->ScreenEdgeHurt -
                                       DeltaTime / SCREEN_EDGE_HURT_SECONDS);
    Tint = Maximum(Tint, 0.55f * AppState->ScreenEdgeHurt * AppState->ScreenEdgeHurt);

    u32 Alpha = (u32)(255.f * Minimum(1.f, Tint));
    u32 Color = (Alpha << 24) | (0x18 << 16) | (0x10 << 8) | 0xD0;
    DrawShaderQuad(RenderContext, Shader_ScreenEdge, 0.f, 0.f,
                   (float)Width, (float)Height, Color);
}
