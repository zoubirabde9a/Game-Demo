/* The Shadowblade's kit (role_abilities.cpp, through class_kits.cpp): twin daggers.
   Numbers, talents and the spell table are role_kits/shadowblade_defs.cpp; its
   state is role_kits/shadowblade.h. */

// NOTE(zoubir): whether Key only starts a wind-up when pressed (the cast
// runs through sim/player_casts.cpp, then FinishShadowbladeCast fires it)
internal bool32
ShadowbladeKeyWindsUp(u32 Key)
{
    return false;
}

// NOTE(zoubir): Key pressed, off cooldown and learned; true when it cast,
// which spends the cooldown
internal bool32
CastShadowbladeKey(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
           world_entity *Player, u32 Key)
{
    return false;
}

// NOTE(zoubir): a wind-up of this class is over
internal void
FinishShadowbladeCast(app_state *AppState, player_slot *Slot, world_entity *Player, player_spell Spell)
{
}

// NOTE(zoubir): any hit of this class's player on a monster dealt Damage
internal void
OnShadowbladeHit(app_state *AppState, player_slot *Attacker, world_entity *Target,
         world_entity *Source, float Damage)
{
}

// NOTE(zoubir): the share of a hit on Target the player deals, and of a
// hit on it the player takes, after its talents and buffs
internal float
ShadowbladeDealtScale(player_slot *Slot, world_entity *Target)
{
    return 1.f;
}

internal float
ShadowbladeTakenScale(player_slot *Slot, world_entity *Player)
{
    return 1.f;
}

// NOTE(zoubir): Key's cooldown and a ground spell's radius after talents,
// from the table's Base
internal float
ShadowbladeSpellCooldown(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

internal float
ShadowbladeSpellRadius(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

// NOTE(zoubir): once a tick, from UpdateRoleEffects: what the class's
// spells left behind, for every player of the class
internal void
UpdateShadowbladeEffects(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
}
