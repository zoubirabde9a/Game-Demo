/* Foe marks (sim/dungeon/role_kits/foe_marks.cpp) as the party sees
   them, included by role_fx.cpp, read from the run so the same offline
   and online (client/dungeon/dungeon_net.cpp):
   - Searing: a little flame over the monster's head for each stack,
     burning hotter at a full mark, and embers rising off the monster
     while the mark burns, more of them the more stacks it has.
   - Sunder: a cracked steel ring at the monster's feet while it takes
     more from everyone, so the tank sees when to slam again. */

#define SEARING_PIP_GAP 15.f
// NOTE(zoubir): over the head and clear of the health bar above it
#define SEARING_PIP_LIFT 24.f

internal void
DrawFoeMarks(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    dungeon_run *Run = AppState->Dungeon;
    world *World = &AppState->World;
    float Clock = GetFxClock(AppState);
    for(u32 Index = 0; Index < MAX_FOE_MARKS; Index++)
    {
        foe_mark *Mark = &Run->Marks[Index];
        if (Mark->Slot >= World->EntityCount)
        {
            continue;
        }
        world_entity *Monster = &World->Entities[Mark->Slot];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f)
        {
            continue;
        }
        if (Mark->SunderSeconds > 0.f)
        {
            v2 Feet = BurstToScreen(V3(Monster->Position.X, Monster->Position.Y,
                                       Monster->GroundZ), CameraOffset);
            float Radius = Maximum(16.f, 0.55f * Monster->Dimensions.X);
            DrawCastPreviewArea(RenderContext, Feet, Radius, 0.f, Pi32, 0.25f, 0.8f,
                                ROLE_FX_SLAM_RGB);
            for(u32 Crack = 0; Crack < 5; Crack++)
            {
                float A = 2.f * Pi32 * ((float)Crack + 0.3f * BurstJitter(Crack + Index, 211)) / 5.f;
                v2 Dir = GroundCircle(A, 1.f);
                DrawFxStreak(RenderContext, Feet + (0.6f * Radius) * Dir, Feet + (1.1f * Radius) * Dir,
                             2.f, FxColor(0.f, ROLE_FX_SLAM_RGB), FxColor(0.8f, ROLE_FX_SLAM_RGB));
            }
        }
        if (!Mark->Stacks)
        {
            continue;
        }
        bool32 Full = Mark->Stacks >= SEARING_MOST;
        // NOTE(zoubir): embers off the body, two a stack
        for(u32 Ember = 0; Ember < 2 * Mark->Stacks; Ember++)
        {
            float Phase = Clock * (1.1f + 0.15f * (float)(Ember % 3)) + 0.37f * (float)Ember;
            float Rise = DungeonFxFraction(Phase);
            float Side = (BurstJitter(Ember + 7 * Index, 97) - 0.5f) * Maximum(16.f, Monster->Dimensions.X);
            v3 Point = Monster->Position +
                V3(Side, 0.f, 6.f + Rise * (Maximum(24.f, Monster->Dimensions.Y) + 10.f));
            DrawFxDot(RenderContext, BurstToScreen(Point, CameraOffset), 3.f - 1.5f * Rise,
                      FxColor(0.9f * (1.f - Rise), Ember % 2 ? ROLE_FX_EMBER_RGB : ROLE_FX_FIRE_RGB));
        }
        float Height = Maximum(24.f, Monster->Dimensions.Y) + SEARING_PIP_LIFT;
        v2 Head = BurstToScreen(V3(Monster->Position.X, Monster->Position.Y,
                                   Monster->Position.Z + Height), CameraOffset);
        float Left = -0.5f * SEARING_PIP_GAP * (float)(SEARING_MOST - 1);
        for(u32 Pip = 0; Pip < SEARING_MOST; Pip++)
        {
            v2 At = Head + V2(Left + SEARING_PIP_GAP * (float)Pip, 0.f);
            // NOTE(zoubir): a dark socket under every pip, so an empty one
            // shows what is missing and a lit one stands off the floor
            DrawFxDot(RenderContext, At, 7.5f, FxColor(0.7f, 0x00101418));
            if (Pip >= Mark->Stacks)
            {
                DrawFxDot(RenderContext, At, 4.f, FxColor(0.5f, 0x00404850));
                continue;
            }
            float Flicker = 0.5f + 0.5f * Sin(14.f * Clock + 2.f * (float)Pip + (float)Index);
            float Size = (Full ? 7.5f : 6.f) + 1.5f * Flicker;
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
