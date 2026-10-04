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
    // NOTE(zoubir): jumpables: low enough to vault or jump over, solid
    // enough to stand on (collision in sim/arena.cpp, art in
    // art/terrain_art.cpp)
    TerrainProp_Log,
    TerrainProp_Fence,
    TerrainProp_Crate,
    TerrainProp_Count
};
// NOTE(zoubir): drawn size of boulders and dead trees, in world units;
// trees use the packed tree sprite
#define TERRAIN_PROP_PIXELS 48

// NOTE(zoubir): raised ground. A tile's elevation is a whole number of
// steps (ElevationAt, sim/terrain/maps.cpp); each step lifts the ground
// ELEVATION_STEP_HEIGHT units. Each raised tile is a solid box from the
// floor up to its top (sim/arena.cpp), so what blocks a unit is only how
// much higher a tile is than its feet. Climbing, by height difference:
//   up to ELEVATION_WALK_STEPS         walk up on your own (stairs)
//   up to ELEVATION_JUMP_STEPS         jump, or push into it to vault
//   up to ELEVATION_DOUBLE_JUMP_STEPS  double jump
//   ELEVATION_MAX_STEPS                a wall: nobody climbs it from the floor
// The jump numbers follow the jump's physics (peak 36, double about 68,
// player_abilities/jump.cpp); the tests check they still hold
#define ELEVATION_STEP_HEIGHT 8.f
#define ELEVATION_MAX_STEPS 9
#define ELEVATION_WALK_STEPS 1
#define ELEVATION_JUMP_STEPS 4
#define ELEVATION_DOUBLE_JUMP_STEPS 8

// NOTE(zoubir): the tallest edge a walking unit steps onto without a jump
// (MoveEntity, sim/move.cpp, which is compiled before this file): the walk
// steps and a hair, so a log (10 high) still needs a jump
inline float
GetStepUpHeight()
{
    float Result = (float)ELEVATION_WALK_STEPS * ELEVATION_STEP_HEIGHT + 1.f;
    return Result;
}

// NOTE(zoubir): each prop's solid box, half sizes in world units, sitting
// on the ground of its tile. Trees are left out: their trunk-only volume
// is MakeGroundedTreeCollisionVolume (entity.cpp)
struct terrain_prop_def
{
    char *Name;
    v3 HalfDims;
};

global_variable terrain_prop_def PropTable[TerrainProp_Count] =
{
    {"None",      {0.f, 0.f, 0.f}},
    {"Tree",      {0.f, 0.f, 0.f}},
    {"Boulder",   {13.f, 8.f, 14.f}},
    {"Dead tree", {7.f, 5.f, 30.f}},
    // NOTE(zoubir): 28 long and 10 high: vaulted, never walked up
    {"Log",       {14.f, 5.f, 5.f}},
    // NOTE(zoubir): a whole tile wide and thin, so a row of them is a
    // fence; 16 high, two steps
    {"Fence",     {16.f, 2.f, 8.f}},
    // NOTE(zoubir): 22 high, wide enough to stand on
    {"Crate",     {11.f, 11.f, 11.f}},
};
