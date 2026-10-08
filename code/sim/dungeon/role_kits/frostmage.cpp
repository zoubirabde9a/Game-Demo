/* The Frost Mage's kit (role_abilities.cpp, through class_kits.cpp).
   Numbers, talents and the spell table are role_kits/frostmage_defs.cpp; its
   state is role_kits/frostmage.h. */

internal bool32
FrostMageKeyWindsUp(u32 Key)
{
    return false;
}

internal bool32
CastFrostMageKey(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
           world_entity *Player, u32 Key)
{
    return false;
}

internal void
FinishFrostMageCast(app_state *AppState, player_slot *Slot, world_entity *Player, player_spell Spell)
{
}

internal void
OnFrostMageHit(app_state *AppState, player_slot *Attacker, world_entity *Target,
         world_entity *Source, float Damage)
{
}

internal float
FrostMageDealtScale(player_slot *Slot, world_entity *Target)
{
    return 1.f;
}

internal float
FrostMageTakenScale(player_slot *Slot, world_entity *Player)
{
    return 1.f;
}

internal float
FrostMageSpellCooldown(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

internal float
FrostMageSpellRadius(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

internal void
UpdateFrostMageEffects(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
}
