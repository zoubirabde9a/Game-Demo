/* Player ability effects drawn over the world, in screen space, after the
   world and before the HUD. Each effect keeps its own clock, so it looks
   the same offline and online (where the server's entities arrive as
   replicas):

   - aim marker: dots from the local player toward the cursor, where the
     sword and fireball will go (player_fx/aim_marker.cpp);
   - dash streaks: fading dots along a dashing player's path
     (player_fx/dash_streaks.cpp);
   - hit numbers: the damage each hit did, rising from the target
     (player_fx/hit_numbers.cpp);
   - blink preview: where a blink would land, while it is ready
     (player_fx/blink_preview.cpp);
   - fireball trails: embers cooling behind every fireball
     (player_fx/fireball_trails.cpp);
   - kunai: every kunai's blade and the streak behind it
     (player_fx/kunai_fx.cpp);
   - shield bubbles: a sphere of light around a player whose Shield is
     up (player_fx/shield_bubble.cpp);
   - status motes: embers, bubbles, drops and sparkles round units with a
     status effect running (player_fx/status_fx.cpp);
   - status words: an effect's name over a unit as it starts
     ("Stunned", "Rooted") (player_fx/status_words.cpp);
   - cast glow: light gathering in the hand of every player winding up a
     spell, and its flash when the spell goes off (player_fx/cast_glow.cpp,
     cast_fx.cpp);
   - cast bars: a bar over every player winding up a spell
     (cast_bars.cpp);
   - bursts the simulation asks for (sword swings, casts, Shockwave,
     Push, Launch, jumps, landings) and stars over stunned heads (fx_bursts.cpp).

   Entry point: DrawPlayerAbilityFx, once a frame from app.cpp. */

#include "player_fx/aim_marker.cpp"
#include "player_fx/dash_streaks.cpp"
#include "player_fx/hit_numbers.cpp"
#include "player_fx/blink_preview.cpp"
#include "player_fx/fireball_trails.cpp"
#include "player_fx/kunai_fx.cpp"
#include "player_fx/shield_bubble.cpp"
#include "player_fx/status_fx.cpp"
#include "player_fx/status_words.cpp"
#include "player_fx/cast_glow.cpp"

struct player_fx
{
    dash_streaks Dashes;
    hit_numbers Hits;
    status_words Words;
    fireball_trails Embers;
    kunai_trails Kunai;
    float ShieldGrow[MAX_PLAYERS];
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
    UpdateBodyPoses(AppState, DeltaTime);
    UpdateCastFx(AppState, DeltaTime);
    UpdateDashStreaks(&Fx->Dashes, AppState, DeltaTime);
    UpdateHitNumbers(&Fx->Hits, AppState, DeltaTime);
    UpdateStatusWords(&Fx->Words, AppState, DeltaTime);
    UpdateFireBallTrails(&Fx->Embers, AppState, DeltaTime);
    UpdateKunaiTrails(&Fx->Kunai, AppState, DeltaTime);

    DrawFireBallTrails(RenderContext, &Fx->Embers, CameraOffset);
    DrawKunaiFx(RenderContext, AppState, &Fx->Kunai, CameraOffset);
    DrawDashGhosts(RenderContext, AppState, &Fx->Dashes, CameraOffset);
    DrawDashStreaks(RenderContext, &Fx->Dashes, CameraOffset);
    DrawStatusMotes(RenderContext, AppState, CameraOffset);
    DrawShieldBubbles(RenderContext, AppState, Fx->ShieldGrow, CameraOffset, DeltaTime);
    DrawBlinkPreview(RenderContext, AppState, CameraOffset);
    DrawAimMarker(RenderContext, AppState, CameraOffset);
    DrawHitNumbers(RenderContext, AppState, &Fx->Hits, CameraOffset);
    DrawHitCombo(RenderContext, AppState, &Fx->Hits, CameraOffset);
    DrawStatusWords(RenderContext, AppState, &Fx->Words, CameraOffset);
    DrawCastGlows(RenderContext, AppState, CameraOffset);
    DrawPlayerCastBars(RenderContext, AppState, CameraOffset);
    DrawFxBursts(RenderContext, AppState, CameraOffset, DeltaTime);
}
