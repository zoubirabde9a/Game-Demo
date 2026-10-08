/* Twin Strike (role_kits/shadowblade.cpp, the right click): two cuts at what
   is in front, the second TWIN_STRIKE_CUT_GAP after the first (from
   UpdateShadowbladeEffects), the nearest foe in the arc taking each cut
   whole and the others TWIN_STRIKE_SPLASH of it; each foe cut is poisoned
   (PoisonFoe), and a combo point when either cut lands. */

// NOTE(zoubir): whether Foe is in reach of a Twin Strike cut along
// Direction, and how far it is
inline bool32
InTwinStrikeArc(world *World, world_entity *Player, world_entity *Foe, u32 Room, v2 Direction,
                float *Distance)
{
    v2 Offset = Foe->Position.XY - Player->Position.XY;
    *Distance = Length(Offset);
    float Reach = TWIN_STRIKE_REACH + 0.5f * Foe->Dimensions.X;
    bool32 Result = IsShadowbladeFoe(World, Foe, Room) && *Distance <= Reach &&
        (*Distance <= 8.f || DotProduct(Offset, Direction) >= Cos(TWIN_STRIKE_HALF_ARC) * *Distance);
    return Result;
}

// NOTE(zoubir): one of Twin Strike's cuts along Direction: the nearest
// foe in the arc takes it whole, the others TWIN_STRIKE_SPLASH of it, so
// the daggers duel rather than sweep; returns how many foes it cut
internal u32
TwinStrikeCut(app_state *AppState, player_slot *Slot, world_entity *Player, v2 Direction)
{
    world *World = &AppState->World;
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    world_entity *Nearest = 0;
    float Best = 0.f;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Foe = &World->Entities[EntityIndex];
        float Distance;
        if (InTwinStrikeArc(World, Player, Foe, Room, Direction, &Distance) && (!Nearest || Distance < Best))
        {
            Nearest = Foe;
            Best = Distance;
        }
    }
    u32 Result = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount && Nearest; EntityIndex++)
    {
        world_entity *Foe = &World->Entities[EntityIndex];
        float Distance;
        if (!InTwinStrikeArc(World, Player, Foe, Room, Direction, &Distance))
        {
            continue;
        }
        v2 Offset = Foe->Position.XY - Player->Position.XY;
        float Damage = TWIN_STRIKE_DAMAGE * (Foe == Nearest ? 1.f : TWIN_STRIKE_SPLASH);
        ShadowbladeStrike(AppState, Slot, Player, Foe, Damage, TWIN_STRIKE_SHOVE,
                          Distance > 0.f ? (1.f / Distance) * Offset : Direction);
        PoisonFoe(AppState, Slot, Foe);
        Result++;
    }
    return Result;
}

internal void
CastTwinStrike(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    // NOTE(zoubir): the foe under the cursor or nearest it, if one is in
    // reach, else where the player aims
    v2 Direction = LengthSq(Player->Aim) > 0.0001f ? DirectionTo(Player->Aim) : V2(1.f, 0.f);
    world_entity *Foe = AttackTarget(AppState, Slot, Player, TWIN_STRIKE_REACH + 40.f);
    if (Foe && LengthSq(Foe->Position.XY - Player->Position.XY) > 1.f)
    {
        Direction = DirectionTo(Foe->Position.XY - Player->Position.XY);
    }
    Slot->Shadowblade.CutLanded = TwinStrikeCut(AppState, Slot, Player, Direction) > 0;
    if (Slot->Shadowblade.CutLanded)
    {
        AddComboPoints(Slot, 1);
    }
    Slot->Shadowblade.CutDelay = TWIN_STRIKE_CUT_GAP;
    Slot->Shadowblade.CutDirection = Direction;
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_ShadowbladeFirst, ShadowbladeBurst_TwinStrike),
              (u8)Player->PlayerIndex, ChestOf(Player), ATan2(Direction.Y, Direction.X));
    EmitSound(&AppState->Events, AssetType_SfxSword, Player->Position);
}

// NOTE(zoubir): Twin Strike's second cut, from UpdateShadowbladeEffects;
// a point when the first one missed and this lands
internal void
SecondTwinCut(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    if (TwinStrikeCut(AppState, Slot, Player, Slot->Shadowblade.CutDirection) &&
        !Slot->Shadowblade.CutLanded)
    {
        AddComboPoints(Slot, 1);
    }
}

