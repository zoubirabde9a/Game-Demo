/* Giant fireball look (sim/dungeon/role_kits/striker.cpp). The ball in
   flight is not in the snapshot, which has no room for it: the server
   sends a SimBurst_GiantFireball where it leaves the striker's hand,
   along its heading, and every client, offline too, flies its own copy
   at GIANT_FIREBALL_SPEED for the burst's life, the whole flight. Where
   the real one bursts the server sends SimBurst_GiantFireballBlast, which
   ends the striker's oldest ball in flight (AddRoleBurst, role_fx.cpp)
   and plays the Meteor's blast, wider. Included by role_fx.cpp. */

#define GIANT_FIREBALL_FX_RADIUS 22.f

// NOTE(zoubir): the ball T of the way through its flight
internal void
DrawGiantFireball(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                  float T, v3 CameraOffset)
{
    float Clock = GetFxClock(AppState);
    v2 Dir = V2(Cos(Burst->Angle), Sin(Burst->Angle));
    float Flown = T * RoleBurstLife(Burst->Kind) * GIANT_FIREBALL_SPEED;
    v3 Position = Burst->Position;
    Position.XY += Flown * Dir;
    v2 Centre = BurstToScreen(Position, CameraOffset);
    v2 Back = -1.f * Dir;
    float Flicker = 1.f + 0.08f * Sin(23.f * Clock + (float)Burst->Slot);
    float Radius = GIANT_FIREBALL_FX_RADIUS * Flicker;
    DrawFxStreak(RenderContext, Centre + 3.2f * Radius * Back, Centre, 1.6f * Radius,
                 FxColor(0.f, ROLE_FX_FIRE_RGB), FxColor(0.7f, ROLE_FX_FIRE_RGB));
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 2.6f * Radius, Centre.Y - 2.6f * Radius,
                   5.2f * Radius, 5.2f * Radius, FxColor(0.9f, ROLE_FX_FIRE_RGB),
                   RenderBlend_Additive);
    DrawFxDot(RenderContext, Centre, Radius, FxColor(1.f, ROLE_FX_FIRE_RGB));
    DrawFxDot(RenderContext, Centre, 0.6f * Radius, FxColor(1.f, ROLE_FX_EMBER_RGB));
    DrawFxDot(RenderContext, Centre - V2(3.f, 3.f), 0.3f * Radius, FxColor(1.f, 0x00D0F0FF));
    for(u32 Spark = 0; Spark < 6; Spark++)
    {
        float Phase = DungeonFxFraction(2.2f * Clock + BurstJitter(Spark, 111));
        v2 Side = V2(-Back.Y, Back.X) * (BurstJitter(Spark, 112) - 0.5f) * Radius;
        v2 P = Centre + (Radius + 2.5f * Radius * Phase) * Back + Side;
        DrawFxDot(RenderContext, P, 4.f * (1.f - Phase), FxColor(1.f - Phase, ROLE_FX_EMBER_RGB));
    }
}
