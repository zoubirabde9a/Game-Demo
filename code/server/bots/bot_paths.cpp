/* Bot paths (bots/hazard_steer.cpp): on a bounded map, the way round
   lava, pits, walls and blocking props to a goal, over the map's tiles.
   A bot walks straight at what it chases; when that straight way runs
   into lava (the Ember Depths' lake beside the Ashfall causeway), it
   follows BotPathWay instead.

   The tiles a body may cross are worked out once per map (a tile is
   open unless its ground blocks or hurts, or a boulder, dead tree, fence
   or crate stands on it). A search from the goal then gives every open
   tile its steps to the goal; the bot steps down that count, aiming a
   few tiles ahead so it walks straight lines rather than a staircase.
   The search first keeps a tile's width off hazards, as a body is
   about a tile wide; when that leaves no way (a causeway one tile wide),
   it searches again with only the hazards themselves closed. */

#define BOT_PATH_MAX_SIDE 256
#define BOT_PATH_TILES (BOT_PATH_MAX_SIDE * BOT_PATH_MAX_SIDE)
#define BOT_PATH_FAR 0xFFFFu
// NOTE(zoubir): how many steps down the path a bot aims
#define BOT_PATH_AHEAD 3

enum bot_tile
{
    BotTile_Open,
    // NOTE(zoubir): open, but next to a hazard
    BotTile_Edge,
    BotTile_Closed,
};

struct bot_path_grid
{
    u32 MapId;
    bool32 Built;
    i32 Width;
    i32 Height;
    u8 Tiles[BOT_PATH_TILES];
    u16 Steps[BOT_PATH_TILES];
    u32 Queue[BOT_PATH_TILES];
};

global_variable bot_path_grid BotPathGrid;

inline bool32
PropBlocksBots(terrain_prop Prop)
{
    bool32 Result = Prop != TerrainProp_None && Prop != TerrainProp_Tree &&
        Prop != TerrainProp_Log;
    return Result;
}

// NOTE(zoubir): Grid's tiles for World's map; false when the map has no
// edges or is too big to search
internal bool32
BuildBotPathGrid(bot_path_grid *Grid, world *World)
{
    map_def *Map = GetMapDef((map_id)World->MapId);
    if (Map->Kind == MapKind_Infinite || !Map->Width || !Map->Height ||
        Map->Width > BOT_PATH_MAX_SIDE || Map->Height > BOT_PATH_MAX_SIDE)
    {
        return false;
    }
    if (Grid->Built && Grid->MapId == World->MapId)
    {
        return true;
    }
    Grid->MapId = World->MapId;
    Grid->Width = (i32)Map->Width;
    Grid->Height = (i32)Map->Height;
    for(i32 Y = 0; Y < Grid->Height; Y++)
    {
        for(i32 X = 0; X < Grid->Width; X++)
        {
            terrain_def *Def = GetTerrainDef(TerrainAt(Map, X, Y));
            bool32 Closed = Def->Blocks || Def->Hazard || PropBlocksBots(PropAt(Map, X, Y));
            Grid->Tiles[Y * Grid->Width + X] = (u8)(Closed ? BotTile_Closed : BotTile_Open);
        }
    }
    for(i32 Y = 0; Y < Grid->Height; Y++)
    {
        for(i32 X = 0; X < Grid->Width; X++)
        {
            u8 *Tile = &Grid->Tiles[Y * Grid->Width + X];
            for(i32 DY = -1; DY <= 1 && *Tile == BotTile_Open; DY++)
            {
                for(i32 DX = -1; DX <= 1; DX++)
                {
                    i32 NX = X + DX;
                    i32 NY = Y + DY;
                    if (NX >= 0 && NY >= 0 && NX < Grid->Width && NY < Grid->Height &&
                        GetTerrainDef(TerrainAt(Map, NX, NY))->Hazard)
                    {
                        *Tile = BotTile_Edge;
                        break;
                    }
                }
            }
        }
    }
    Grid->Built = true;
    return true;
}

inline i32
BotTileOf(world *World, float Coordinate)
{
    i32 TileSize = World->TileWidth ? (i32)World->TileWidth : ARENA_TILE_SIZE;
    i32 Result = FloorDiv((i32)floorf(Coordinate), TileSize);
    return Result;
}

// NOTE(zoubir): whether a body may step from tile (X, Y) by (DX, DY): a
// diagonal step needs both tiles beside it open, so it never cuts a corner
inline bool32
BotStepOpen(bot_path_grid *Grid, i32 X, i32 Y, i32 DX, i32 DY, u8 Worst)
{
    i32 NX = X + DX;
    i32 NY = Y + DY;
    if (NX < 0 || NY < 0 || NX >= Grid->Width || NY >= Grid->Height ||
        Grid->Tiles[NY * Grid->Width + NX] > Worst)
    {
        return false;
    }
    bool32 Result = !DX || !DY ||
        (Grid->Tiles[Y * Grid->Width + NX] <= Worst && Grid->Tiles[NY * Grid->Width + X] <= Worst);
    return Result;
}

// NOTE(zoubir): every tile's steps to (GoalX, GoalY) over tiles no worse
// than Worst (the goal's and Start's own tiles always count as open);
// true when Start is reached
internal bool32
SearchBotSteps(bot_path_grid *Grid, i32 GoalX, i32 GoalY, i32 StartX, i32 StartY, u8 Worst)
{
    i32 Count = Grid->Width * Grid->Height;
    for(i32 Index = 0; Index < Count; Index++)
    {
        Grid->Steps[Index] = BOT_PATH_FAR;
    }
    u32 Start = (u32)(StartY * Grid->Width + StartX);
    u8 StartTile = Grid->Tiles[Start];
    Grid->Tiles[Start] = BotTile_Open;
    u32 Head = 0;
    u32 Tail = 0;
    Grid->Steps[GoalY * Grid->Width + GoalX] = 0;
    Grid->Queue[Tail++] = (u32)(GoalY * Grid->Width + GoalX);
    while(Head < Tail && Grid->Steps[Start] == BOT_PATH_FAR)
    {
        u32 Index = Grid->Queue[Head++];
        i32 X = (i32)Index % Grid->Width;
        i32 Y = (i32)Index / Grid->Width;
        for(i32 DY = -1; DY <= 1; DY++)
        {
            for(i32 DX = -1; DX <= 1; DX++)
            {
                u32 Next = (u32)((Y + DY) * Grid->Width + X + DX);
                if ((DX || DY) && BotStepOpen(Grid, X, Y, DX, DY, Worst) &&
                    Grid->Steps[Next] == BOT_PATH_FAR)
                {
                    Grid->Steps[Next] = (u16)(Grid->Steps[Index] + 1);
                    Grid->Queue[Tail++] = Next;
                }
            }
        }
    }
    Grid->Tiles[Start] = StartTile;
    bool32 Result = Grid->Steps[Start] != BOT_PATH_FAR;
    return Result;
}

// NOTE(zoubir): a goal on closed ground (the middle of a room with a
// pit or a lava lake in it) moves to the nearest open tile, ring by
// ring out to BOT_PATH_GOAL_RINGS; false when none is that close
#define BOT_PATH_GOAL_RINGS 10

internal bool32
OpenBotGoal(bot_path_grid *Grid, i32 *GoalX, i32 *GoalY)
{
    for(i32 Ring = 0; Ring <= BOT_PATH_GOAL_RINGS; Ring++)
    {
        for(i32 DY = -Ring; DY <= Ring; DY++)
        {
            for(i32 DX = -Ring; DX <= Ring; DX++)
            {
                i32 X = *GoalX + DX;
                i32 Y = *GoalY + DY;
                if ((DX == -Ring || DX == Ring || DY == -Ring || DY == Ring) &&
                    X >= 0 && Y >= 0 && X < Grid->Width && Y < Grid->Height &&
                    Grid->Tiles[Y * Grid->Width + X] != BotTile_Closed)
                {
                    *GoalX = X;
                    *GoalY = Y;
                    return true;
                }
            }
        }
    }
    return false;
}

// NOTE(zoubir): the way from From toward Goal round what hurts or
// blocks, one long; false when the map has no grid or there is no way
internal bool32
BotPathWay(world *World, v2 From, v2 Goal, v2 *Way)
{
    bot_path_grid *Grid = &BotPathGrid;
    if (!BuildBotPathGrid(Grid, World))
    {
        return false;
    }
    i32 StartX = BotTileOf(World, From.X);
    i32 StartY = BotTileOf(World, From.Y);
    i32 GoalX = BotTileOf(World, Goal.X);
    i32 GoalY = BotTileOf(World, Goal.Y);
    if (StartX < 0 || StartY < 0 || StartX >= Grid->Width || StartY >= Grid->Height ||
        GoalX < 0 || GoalY < 0 || GoalX >= Grid->Width || GoalY >= Grid->Height ||
        (StartX == GoalX && StartY == GoalY))
    {
        return false;
    }
    if (!OpenBotGoal(Grid, &GoalX, &GoalY) || (StartX == GoalX && StartY == GoalY))
    {
        return false;
    }
    u8 Worst = BotTile_Open;
    if (!SearchBotSteps(Grid, GoalX, GoalY, StartX, StartY, Worst))
    {
        Worst = BotTile_Edge;
        if (!SearchBotSteps(Grid, GoalX, GoalY, StartX, StartY, Worst))
        {
            return false;
        }
    }
    i32 X = StartX;
    i32 Y = StartY;
    for(u32 Step = 0; Step < BOT_PATH_AHEAD; Step++)
    {
        u16 Here = Grid->Steps[Y * Grid->Width + X];
        i32 BestX = X;
        i32 BestY = Y;
        u16 Best = Here;
        for(i32 DY = -1; DY <= 1; DY++)
        {
            for(i32 DX = -1; DX <= 1; DX++)
            {
                if ((DX || DY) && BotStepOpen(Grid, X, Y, DX, DY, Worst) &&
                    Grid->Steps[(Y + DY) * Grid->Width + X + DX] < Best)
                {
                    Best = Grid->Steps[(Y + DY) * Grid->Width + X + DX];
                    BestX = X + DX;
                    BestY = Y + DY;
                }
            }
        }
        if (Best == Here)
        {
            break;
        }
        X = BestX;
        Y = BestY;
    }
    float TileSize = World->TileWidth ? (float)World->TileWidth : (float)ARENA_TILE_SIZE;
    v2 Aim = V2(((float)X + 0.5f) * TileSize, ((float)Y + 0.5f) * TileSize) - From;
    if (LengthSq(Aim) < 1.f)
    {
        return false;
    }
    *Way = Aim * (1.f / Length(Aim));
    return true;
}
