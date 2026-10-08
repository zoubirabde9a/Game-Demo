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
                 player_spell Spell, player_tick *Tick)
{
    bool32 Authoritative = !IsPredictedPlayer(AppState, Player);
    switch(Spell)
    {
        case PlayerSpell_Shockwave:
        case PlayerSpell_Push:
        case PlayerSpell_Launch:
        case PlayerSpell_FrostNova:
        case PlayerSpell_GravityWell:
        {
            if (Authoritative)
            {
                FireAreaCast(AppState, World, Player, Spell);
            }
        } break;

        case PlayerSpell_Slam:
        case PlayerSpell_Blink:
        {
            RunMovementCast(AppState, World, Arena, Player, Input, DeltaTime,
                            Spell, Tick);
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

        case PlayerSpell_Meteor:
        case PlayerSpell_GiantFireball:
        case PlayerSpell_RangerA:
        case PlayerSpell_RangerB:
        case PlayerSpell_BerserkerA:
        case PlayerSpell_BerserkerB:
        case PlayerSpell_ShadowbladeA:
        case PlayerSpell_ShadowbladeB:
        case PlayerSpell_StormcallerA:
        case PlayerSpell_StormcallerB:
        case PlayerSpell_DuelistA:
        case PlayerSpell_DuelistB:
        case PlayerSpell_FrostMageA:
        case PlayerSpell_FrostMageB:
        case PlayerSpell_DruidA:
        case PlayerSpell_DruidB:
        {
            if (Authoritative)
            {
                FinishRoleCast(AppState, Player, Spell);
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
        FinishPlayerCast(AppState, World, Arena, Player, Input, DeltaTime, Spell,
                         Tick);
    }
}
