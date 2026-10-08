/* Ranger effects (sim/dungeon/role_kits/ranger.cpp), included by
   classes/class_fx.cpp: how a Ranger looks in a run, its bursts
   (SimBurst_RangerFirst on, their rows in ranger_bursts.inc) and what its
   spells leave in the world. */

// NOTE(zoubir): a living Ranger's look, over its sprite, every frame
internal void
DrawRangerLook(render_context *RenderContext, app_state *AppState, player_slot *Slot,
            world_entity *Player, float Clock, v3 CameraOffset)
{
}

// NOTE(zoubir): burst Index of the class's (Burst->Kind -
// SimBurst_RangerFirst), T from 0 to 1 over its row's Seconds
internal void
DrawRangerBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
             u32 Index, float T, v3 CameraOffset)
{
}

// NOTE(zoubir): every frame in a run, over the world: what lasts (zones,
// marks, projectiles)
internal void
DrawRangerFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
}
