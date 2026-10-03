/* Monster kinds: every monster is EntityType_Monster (so sword, fireball
   and collision rules apply to all of them); the kind picks its stats,
   look and whether it walks or flies. Add a kind to monster_kind in
   entity.h and a row here. */

struct monster_stats
{
    float MaxHp;
    float Acceleration;
    float AggroRange;
    float StopRange;
    float AttackRange;
    float AttackDamage;
    float AttackInterval;
    // NOTE(zoubir): 0 for walkers, hover height for flyers
    float FlyHeight;
    u32 Tint;
};

global_variable monster_stats MonsterStatsTable[MonsterKind_Count] =
{
    //            Hp   Accel  Aggro Stop Reach Dmg  Every Fly  Tint
    /* Brute */ {100.f, 28000.f, 320.f, 40.f, 52.f, 10.f, 1.0f, 0.f, 0xFF9090FF},
    /* Bat   */ { 40.f, 52000.f, 420.f, 20.f, 36.f,  4.f, 0.6f, 20.f, 0xFFFF80C0},
};

inline monster_stats *
GetMonsterStats(monster_kind Kind)
{
    Assert(Kind < MonsterKind_Count);
    monster_stats *Result = &MonsterStatsTable[Kind];
    return Result;
}
