/* Maps: what ground lies at every tile, and what stands on it. A map is a
   map_def, defined in its own file under sim/maps/ and listed once in
   sim/maps/map_list.inc (see sim/maps/README.md).

   Infinite maps generate terrain from their seed for any tile, positive
   or negative, through their Generate function. Bounded maps are a text
   layout of Width x Height characters (legend below); outside the layout
   is Outside (a wall), so the map is closed.

   TerrainAt and PropAt are the only way the rest of the game asks about
   the ground; both are pure functions of (map, tile). */

enum map_kind
{
    MapKind_Infinite,
    MapKind_Bounded,
};

struct map_def;
typedef terrain_kind map_terrain_function(map_def *Map, i32 X, i32 Y);
typedef terrain_prop map_prop_function(map_def *Map, i32 X, i32 Y,
                                       terrain_kind Ground);

#define MAX_MAP_SPAWNS 8

struct map_def
{
    // NOTE(zoubir): its own map_id, filled in by GetMapDef
    u32 Id;
    char *Name;
    map_kind Kind;
    u32 Seed;

    // NOTE(zoubir): infinite maps
    map_terrain_function *Generate;
    map_prop_function *PlaceProp;

    // NOTE(zoubir): bounded maps: Height rows of Width characters
    u32 Width;
    u32 Height;
    char **Layout;
    terrain_kind Outside;

    // NOTE(zoubir): player spawn tiles; infinite maps keep the area around
    // the origin open and spawn there
    u32 SpawnCount;
    i32 SpawnX[MAX_MAP_SPAWNS];
    i32 SpawnY[MAX_MAP_SPAWNS];

    // NOTE(zoubir): multiplies each monster kind's SpawnWeight on this map
    float MonsterWeight[MonsterKind_Count];
};

/* Layout legend for bounded maps. Each character is ground, plus at most
   one prop or marker standing on it. */
struct layout_symbol
{
    char Symbol;
    terrain_kind Ground;
    terrain_prop Prop;
    bool32 Spawn;
};

global_variable layout_symbol LayoutLegend[] =
{
    {'.', TerrainKind_StoneFloor, TerrainProp_None, false},
    {'#', TerrainKind_StoneWall, TerrainProp_None, false},
    {',', TerrainKind_Grass, TerrainProp_None, false},
    {':', TerrainKind_Dirt, TerrainProp_None, false},
    {'%', TerrainKind_Mud, TerrainProp_None, false},
    {'~', TerrainKind_ShallowWater, TerrainProp_None, false},
    {'W', TerrainKind_DeepWater, TerrainProp_None, false},
    {'R', TerrainKind_Rock, TerrainProp_None, false},
    {'*', TerrainKind_Snow, TerrainProp_None, false},
    {'_', TerrainKind_Ice, TerrainProp_None, false},
    {'L', TerrainKind_Lava, TerrainProp_None, false},
    {'a', TerrainKind_Ash, TerrainProp_None, false},
    {'T', TerrainKind_Grass, TerrainProp_Tree, false},
    {'t', TerrainKind_Snow, TerrainProp_DeadTree, false},
    {'o', TerrainKind_Grass, TerrainProp_Boulder, false},
    {'O', TerrainKind_Snow, TerrainProp_Boulder, false},
    {'s', TerrainKind_StoneFloor, TerrainProp_None, true},
    {'g', TerrainKind_Grass, TerrainProp_None, true},
    {'b', TerrainKind_Basalt, TerrainProp_None, false},
    {'X', TerrainKind_BasaltWall, TerrainProp_None, false},
    {'d', TerrainKind_Ash, TerrainProp_DeadTree, false},
    // NOTE(zoubir): landmark guard markers (sim/terrain/landmarks.cpp)
    {'m', TerrainKind_StoneFloor, TerrainProp_None, false},
    {'n', TerrainKind_Dirt, TerrainProp_None, false},
};

inline layout_symbol *
FindLayoutSymbol(char Symbol)
{
    layout_symbol *Result = 0;
    for(u32 Index = 0; Index < ArrayCount(LayoutLegend); Index++)
    {
        if (LayoutLegend[Index].Symbol == Symbol)
        {
            Result = &LayoutLegend[Index];
            break;
        }
    }
    return Result;
}

inline layout_symbol *
GetLayoutSymbol(map_def *Map, i32 X, i32 Y)
{
    layout_symbol *Result = 0;
    if (X >= 0 && Y >= 0 && (u32)X < Map->Width && (u32)Y < Map->Height)
    {
        Result = FindLayoutSymbol(Map->Layout[Y][X]);
    }
    return Result;
}

// NOTE(zoubir): in landmarks.cpp, included after this file
internal char LandmarkSymbolAt(map_def *Map, i32 X, i32 Y);

internal terrain_kind
TerrainAt(map_def *Map, i32 X, i32 Y)
{
    terrain_kind Result = Map->Outside;
    if (Map->Kind == MapKind_Infinite)
    {
        char Landmark = LandmarkSymbolAt(Map, X, Y);
        layout_symbol *Symbol = Landmark ? FindLayoutSymbol(Landmark) : 0;
        Result = Symbol ? Symbol->Ground : Map->Generate(Map, X, Y);
    }
    else
    {
        layout_symbol *Symbol = GetLayoutSymbol(Map, X, Y);
        if (Symbol)
        {
            Result = Symbol->Ground;
        }
    }
    return Result;
}

internal terrain_prop
PropAt(map_def *Map, i32 X, i32 Y)
{
    terrain_prop Result = TerrainProp_None;
    if (Map->Kind == MapKind_Infinite)
    {
        char Landmark = LandmarkSymbolAt(Map, X, Y);
        layout_symbol *Symbol = Landmark ? FindLayoutSymbol(Landmark) : 0;
        if (Symbol)
        {
            return Symbol->Prop;
        }
        terrain_kind Ground = Map->Generate(Map, X, Y);
        if (Map->PlaceProp && !GetTerrainDef(Ground)->Blocks)
        {
            Result = Map->PlaceProp(Map, X, Y, Ground);
        }
    }
    else
    {
        layout_symbol *Symbol = GetLayoutSymbol(Map, X, Y);
        if (Symbol)
        {
            Result = Symbol->Prop;
        }
    }
    return Result;
}

// NOTE(zoubir): a stable 0..99 roll for a tile, for prop placement
inline u32
TileRoll(map_def *Map, i32 X, i32 Y, u32 Salt)
{
    u32 Result = HashLattice(Map->Seed ^ (Salt * 0x51ED27u), X, Y) % 100;
    return Result;
}

// NOTE(zoubir): infinite maps keep a clearing of this many tiles around
// the origin, where players spawn
#define MAP_SPAWN_CLEARING 6

inline bool32
InSpawnClearing(i32 X, i32 Y)
{
    bool32 Result = X * X + Y * Y <= MAP_SPAWN_CLEARING * MAP_SPAWN_CLEARING;
    return Result;
}

inline void
DefaultMapDef(map_def *Map)
{
    ZeroSize(Map, sizeof(*Map));
    Map->Outside = TerrainKind_StoneWall;
    for(u32 Kind = 0; Kind < MonsterKind_Count; Kind++)
    {
        Map->MonsterWeight[Kind] = 1.f;
    }
}

// NOTE(zoubir): bounded maps read their spawn markers from the layout
internal void
CollectLayoutSpawns(map_def *Map)
{
    for(u32 Y = 0; Y < Map->Height; Y++)
    {
        for(u32 X = 0; X < Map->Width; X++)
        {
            layout_symbol *Symbol = FindLayoutSymbol(Map->Layout[Y][X]);
            if (Symbol && Symbol->Spawn && Map->SpawnCount < MAX_MAP_SPAWNS)
            {
                Map->SpawnX[Map->SpawnCount] = (i32)X;
                Map->SpawnY[Map->SpawnCount] = (i32)Y;
                Map->SpawnCount++;
            }
        }
    }
}

enum map_id
{
#define MAP(Name) MapId_##Name,
#define MAP_NAME_PASS
#include "../maps/map_list.inc"
#undef MAP_NAME_PASS
#undef MAP
    MapId_Count
};

#include "../maps/map_list.inc"

typedef void map_define_function(map_def *Map);

global_variable map_define_function *MapDefineFunctions[MapId_Count] =
{
#define MAP(Name) DefineMap_##Name,
#define MAP_NAME_PASS
#include "../maps/map_list.inc"
#undef MAP_NAME_PASS
#undef MAP
};

// NOTE(zoubir): rebuilt on first use after every hot reload, like the
// monster table; the function pointers inside belong to this load
global_variable map_def MapTable[MapId_Count];
global_variable bool32 MapTableReady;

inline map_def *
GetMapDef(map_id Id)
{
    Assert(Id < MapId_Count);
    if (!MapTableReady)
    {
        for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
        {
            map_def *Map = &MapTable[MapIndex];
            DefaultMapDef(Map);
            Map->Id = MapIndex;
            MapDefineFunctions[MapIndex](Map);
            if (Map->Kind == MapKind_Bounded)
            {
                CollectLayoutSpawns(Map);
            }
        }
        MapTableReady = true;
    }
    map_def *Result = &MapTable[Id];
    return Result;
}

// NOTE(zoubir): the map whose name starts with Name, ignoring case and
// spaces ("keep", "frostbite", "ashen wastes"); Fallback when none does
internal map_id
FindMapByName(char *Name, map_id Fallback)
{
    if (!Name || !*Name)
    {
        return Fallback;
    }
    for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
    {
        map_def *Map = GetMapDef((map_id)MapIndex);
        char *A = Name;
        char *B = Map->Name;
        // NOTE(zoubir): match against the full name, or the word after the
        // first space ("Frostbite Keep" answers to "keep")
        for(u32 Try = 0; Try < 2 && B; Try++)
        {
            char *P = A;
            char *Q = B;
            while (*P && *Q)
            {
                char CP = (*P >= 'A' && *P <= 'Z') ? (char)(*P + 32) : *P;
                char CQ = (*Q >= 'A' && *Q <= 'Z') ? (char)(*Q + 32) : *Q;
                if (CP != CQ)
                {
                    break;
                }
                P++;
                Q++;
            }
            if (!*P)
            {
                return (map_id)MapIndex;
            }
            while (*B && *B != ' ')
            {
                B++;
            }
            B = *B ? B + 1 : 0;
        }
    }
    return Fallback;
}

internal u32 HashTerrainRegion(map_def *Map, i32 MinX, i32 MinY, i32 Size);

// NOTE(zoubir): a fingerprint of every map and terrain rule, for the
// client/server build check: the terrain table, each map's kind, size and
// layout text, and a patch of ground from each infinite map, so a change
// to a generator or a layout turns away clients built before it
internal u32
ComputeTerrainContentHash()
{
    u32 Hash = 2166136261u;
    for(u32 Kind = 0; Kind < TerrainKind_Count; Kind++)
    {
        terrain_def *Def = GetTerrainDef((terrain_kind)Kind);
        u32 Parts[] = {(u32)Def->Blocks, (u32)(Def->SpeedScale * 1000.f),
                       (u32)(Def->Friction * 1000.f), (u32)Def->StandStatus,
                       (u32)(Def->StandStatusSeconds * 1000.f)};
        for(u32 Part = 0; Part < ArrayCount(Parts); Part++)
        {
            Hash = (Hash ^ Parts[Part]) * 16777619u;
        }
    }
    for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
    {
        map_def *Map = GetMapDef((map_id)MapIndex);
        Hash = (Hash ^ (u32)Map->Kind) * 16777619u;
        Hash = (Hash ^ Map->Seed) * 16777619u;
        if (Map->Kind == MapKind_Bounded)
        {
            for(u32 Row = 0; Row < Map->Height; Row++)
            {
                for(char *C = Map->Layout[Row]; *C; C++)
                {
                    Hash = (Hash ^ (u8)*C) * 16777619u;
                }
            }
        }
        else
        {
            Hash = (Hash ^ HashTerrainRegion(Map, -16, -16, 32)) * 16777619u;
            // NOTE(zoubir): a region with landmarks in it
            Hash = (Hash ^ HashTerrainRegion(Map, 120, -40, 64)) * 16777619u;
        }
    }
    return Hash;
}

// NOTE(zoubir): a fingerprint of the ground over a square of tiles; the
// tests pin it so a compiler or code change that moves terrain is caught
internal u32
HashTerrainRegion(map_def *Map, i32 MinX, i32 MinY, i32 Size)
{
    u32 Hash = 2166136261u;
    for(i32 Y = MinY; Y < MinY + Size; Y++)
    {
        for(i32 X = MinX; X < MinX + Size; X++)
        {
            u32 Value = (u32)TerrainAt(Map, X, Y) | ((u32)PropAt(Map, X, Y) << 8);
            Hash = (Hash ^ Value) * 16777619u;
        }
    }
    return Hash;
}
