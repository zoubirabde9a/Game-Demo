/* Searing marks (sim/dungeon/role_kits/striker.cpp) as the party sees
   them, included by role_fx.cpp: a little flame over a marked monster's
   head for each stack, burning hotter at a full mark so the striker
   knows Detonate is ready to spend; read from the run, so the same
   offline and online (client/dungeon/dungeon_net.cpp). */

#define SEARING_PIP_GAP 11.f

internal void
DrawSearingMarks(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    dungeon_run *Run = AppState->Dungeon;
    world *World = &AppState->World;
    float Clock = GetFxClock(AppState);
    for(u32 Index = 0; Index < MAX_SEARING; Index++)
    {
        searing_mark *Mark = &Run->Searing[Index];
        if (!Mark->Stacks || Mark->Slot >= World->EntityCount)
        {
            continue;
        }
        world_entity *Monster = &World->Entities[Mark->Slot];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f)
        {
            continue;
        }
        bool32 Full = Mark->Stacks >= SEARING_MOST;
        float Height = Maximum(24.f, Monster->Dimensions.Y) + 10.f;
        v2 Head = BurstToScreen(V3(Monster->Position.X, Monster->Position.Y,
                                   Monster->Position.Z + Height), CameraOffset);
        float Left = -0.5f * SEARING_PIP_GAP * (float)(SEARING_MOST - 1);
        for(u32 Pip = 0; Pip < SEARING_MOST; Pip++)
        {
            v2 At = Head + V2(Left + SEARING_PIP_GAP * (float)Pip, 0.f);
            if (Pip >= Mark->Stacks)
            {
                DrawFxDot(RenderContext, At, 3.f, FxColor(0.35f, 0x00203040));
                continue;
            }
            float Flicker = 0.5f + 0.5f * Sin(14.f * Clock + 2.f * (float)Pip + (float)Index);
            float Size = (Full ? 6.f : 4.5f) + 1.5f * Flicker;
            if (Full)
            {
                DrawShaderQuad(RenderContext, Shader_Glow, At.X - 12.f, At.Y - 12.f, 24.f, 24.f,
                               FxColor(0.6f, ROLE_FX_EMBER_RGB), RenderBlend_Additive);
            }
            DrawFxDot(RenderContext, At, Size, FxColor(0.95f, ROLE_FX_FIRE_RGB));
            DrawFxDot(RenderContext, At - V2(0.f, 0.4f * Size), 0.5f * Size,
                      FxColor(1.f, ROLE_FX_EMBER_RGB));
        }
    }
}
