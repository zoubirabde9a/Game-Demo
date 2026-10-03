/* Per-frame behaviour for units that think: the player (input, movement,
   attacks, spells), the familiar (follows the player) and fireballs. */

internal void
UpdatePlayer(world_entity *Player, world *World,
             memory_arena *Arena,
             app_input *Input, app_state *AppState,
             v3 CameraOffset,
             float *AnimationSpeedRate,
             animation_type *AnimationType,
             animation_direction *AnimationDirection)
{
    float DeltaTime = Input->DeltaTime;
    v3 DDPlayer = {};
    float PlayerAcceleration = 56000.f;
    *AnimationType = AnimationType_Stand;
    *AnimationDirection =
        Player->AnimationState.LastAnimationDirection;
    *AnimationSpeedRate = 1.f;
    v2 MouseWorldPosition =
        V2((float)Input->MouseX, (float)Input->MouseY) +
        CameraOffset.XY;

    bool32 Move = 0;
    bool32 Up = Input->ButtonZ.EndedDown;
    bool32 Down = Input->ButtonS.EndedDown;
    bool32 Right = Input->ButtonD.EndedDown;
    bool32 Left = Input->ButtonQ.EndedDown;

    v2 Dir = {};
    float CastingTimeEventLinger = 0.15f;    
    float MoveEventLinger = 0.15f;    

    if (Up)
    {
        Dir.Y = -1.f;
        Move = true;        
    }
    if (Down)
    {
        Dir.Y = 1.f;
        Move = true;
    }
    if (Right)
    {
        Dir.X = 1.f;
        Move = true;
    }
    if (Left)
    {
        Dir.X = -1.f;
        Move = true;
    }

    if (!Up &&
        !Down &&
        (Right || Left))
    {
        Dir.Y = 0.f;
    }
    
    if (!Right &&
        !Left &&
        (Up || Down))
    {        
        Dir.X = 0.f;
    }
    
    if (Move)
    {
        for(u32 DelayedMoveIndex = 0;
            DelayedMoveIndex < 1;
            DelayedMoveIndex ++)
        {
            AddPlayerDelayedMove(AppState, MoveEventLinger, Dir);
        }
    }

    Move = 0;            
    bool32 Jumping = Player->Velocity.Z != 0.f;
    if (Jumping)
    {
        Player->State = EntityState_Jumping;
    }
    
        
    if (Input->RightButton.Pressed)
    {
        AddPlayerDelayedAttack(AppState, CastingTimeEventLinger, Player->Direction);
    }

    if (Input->LeftButton.Pressed)
    {       
        AddPlayerDelayedCast(AppState, CastingTimeEventLinger, Player->Direction);
    }
    

    if (Player->State == EntityState_Attacking)
    {
        
        if (IsAnimationFinished(Player->AnimationSet,
                    &Player->AnimationState,
                    AnimationType_Attack,
                                *AnimationDirection))
        {
            
            Player->State = EntityState_Standing;
        }
    }
            
    if (Player->State == EntityState_Casting &&// IsSet(Player, EntityFlag_Casting) &&
        IsAnimationFinished(Player->AnimationSet,
                            &Player->AnimationState,
                            AnimationType_Cast,
                            *AnimationDirection))
    {
        Player->State = EntityState_Standing;
//        ClearFlag(Player, EntityFlag_Casting);
    }
    
    bool32 Halted = 0;
    for(u32 DelayedInputIndex = 0;
        DelayedInputIndex < AppState->PlayerDelayedInputCount;)
    {
        player_delayed_input *DelayedInput =
            &AppState->PlayerDelayedInput[DelayedInputIndex];
        bool32 RemoveEvent = 0;
        bool32 Consumed = 0;
        if (Halted)
        {                
            DelayedInput->TimeRemaining -= Input->DeltaTime;
            if (DelayedInput->TimeRemaining <= 0.f)
            {
                RemoveEvent = true;
            }
            else
            {
                DelayedInputIndex++;
            }
        }
        else
        {
            if (DelayedInput->TimeRemaining > 0.f)
            {
                if (DelayedInput->Type == PDI_Attack)
                {
                    if (Player->State != EntityState_Attacking)//!IsSet(Player, EntityFlag_Casting))
                    {
                        Player->State = EntityState_Attacking;
//                    AddFlags(Player, EntityFlag_Casting);

                        v2 DirVec = Player->Direction;
//                        v2 DirVec = DelayedInput->Dir;
                        Player->CastingDirection = DirVec;

                        Player->AnimationState.SlotIndex = 0;
                        PlayerAcceleration *= 0.6f;
                        DDPlayer.XY = DirVec;
                        v3 SwordPosition = Player->Position + V3(16.f * DirVec.X, 16 * DirVec.Y, 0.f);
                        animation_direction SwordAnimationDirection =
                            AnimationDirection_Right;
                        
                        if (DirVec.X == 1.f)
                        {
                            SwordAnimationDirection =
                                AnimationDirection_Right;                            
                        }
                        if (DirVec.X == -1.f)
                        {
                            SwordAnimationDirection =
                                AnimationDirection_Left;    
                        }
                        if (DirVec.Y == 1.f)
                        {
                            SwordAnimationDirection =
                                AnimationDirection_Down;                            
                        }
                        if (DirVec.Y == -1.f)
                        {
                            SwordAnimationDirection =
                                AnimationDirection_Up;    
                        }
                        
                        AddSword(AppState, World, Arena, SwordPosition,
                                 Player, SwordAnimationDirection);
                        PlaySound(AppState, {AssetType_Dash});
                        Consumed = true;
                    }
                }
                else if (DelayedInput->Type == PDI_Move)
                {
                    
                    Player->Direction = DelayedInput->Dir;
                    Move = true;
                    Consumed = true;
                }
                else if (DelayedInput->Type == PDI_Cast)
                {

                    float FireBallVelocityScaler = 450.f;
                    float HandOffsetZ = 30;
                    v2 FireBallPosition =
                        Player->Position.XY +
                        DelayedInput->Dir * V2(32.f, 32.f);
                    v2 FireBallVelocity =
                        FireBallVelocityScaler * DelayedInput->Dir;
                    
                    *AnimationType = AnimationType_Cast;
                    world_entity *FireBall = AddFireBall(AppState, World, Arena,
                                                       Player,
                                                       V3(FireBallPosition.X, FireBallPosition.Y, HandOffsetZ),
                                                       V3(FireBallVelocity.X, FireBallVelocity.Y, 0.f));
                    FireBall->AnimationSpeed = 1.f;
                    FireBall->AnimationType = AnimationType_Move;
                    FireBall->AnimationDirection = AnimationDirection_Right;
                    
                    if (DelayedInput->Dir.X == 1.f)
                    {                        
                        FireBall->AnimationDirection = AnimationDirection_Right;
                    }
                    if (DelayedInput->Dir.X == -1.f)
                    {                        
                        FireBall->AnimationDirection = AnimationDirection_Left;                        
                    }
                    if (DelayedInput->Dir.Y == 1.f)
                    {                        
                        FireBall->AnimationDirection = AnimationDirection_Down;                        
                    }
                    if (DelayedInput->Dir.Y == -1.f)
                    {
                        FireBall->AnimationDirection = AnimationDirection_Up;                        
                    }
                    
                    PlaySound(AppState, {AssetType_FireCast});
                    Player->State = EntityState_Standing;
                    Player->State = EntityState_Casting;
//                    AddFlags(Player, EntityFlag_Casting);
                    Player->AnimationState.SlotIndex = 0;
                    Consumed = true;
                }
            }
            else
            {
                Consumed = true;
            }
            
            if (Consumed)
            {
                RemoveEvent = true;
                Halted = true;
            }
            else
            {                    
                DelayedInput->TimeRemaining -= Input->DeltaTime;
                DelayedInputIndex++;
            }

        }
        if (RemoveEvent)
        {
            AppState->PlayerDelayedInput[DelayedInputIndex] =
                AppState->PlayerDelayedInput[--AppState->PlayerDelayedInputCount];
        }
    }
    
    if (Player->State == EntityState_Attacking ||
        Player->State == EntityState_Casting)//IsSet(Player, EntityFlag_Casting))
    {
        Move = 0;
    }
    
    if (Move)
    {
        Player->State = EntityState_Moving;
    }

    // Stop
    if (!Move &&
        Player->State == EntityState_Moving &&
        !(Player->State == EntityState_Casting))//IsSet(Player, EntityFlag_Casting))
    {
        Player->State = EntityState_Stopping;
        Player->AnimationState.SlotIndex = 0;
    }
    else
    {

    }
    
    // NOTE(zoubir): only from the ground, pressing again mid-air used
    // to restart the jump and let the player fly
    if (Input->SpaceButton.Pressed && !Jumping)
    {
        Player->State = EntityState_Jumping;
        Player->Velocity.Z = 230.f;
        PlaySound(AppState, {AssetType_ZoubirAudio});
    }
    
    Player->ShockwaveCooldown = Maximum(0.f, Player->ShockwaveCooldown - DeltaTime);
    Player->ShockwaveFlash = Maximum(0.f, Player->ShockwaveFlash - DeltaTime);
    if (Input->ButtonE.Pressed && Player->ShockwaveCooldown <= 0.f)
    {
        Player->ShockwaveCooldown = PLAYER_SHOCKWAVE_COOLDOWN;
        TriggerShockwave(AppState, World, Player);
        PlaySound(AppState, {AssetType_FireCast});
    }

    Player->DashCooldown = Maximum(0.f, Player->DashCooldown - DeltaTime);
    if (Input->AltButton.Pressed && Player->DashCooldown <= 0.f)
    {
        Player->DashCooldown = PLAYER_DASH_COOLDOWN;
        PlayerAcceleration *= 10;
        PlaySound(AppState, {AssetType_Dash});
    }

    // Animation
    if (Player->State == EntityState_Stopping &&
        IsAnimationFinished(Player->AnimationSet,
                            &Player->AnimationState,
                            AnimationType_Stop,
                            *AnimationDirection))
    {
        Player->State = EntityState_Standing;
    }
    
    if (Player->State == EntityState_Stopping)
    {
        Assert(!Move);
        if (Player->Direction.Y == -1.f)
        {            
            *AnimationType = AnimationType_Stop;
            *AnimationDirection = AnimationDirection_Up;
        }
        if (Player->Direction.Y == 1.f)
        {        
            *AnimationType = AnimationType_Stop;
            *AnimationDirection = AnimationDirection_Down;
        }
        if (Player->Direction.X == 1.f)
        {
            *AnimationType = AnimationType_Stop;
            *AnimationDirection = AnimationDirection_Right;
        }
        if (Player->Direction.X == -1.f)
        {
            *AnimationType = AnimationType_Stop;
            *AnimationDirection = AnimationDirection_Left;
        }
                
    }
    else if (Player->State == EntityState_Attacking)
    {
        if (Player->CastingDirection.Y == -1.f)
        {            
            *AnimationType = AnimationType_Attack;
            *AnimationDirection = AnimationDirection_Up;
        }
        if (Player->CastingDirection.Y == 1.f)
        {        
            *AnimationType = AnimationType_Attack;
            *AnimationDirection = AnimationDirection_Down;
        }
        if (Player->CastingDirection.X == 1.f)
        {
            *AnimationType = AnimationType_Attack;
            *AnimationDirection = AnimationDirection_Right;
        }
        if (Player->CastingDirection.X == -1.f)
        {
            *AnimationType = AnimationType_Attack;
            *AnimationDirection = AnimationDirection_Left;
        }
    }
    else
    {
        // Stand And Move
        if (Player->State == EntityState_Casting)//IsSet(Player, EntityFlag_Casting))
        {
            *AnimationType = AnimationType_Cast;
        }
        else if (Move)
        {
            *AnimationType = AnimationType_Move;             
        }
        else   
        {
            *AnimationType = AnimationType_Stand;
        }
        
        if (Player->Direction.Y == -1.f)
        {            
            if (Move)
            {
                DDPlayer.Y = -1;
            }
            *AnimationDirection = AnimationDirection_Up;
        }
        if (Player->Direction.Y == 1.f)
        {
            
            if (Move)
            {
                DDPlayer.Y = 1;
            }
            *AnimationDirection = AnimationDirection_Down;
        }
        if (Player->Direction.X == 1.f)
        {
            
            if (Move)
            {
                DDPlayer.X = 1;
            }
            *AnimationDirection = AnimationDirection_Right;
        }
        if (Player->Direction.X == -1.f)
        {
            
            if (Move)
            {
                DDPlayer.X = -1.f;
            }
            *AnimationDirection = AnimationDirection_Left;
        }
    
        if (Input->ButtonJ.Pressed)
        {
            playing_sound *BattleTheme =
                PlaySound(AppState, {AssetType_BattleTheme});
//            ChangeVolume(BattleTheme, 5, {});
        }
    
    }


#if 1
    float DDPlayerLengthSq = LengthSq(DDPlayer);
    if (DDPlayerLengthSq > 1.f)
    {
        DDPlayer *= 1.f / SquareRoot(DDPlayerLengthSq);
    }
#else
    if (DDPlayer.X != 0 && DDPlayer.Y != 0)
    {
        DDPlayer *= .707106781187f;
    }
#endif
    
    

    DDPlayer *= PlayerAcceleration * DeltaTime;
    // Drag
    DDPlayer -= (10.f * Player->Velocity);
    //Gravity
    DDPlayer.Z = -1000.f;
    
    
//    float DeltaZ = 0.5f * DDPlayer.Z * Square(DeltaTime) +
//        Player->Velocity.Z * DeltaTime;    
//    Player->Velocity.Z = DDPlayer.Z * DeltaTime +
//        Player->Velocity.Z;    
//    Player->Position.Z += DeltaZ * DeltaTime;
//    Player->Position.Z = Maximum(0.f, Player->Position.Z);
    
    float MaxDistance = 10000.f;
    MoveEntity(Player, World, Arena, Input, AppState,
               DDPlayer, &MaxDistance);



    if (Player->Velocity.Z > 0.f)
    {
        *AnimationType = AnimationType_JumpUp;
    }
    if (Player->Velocity.Z < 0.f)
    {
        *AnimationType = AnimationType_JumpDown;
    }

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
              app_input *Input, app_state *AppState,
              float *AnimationSpeed,
              animation_type *AnimationType,
              animation_direction *AnimationDirection)
{
    monster_stats *Stats = GetMonsterStats(Entity->MonsterKind);
    bool32 Flies = Stats->FlyHeight > 0.f;
    float DeltaTime = Input->DeltaTime;

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

    v3 DDEntity = {};
    world_entity *Target = AppState->Player;
    if (Target && Target->IsPresent)
    {
        v2 ToTarget = Target->Position.XY - Entity->Position.XY;
        float DistanceToTarget = Length(ToTarget);
        if (DistanceToTarget < Stats->AttackRange &&
            Entity->AttackCooldown <= 0.f)
        {
            Target->Hp -= Stats->AttackDamage;
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

    DDEntity *= Stats->Acceleration * DeltaTime;
    // Drag
    DDEntity -= (10.f * Entity->Velocity);
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
    MoveEntity(Entity, World, Arena, Input, AppState,
               DDEntity, &MaxDistance);
}

// NOTE(zoubir): the player is never removed, it goes back to the spawn
// point with full health so nothing holding AppState->Player dangles
internal void
RespawnPlayerIfDead(world_entity *Player, world *World,
                    memory_arena *Arena, app_state *AppState)
{
    if (Player->Hp <= 0.f)
    {
        v3 OldPosition = Player->Position;
        Player->Position = AppState->PlayerSpawnPosition;
        Player->Velocity = {};
        Player->Hp = Player->MaxHp;
        CheckAndChangeEntityChunk(AppState, World, Arena,
                                  OldPosition, Player);
    }
}

internal void
UpdateFamiliar(world_entity *Entity, world *World,
             memory_arena *Arena,
             app_input *Input, app_state *AppState,
             float *AnimationSpeed,
             animation_type *AnimationType,
             animation_direction *AnimationDirection)
{
    float DeltaTime = Input->DeltaTime;
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
        MoveEntity(Entity, World, Arena, Input, AppState,
                   DDEntity, &MaxDistance);
    }
}

internal void
UpdateFireBall(world_entity *Entity, world *World,
               memory_arena *Arena,
               app_input *Input, app_state *AppState)
{
    float DeltaTime = Input->DeltaTime;
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
        MoveEntity(Entity, World, Arena, Input, AppState,
                   DDEntity, &Entity->DistanceRemaining);
        Entity->TimeLeft -= DeltaTime;
    }
}

