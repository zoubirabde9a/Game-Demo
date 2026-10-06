/* Threat pointers: a small red chevron at the edge of the screen for each
   monster or other player that is close but off screen, pointing at it,
   brighter the closer it is. Ranged monsters (the toad's Bile Barrage
   reaches 380 units) can fire from past the edge of the view; this shows
   where the shots come from. Read from the entities the client draws,
   so it works the same offline and online. Drawn in world units, like
   the landmark pointer (landmark_pointer.cpp). */

// NOTE(zoubir): farther than this from the local player, nothing shows
#define THREAT_POINTER_RANGE 650.f
#define THREAT_POINTER_MAX 8
#define THREAT_POINTER_RGB 0x003848F0 // red, as 0x00BBGGRR
#define THREAT_POINTER_INSET 26.f

internal void
DrawThreatPointers(render_context *RenderContext, app_state *AppState,
                   v3 CameraOffset, app_window *View)
{
    world *World = &AppState->World;
    world_entity *Player = GetLocalPlayer(AppState);
    if (!Player || !Player->IsPresent || IsDeadPlayer(Player))
    {
        return;
    }
    v2 Screen = V2((float)View->Width, (float)View->Height);
    v2 Middle = 0.5f * Screen;
    u32 Shown = 0;
    for(u32 Index = 0; Index < World->EntityCount && Shown < THREAT_POINTER_MAX; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        bool32 Threat = Entity->IsPresent && Entity != Player && Entity->Hp > 0.f &&
            (Entity->Type == EntityType_Monster || Entity->Type == EntityType_Player);
        if (!Threat)
        {
            continue;
        }
        float Distance = Length(Entity->Position.XY - Player->Position.XY);
        v2 OnScreen = Entity->Position.XY - CameraOffset.XY;
        bool32 Visible = OnScreen.X > 0.f && OnScreen.Y > 0.f &&
            OnScreen.X < Screen.X && OnScreen.Y < Screen.Y;
        if (Visible || Distance > THREAT_POINTER_RANGE)
        {
            continue;
        }
        v2 Toward = OnScreen - Middle;
        float Span = Length(Toward);
        if (Span <= 0.f)
        {
            continue;
        }
        Toward *= 1.f / Span;
        // NOTE(zoubir): where the line from the middle leaves the screen,
        // pulled in by the inset
        float ScaleX = Toward.X != 0.f ? (Middle.X - THREAT_POINTER_INSET) / fabsf(Toward.X) : 1e9f;
        float ScaleY = Toward.Y != 0.f ? (Middle.Y - THREAT_POINTER_INSET) / fabsf(Toward.Y) : 1e9f;
        v2 Anchor = Middle + Minimum(ScaleX, ScaleY) * Toward;

        float Near = 1.f - Distance / THREAT_POINTER_RANGE;
        u32 Alpha = (u32)(110.f + 145.f * Near);
        u32 Color = (Alpha << 24) | THREAT_POINTER_RGB;
        v2 Side = V2(-Toward.Y, Toward.X);
        for(i32 Arm = -3; Arm <= 3; Arm++)
        {
            float Back = (float)(Arm < 0 ? -Arm : Arm) * 4.f;
            v2 P = Anchor + 4.f * (float)Arm * Side - Back * Toward;
            DrawPointerDot(RenderContext, P + V2(1.f, 1.f), 5.f, 0xA0000000);
            DrawPointerDot(RenderContext, P, 4.f, Color);
        }
        Shown++;
    }
}
