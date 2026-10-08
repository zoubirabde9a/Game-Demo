/* Class kits (role_abilities.cpp): the classes after the first three,
   one file each, and the switches that send each hook to the class of
   the slot. A class's kit implements every hook here, even as a stub. */

#include "ranger.cpp"
#include "berserker.cpp"
#include "shadowblade.cpp"

internal bool32
ClassKeyWindsUp(player_slot *Slot, u32 Key)
{
    switch(Slot->Role)
    {
        case PlayerRole_Ranger: return RangerKeyWindsUp(Key);
        case PlayerRole_Berserker: return BerserkerKeyWindsUp(Key);
        case PlayerRole_Shadowblade: return ShadowbladeKeyWindsUp(Key);
    }
    return false;
}

internal bool32
CastClassKey(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
             world_entity *Player, u32 Key)
{
    switch(Slot->Role)
    {
        case PlayerRole_Ranger: return CastRangerKey(AppState, World, Arena, Slot, Player, Key);
        case PlayerRole_Berserker: return CastBerserkerKey(AppState, World, Arena, Slot, Player, Key);
        case PlayerRole_Shadowblade: return CastShadowbladeKey(AppState, World, Arena, Slot, Player, Key);
    }
    return false;
}

internal void
FinishClassCast(app_state *AppState, player_slot *Slot, world_entity *Player, player_spell Spell)
{
    switch(Slot->Role)
    {
        case PlayerRole_Ranger: FinishRangerCast(AppState, Slot, Player, Spell); break;
        case PlayerRole_Berserker: FinishBerserkerCast(AppState, Slot, Player, Spell); break;
        case PlayerRole_Shadowblade: FinishShadowbladeCast(AppState, Slot, Player, Spell); break;
    }
}

internal void
OnClassHit(app_state *AppState, player_slot *Attacker, world_entity *Target,
           world_entity *Source, float Damage)
{
    switch(Attacker->Role)
    {
        case PlayerRole_Ranger: OnRangerHit(AppState, Attacker, Target, Source, Damage); break;
        case PlayerRole_Berserker: OnBerserkerHit(AppState, Attacker, Target, Source, Damage); break;
        case PlayerRole_Shadowblade: OnShadowbladeHit(AppState, Attacker, Target, Source, Damage); break;
    }
}

internal float
ClassDealtScale(player_slot *Slot, world_entity *Target)
{
    switch(Slot->Role)
    {
        case PlayerRole_Ranger: return RangerDealtScale(Slot, Target);
        case PlayerRole_Berserker: return BerserkerDealtScale(Slot, Target);
        case PlayerRole_Shadowblade: return ShadowbladeDealtScale(Slot, Target);
    }
    return 1.f;
}

internal float
ClassTakenScale(player_slot *Slot, world_entity *Player)
{
    switch(Slot->Role)
    {
        case PlayerRole_Ranger: return RangerTakenScale(Slot, Player);
        case PlayerRole_Berserker: return BerserkerTakenScale(Slot, Player);
        case PlayerRole_Shadowblade: return ShadowbladeTakenScale(Slot, Player);
    }
    return 1.f;
}

internal float
ClassSpellCooldown(player_slot *Slot, u32 Key, float Base)
{
    switch(Slot->Role)
    {
        case PlayerRole_Ranger: return RangerSpellCooldown(Slot, Key, Base);
        case PlayerRole_Berserker: return BerserkerSpellCooldown(Slot, Key, Base);
        case PlayerRole_Shadowblade: return ShadowbladeSpellCooldown(Slot, Key, Base);
    }
    return Base;
}

internal float
ClassSpellRadius(player_slot *Slot, u32 Key, float Base)
{
    switch(Slot->Role)
    {
        case PlayerRole_Ranger: return RangerSpellRadius(Slot, Key, Base);
        case PlayerRole_Berserker: return BerserkerSpellRadius(Slot, Key, Base);
        case PlayerRole_Shadowblade: return ShadowbladeSpellRadius(Slot, Key, Base);
    }
    return Base;
}

internal void
UpdateClassEffects(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    UpdateRangerEffects(AppState, Run, DeltaTime);
    UpdateBerserkerEffects(AppState, Run, DeltaTime);
    UpdateShadowbladeEffects(AppState, Run, DeltaTime);
}

// NOTE(zoubir): whether a class flies Player through the air on its own
// course (a Berserker's Leap): MovePlayer (player_update/movement.cpp)
// then lets neither the keys nor the drag touch its speed across, so the
// flight is a plain arc. Read from ClassFlags, which snapshots carry, so
// a client predicting its own Berserker flies the same arc the server
// does instead of a long jump the server keeps correcting
internal bool32
ClassCarriesPlayer(app_state *AppState, world_entity *Player)
{
    if (!IsDungeon(AppState) || Player->PlayerIndex >= MAX_PLAYERS)
    {
        return false;
    }
    player_slot *Slot = &AppState->Players[Player->PlayerIndex];
    bool32 Result = Slot->Role == PlayerRole_Berserker &&
        (Slot->ClassFlags & BERSERKER_FLAG_LEAPING);
    return Result;
}
