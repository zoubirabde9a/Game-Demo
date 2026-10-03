/* Player ability effects drawn over the world, in screen space, after the
   world and before the HUD. Each effect keeps its own clock, so it looks
   the same offline and online (where the server's entities arrive as
   replicas):

   - aim marker: dots from the local player toward the cursor, where the
     sword and fireball will go (player_fx/aim_marker.cpp);
   - shockwave rings: a circle growing to the hit radius around a player
     whose shockwave fires (player_fx/shockwave_rings.cpp);
   - sword arcs: a sweep along each new sword swing
     (player_fx/sword_arcs.cpp);
   - dash streaks: fading dots along a dashing player's path
     (player_fx/dash_streaks.cpp);
   - hit numbers: the damage each hit did, rising from the target
     (player_fx/hit_numbers.cpp);
   - blink preview: where a blink would land, while it is ready
     (player_fx/blink_preview.cpp);
   - fireball trails: embers cooling behind every fireball
     (player_fx/fireball_trails.cpp).

   Entry point: DrawPlayerAbilityFx, once a frame from app.cpp. */

#include "player_fx/aim_marker.cpp"
#include "player_fx/shockwave_rings.cpp"
#include "player_fx/sword_arcs.cpp"
#include "player_fx/dash_streaks.cpp"
#include "player_fx/hit_numbers.cpp"
#include "player_fx/blink_preview.cpp"
#include "player_fx/fireball_trails.cpp"

struct player_fx
{
    shockwave_rings Rings;
    sword_arcs Swords;
    dash_streaks Dashes;
    hit_numbers Hits;
    fireball_trails Embers;
};

// NOTE(zoubir): the state lives in MemoryArena, not the world arena, so a
// map switch does not drop it
internal void
DrawPlayerAbilityFx(render_context *RenderContext, app_state *AppState,
                    v3 CameraOffset, float DeltaTime)
{
    if (!AppState->PlayerFx)
    {
        AppState->PlayerFx = AllocateStruct(&AppState->MemoryArena, player_fx);
        *AppState->PlayerFx = {};
    }
    player_fx *Fx = AppState->PlayerFx;
    UpdateShockwaveRings(&Fx->Rings, AppState, DeltaTime);
    UpdateSwordArcs(&Fx->Swords, AppState, DeltaTime);
    UpdateDashStreaks(&Fx->Dashes, AppState, DeltaTime);
    UpdateHitNumbers(&Fx->Hits, AppState, DeltaTime);
    UpdateFireBallTrails(&Fx->Embers, AppState, DeltaTime);

    DrawFireBallTrails(RenderContext, &Fx->Embers, CameraOffset);
    DrawDashStreaks(RenderContext, &Fx->Dashes, CameraOffset);
    DrawShockwaveRings(RenderContext, &Fx->Rings, CameraOffset);
    DrawSwordArcs(RenderContext, &Fx->Swords, CameraOffset);
    DrawBlinkPreview(RenderContext, AppState, CameraOffset);
    DrawAimMarker(RenderContext, AppState, CameraOffset);
    DrawHitNumbers(RenderContext, AppState, &Fx->Hits, CameraOffset);
}
