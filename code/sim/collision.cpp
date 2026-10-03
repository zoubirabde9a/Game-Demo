/* What happens when two entities meet: HandleCollision (a blocking or
   passing hit: damage, fireballs bursting) and HandleOverlap (a sword
   touching something), and the geometry tests behind them (TestWall for
   one face of a swept box, EntityOverlap). Which pairs may meet at all is
   collision_rules.cpp. */

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
    if (A->Type == EntityType_FireBall &&
        (B->Type == EntityType_Monster || B->Type == EntityType_Player))
    {
        DamageEntity(AppState, World, B, FIREBALL_DAMAGE, A);
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

internal void
HandleOverlap(app_state *AppState, world *World, memory_arena *Arena,
              world_entity *Entity, world_entity *Region)
{
    if (Entity->Type == EntityType_Sword)
    {
        // NOTE(zoubir): a survivor is shoved away from the swinger (or the
        // blade when the swinger is unknown), so a hit is felt
        v2 From = Entity->Position.XY;
        world_entity *Swinger = Entity->HasOwner ?
            AppState->Players[Entity->OwnerSlot].Entity : 0;
        if (Swinger && Swinger->IsPresent)
        {
            From = Swinger->Position.XY;
        }
        v2 Away = Region->Position.XY - From;
        float Distance = Length(Away);
        if (!DamageEntity(AppState, World, Region, SWORD_DAMAGE, Entity) &&
            Region->IsPresent && Distance > 0.f)
        {
            Region->Velocity.XY += (SWORD_KNOCKBACK / Distance) * Away;
        }
        // NOTE(zoubir): one hit per swing per target
        AddCollisionRule(AppState, Arena,
                         Entity->ID, Region->ID,
                         false);
    }
}
