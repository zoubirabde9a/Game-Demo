/* Shadowblade effects (sim/dungeon/role_kits/shadowblade.cpp), included by
   classes/class_fx.cpp: how a Shadowblade looks in a run, its bursts
   (SimBurst_ShadowbladeFirst on, their rows in shadowblade_bursts.inc) and what its
   spells leave in the world. */

// NOTE(zoubir): a living Shadowblade's look, over its sprite, every frame
internal void
DrawShadowbladeLook(render_context *RenderContext, app_state *AppState, player_slot *Slot,
            world_entity *Player, float Clock, v3 CameraOffset)
{
}

// NOTE(zoubir): burst Index of the class's (Burst->Kind -
// SimBurst_ShadowbladeFirst), T from 0 to 1 over its row's Seconds
internal void
DrawShadowbladeBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
             u32 Index, float T, v3 CameraOffset)
{
}

// NOTE(zoubir): every frame in a run, over the world: what lasts (zones,
// marks, projectiles)
internal void
DrawShadowbladeFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
}
