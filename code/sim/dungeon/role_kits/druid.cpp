/* The Druid's kit (role_abilities.cpp, through class_kits.cpp).
   Numbers, talents and the spell table are role_kits/druid_defs.cpp; its
   state is role_kits/druid.h. */

internal bool32
DruidKeyWindsUp(u32 Key)
{
    return false;
}

internal bool32
CastDruidKey(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
           world_entity *Player, u32 Key)
{
    return false;
}

internal void
FinishDruidCast(app_state *AppState, player_slot *Slot, world_entity *Player, player_spell Spell)
{
}

internal void
OnDruidHit(app_state *AppState, player_slot *Attacker, world_entity *Target,
         world_entity *Source, float Damage)
{
}

internal float
DruidDealtScale(player_slot *Slot, world_entity *Target)
{
    return 1.f;
}

internal float
DruidTakenScale(player_slot *Slot, world_entity *Player)
{
    return 1.f;
}

internal float
DruidSpellCooldown(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

internal float
DruidSpellRadius(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

internal void
UpdateDruidEffects(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
}
