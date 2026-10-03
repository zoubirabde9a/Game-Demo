/* Elite affixes: a monster can spawn with one affix that changes its
   numbers and adds a behaviour on top of its kind. Affixes are a table,
   so a new one is a new row plus its enum value. Any kind can roll any
   affix; ELITE_CHANCE of refills roll one.

   Frenzied   abilities recharge much faster, moves faster
   Armored    twice the health, a little slower
   Vampiric   heals for part of the damage it deals
   Chilling   every hit slows the player

   Shots and hazards carry the affix of the monster that made them, so
   a Chilling imp's embers slow too. */

#define ELITE_CHANCE 0.15f

enum monster_affix
{
    MonsterAffix_None,
    MonsterAffix_Frenzied,
    MonsterAffix_Armored,
    MonsterAffix_Vampiric,
    MonsterAffix_Chilling,
    MonsterAffix_Count
};

struct monster_affix_def
{
    char *Name;
    float HpScale;
    float SpeedScale;
    float CooldownScale;
    float DamageScale;
    // NOTE(zoubir): share of damage dealt that heals the monster
    float LifeSteal;
    status_effect OnHitStatus;
    float OnHitStatusSeconds;
    // NOTE(zoubir): multiplied with the sprite (A B G R), and the color of
    // the ring drawn under it
    u32 Tint;
    u32 AuraColor;
};

global_variable monster_affix_def MonsterAffixTable[MonsterAffix_Count] =
{
    //              Hp    Speed Cool  Dmg   Steal  Status                 Secs  Tint        Aura
    {"",           1.f,  1.f,  1.f,  1.f,  0.f,   StatusEffect_None,     0.f,  0,          0},
    {"Frenzied",   1.2f, 1.35f, 0.55f, 1.f, 0.f,  StatusEffect_None,     0.f,  0xFF9090FF, 0xFF3060FF},
    {"Armored",    2.2f, 0.8f, 1.f,  1.f,  0.f,   StatusEffect_None,     0.f,  0xFFE0D0C0, 0xFFD0C0A0},
    {"Vampiric",   1.4f, 1.f,  1.f,  1.15f, 0.6f, StatusEffect_None,     0.f,  0xFFA0A0FF, 0xFF4020C0},
    {"Chilling",   1.4f, 1.f,  1.f,  1.f,  0.f,   StatusEffect_Slowed,   1.5f, 0xFFFFE0C0, 0xFFFFD080},
};

inline monster_affix_def *
GetAffix(u32 Affix)
{
    Assert(Affix < MonsterAffix_Count);
    monster_affix_def *Result = &MonsterAffixTable[Affix];
    return Result;
}

inline float
GetAffixSpeedScale(world_entity *Entity)
{
    float Result = 1.f;
    if (Entity->Type == EntityType_Monster)
    {
        Result = GetAffix(Entity->EliteAffix)->SpeedScale;
    }
    return Result;
}

// NOTE(zoubir): MonsterAffix_None most of the time
internal u32
RollEliteAffix(random_series *Series)
{
    u32 Result = MonsterAffix_None;
    if (RandomUnilateral(Series) < ELITE_CHANCE)
    {
        Result = 1 + RandomChoice(Series, MonsterAffix_Count - 1);
    }
    return Result;
}

internal void
ApplyEliteAffix(world_entity *Monster, u32 Affix)
{
    monster_affix_def *Def = GetAffix(Affix);
    Monster->EliteAffix = Affix;
    Monster->MaxHp *= Def->HpScale;
    Monster->Hp = Monster->MaxHp;
    if (Def->Tint)
    {
        Monster->Tint = Def->Tint;
    }
}
