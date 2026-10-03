/* Landmarks: small hand-made layouts set into the infinite maps, so the
   endless ground has places to head for. Infinite maps are cut into
   regions of LANDMARK_REGION_TILES square; from the map's seed each region
   either holds one landmark, at a fixed spot inside it, or nothing. The
   region around the origin is kept empty for spawning.

   TerrainAt and PropAt ask here first, so a landmark is part of the ground
   like everything else: pure, the same on server and clients, never sent.
   In a layout, '?' keeps the generated ground underneath, so ruins blend
   into whatever they sit on. 'm' marks where its guards stand: the first
   time a player comes near, they appear (monster_population.cpp), the
   first of them an elite. */

#define LANDMARK_REGION_TILES 80
// NOTE(zoubir): percent of regions that hold a landmark
#define LANDMARK_CHANCE 55
#define MAX_LANDMARK_GUARDS 6

struct landmark_def
{
    char *Name;
    u32 Width;
    u32 Height;
    char **Layout;
    // NOTE(zoubir): bit (1 << map_id) for each map it may appear on
    u32 MapMask;
    // NOTE(zoubir): one per 'm' in the layout, in reading order
    u32 GuardCount;
    monster_kind Guards[MAX_LANDMARK_GUARDS];
};

global_variable char *WatchtowerLayout[] =
{
    "????##.##????",
    "??##.....##??",
    "?#....o....#?",
    "?#.........#?",
    "#....###....#",
    ".....#m#.....",
    "#..m.....m..#",
    "?#.........#?",
    "?#...o.....#?",
    "??##.....##??",
    "????##.##????",
};

global_variable char *SunkenShrineLayout[] =
{
    "????~~~~~~~????",
    "??~~WWWWWWW~~??",
    "?~WWW.....WWW~?",
    "~WW...#.#...WW~",
    "~W..#.....#..W~",
    "~W.....m......~",
    "~W..m.....m..W~",
    "~W..#.....#..W~",
    "~WW...#.#...WW~",
    "?~WWW.....WWW~?",
    "??~~WWW.WWW~~??",
    "????~~~:~~~????",
};

global_variable char *SpiderHollowLayout[] =
{
    "???,,TT,,???",
    "??,T%%%%T,??",
    "?,T%%::%%T,?",
    ",T%:n::n:%T,",
    "T%%::::::%%T",
    "T%::::n:::%T",
    ",T%%::::%%T,",
    "?,TT%::%TT,?",
    "???,,::,,???",
};

global_variable char *ObsidianAltarLayout[] =
{
    "???aaaaaaa???",
    "?aaLLLLLLLaa?",
    "aaLLbbbbbLLaa",
    "aLLbbXbXbbLLa",
    "aLbbb.m.bbbLa",
    "aLbb.....bbLa",
    "aLbbm.L.mbbLa",
    "aLbb.....bbLa",
    "aLLbbbbbbbLLa",
    "aaLLLLbLLLLaa",
    "?aaaaabaaaaa?",
    "???aaabaaa???",
};

global_variable char *AshCampLayout[] =
{
    "??aaaaaaaa??",
    "?aabbbbbbaa?",
    "aab..d...baa",
    "ab..o..m..ba",
    "ab.m.LL...ba",
    "ab....LL..ba",
    "ab..m...o.ba",
    "aab...d..baa",
    "?aabb..bbaa?",
    "??aaa..aaa??",
};

global_variable landmark_def LandmarkTable[] =
{
    {"Ruined Watchtower", 13, ArrayCount(WatchtowerLayout), WatchtowerLayout,
     (1u << MapId_Wilds) | (1u << MapId_Wastes), 3,
     {MonsterKind_Warden, MonsterKind_Brute, MonsterKind_Bat}},
    {"Sunken Shrine", 15, ArrayCount(SunkenShrineLayout), SunkenShrineLayout,
     (1u << MapId_Wilds), 3,
     {MonsterKind_Shaman, MonsterKind_Toad, MonsterKind_Toad}},
    {"Spider Hollow", 12, ArrayCount(SpiderHollowLayout), SpiderHollowLayout,
     (1u << MapId_Wilds), 3,
     {MonsterKind_Spider, MonsterKind_Spider, MonsterKind_Slime}},
    {"Obsidian Altar", 13, ArrayCount(ObsidianAltarLayout), ObsidianAltarLayout,
     (1u << MapId_Wastes), 3,
     {MonsterKind_Warlord, MonsterKind_Imp, MonsterKind_Imp}},
    {"Ash Camp", 12, ArrayCount(AshCampLayout), AshCampLayout,
     (1u << MapId_Wastes), 3,
     {MonsterKind_Brute, MonsterKind_Imp, MonsterKind_Lurker}},
};

// NOTE(zoubir): where the landmark of a region sits, if it has one
struct landmark_spot
{
    bool32 Present;
    u32 Landmark;
    i32 RegionX;
    i32 RegionY;
    // NOTE(zoubir): the layout's top-left tile
    i32 MinX;
    i32 MinY;
};

internal landmark_spot
GetRegionLandmark(map_def *Map, i32 RegionX, i32 RegionY)
{
    landmark_spot Result = {};
    Result.RegionX = RegionX;
    Result.RegionY = RegionY;
    if (Map->Kind != MapKind_Infinite || (RegionX == 0 && RegionY == 0) ||
        (RegionX == -1 && RegionY == 0) || (RegionX == 0 && RegionY == -1) ||
        (RegionX == -1 && RegionY == -1))
    {
        return Result;
    }
    u32 Roll = HashLattice(Map->Seed ^ 0x1A4D3A2Bu, RegionX, RegionY);
    if (Roll % 100 >= LANDMARK_CHANCE)
    {
        return Result;
    }
    // NOTE(zoubir): pick among the landmarks allowed on this map
    u32 Allowed[ArrayCount(LandmarkTable)];
    u32 AllowedCount = 0;
    for(u32 Index = 0; Index < ArrayCount(LandmarkTable); Index++)
    {
        if (LandmarkTable[Index].MapMask & (1u << Map->Id))
        {
            Allowed[AllowedCount++] = Index;
        }
    }
    if (AllowedCount == 0)
    {
        return Result;
    }
    landmark_def *Def = &LandmarkTable[Allowed[(Roll >> 8) % AllowedCount]];
    i32 Room = LANDMARK_REGION_TILES - 8;
    // NOTE(zoubir): a few tries at a spot whose middle and edges stand on
    // open generated ground, so a landmark is never sunk in a lake or a
    // cliff; a region where every try fails gets none
    for(u32 Try = 0; Try < 4; Try++)
    {
        u32 Place = HashLattice(Roll + Try * 0x9E37u, RegionX, RegionY);
        i32 MinX = RegionX * LANDMARK_REGION_TILES + 4 +
            (i32)((Place & 0xFFFF) % (u32)(Room - (i32)Def->Width));
        i32 MinY = RegionY * LANDMARK_REGION_TILES + 4 +
            (i32)((Place >> 16) % (u32)(Room - (i32)Def->Height));
        i32 W = (i32)Def->Width;
        i32 H = (i32)Def->Height;
        i32 CheckX[5] = {MinX + W / 2, MinX, MinX + W - 1, MinX + W / 2, MinX + W / 2};
        i32 CheckY[5] = {MinY + H / 2, MinY + H / 2, MinY + H / 2, MinY, MinY + H - 1};
        bool32 Open = true;
        for(u32 Check = 0; Check < 5 && Open; Check++)
        {
            terrain_kind Ground = Map->Generate(Map, CheckX[Check], CheckY[Check]);
            Open = !GetTerrainDef(Ground)->Blocks && Ground != TerrainKind_ShallowWater &&
                Ground != TerrainKind_Lava;
        }
        if (Open)
        {
            Result.Present = true;
            Result.Landmark = (u32)(Def - LandmarkTable);
            Result.MinX = MinX;
            Result.MinY = MinY;
            break;
        }
    }
    return Result;
}

// NOTE(zoubir): the layout character of a landmark at a tile, or 0 when no
// landmark covers it or the layout keeps the generated ground there ('?')
internal char
LandmarkSymbolAt(map_def *Map, i32 X, i32 Y)
{
    if (Map->Kind != MapKind_Infinite)
    {
        return 0;
    }
    landmark_spot Spot = GetRegionLandmark(Map, FloorDiv(X, LANDMARK_REGION_TILES),
                                           FloorDiv(Y, LANDMARK_REGION_TILES));
    if (!Spot.Present)
    {
        return 0;
    }
    landmark_def *Def = &LandmarkTable[Spot.Landmark];
    i32 LocalX = X - Spot.MinX;
    i32 LocalY = Y - Spot.MinY;
    if (LocalX < 0 || LocalY < 0 || LocalX >= (i32)Def->Width || LocalY >= (i32)Def->Height)
    {
        return 0;
    }
    char Result = Def->Layout[LocalY][LocalX];
    return Result == '?' ? 0 : Result;
}

// NOTE(zoubir): world position of each guard marker, in reading order
internal u32
GetLandmarkGuardSpots(landmark_spot *Spot, i32 TileSize, v3 *Out, u32 MaxCount)
{
    landmark_def *Def = &LandmarkTable[Spot->Landmark];
    u32 Count = 0;
    for(u32 Y = 0; Y < Def->Height; Y++)
    {
        for(u32 X = 0; X < Def->Width && Count < MaxCount; X++)
        {
            char C = Def->Layout[Y][X];
            if (C == 'm' || C == 'n')
            {
                Out[Count++] = V3(((float)(Spot->MinX + (i32)X) + 0.5f) * TileSize,
                                  ((float)(Spot->MinY + (i32)Y) + 0.5f) * TileSize, 0.f);
            }
        }
    }
    return Count;
}

inline v3
GetLandmarkCenter(landmark_spot *Spot, i32 TileSize)
{
    landmark_def *Def = &LandmarkTable[Spot->Landmark];
    v3 Result = V3(((float)Spot->MinX + 0.5f * Def->Width) * TileSize,
                   ((float)Spot->MinY + 0.5f * Def->Height) * TileSize, 0.f);
    return Result;
}
