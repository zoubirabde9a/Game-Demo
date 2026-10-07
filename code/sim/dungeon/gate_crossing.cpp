/* Gate crossing (dungeon.cpp): a closed gate is walls (encounters.cpp),
   and a blink jumps through walls (sim/player_abilities/blink_landing.cpp).
   Without this a player could blink past a gate the fight has shut, or
   into the room while its monsters are not out yet. So in a dungeon run
   a blink toward a target beyond a closed gate aims short of it. */

#define GATE_CROSSING_STEP 8.f
// NOTE(zoubir): how far before the gate's tile a cut-short blink aims
#define GATE_CROSSING_MARGIN 24.f

// NOTE(zoubir): whether Gate's walls stand now
inline bool32
IsGateClosed(dungeon_run *Run, u32 Gate)
{
    bool32 Result = Gate < DUNGEON_MAX_GATES && Run->GateWalls[Gate][0] != 0;
    return Result;
}

// NOTE(zoubir): Target, or the point short of the first closed gate on
// the way from From to it
internal v2
DungeonClampBlinkTarget(app_state *AppState, v2 From, v2 Target)
{
    dungeon_run *Run = AppState->Dungeon;
    if (!Run)
    {
        return Target;
    }
    world *World = &AppState->World;
    float Tile = (float)World->TileWidth;
    v2 Way = Target - From;
    float Distance = Length(Way);
    if (Distance <= 0.f || Tile <= 0.f)
    {
        return Target;
    }
    v2 Direction = Way * (1.f / Distance);
    for(float Along = 0.f; Along <= Distance; Along += GATE_CROSSING_STEP)
    {
        v2 Point = From + Along * Direction;
        u32 Gate = GateAtTile(World->MapId, (i32)floorf(Point.X / Tile),
                              (i32)floorf(Point.Y / Tile));
        if (IsGateClosed(Run, Gate))
        {
            v2 Result = From + Maximum(0.f, Along - GATE_CROSSING_MARGIN) * Direction;
            return Result;
        }
    }
    return Target;
}
