/* The moment a windup ends: what each ability kind does when it lands
   (TriggerMonsterAbility). */

// NOTE(zoubir): the player a Smite is aimed at, while they are alive and
// within reach (1.25 MaxRange, room to walk a little)
internal world_entity *
GetSmiteVictim(world *World, world_entity *Entity, monster_ability *Ability)
{
    world_entity *Result = 0;
    if (Entity->AbilityTargetSlot < World->EntityCount)
    {
        world_entity *Victim = &World->Entities[Entity->AbilityTargetSlot];
        if (Victim->IsPresent && Victim->Type == EntityType_Player && Victim->Hp > 0.f &&
            Length(Victim->Position.XY - Entity->Position.XY) <= 1.25f * Ability->MaxRange)
        {
            Result = Victim;
        }
    }
    return Result;
}

// NOTE(zoubir): during a Smite's windup the victim is whoever the monster
// is after now (a taunt moves it), the mark follows them and the monster
// turns to them
internal void
TrackSmiteVictim(app_state *AppState, world *World, world_entity *Entity,
                 monster_ability *Ability)
{
    world_entity *Wanted = FindMonsterTarget(AppState, World, Entity, 0);
    if (Wanted)
    {
        Entity->AbilityTargetSlot = Wanted->ID;
    }
    world_entity *Victim = GetSmiteVictim(World, Entity, Ability);
    if (Victim && Entity->AbilityPointCount)
    {
        Entity->AbilityPoints[0] = Victim->Position.XY;
        v2 ToVictim = Victim->Position.XY - Entity->Position.XY;
        float Distance = Length(ToVictim);
        if (Distance > 0.f)
        {
            Entity->AbilityAim = (1.f / Distance) * ToVictim;
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

        case MonsterAbility_Summon:
        {
            monster_def *SummonDef = GetMonsterDef(Ability->SummonKind);
            entity_collision_volume_group *Volume = SummonDef->FlyHeight > 0.f ?
                AppState->BatCollision : AppState->PlayerCollision;
            for(u32 PointIndex = 0;
                PointIndex < Entity->AbilityPointCount;
                PointIndex++)
            {
                v2 Point = Entity->AbilityPoints[PointIndex];
                v3 Point3 = V3(Point.X, Point.Y, 0.f);
                // NOTE(zoubir): a player standing on the mark stops that one
                if (IsSpawnSpotFree(AppState, World, Point3, Volume))
                {
                    world_entity *Summon = SpawnMonster(AppState, World, Arena,
                                                        Point3, Ability->SummonKind);
                    Summon->SummonerSlot = Entity->ID;
                    Summon->SummonerSerial = Entity->MonsterSerial;
                }
            }
        } break;

        case MonsterAbility_Burrow:
        {
            Entity->Burrowed = true;
            Entity->Velocity = {};
        } break;

        case MonsterAbility_Smite:
        {
            world_entity *Victim = GetSmiteVictim(World, Entity, Ability);
            if (!Victim)
            {
                break;
            }
            v2 ToVictim = Victim->Position.XY - Entity->Position.XY;
            v2 Spot;
            if (Ability->Spread > 0.f && Length(ToVictim) > Ability->Spread + 20.f &&
                FindBlinkSpot(AppState, World, Entity, Victim, Ability->Spread, &Spot))
            {
                v3 OldPosition = Entity->Position;
                Entity->Position = V3(Spot.X, Spot.Y, Entity->Position.Z);
                Entity->Velocity = {};
                CheckAndChangeEntityChunk(AppState, World, Arena, OldPosition, Entity);
                ToVictim = Victim->Position.XY - Spot;
            }
            float Distance = Length(ToVictim);
            if (Distance > 0.f)
            {
                Entity->AbilityAim = (1.f / Distance) * ToVictim;
            }
            Entity->AbilityPoints[0] = Victim->Position.XY;
            SmitePlayer(AppState, World, Entity, Victim, Ability);
        } break;

        case MonsterAbility_Mend:
        {
            world_entity *Ally = FindMonsterBySerial(World, Entity->AbilityTargetSlot,
                                                     Entity->AbilityTargetSerial);
            // NOTE(zoubir): the ally can walk a little during the windup
            if (Ally && Length(Ally->Position.XY - Entity->Position.XY) <=
                1.5f * Ability->Radius)
            {
                Ally->Hp = Minimum(Ally->MaxHp, Ally->Hp + Ability->Heal);
            }
        } break;

        default:
        {
        } break;
    }
}
