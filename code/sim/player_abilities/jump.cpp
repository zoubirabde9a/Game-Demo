/* Jump (Space): an upward kick, only from the ground. */

internal void
UseJump(app_state *AppState, world_entity *Player, player_input *Input,
        player_tick *Tick)
{
    // NOTE(zoubir): only from the ground, pressing again mid-air used
    // to restart the jump and let the player fly
    if (WasPressed(Input, PlayerButton_Jump) && !Tick->Jumping)
    {
        Player->State = EntityState_Jumping;
        Player->Velocity.Z = 230.f;
        EmitSound(&AppState->Events, AssetType_ZoubirAudio, Player->Position);
    }
}
