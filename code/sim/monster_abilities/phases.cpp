/* Enrage phases: the one-way switch into a stronger phase at low health,
   which abilities each phase allows, and its cooldown scale. */

#define ENRAGE_FLASH_SECONDS 0.6f
// NOTE(zoubir): on enrage, abilities further out than this are pulled in
#define ENRAGE_COOLDOWN_CAP 0.5f

inline u32
GetPhaseBit(world_entity *Entity)
{
    u32 Result = Entity->Phase ? PHASE_ENRAGED : PHASE_CALM;
    return Result;
}

inline bool32
AbilityAllowedInPhase(monster_ability *Ability, world_entity *Entity)
{
    bool32 Result = !Ability->PhaseMask ||
        (Ability->PhaseMask & GetPhaseBit(Entity));
    return Result;
}

// NOTE(zoubir): the one-way switch into the enraged phase
internal void
UpdateMonsterPhase(world_entity *Entity, monster_def *Def, float DeltaTime)
{
    Entity->PhaseFlash = Maximum(0.f, Entity->PhaseFlash - DeltaTime);
    if (Entity->Phase || Def->EnrageHpShare <= 0.f ||
        Entity->Hp >= Def->EnrageHpShare * Entity->MaxHp)
    {
        return;
    }
    Entity->Phase = 1;
    Entity->PhaseSpeedScale = Def->EnrageSpeedScale > 0.f ? Def->EnrageSpeedScale : 1.f;
    Entity->PhaseFlash = ENRAGE_FLASH_SECONDS;
    if (Def->EnrageTint)
    {
        Entity->Tint = Def->EnrageTint;
    }
    for(u32 AbilityIndex = 0; AbilityIndex < Def->AbilityCount; AbilityIndex++)
    {
        Entity->AbilityCooldowns[AbilityIndex] =
            Minimum(Entity->AbilityCooldowns[AbilityIndex], ENRAGE_COOLDOWN_CAP);
    }
}

inline float
GetPhaseCooldownScale(world_entity *Entity, monster_def *Def)
{
    float Result = 1.f;
    if (Entity->Phase && Def->EnrageCooldownScale > 0.f)
    {
        Result = Def->EnrageCooldownScale;
    }
    return Result;
}
