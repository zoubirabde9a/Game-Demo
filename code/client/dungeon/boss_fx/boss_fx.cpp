/* Boss effects: how the big boss abilities look, drawn with their own
   shaders (build/shaders/fx/boss_ground.frag, boss_beam.frag,
   boss_shock.frag) instead of rows of dots.

   On the floor, in the ground pass under the bodies (boss_ground_fx.cpp):
   gravity wells, beams, eclipses, smites and what a Starless boss leaves
   when it goes. When a boss's blow lands (boss_impacts.cpp): a shockwave,
   a flash of light on the floor (boss_lights.cpp, included after
   world_lights.cpp), a shake of the camera, and for a Smite or a Falling
   Star a shaft of light out of the sky. The overlays that stay over the
   bodies (chevrons, motes, the falling star itself) are drawn by
   rift_fx.cpp, starless_fx.cpp and boss_departure_fx.cpp, which skip
   their dotted floor when these shaders built.

   Entry points: DrawBossGroundFx from DrawDangerZones (danger_zones.cpp),
   AddBossImpact from AddDangerImpact, DrawBossOverlayFx from
   screen_pass.inc, GatherBossLights from GatherWorldLights. Depends on
   danger_zones.cpp, fx_bursts.cpp (camera shake) and
   draw_entities/windup_glow.cpp (each kind's colour). */

#include "boss_fx_draw.cpp"
#include "boss_impacts.cpp"
#include "boss_ground_fx.cpp"

internal void
DrawBossOverlayFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    DrawBossImpactShafts(RenderContext, AppState, CameraOffset, GetFxClock(AppState));
}
