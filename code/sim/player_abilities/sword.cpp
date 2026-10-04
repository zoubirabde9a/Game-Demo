/* Sword (right click): a swing toward the aim that hits each thing in its
   slice once (UpdateSword, update.cpp; reach and width in entity.h). The
   player lunges a little that way. Timing is its row in
   spawn_actions.cpp. */

internal void
SpawnSwordSwing(app_state *AppState, world *World, memory_arena *Arena,
                world_entity *Player, v2 Dir, player_tick *Tick)
{
    Tick->Acceleration *= 0.6f;
    Tick->DDPlayer.XY = Dir;
    v3 SwordPosition = Player->Position +
        V3(SWORD_OFFSET * Dir.X, SWORD_OFFSET * Dir.Y, 0.f);
    world_entity *Sword = AddSword(AppState, World, Arena, SwordPosition,
                                   Player, DominantFacing(Dir));
    Sword->CastingDirection = Dir;
}
