/* Player hits: what one of a player's attacks does to one target, as
   data. The area abilities and the sword combo are both lists of these;
   ApplyPlayerHit is the one place a player's hit is applied. */

struct player_hit
{
    float Damage;
    // NOTE(zoubir): horizontal speed given to the target, away from the
    // attacker; Lift is the vertical one
    float Shove;
    float Lift;
    // NOTE(zoubir): a stunned target flying into something is an impact
    // (impacts.cpp), credited to the attacker
    float StunSeconds;
    // NOTE(zoubir): drawn on each target hit; SimBurst_Count for none
    sim_burst Burst;
};

// NOTE(zoubir): Hit on Target, thrown along Away (a unit vector), by the
// player in slot BySlot through Source (the player, or its sword).
// Returns whether the target survived to be thrown.
internal bool32
ApplyPlayerHit(app_state *AppState, world *World, world_entity *Target,
               player_hit *Hit, v2 Away, u32 BySlot, world_entity *Source)
{
    if (Hit->Burst != SimBurst_Count && !IsDodging(Target))
    {
        v3 Chest = Target->Position;
        Chest.Z += 16.f;
        EmitBurst(&AppState->Events, Hit->Burst, (u8)BySlot, Chest,
                  ATan2(Away.Y, Away.X));
    }
    if (DamageEntity(AppState, World, Target, Hit->Damage, Source) ||
        !Target->IsPresent || IsDodging(Target))
    {
        return false;
    }
    Target->Velocity.XY += Hit->Shove * Away;
    if (Hit->Lift > 0.f)
    {
        Target->Velocity.Z = Hit->Lift;
    }
    if (Hit->StunSeconds > 0.f)
    {
        ApplyStatus(Target, StatusEffect_Stunned, Hit->StunSeconds);
        Target->ThrownBySlot = BySlot + 1;
    }
    return true;
}
