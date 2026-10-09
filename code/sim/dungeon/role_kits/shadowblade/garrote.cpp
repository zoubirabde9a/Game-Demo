/* Garrote (role_kits/shadowblade.cpp, C): the Assassination branch's spell
   against Deadly Throw. A wire round the foe in reach: whatever it was
   winding up is broken off (BreakWindup, role_kits/tank.cpp), it is held
   for GARROTE_STUN_SECONDS, cut for GARROTE_DAMAGE, poisoned, and the
   Shadowblade gains GARROTE_POINTS combo points. Deadly Throw finishes
   from afar; Garrote opens up close and stops a big blow. With no foe in
   reach it does not cast. */

internal bool32
CastGarrote(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world_entity *Foe = AttackTarget(AppState, Slot, Player, GARROTE_REACH);
    if (!Foe)
    {
        return false;
    }
    u8 SlotIndex = (u8)Player->PlayerIndex;
    v2 ToFoe = Foe->Position.XY - Player->Position.XY;
    v2 Away = LengthSq(ToFoe) > 1.f ? DirectionTo(ToFoe) : NormalizeOr(Player->Aim, V2(1.f, 0.f));
    BreakWindup(Foe);
    bool32 Killed = ShadowbladeStrike(AppState, Slot, Player, Foe, GARROTE_DAMAGE, 0.f, Away);
    if (!Killed)
    {
        ApplyStatus(Foe, StatusEffect_Stunned, GARROTE_STUN_SECONDS);
        PoisonFoe(AppState, Slot, Foe);
    }
    AddComboPoints(Slot, GARROTE_POINTS);
    EmitBurst(&AppState->Events, SimBurst_Finisher, SlotIndex, ChestOf(Foe), ATan2(Away.Y, Away.X));
    EmitSound(&AppState->Events, AssetType_SfxSword, Player->Position);
    return true;
}
