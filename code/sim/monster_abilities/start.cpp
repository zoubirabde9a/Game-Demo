/* Starting an ability (the windup): what each kind locks in when it
   begins, and the searches some need (blink spot, summon count, mend
   target). StartMonsterAbility returns false when it cannot run now. */

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

#define MEND_THRESHOLD 0.7f
// NOTE(zoubir): share of a burrow's Active time, at the end, during which
// the landing spot no longer follows the target
#define BURROW_LOCK_SHARE 0.4f

inline u32
CountActiveSummons(world *World, world_entity *Summoner)
{
    u32 Result = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster &&
            Entity->SummonerSerial == Summoner->MonsterSerial &&
            Entity->SummonerSlot == Summoner->ID)
        {
            Result++;
        }
    }
    return Result;
}

// NOTE(zoubir): the ally within Radius with the lowest share of its health,
// if that share is under MEND_THRESHOLD; never the healer itself
internal world_entity *
FindMendTarget(world *World, world_entity *Healer, float Radius)
{
    world_entity *Result = 0;
    float LowestShare = MEND_THRESHOLD;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Ally = &World->Entities[EntityIndex];
        if (Ally == Healer || !Ally->IsPresent ||
            Ally->Type != EntityType_Monster || Ally->MaxHp <= 0.f ||
            !Ally->MonsterSerial ||
            Length(Ally->Position.XY - Healer->Position.XY) > Radius)
        {
            continue;
        }
        float Share = Ally->Hp / Ally->MaxHp;
        if (Share < LowestShare)
        {
            LowestShare = Share;
            Result = Ally;
        }
    }
    return Result;
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

        case MonsterAbility_Summon:
        {
            u32 Active = CountActiveSummons(World, Entity);
            if (!Entity->MonsterSerial || Active >= Ability->MaxActive)
            {
                return false;
            }
            // NOTE(zoubir): rise between the summoner and its target
            monster_def *SummonDef = GetMonsterDef(Ability->SummonKind);
            entity_collision_volume_group *Volume = SummonDef->FlyHeight > 0.f ?
                AppState->BatCollision : AppState->PlayerCollision;
            u32 Count = Minimum(Ability->Count, Ability->MaxActive - Active);
            Count = Minimum(Count, (u32)MAX_ABILITY_POINTS);
            v2 Side = V2(-Entity->AbilityAim.Y, Entity->AbilityAim.X);
            for(u32 PointIndex = 0; PointIndex < Count; PointIndex++)
            {
                float Offset = Count > 1 ?
                    ((float)PointIndex / (float)(Count - 1) - 0.5f) : 0.f;
                v2 Point = Entity->Position.XY +
                    Ability->Spread * Entity->AbilityAim +
                    (2.f * Ability->Spread * Offset) * Side;
                if (IsInsideArena(World, Point, 40.f) &&
                    IsSpawnSpotFree(AppState, World,
                                    V3(Point.X, Point.Y, 0.f), Volume))
                {
                    Entity->AbilityPoints[Entity->AbilityPointCount++] = Point;
                }
            }
            if (Entity->AbilityPointCount == 0)
            {
                return false;
            }
        } break;

        case MonsterAbility_Burrow:
        {
            Entity->AbilityPoints[Entity->AbilityPointCount++] = Target->Position.XY;
            Entity->AbilityTargetSlot = Target->ID;
            // NOTE(zoubir): AbilityPoints[1] remembers where it dug in, for
            // the ripple drawn between there and the landing spot
            Entity->AbilityPoints[Entity->AbilityPointCount++] = Entity->Position.XY;
        } break;

        case MonsterAbility_Smite:
        {
            // NOTE(zoubir): AbilityPoints[0] follows the victim through the
            // windup (TrackSmiteVictim), so every client sees the mark on
            // them, and moves to whoever takes the threat meanwhile
            Entity->AbilityTargetSlot = Target->ID;
            Entity->AbilityPoints[Entity->AbilityPointCount++] = Target->Position.XY;
        } break;

        case MonsterAbility_Mend:
        {
            world_entity *Ally = FindMendTarget(World, Entity, Ability->Radius);
            if (!Ally)
            {
                return false;
            }
            Entity->AbilityTargetSlot = Ally->ID;
            Entity->AbilityTargetSerial = Ally->MonsterSerial;
            Entity->AbilityPoints[Entity->AbilityPointCount++] = Ally->Position.XY;
            v2 ToAlly = Ally->Position.XY - Entity->Position.XY;
            float AllyDistance = Length(ToAlly);
            if (AllyDistance > 0.f)
            {
                Entity->AbilityAim = (1.f / AllyDistance) * ToAlly;
            }
        } break;

        default:
        {
        } break;
    }

    SetMonsterPhase(Entity, AbilityPhase_Windup, Ability->Windup);
    return true;
}
