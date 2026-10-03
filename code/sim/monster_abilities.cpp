/* Monster abilities: the state machine that runs a monster_ability.

   Ready    the monster walks and bites as usual (UpdateMonster). Each
            frame it picks the first ability that is off cooldown and whose
            range fits the distance to the nearest player.
   Windup   rooted in place, playing its windup row. The danger zone is
            drawn on the ground (art/monster_render.cpp), so players can
            read it and step out.
   Active   the hit lands, or the monster moves (charges).
   Recover  rooted again, playing its recover row: the window to punish.

   UpdateMonsterAbilities returns true while an ability owns the monster,
   in which case it has already moved it this frame. */

#define MONSTER_ABILITY_RETRY_SECONDS 0.5f
// NOTE(zoubir): a charge that covers less than this share of its speed in a
// frame has hit a wall and stuns the monster for longer
#define CHARGE_BLOCKED_SHARE 0.35f
#define CHARGE_WALL_STUN_SCALE 1.75f

internal world_entity *
FindMonsterTarget(world *World, v2 From, float *DistanceOut)
{
    world_entity *Result = 0;
    float BestDistance = 0.f;
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (Entity->IsPresent && Entity->Type == EntityType_Player &&
            Entity->Hp > 0.f)
        {
            float Distance = Length(Entity->Position.XY - From);
            if (!Result || Distance < BestDistance)
            {
                Result = Entity;
                BestDistance = Distance;
            }
        }
    }
    if (DistanceOut)
    {
        *DistanceOut = BestDistance;
    }
    return Result;
}

// NOTE(zoubir): every monster hit on a player goes through here: damage
// (through DamageEntity, so deaths are counted), push, and the ability's
// status effect. Source is the monster or shot, which earns no kill credit
internal void
HitPlayer(app_state *AppState, world *World, world_entity *Player,
          world_entity *Source, monster_ability *Ability, v2 Push)
{
    if (!Player->IsPresent || Player->Hp <= 0.f)
    {
        return;
    }
    Player->Velocity.XY += Push;
    ApplyStatus(Player, Ability->Status, Ability->StatusSeconds);
    DamageEntity(AppState, World, Player, Ability->Damage, Source);
}

// NOTE(zoubir): hits every player within Radius of Center, pushing them
// away from it. Returns how many were hit
internal u32
HurtPlayersInRadius(app_state *AppState, world *World, world_entity *Source,
                    v2 Center, monster_ability *Ability)
{
    u32 HitCount = 0;
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player ||
            Player->Hp <= 0.f)
        {
            continue;
        }
        v2 Away = Player->Position.XY - Center;
        float Distance = Length(Away);
        if (Distance > Ability->Radius)
        {
            continue;
        }
        HitCount++;
        v2 Push = Distance > 0.f ? (Ability->Knockback / Distance) * Away : V2(0.f);
        HitPlayer(AppState, World, Player, Source, Ability, Push);
    }
    return HitCount;
}

inline random_series *
GetMonsterSeries(app_state *AppState)
{
    random_series *Result = 0;
    if (AppState->Monsters)
    {
        Result = &AppState->Monsters->Series;
    }
    return Result;
}

inline float
MonsterRandomBetween(app_state *AppState, float Min, float Max)
{
    random_series *Series = GetMonsterSeries(AppState);
    float Result = Series ? RandomBetween(Series, Min, Max) : 0.5f * (Min + Max);
    return Result;
}

inline bool32
IsInsideArena(world *World, v2 Position, float Margin)
{
    float MapWidth = (float)(World->NumTilesX * World->TileWidth);
    float MapHeight = (float)(World->NumTilesY * World->TileHeight);
    bool32 Result = Position.X > Margin && Position.Y > Margin &&
        Position.X < MapWidth - Margin && Position.Y < MapHeight - Margin;
    return Result;
}

// NOTE(zoubir): a monster that just spawned does not fire everything at
// once; each ability starts part way through its cooldown
internal void
StaggerMonsterCooldowns(app_state *AppState, world_entity *Entity)
{
    monster_def *Def = GetMonsterDef(Entity->MonsterKind);
    for(u32 AbilityIndex = 0;
        AbilityIndex < Def->AbilityCount;
        AbilityIndex++)
    {
        float Cooldown = Def->Abilities[AbilityIndex].Cooldown;
        Entity->AbilityCooldowns[AbilityIndex] =
            MonsterRandomBetween(AppState, 0.3f * Cooldown, Cooldown);
    }
}

inline void
RestartMonsterAnimation(world_entity *Entity)
{
    Entity->AnimationState.SlotIndex = 0;
    Entity->AnimationState.DeltaTime = 0.f;
}

inline void
SetMonsterPhase(world_entity *Entity, ability_phase Phase, float Seconds)
{
    Entity->AbilityPhase = Phase;
    Entity->AbilityTimer = Seconds;
    RestartMonsterAnimation(Entity);
}

// NOTE(zoubir): the spot a blink lands on: Distance past the target on the
// far side from the monster, or to its sides if that is blocked
internal bool32
FindBlinkSpot(app_state *AppState, world *World, world_entity *Entity,
              world_entity *Target, float Distance, v2 *Spot)
{
    v2 Through = Target->Position.XY - Entity->Position.XY;
    float Length0 = Length(Through);
    Through = Length0 > 0.f ? (1.f / Length0) * Through : V2(1.f, 0.f);
    v2 Side = V2(-Through.Y, Through.X);
    v2 Candidates[] =
        {
            Through,
            Side,
            -Side,
        };
    for(u32 CandidateIndex = 0;
        CandidateIndex < ArrayCount(Candidates);
        CandidateIndex++)
    {
        v2 Position = Target->Position.XY + Distance * Candidates[CandidateIndex];
        v3 Position3 = V3(Position.X, Position.Y, Entity->Position.Z);
        if (IsInsideArena(World, Position, 40.f) &&
            IsSpawnSpotFree(AppState, World, Position3, Entity->Collision))
        {
            *Spot = Position;
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): locks in what the ability needs to know at windup start.
// Returns false when it cannot be used right now (blink with no room)
internal bool32
StartMonsterAbility(app_state *AppState, world *World, world_entity *Entity,
                    u32 AbilityIndex, world_entity *Target)
{
    monster_def *Def = GetMonsterDef(Entity->MonsterKind);
    monster_ability *Ability = &Def->Abilities[AbilityIndex];
    v2 ToTarget = Target->Position.XY - Entity->Position.XY;
    float Distance = Length(ToTarget);

    Entity->AbilityIndex = AbilityIndex;
    Entity->AbilityHasHit = false;
    Entity->AbilityPointCount = 0;
    Entity->AbilityAim = Distance > 0.f ? (1.f / Distance) * ToTarget :
        V2(1.f, 0.f);

    switch(Ability->Kind)
    {
        case MonsterAbility_Mortar:
        {
            // NOTE(zoubir): the first shell leads a moving target, the rest
            // scatter around it to cut off the escape
            v2 Lead = Target->Position.XY +
                (0.5f * Ability->Windup) * Target->Velocity.XY;
            u32 Count = Minimum(Ability->Count, (u32)MAX_ABILITY_POINTS);
            for(u32 PointIndex = 0; PointIndex < Count; PointIndex++)
            {
                v2 Point = Lead;
                if (PointIndex > 0)
                {
                    float Angle = MonsterRandomBetween(AppState, 0.f, 2.f * Pi32);
                    float Reach = MonsterRandomBetween(AppState, 0.5f, 1.f) *
                        Ability->Spread;
                    Point += Reach * V2(Cos(Angle), Sin(Angle));
                }
                Entity->AbilityPoints[Entity->AbilityPointCount++] = Point;
            }
        } break;

        case MonsterAbility_Blink:
        {
            v2 Spot;
            if (!FindBlinkSpot(AppState, World, Entity, Target,
                               Ability->Spread, &Spot))
            {
                return false;
            }
            Entity->AbilityPoints[Entity->AbilityPointCount++] = Spot;
        } break;

        default:
        {
        } break;
    }

    SetMonsterPhase(Entity, AbilityPhase_Windup, Ability->Windup);
    return true;
}

// NOTE(zoubir): Direction must be unit length
internal world_entity *
AddMonsterShot(app_state *AppState, world *World, memory_arena *Arena,
               world_entity *Owner, monster_ability *Ability, v2 Direction)
{
    v3 Start = Owner->Position;
    Start.XY += 12.f * Direction;
    // NOTE(zoubir): hand height, so the shot reads as thrown, and its
    // shadow shows where it really is
    Start.Z = Maximum(Owner->Position.Z, 14.f);
    world_entity *Shot = AddEntity(AppState, World, Arena,
                                   EntityType_MonsterShot, Start,
                                   AppState->FireBallCollision);
    Shot->MonsterKind = Owner->MonsterKind;
    Shot->AbilityIndex = Owner->AbilityIndex;
    Shot->Velocity.XY = Ability->Speed * Direction;
    Shot->TimeLeft = Ability->Active;
    Shot->Dimensions = V2((float)SHOT_FRAME_SIZE, (float)SHOT_FRAME_SIZE);
    Shot->Texture = {AssetType_MonsterShot, (u32)Ability->ShotStyle};
    Shot->ShadowTexture = {AssetType_Shadow};
    Shot->AnimationDirection = Direction.X < 0.f ?
        AnimationDirection_Left : AnimationDirection_Right;
    if (AppState->Monsters)
    {
        Shot->AnimationSet =
            &AppState->Monsters->ShotAnimationSets[Ability->ShotStyle];
    }
    return Shot;
}

// NOTE(zoubir): the directions of a volley's shots, evenly fanned over
// Spread degrees around Aim. Returns how many were written
internal u32
GetVolleyDirections(monster_ability *Ability, v2 Aim, v2 *Directions,
                    u32 MaxDirections)
{
    u32 Count = Minimum(Ability->Count, MaxDirections);
    float BaseAngle = ATan2(Aim.Y, Aim.X);
    float Fan = Ability->Spread * (Pi32 / 180.f);
    for(u32 ShotIndex = 0; ShotIndex < Count; ShotIndex++)
    {
        float Offset = Count > 1 ?
            Fan * ((float)ShotIndex / (float)(Count - 1) - 0.5f) : 0.f;
        Directions[ShotIndex] = V2(Cos(BaseAngle + Offset),
                                   Sin(BaseAngle + Offset));
    }
    return Count;
}

#define MAX_VOLLEY_SHOTS 7

// NOTE(zoubir): a patch of ground left behind by an ability (bile, webs,
// embers). Anyone standing in it keeps getting the ability's status
internal world_entity *
AddMonsterHazard(app_state *AppState, world *World, memory_arena *Arena,
                 world_entity *Owner, monster_ability *Ability, v2 Center)
{
    world_entity *Hazard = AddEntity(AppState, World, Arena,
                                     EntityType_MonsterHazard,
                                     V3(Center.X, Center.Y, 0.f),
                                     AppState->FireBallCollision);
    Hazard->MonsterKind = Owner->MonsterKind;
    Hazard->AbilityIndex = Owner->AbilityIndex;
    Hazard->TimeLeft = Ability->HazardSeconds;
    float Size = 2.f * Ability->Radius;
    Hazard->Dimensions = V2(Size, Size);
    Hazard->Texture = {AssetType_MonsterHazard, (u32)Ability->HazardStyle};
    if (AppState->Monsters)
    {
        Hazard->AnimationSet =
            &AppState->Monsters->HazardAnimationSets[Ability->HazardStyle];
    }
    return Hazard;
}

internal void
UpdateMonsterHazard(world_entity *Hazard, world *World, app_state *AppState,
                    float DeltaTime)
{
    monster_def *Def = GetMonsterDef(Hazard->MonsterKind);
    monster_ability *Ability = &Def->Abilities[Hazard->AbilityIndex];
    Hazard->TimeLeft -= DeltaTime;
    if (Hazard->TimeLeft <= 0.f)
    {
        RemoveEntity(World, Hazard);
        return;
    }
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (Player->IsPresent && Player->Type == EntityType_Player &&
            Player->Hp > 0.f &&
            Length(Player->Position.XY - Hazard->Position.XY) <= Ability->Radius)
        {
            ApplyStatus(Player, Ability->Status, Ability->StatusSeconds);
        }
    }
}

// NOTE(zoubir): the moment the windup ends
internal void
TriggerMonsterAbility(app_state *AppState, world *World, memory_arena *Arena,
                      world_entity *Entity, monster_ability *Ability)
{
    switch(Ability->Kind)
    {
        case MonsterAbility_Slam:
        {
            HurtPlayersInRadius(AppState, World, Entity, Entity->Position.XY,
                                Ability);
            if (Ability->HazardSeconds > 0.f)
            {
                AddMonsterHazard(AppState, World, Arena, Entity, Ability,
                                 Entity->Position.XY);
            }
        } break;

        case MonsterAbility_Mortar:
        {
            for(u32 PointIndex = 0;
                PointIndex < Entity->AbilityPointCount;
                PointIndex++)
            {
                v2 Point = Entity->AbilityPoints[PointIndex];
                HurtPlayersInRadius(AppState, World, Entity, Point, Ability);
                if (Ability->HazardSeconds > 0.f)
                {
                    AddMonsterHazard(AppState, World, Arena, Entity, Ability,
                                     Point);
                }
            }
        } break;

        case MonsterAbility_Blink:
        {
            v2 Spot = Entity->AbilityPoints[0];
            v3 Spot3 = V3(Spot.X, Spot.Y, Entity->Position.Z);
            // NOTE(zoubir): someone may have walked onto the spot during the
            // windup; then the monster strikes from where it stands
            if (IsSpawnSpotFree(AppState, World, Spot3, Entity->Collision))
            {
                v2 Facing = Entity->Position.XY - Spot;
                v3 OldPosition = Entity->Position;
                Entity->Position = Spot3;
                Entity->Velocity = {};
                // NOTE(zoubir): a teleport skips MoveEntity, so the chunk
                // lists must be told; otherwise the next move asserts
                CheckAndChangeEntityChunk(AppState, World,
                                          Arena,
                                          OldPosition, Entity);
                float FacingLength = Length(Facing);
                if (FacingLength > 0.f)
                {
                    Entity->AbilityAim = (1.f / FacingLength) * Facing;
                }
            }
            HurtPlayersInRadius(AppState, World, Entity, Entity->Position.XY,
                                Ability);
        } break;

        case MonsterAbility_Volley:
        {
            v2 Directions[MAX_VOLLEY_SHOTS];
            u32 Count = GetVolleyDirections(Ability, Entity->AbilityAim,
                                            Directions, MAX_VOLLEY_SHOTS);
            for(u32 ShotIndex = 0; ShotIndex < Count; ShotIndex++)
            {
                AddMonsterShot(AppState, World, Arena, Entity, Ability,
                               Directions[ShotIndex]);
            }
        } break;

        default:
        {
        } break;
    }
}

// NOTE(zoubir): a shot flies straight until it runs out of time, hits a
// wall, or comes within its ability's Radius of a player
internal void
UpdateMonsterShot(world_entity *Shot, world *World, memory_arena *Arena,
                  float DeltaTime, app_state *AppState)
{
    monster_def *Def = GetMonsterDef(Shot->MonsterKind);
    monster_ability *Ability = &Def->Abilities[Shot->AbilityIndex];

    Shot->TimeLeft -= DeltaTime;
    if (Shot->TimeLeft <= 0.f)
    {
        RemoveEntity(World, Shot);
        return;
    }

    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (Player->IsPresent && Player->Type == EntityType_Player &&
            Player->Hp > 0.f &&
            Length(Player->Position.XY - Shot->Position.XY) <= Ability->Radius)
        {
            float Speed = Length(Shot->Velocity.XY);
            v2 Push = Speed > 0.f ?
                (Ability->Knockback / Speed) * Shot->Velocity.XY : V2(0.f);
            HitPlayer(AppState, World, Player, Shot, Ability, Push);
            RemoveEntity(World, Shot);
            return;
        }
    }

    v3 Start = Shot->Position;
    float Expected = Length(Shot->Velocity.XY) * DeltaTime;
    v3 DDEntity = {};
    float MaxDistance = 10000.f;
    MoveEntity(Shot, World, Arena, DeltaTime, AppState, DDEntity, &MaxDistance);
    if (Shot->IsPresent &&
        Length(Shot->Position.XY - Start.XY) < 0.5f * Expected)
    {
        RemoveEntity(World, Shot);
    }
}

// NOTE(zoubir): charges move the monster and hit whoever they reach
internal void
UpdateCharge(app_state *AppState, world *World, world_entity *Entity,
             monster_ability *Ability)
{
    if (Entity->AbilityHasHit)
    {
        return;
    }
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (Player->IsPresent && Player->Type == EntityType_Player &&
            Player->Hp > 0.f &&
            Length(Player->Position.XY - Entity->Position.XY) <=
            Ability->Radius)
        {
            HitPlayer(AppState, World, Player, Entity, Ability,
                      Ability->Knockback * Entity->AbilityAim);
            Entity->AbilityHasHit = true;
        }
    }
}

inline animation_direction
FacingFromAim(v2 Aim, animation_direction Current)
{
    animation_direction Result = Current;
    if (Aim.X > 0.05f)
    {
        Result = AnimationDirection_Right;
    }
    else if (Aim.X < -0.05f)
    {
        Result = AnimationDirection_Left;
    }
    return Result;
}

// NOTE(zoubir): plays a sheet row so that it lasts exactly Seconds
inline float
AnimationSpeedToFit(monster_def *Def, monster_sheet_row Row, float Seconds)
{
    float RowSeconds = Def->FrameCounts[Row] * Def->SecondsPerFrame[Row];
    float Result = RowSeconds > 0.f ? Seconds / RowSeconds : 1.f;
    return Result;
}

internal bool32
UpdateMonsterAbilities(world_entity *Entity, world *World,
                       memory_arena *Arena,
                       float DeltaTime, app_state *AppState,
                       float *AnimationSpeed,
                       animation_type *AnimationType,
                       animation_direction *AnimationDirection)
{
    monster_def *Def = GetMonsterDef(Entity->MonsterKind);
    for(u32 AbilityIndex = 0;
        AbilityIndex < Def->AbilityCount;
        AbilityIndex++)
    {
        float *Cooldown = &Entity->AbilityCooldowns[AbilityIndex];
        *Cooldown = Maximum(0.f, *Cooldown - DeltaTime);
    }

    if (Entity->AbilityPhase == AbilityPhase_Ready)
    {
        float Distance;
        world_entity *Target = FindMonsterTarget(World, Entity->Position.XY,
                                                 &Distance);
        if (!Target || Distance > Def->AggroRange)
        {
            return false;
        }
        // NOTE(zoubir): a bite that is ready lands first; the ability can
        // start next frame, once the bite is on cooldown
        if (Distance < Def->AttackRange && Entity->AttackCooldown <= 0.f)
        {
            return false;
        }
        for(u32 AbilityIndex = 0;
            AbilityIndex < Def->AbilityCount;
            AbilityIndex++)
        {
            monster_ability *Ability = &Def->Abilities[AbilityIndex];
            if (Entity->AbilityCooldowns[AbilityIndex] <= 0.f &&
                Distance >= Ability->MinRange &&
                Distance <= Ability->MaxRange)
            {
                if (StartMonsterAbility(AppState, World, Entity,
                                        AbilityIndex, Target))
                {
                    break;
                }
                Entity->AbilityCooldowns[AbilityIndex] =
                    MONSTER_ABILITY_RETRY_SECONDS;
            }
        }
        if (Entity->AbilityPhase == AbilityPhase_Ready)
        {
            return false;
        }
    }

    monster_ability *Ability = &Def->Abilities[Entity->AbilityIndex];
    Entity->AbilityTimer -= DeltaTime;
    v3 DDEntity = {};
    v3 StartPosition = Entity->Position;
    bool32 Charging = false;

    switch(Entity->AbilityPhase)
    {
        case AbilityPhase_Windup:
        {
            *AnimationType = AnimationType_Cast;
            *AnimationSpeed = AnimationSpeedToFit(Def, MonsterRow_Windup,
                                                  Ability->Windup);
            if (Entity->AbilityTimer <= 0.f)
            {
                TriggerMonsterAbility(AppState, World, Arena, Entity, Ability);
                SetMonsterPhase(Entity, AbilityPhase_Active, Ability->Active);
            }
        } break;

        case AbilityPhase_Active:
        {
            *AnimationType = AnimationType_Attack;
            *AnimationSpeed = AnimationSpeedToFit(Def, MonsterRow_Attack,
                                                  Ability->Active);
            if (Ability->Kind == MonsterAbility_Charge)
            {
                *AnimationSpeed = 1.f;
                Charging = true;
                UpdateCharge(AppState, World, Entity, Ability);
                if (Entity->AbilityHasHit)
                {
                    Entity->AbilityTimer = 0.f;
                }
            }
            if (Entity->AbilityTimer <= 0.f)
            {
                SetMonsterPhase(Entity, AbilityPhase_Recover, Ability->Recover);
                Charging = false;
            }
        } break;

        case AbilityPhase_Recover:
        {
            *AnimationType = AnimationType_Stop;
            if (Entity->AbilityTimer <= 0.f)
            {
                Entity->AbilityCooldowns[Entity->AbilityIndex] = Ability->Cooldown;
                SetMonsterPhase(Entity, AbilityPhase_Ready, 0.f);
                *AnimationType = AnimationType_Stand;
            }
        } break;

        default:
        {
            InvalidCodePath;
        } break;
    }

    *AnimationDirection = FacingFromAim(Entity->AbilityAim,
                                        Entity->AnimationState.LastAnimationDirection);

    bool32 Flies = Def->FlyHeight > 0.f;
    if (Charging)
    {
        // NOTE(zoubir): a charge ignores drag and runs at full speed
        Entity->Velocity.XY = Ability->Speed * Entity->AbilityAim;
    }
    else
    {
        DDEntity -= 10.f * Entity->Velocity;
    }
    if (Flies)
    {
        Entity->tFlying += DeltaTime * 6.f;
        if (Entity->tFlying > 2.f * Pi32)
        {
            Entity->tFlying -= 2.f * Pi32;
        }
        Entity->Position.Z = Def->FlyHeight + 4.f * Sin(Entity->tFlying);
        DDEntity.Z = 0.f;
        Entity->Velocity.Z = 0.f;
    }
    else
    {
        DDEntity.Z = -1000.f;
    }

    float MaxDistance = 10000.f;
    MoveEntity(Entity, World, Arena, DeltaTime, AppState, DDEntity, &MaxDistance);

    if (Charging && Entity->IsPresent)
    {
        float Moved = Length(Entity->Position.XY - StartPosition.XY);
        if (Moved < CHARGE_BLOCKED_SHARE * Ability->Speed * DeltaTime)
        {
            SetMonsterPhase(Entity, AbilityPhase_Recover,
                            CHARGE_WALL_STUN_SCALE * Ability->Recover);
        }
    }
    return true;
}
