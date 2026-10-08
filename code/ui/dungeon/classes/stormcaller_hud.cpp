/* Stormcaller HUD (ui/dungeon/classes/class_hud.cpp): the class's emblem on
   the party frames and the meter, and what the local Stormcaller sees of its own
   class over the ability bar. Stubs until the kit lands. */

// NOTE(zoubir): the emblem's picture, centred on C, S about a third of
// its size; Fill in the class colour, Light paler
internal void
DrawStormcallerEmblem(render_context *RenderContext, v2 C, float S, u32 Fill, u32 Light)
{
    DrawPartyQuad(RenderContext, C + V2(-S, -S), C + V2(S, -S), C + V2(S, S), C + V2(-S, S), Fill);
}

// NOTE(zoubir): every frame while the local player is a Stormcaller
internal void
DrawStormcallerHud(render_context *RenderContext, app_state *AppState, player_slot *Slot,
                   u32 WindowWidth, u32 WindowHeight)
{
}
