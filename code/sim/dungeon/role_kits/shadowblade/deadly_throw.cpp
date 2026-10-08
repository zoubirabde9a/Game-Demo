/* Deadly Throw (role_kits/shadowblade.cpp, X): the Shadowblade's ranged
   finisher. A poisoned dagger flies at the foe under the cursor, up to
   DEADLY_THROW_RANGE off, at once (no wind-up), for DEADLY_THROW_DAMAGE
   and DEADLY_THROW_PER_POINT a combo point, spending them all. It
   poisons the foe and slows it for DEADLY_THROW_SLOW_PER_POINT a point,
   so a fleeing or kited foe can be finished from afar. It hits softer a
   point than Eviscerate, which the Shadowblade has to stand in reach for.
   With no points, or no foe in range, it does not cast. */

internal bool32
CastDeadlyThrow(app_state *AppState, world *World, player_slot *Slot, world_entity *Player)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    u32 Points = Slot->ClassMeter;
    if (Points == 0)
    {
        if (!Slot->Predicted)
        {
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_ShadowbladeFirst, ShadowbladeBurst_Empty),
                      SlotIndex, ChestOf(Player));
        }
        return false;
    }
    world_entity *Foe = AttackTarget(AppState, Slot, Player, DEADLY_THROW_RANGE);
    if (!Foe)
    {
        return false;
    }
    v2 ToFoe = Foe->Position.XY - Player->Position.XY;
    v2 Away = LengthSq(ToFoe) > 1.f ? DirectionTo(ToFoe) :
        (LengthSq(Player->Aim) > 0.0001f ? DirectionTo(Player->Aim) : V2(1.f, 0.f));
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_ShadowbladeFirst, ShadowbladeBurst_Throw),
              SlotIndex, ShadowbladeBurstSpot(ChestOf(Foe), Points), ATan2(Away.Y, Away.X));
    EmitSound(&AppState->Events, AssetType_SfxKunai, Player->Position);
    Slot->ClassMeter = 0;
    bool32 Killed = ShadowbladeStrike(AppState, Slot, Player, Foe,
                                      DEADLY_THROW_DAMAGE + DEADLY_THROW_PER_POINT * (float)Points,
                                      DEADLY_THROW_SHOVE, Away);
    if (!Killed)
    {
        PoisonFoe(AppState, Slot, Foe);
        ApplyStatus(Foe, StatusEffect_Slowed, DEADLY_THROW_SLOW_PER_POINT * (float)Points);
    }
    return true;
}
