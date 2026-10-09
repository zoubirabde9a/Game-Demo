/* Flame Wave (role_kits/striker.cpp, the right click): the Wildfire
   branch's spell against Fireguard. Fire rolls out in a cone in front of
   the striker, FLAME_WAVE_REACH deep: every foe in it takes
   FLAME_WAVE_DAMAGE, is thrown back, and takes FLAME_WAVE_STACKS of
   Searing, so a pack walks into the striker's Detonate or Meteor already
   marked. Meteor's burst stands in for its look on clients. Its numbers
   are in role_numbers.h. */

internal void
CastFlameWave(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    u32 By = Player->PlayerIndex;
    v2 Aim = NormalizeOr(GetPlayerAim(Player), V2(1.f, 0.f));
    float Edge = Cos(FLAME_WAVE_HALF_ANGLE);
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        v2 Offset = Monster->Position.XY - Player->Position.XY;
        float Distance = Length(Offset);
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            RoomAtPosition(World, Monster->Position.XY) != Room ||
            Distance > FLAME_WAVE_REACH + 0.5f * Monster->Dimensions.X ||
            (Distance > 1.f && DotProduct(Offset, Aim) < Edge * Distance))
        {
            continue;
        }
        hit Hit = {FLAME_WAVE_DAMAGE, FLAME_WAVE_SHOVE, 80.f, 80.f, 0.f, SimBurst_Count};
        ApplyHit(AppState, World, Monster, &Hit, NormalizeOr(Offset, Aim), Player, By);
        if (Monster->IsPresent && Monster->Hp > 0.f)
        {
            AddSearing(AppState->Dungeon, World, Monster, FLAME_WAVE_STACKS, By);
        }
    }
    v3 Front = Player->Position;
    Front.XY += 0.5f * FLAME_WAVE_REACH * Aim;
    EmitBurst(&AppState->Events, SimBurst_InfernoBlast, (u8)By, Front);
    EmitSound(&AppState->Events, AssetType_SfxFireCast, Player->Position);
}
