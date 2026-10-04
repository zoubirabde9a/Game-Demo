/* What happens when two entities meet: HandleCollision (a blocking or
   passing hit: damage, fireballs bursting) and HandleOverlap (a sword
   touching something), and the geometry tests behind them (TestWall for
   one face of a swept box, EntityOverlap). Which pairs may meet at all is
   collision_rules.cpp. */

// NOTE(zoubir): Entity is a player the client is stepping to predict it
// (client/prediction.cpp). The client's world holds only replicas: a
// fireball there has no owner and may already have missed on the server,
// so whatever a meeting does to health, status or other units is left to
// the server and arrives with its snapshot. Only the movement is kept.
inline bool32
IsPredictedPlayer(app_state *AppState, world_entity *Entity)
{
    bool32 Result = Entity->Type == EntityType_Player &&
        AppState->Players[Entity->PlayerIndex].Predicted;
    return Result;
}

// NOTE(zoubir): a fireball burning Target (player_abilities/fireball.cpp,
// included later)
internal void FireBallHit(app_state *AppState, world *World,
                          world_entity *FireBall, world_entity *Target);

internal bool32
HandleCollision(app_state *AppState, world *World,
                world_entity *A,
                world_entity *B)
{
    bool32 Result = true;

    if (B->Type == EntityType_FireBall)
    {
        world_entity *Tmp = A;
        A = B;
        B = Tmp;
    }

    // NOTE(zoubir): fireballs pierce; the caller adds a pass-through rule
    // so each fireball hits each target once. The owner already has one.
    // A predicted player passes through without the hit: it once took 25
    // health on its own screen from a fireball the server said missed.
    if (A->Type == EntityType_FireBall &&
        (B->Type == EntityType_Monster || B->Type == EntityType_Player))
    {
        if (!IsPredictedPlayer(AppState, B))
        {
            FireBallHit(AppState, World, A, B);
        }
        Result = false;
    }
    return Result;
}

struct test_wall
{
    float X;
    v3 Rel;
    v3 Delta;
    float MinY;
    float MaxY;
    float MinZ;
    float MaxZ;
    v3 Normal;
};

internal bool32
TestWall(float WallX, float RelX, float RelY, float RelZ,
         float PlayerDeltaX, float PlayerDeltaY, float PlayerDeltaZ,
         float *tMin,
         float MinY, float MaxY,
         float MinZ, float MaxZ)
{
    bool32 Hit = false;
    float tEpsilon = 0.01f;
    if(PlayerDeltaX != 0.0f)
    {
        float tResult = (WallX - RelX) / PlayerDeltaX;
        float Y = RelY + tResult*PlayerDeltaY;
        float Z = RelZ + tResult*PlayerDeltaZ;
        if((tResult >= 0.0f) && (*tMin > tResult))
        {
            if((Y >= MinY) && (Y <= MaxY) &&
               (Z >= MinZ) && (Z <= MaxZ))
            {
                *tMin = Maximum(0.0f, tResult - tEpsilon);
                Hit = true;
            }
        }
    }
    
    return Hit;
}

internal bool32
CanOverlap(world_entity *Entity, world_entity *Region)
{
    bool32 Result = false;
    if (Entity->Type == EntityType_Player &&
        Region->Type == EntityType_Familiar)
    {
        Result = true;
    }
    
    if (Entity->Type == EntityType_Sword &&
        (Region->Type == EntityType_Monster ||
         Region->Type == EntityType_Player))
    {
        Result = true;
    }
    
    return Result;
}

internal bool32
EntityOverlap(world_entity *Entity, world_entity *Region)
{
    bool32 Result = false;
    
    for(u32 VolumeIndex = 0;
        VolumeIndex < Entity->Collision->VolumesCount;
        VolumeIndex++)
    {
        entity_collision_volume *Volume =
            &Entity->Collision->Volumes[VolumeIndex];
        
        rectangle3 EntityRect =
            RectCenterHalfDims(Entity->Position +
                               Volume->Offset, Volume->HalfDims);
                                                                     
        for(u32 RegionVolumeIndex = 0;
            RegionVolumeIndex < Region->Collision->VolumesCount;
            RegionVolumeIndex++)
        {
            entity_collision_volume *RegionVolume =
                &Region->Collision->Volumes[RegionVolumeIndex];
            rectangle3 RegionRect =
                RectCenterHalfDims(Region->Position +
                                   RegionVolume->Offset, RegionVolume->HalfDims);
            if (RectanglesIntersect(EntityRect, RegionRect))
            {
                Result = true;
            }
        }
    }
    return Result;
}

// NOTE(zoubir): a sword swing's hit on Target (sword.cpp, included later)
internal void SwordHit(app_state *AppState, world *World, world_entity *Sword,
                       world_entity *Target);

internal void
HandleOverlap(app_state *AppState, world *World, memory_arena *Arena,
              world_entity *Entity, world_entity *Region)
{
    if (IsPredictedPlayer(AppState, Entity) ||
        IsPredictedPlayer(AppState, Region))
    {
        return;
    }
    if (Entity->Type == EntityType_Sword)
    {
        SwordHit(AppState, World, Entity, Region);
        // NOTE(zoubir): one hit per swing per target
        AddCollisionRule(AppState, Arena,
                         Entity->ID, Region->ID,
                         false);
    }
}
