/* Duelist effects (sim/dungeon/role_kits/duelist.cpp), included by
   classes/class_fx.cpp: how a Duelist looks in a run, its bursts
   (SimBurst_DuelistFirst on, their rows in duelist_bursts.inc) and what its
   spells leave in the world. Stubs until the kit lands. */

// NOTE(zoubir): a living Duelist's look, over its sprite, every frame
internal void
DrawDuelistLook(render_context *RenderContext, app_state *AppState, player_slot *Slot,
                world_entity *Player, float Clock, v3 CameraOffset)
{
}

// NOTE(zoubir): burst Index of the class's (Burst->Kind -
// SimBurst_DuelistFirst), T from 0 to 1 over its row's Seconds
internal void
DrawDuelistBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                 u32 Index, float T, v3 CameraOffset)
{
}

// NOTE(zoubir): every frame in a run, over the world: what lasts (zones,
// marks, projectiles)
internal void
DrawDuelistFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
}
