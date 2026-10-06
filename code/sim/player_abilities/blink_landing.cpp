/* Blink landing: where a blink toward a target puts the player. The jump
   goes through walls, trees and rocks; only the landing spot has to be
   clear. When the target is inside something, the player lands on the
   first clear spot back from it toward where they stand, which is on the
   far side of an obstacle whenever there is room there, and short of it
   when there is none. Monsters and players are not counted: they move,
   and landing inside one is undone by separation at the end of the tick.
   The blink (movement_abilities.cpp) and the client's landing rings
   (client/player_fx/blink_preview.cpp, client/cast_targeting/previews.cpp)
   both use it, so the ring shows where the jump lands. */

#define BLINK_LANDING_STEP 6.f

// NOTE(zoubir): whether a player standing at Position would overlap a
// wall, tree or terrain prop, or stick out of a bounded map
internal bool32
IsBlinkSpotBlocked(app_state *AppState, world *World, v2 Position, float Z)
{
    world_entity Probe = {};
    Probe.Type = EntityType_Player;
    Probe.Position = V3(Position.X, Position.Y, Z);
    Probe.Collision = AppState->PlayerCollision;
    entity_collision_volume *Total = &Probe.Collision->TotalVolume;
    rectangle3 Box = RectCenterHalfDims(Probe.Position + Total->Offset,
                                        Total->HalfDims);
    // NOTE(zoubir): past the outer wall there is no ground at all
    if (!World->Unbounded)
    {
        float Width = (float)(World->NumTilesX * World->TileWidth);
        float Height = (float)(World->NumTilesY * World->TileHeight);
        if (Box.Min.X < 0.f || Box.Min.Y < 0.f ||
            Box.Max.X > Width || Box.Max.Y > Height)
        {
            return true;
        }
    }
    world_entity *Nearby[MOVE_MAX_NEARBY];
    u32 Count = GatherEntitiesInBox(World, Box, Nearby, ArrayCount(Nearby));
    for(u32 Index = 0; Index < Count; Index++)
    {
        world_entity *Other = Nearby[Index];
        // NOTE(zoubir): only what stands still; counting everything the
        // player collides with but units took in the player's own sword
        // and fireballs, so a swing pulled the ring back to the feet
        bool32 Solid = Other->Type == EntityType_StaticObject ||
            Other->Type == EntityType_Tiled;
        if (Other->IsPresent && Solid &&
            CanCollide(AppState, EntityType_Player, Other->Type) &&
            EntityOverlap(&Probe, Other))
        {
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): Target when it is clear, else the clear spot nearest to it
// on the way back to the player; where the player stands when there is none
internal v2
FindBlinkLanding(app_state *AppState, world_entity *Player, v2 Target)
{
    v2 From = Player->Position.XY;
    v2 Way = Target - From;
    float Distance = Length(Way);
    u32 Steps = (u32)(Distance / BLINK_LANDING_STEP) + 1;
    for(u32 Step = Steps; Step > 0; Step--)
    {
        v2 Spot = From + ((float)Step / (float)Steps) * Way;
        if (!IsBlinkSpotBlocked(AppState, &AppState->World, Spot, Player->Position.Z))
        {
            return Spot;
        }
    }
    return From;
}
