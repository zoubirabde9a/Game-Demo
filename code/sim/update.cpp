/* Per-frame behaviour for the units that are not players: swords,
   monsters, the familiar and fireballs. Players are player_update.cpp. */

// NOTE(zoubir): a sword is a short-lived hitbox that never moves, so
// MoveEntity never checks it; test its overlaps every frame instead.
// Returns false once the swing is over and the sword is removed.
internal bool32
UpdateSword(world_entity *Sword, world *World, memory_arena *Arena,
            app_state *AppState, float DeltaTime)
{
    Sword->TimeLeft -= DeltaTime;
    if (Sword->TimeLeft <= 0.f)
    {
        RemoveEntity(World, Sword);
        return false;
    }

    entity_collision_volume *Total = &Sword->Collision->TotalVolume;
    rectangle3 Box = RectCenterHalfDims(Sword->Position + Total->Offset,
                                        Total->HalfDims);
    world_entity *Nearby[MOVE_MAX_NEARBY];
    u32 NearbyCount = GatherEntitiesInBox(World, Box, Nearby, MOVE_MAX_NEARBY);
    CheckOverlapsWith(AppState, World, Arena, Sword, Nearby, NearbyCount);
    return true;
}

#define MONSTER_WANDER_SPEED_SCALE 0.35f

// NOTE(zoubir): returns the scaled push for an idle monster; half of the
// time it picks a direction, the other half it stands still for a while
internal v2
MonsterWander(world_entity *Entity, app_state *AppState, float DeltaTime)
{
    Entity->WanderTimer -= DeltaTime;
    if (Entity->WanderTimer <= 0.f && AppState->Monsters)
    {
        random_series *Series = &AppState->Monsters->Series;
        Entity->WanderTimer = RandomBetween(Series, 1.f, 3.f);
        Entity->WanderDirection = V2(0.f);
        if (RandomChoice(Series, 2))
        {
            float Angle = RandomBetween(Series, 0.f, 2.f * Pi32);
            Entity->WanderDirection = V2(Cos(Angle), Sin(Angle));
        }
    }
    v2 Result = MONSTER_WANDER_SPEED_SCALE * Entity->WanderDirection;
    return Result;
}

// NOTE(zoubir): monsters walk toward the player once it comes within
// AggroRange, and stop at arm's length so they do not shove it around
internal void
UpdateMonster(world_entity *Entity, world *World,
              memory_arena *Arena,
              float DeltaTime, app_state *AppState,
              float *AnimationSpeed,
              animation_type *AnimationType,
              animation_direction *AnimationDirection)
{
    monster_stats *Stats = GetMonsterStats(Entity->MonsterKind);
    bool32 Flies = Stats->FlyHeight > 0.f;

    Entity->AttackCooldown = Maximum(0.f, Entity->AttackCooldown -
                                     DeltaTime);

    *AnimationType = AnimationType_Stand;
    *AnimationDirection =
        Entity->AnimationState.LastAnimationDirection;
    *AnimationSpeed = 1.f;

    if (Flies && *AnimationDirection != AnimationDirection_Left)
    {
        *AnimationDirection = AnimationDirection_Right;
    }

    if (UpdateMonsterAbilities(Entity, World, Arena, DeltaTime, AppState,
                               AnimationSpeed, AnimationType,
                               AnimationDirection))
    {
        return;
    }

    v3 DDEntity = {};
    float DistanceToTarget;
    world_entity *Target = FindNearestPlayer(AppState, Entity->Position.XY,
                                             &DistanceToTarget);
    if (Target)
    {
        v2 ToTarget = Target->Position.XY - Entity->Position.XY;
        if (DistanceToTarget < Stats->AttackRange &&
            Entity->AttackCooldown <= 0.f)
        {
            MonsterBite(AppState, World, Entity, Target);
            Entity->AttackCooldown = Stats->AttackInterval;
        }

        if (DistanceToTarget < Stats->AggroRange &&
            DistanceToTarget > Stats->StopRange)
        {
            ToTarget *= 1.f / DistanceToTarget;
            DDEntity.XY = ToTarget;
            *AnimationType = AnimationType_Move;
            // NOTE(zoubir): flyer sprites only face left and right
            if (Flies || Absolute(ToTarget.X) > Absolute(ToTarget.Y))
            {
                *AnimationDirection = ToTarget.X > 0 ?
                    AnimationDirection_Right : AnimationDirection_Left;
            }
            else
            {
                *AnimationDirection = ToTarget.Y > 0 ?
                    AnimationDirection_Up : AnimationDirection_Down;
            }
        }
        else if (DistanceToTarget >= Stats->AggroRange)
        {
            DDEntity.XY = MonsterWander(Entity, AppState, DeltaTime);
        }
    }
    else
    {
        DDEntity.XY = MonsterWander(Entity, AppState, DeltaTime);
    }
    if (LengthSq(DDEntity.XY) > 0.f && *AnimationType != AnimationType_Move)
    {
        *AnimationType = AnimationType_Move;
        *AnimationDirection = DDEntity.X >= 0.f ?
            AnimationDirection_Right : AnimationDirection_Left;
    }

    DDEntity *= Stats->Acceleration * GetMoveSpeedScale(Entity) * DeltaTime;
    // Drag
    DDEntity -= (10.f * GetGroundFriction(Entity) * Entity->Velocity);
    if (Flies)
    {
        // NOTE(zoubir): bob around the hover height, like the familiar
        Entity->tFlying += DeltaTime * 6.f;
        if (Entity->tFlying > 2.f * Pi32)
        {
            Entity->tFlying -= 2.f * Pi32;
        }
        Entity->Position.Z = Stats->FlyHeight + 4.f * Sin(Entity->tFlying);
        DDEntity.Z = 0.f;
        Entity->Velocity.Z = 0.f;
    }
    else
    {
        //Gravity
        DDEntity.Z = -1000.f;
    }

    float MaxDistance = 10000.f;
    MoveEntity(Entity, World, Arena, DeltaTime, AppState,
               DDEntity, &MaxDistance);
}

internal void
UpdateFamiliar(world_entity *Entity, world *World,
             memory_arena *Arena,
             float DeltaTime, app_state *AppState,
             float *AnimationSpeed,
             animation_type *AnimationType,
             animation_direction *AnimationDirection)
{
    v3 DDEntity = {};
    float EntityAcceleration = 56000.f;
    *AnimationType = AnimationType_Stand;
    *AnimationDirection =
        Entity->AnimationState.LastAnimationDirection;
    *AnimationSpeed = 1.f;
    
    Assert(Entity->FollowingEntity);
    v3 FollowingPos = Entity->FollowingEntity->Position;
    v3 EntityPos = Entity->Position;
    
    DDEntity = FollowingPos - EntityPos;
    
    float DDEntityAbsoluteX = Absolute(DDEntity.X);
    float DDEntityAbsoluteY = Absolute(DDEntity.Y);

        if (DDEntity.X > 0)
        {
            *AnimationDirection = AnimationDirection_Right;
        }
        else
        {
            *AnimationDirection = AnimationDirection_Left;
        }
    
    float DDEntityLength = Length(DDEntity);
    if (DDEntityLength > 1.f)
    {
        DDEntity *= 1.f / DDEntityLength;
    }

    DDEntity *= EntityAcceleration * DeltaTime;
    DDEntity -= (10.f * Entity->Velocity);

    Entity->tFlying += DeltaTime * 5;
    float TwoPi = 2.f * Pi32;
    if (Entity->tFlying > TwoPi)
    {
        Entity->tFlying -= TwoPi;
    }
    Entity->Position.Z = 30 + 4.f * Sin(Entity->tFlying);
    
#if 0        
    float DeltaZ = 0.5f * DDEntityZ * Square(DeltaTime) +
        Entity->VelocityZ * DeltaTime;
    Entity->VelocityZ = DDEntityZ * DeltaTime +
        Entity->VelocityZ;
    Entity->Z += DeltaZ * DeltaTime;
    Entity->Z = Maximum(0.f, Entity->Z);
#endif
    
    float MaxDistanceFromFollowingEntity = 45.f;
    if (DDEntityLength > MaxDistanceFromFollowingEntity)
    {        
        float MaxDistance =  DDEntityLength -
            MaxDistanceFromFollowingEntity;
        MoveEntity(Entity, World, Arena, DeltaTime, AppState,
                   DDEntity, &MaxDistance);
    }
}

internal void
UpdateFireBall(world_entity *Entity, world *World,
               memory_arena *Arena,
               float DeltaTime, app_state *AppState)
{
    v3 DDEntity = {};
    float DeltaZ = 0.5f * DDEntity.Z * Square(DeltaTime) +
        Entity->Velocity.Z * DeltaTime;    
    Entity->Velocity.Z = DDEntity.Z * DeltaTime +
        Entity->Velocity.Z;
    
    Entity->Position.Z += DeltaZ * DeltaTime;
    
    DDEntity.X -= (0.75f * Entity->Velocity.X);
    DDEntity.Y -= (0.75f * Entity->Velocity.Y);
    
    if (Entity->Position.Z <= 0.f ||
        Entity->DistanceRemaining <= 0.f ||
        Entity->TimeLeft <= 0.f)
    {
        RemoveEntity(World, Entity);
    }
    else
    {
        v3 Start = Entity->Position;
        v2 StartVelocity = Entity->Velocity.XY;
        MoveEntity(Entity, World, Arena, DeltaTime, AppState,
                   DDEntity, &Entity->DistanceRemaining);
        Entity->TimeLeft -= DeltaTime;

        // NOTE(zoubir): a wall either stops the fireball or turns its
        // velocity along the wall; either way it burst on the wall
        float StartSpeed = Length(StartVelocity);
        float EndSpeed = Length(Entity->Velocity.XY);
        float Expected = StartSpeed * DeltaTime;
        float Moved = Length(Entity->Position.XY - Start.XY);
        // NOTE(zoubir): cosine of the turn; 1 when it flew straight on
        float Turn = (StartSpeed > 0.f && EndSpeed > 0.f) ?
            DotProduct(StartVelocity, Entity->Velocity.XY) / (StartSpeed * EndSpeed) : 0.f;
        if (Entity->IsPresent && Expected > 0.f &&
            (Moved < 0.5f * Expected || Turn < 0.99f))
        {
            RemoveEntity(World, Entity);
        }
    }
}

