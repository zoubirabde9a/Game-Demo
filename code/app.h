#if !defined(APP_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */
/*
  //NOTE(soubir):
  APP_SLOW:
  1- slow code allowed
  0- fast code only
  APP_DEV:
  1- build for developer only
  0- build for public release
*/
// TODO(zoubir):
// make a dash ability and a new DASHINg animation type
// Fix Weird bug in soda.wav not clean sound at the end.
// Next Todo Is Wraping Get Asset Calls
// Z Pos as v3 mb
//TODO(zoubir): more Todos
// remove entity does not work
   // make it so that the entity has a boolean IsValid or smth
//make a better MoveEntityFunction with delta time
// by making it able to move at high speed and not
  // fail
// make a familliar
// make pair wise collision rules for familliar
// make a flowing
// tile lighting
// layers and map levels(Z)
// audio mixer
// CameraShake

#include "app_platform.h"
// NOTE(zoubir): Services that the app provides to the platform layer

struct font
{    
    u32 Texture;
    u32 BitmapWidth;
    u32 BitmapHeight;
    float UpperLimit;
    float LowerLimit;
    void *Glyphs;
    u32 FirstGlyph;
    u32 GlyphsSize;
};


#include "engine/asset.h"
#include "engine/utility.h"
#include "engine/ui.h"
#include "sim/world.h"
#include "sim/player.h"
#include "sim/events.h"
#include "engine/audio.h"

// asset_id
struct pairwise_collision_rule
{
    u32 EntityA;
    u32 EntityB;

    bool32 Tag;
    pairwise_collision_rule *Next;
};

struct app_state
{
    bool32 IsInitialized;
    memory_arena MemoryArena;
    memory_arena ConstantsArena;

    
    font *DefaultFont;    
    texture_cache *TextureCache;
    ui_context *UIContext;

    world World;
    u32 UpdateID;
    
    player_slot Players[MAX_PLAYERS];
    // NOTE(zoubir): the slot this machine's keyboard drives
    u32 LocalPlayerIndex;
    // NOTE(zoubir): filled by SimulateTick, drained by whoever runs it
    sim_events Events;
    // NOTE(zoubir): connection to a dedicated server, see client/online.cpp
    struct online_session *Online;
    // NOTE(zoubir): address and name form, see ui/connect_screen.cpp
    struct connect_screen *ConnectScreen;
    struct monster_population *Monsters;

    v3 TargetCamera;
    v3 CameraOffset;
    
    assets Assets;

    audio_state AudioState;

    ui_state TilePickerWidget;
    bool32 TileEditing;

    bool32 EntityCollisionTable[EntityType_Count][EntityType_Count];

    //NOTE(zoubir): Power Of 2
    pairwise_collision_rule *CollisionRuleHash[256];
    pairwise_collision_rule *FirstFreeCollisionRule;


    entity_collision_volume_group *TileObjectCollision;
    entity_collision_volume_group *TreeCollision;
    entity_collision_volume_group *WallCollision;
    entity_collision_volume_group *PlayerCollision;
    entity_collision_volume_group *FamiliarCollision;
    entity_collision_volume_group *BatCollision;
    entity_collision_volume_group *FireBallCollision;
    entity_collision_volume_group *SwordCollision;

    animation_set ZoubirAnimationSet;
    animation_set FamiliarAnimationSet;
    animation_set FireballAnimationSet;
    animation_set SwordAnimationSet;
    

    platform_work_queue *WorkQueue;

    opengl_texture_queue OpenglTextureQueue;
    open_gl *OpenGL;
};

struct transient_state
{
    bool32 IsInitialized;
    memory_arena MemoryArena;
};

internal void
AddCollisionRule(app_state *AppState, memory_arena *Arena,
                 u32 EntityA, u32 EntityB,
                 bool32 Tag);

internal bool32
CanCollide(app_state *AppState, world_entity *A,
           world_entity *B);

internal void
ClearCollisionRulesFor(app_state *AppState, u32 Entity);
inline void
SetCollision(app_state *AppState, entity_type A,
             entity_type B, bool32 Collides)
{
    AppState->EntityCollisionTable[A][B] = Collides;
    AppState->EntityCollisionTable[B][A] = Collides;
}

inline bool32
CanCollide(app_state *AppState, entity_type A,
             entity_type B)
{
    bool32 Result = AppState->EntityCollisionTable[A][B];
    return Result;
}


global_variable platform_api Platform;

#define APP_H
#endif
