/* Terrain kinds: what a tile of ground is, and the rules it imposes on
   whoever stands on it. One row per kind; the art for each kind lives in
   art/terrain_art.cpp. Rules are applied by later steps of
   docs/terrain-plan.md (blocking in collision, the rest on units). */

enum terrain_kind
{
    TerrainKind_Grass,
    TerrainKind_Dirt,
    TerrainKind_Mud,
    TerrainKind_ShallowWater,
    TerrainKind_DeepWater,
    TerrainKind_Rock,
    TerrainKind_Ash,
    TerrainKind_Basalt,
    TerrainKind_BasaltWall,
    TerrainKind_Lava,
    TerrainKind_Snow,
    TerrainKind_Ice,
    TerrainKind_StoneFloor,
    TerrainKind_StoneWall,
    TerrainKind_Count
};

struct terrain_def
{
    char *Name;
    // NOTE(zoubir): units cannot enter it at all
    bool32 Blocks;
    // NOTE(zoubir): multiplies movement acceleration of units on it
    float SpeedScale;
    // NOTE(zoubir): multiplies the drag that stops units; below 1 slides
    float Friction;
    // NOTE(zoubir): kept on anyone standing on it, refreshed every tick
    status_effect StandStatus;
    float StandStatusSeconds;
};

global_variable terrain_def TerrainTable[TerrainKind_Count] =
{
    //                 Blocks Speed  Friction Status                  Secs
    {"Grass",          false, 1.f,   1.f,  StatusEffect_None,      0.f},
    {"Dirt",           false, 1.f,   1.f,  StatusEffect_None,      0.f},
    {"Mud",            false, 0.6f,  1.f,  StatusEffect_None,      0.f},
    {"Shallow water",  false, 0.7f,  1.f,  StatusEffect_None,      0.f},
    {"Deep water",     true,  1.f,   1.f,  StatusEffect_None,      0.f},
    {"Rock",           true,  1.f,   1.f,  StatusEffect_None,      0.f},
    {"Ash",            false, 0.9f,  1.f,  StatusEffect_None,      0.f},
    {"Basalt",         false, 1.f,   1.f,  StatusEffect_None,      0.f},
    {"Basalt wall",    true,  1.f,   1.f,  StatusEffect_None,      0.f},
    {"Lava",           false, 0.8f,  1.f,  StatusEffect_Burning,   1.f},
    {"Snow",           false, 0.75f, 1.f,  StatusEffect_None,      0.f},
    // NOTE(zoubir): ice cuts grip both ways: same top speed, slow to start
    // and slow to stop
    {"Ice",            false, 0.25f, 0.25f, StatusEffect_None,     0.f},
    {"Stone floor",    false, 1.f,   1.f,  StatusEffect_None,      0.f},
    {"Stone wall",     true,  1.f,   1.f,  StatusEffect_None,      0.f},
};

inline terrain_def *
GetTerrainDef(terrain_kind Kind)
{
    Assert(Kind < TerrainKind_Count);
    terrain_def *Result = &TerrainTable[Kind];
    return Result;
}

// NOTE(zoubir): things standing on a tile that are not ground: trees,
// boulders, ruins. Placed by the map, spawned as obstacle entities
enum terrain_prop
{
    TerrainProp_None,
    TerrainProp_Tree,
    TerrainProp_Boulder,
    TerrainProp_DeadTree,
    TerrainProp_Count
};
// NOTE(zoubir): drawn size of boulders and dead trees, in world units;
// trees use the packed tree sprite
#define TERRAIN_PROP_PIXELS 48
