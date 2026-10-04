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
   in which case it has already moved it this frame. It is the only part
   here; the rest lives in monster_abilities/:

     hits.cpp               targets, damage, bite, area hits
     helpers.cpp            random series, arena bounds, phase switching
     start.cpp              what each kind locks in at windup start
     shots_and_hazards.cpp  shots, volleys and ground hazards
     trigger.cpp            what each kind does when the windup ends
     movement.cpp           burrow, charge, slow turning
     armor.cpp              front shells
     phases.cpp             enrage phases */

#define MONSTER_ABILITY_RETRY_SECONDS 0.5f
// NOTE(zoubir): how long the shell glints after blocking a hit
#define BLOCK_FLASH_SECONDS 0.2f
// NOTE(zoubir): a charge that covers less than this share of its speed in a
// frame has hit a wall and stuns the monster for longer
#define CHARGE_BLOCKED_SHARE 0.35f
#define CHARGE_WALL_STUN_SCALE 1.75f

// NOTE(zoubir): the parts, in the order they depend on each other
#include "monster_abilities/hits.cpp"
#include "monster_abilities/helpers.cpp"
#include "monster_abilities/start.cpp"
#include "monster_abilities/shots_and_hazards.cpp"
#include "monster_abilities/trigger.cpp"
#include "monster_abilities/movement.cpp"
#include "monster_abilities/armor.cpp"
#include "monster_abilities/phases.cpp"

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
    Entity->BlockFlash = Maximum(0.f, Entity->BlockFlash - DeltaTime);
    UpdateMonsterPhase(Entity, Def, DeltaTime);
    UpdateMonsterFacing(World, Entity, Def, DeltaTime);
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
                AbilityAllowedInPhase(Ability, Entity) &&
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
            if (Ability->Kind == MonsterAbility_Burrow)
            {
                *AnimationType = AnimationType_JumpDown;
                *AnimationSpeed = 1.f;
                UpdateBurrow(World, Entity, Ability);
            }
            if (Entity->AbilityTimer <= 0.f)
            {
                if (Ability->Kind == MonsterAbility_Burrow)
                {
                    EruptFromBurrow(AppState, World, Arena, Entity, Ability);
                }
                SetMonsterPhase(Entity, AbilityPhase_Recover, Ability->Recover);
                Charging = false;
            }
        } break;

        case AbilityPhase_Recover:
        {
            *AnimationType = AnimationType_Stop;
            if (Entity->AbilityTimer <= 0.f)
            {
                Entity->AbilityCooldowns[Entity->AbilityIndex] =
                    Ability->Cooldown * GetAffix(Entity->EliteAffix)->CooldownScale *
                    GetPhaseCooldownScale(Entity, Def);
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
        DDEntity -= 10.f * GetGroundFriction(Entity) * Entity->Velocity;
    }
    if (Flies)
    {
        Entity->tFlying += DeltaTime * 6.f;
        if (Entity->tFlying > 2.f * Pi32)
        {
            Entity->tFlying -= 2.f * Pi32;
        }
        v3 OldPosition = Entity->Position;
        Entity->Position.Z = Def->FlyHeight + 4.f * Sin(Entity->tFlying) +
            GetHoverBase(World, Entity);
        CheckAndChangeEntityChunk(AppState, World, Arena, OldPosition, Entity);
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
