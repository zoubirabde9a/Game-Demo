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
    TerrainKind_Pit,
    TerrainKind_Spring,
    TerrainKind_Bramble,
    TerrainKind_Bog,
    TerrainKind_Rune,
    TerrainKind_Count
};

// NOTE(zoubir): what a tile does to a unit standing on it, refreshed every
// tick (sim/terrain_effects.cpp)
struct terrain_stand_status
{
    status_effect Effect;
    float Seconds;
};

struct terrain_def
{
    char *Name;
    // NOTE(zoubir): units cannot enter it at all
    bool32 Blocks;
    // NOTE(zoubir): deadly or close to it: monsters walk around it and
    // nothing spawns on it (a pit, lava)
    bool32 Hazard;
    // NOTE(zoubir): multiplies movement acceleration of units on it
    float SpeedScale;
    // NOTE(zoubir): multiplies the drag that stops units; below 1 slides
    float Friction;
    // NOTE(zoubir): kept on anyone standing on it; Seconds is how long each
    // lingers after they step off
    terrain_stand_status Stand[2];
};

/* Every kind of ground can be walked on, except the walls that close a
   map in (stone walls). Crags (rock, basalt walls) are rough high ground,
   deep water is swum, slowly. The hazards:
     Lava     burns, and keeps burning a moment after you leave it
     Pit      you fall and die; jump or dash over it. Whoever threw you
              in gets the kill
     Spring   heals over time and washes off poison; soaks you
     Bramble  grabs you once (rooted) and cuts you while you push through
     Bog      poisons
     Rune     a glowing ley line: a burst of speed, which lifts slows
   Water soaks: no burning while soaked. */
global_variable terrain_def TerrainTable[TerrainKind_Count] =
{
    //                 Blocks Hazard Speed  Friction  Stand
    {"Grass",          false, false, 1.f,   1.f,   {}},
    {"Dirt",           false, false, 1.f,   1.f,   {}},
    {"Mud",            false, false, 0.6f,  1.f,   {}},
    {"Shallow water",  false, false, 0.7f,  1.f,   {{StatusEffect_Soaked, 1.5f}}},
    {"Deep water",     false, false, 0.45f, 0.6f,  {{StatusEffect_Soaked, 3.f}}},
    {"Crag",           false, false, 0.85f, 1.f,   {}},
    {"Ash",            false, false, 0.9f,  1.f,   {}},
    {"Basalt",         false, false, 1.f,   1.f,   {}},
    {"Basalt crag",    false, false, 0.85f, 1.f,   {}},
    {"Lava",           false, true,  0.8f,  1.f,   {{StatusEffect_Burning, 2.f}}},
    {"Snow",           false, false, 0.75f, 1.f,   {}},
    // NOTE(zoubir): ice cuts grip both ways: same top speed, slow to start
    // and slow to stop
    {"Ice",            false, false, 0.25f, 0.25f, {}},
    {"Stone floor",    false, false, 1.f,   1.f,   {}},
    {"Stone wall",     true,  false, 1.f,   1.f,   {}},
    {"Pit",            false, true,  1.f,   1.f,   {{StatusEffect_Falling, 0.7f}}},
    {"Spring",         false, false, 0.8f,  1.f,   {{StatusEffect_Regenerating, 3.f}, {StatusEffect_Soaked, 2.f}}},
    {"Bramble",        false, false, 0.7f,  1.f,   {{StatusEffect_Bleeding, 2.f}, {StatusEffect_Rooted, 0.6f}}},
    {"Bog",            false, false, 0.6f,  1.f,   {{StatusEffect_Poisoned, 3.f}}},
    {"Rune",           false, false, 1.f,   1.f,   {{StatusEffect_Hasted, 4.f}}},
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
