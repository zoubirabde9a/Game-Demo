/* Player cast, each tick (sim/player_casts.cpp has the spells and the
   rules): slows the walk and holds a hovering caster's height while the
   wind-up runs, then hands the spell back to the ability that owns it.
   Runs after the abilities, so a cast started this tick counts this tick
   too, and a dash this tick has already cut the cast. */

// NOTE(zoubir): the spell's effect, now its wind-up is over. A client
// predicting its own player moves it (the slam's dive) but leaves what
// touches anyone else to the server
internal void
FinishPlayerCast(app_state *AppState, world *World, memory_arena *Arena,
                 world_entity *Player, player_input *Input, float DeltaTime,
                 player_spell Spell)
{
    bool32 Authoritative = !IsPredictedPlayer(AppState, Player);
    switch(Spell)
    {
        case PlayerSpell_Shockwave:
        case PlayerSpell_Push:
        case PlayerSpell_Launch:
        {
            if (Authoritative)
            {
                FireAreaCast(AppState, World, Player, Spell);
            }
        } break;

        case PlayerSpell_Slam:
        {
            RunMovementCast(AppState, World, Arena, Player, Input, DeltaTime,
                            Spell);
        } break;

        case PlayerSpell_RewindSelf:
        case PlayerSpell_RewindBubble:
        case PlayerSpell_RewindWorld:
        {
            if (Authoritative)
            {
                FinishRewindWindUp(AppState, Player,
                                   (rewind_kind)(Spell - PlayerSpell_RewindSelf));
            }
        } break;

        case PlayerSpell_None:
        case PlayerSpell_Count:
        {
            InvalidCodePath;
        } break;
    }
}

internal void
UpdatePlayerCast(app_state *AppState, world *World, memory_arena *Arena,
                 world_entity *Player, player_input *Input, float DeltaTime,
                 player_tick *Tick)
{
    if (!IsPlayerCasting(Player))
    {
        return;
    }
    player_spell Spell = (player_spell)Player->CastSpell;
    player_spell_cast *Cast = &PlayerSpells[Spell];
    if (Player->CastLeft > 0.f)
    {
        Tick->Acceleration *= Cast->MoveScale;
        Tick->Hover = Cast->Hover;
    }
    Player->CastLeft -= DeltaTime;
    if (Player->CastLeft <= 0.f)
    {
        // NOTE(zoubir): the slam's dive starts this tick, so it must fall
        Tick->Hover = false;
        CancelPlayerCast(Player);
        FinishPlayerCast(AppState, World, Arena, Player, Input, DeltaTime, Spell);
    }
}
