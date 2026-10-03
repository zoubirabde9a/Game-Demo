/* Monster hits on players: finding the nearest target, dealing damage
   (with push and status), the plain bite, and area hits. */

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
DealMonsterDamage(app_state *AppState, world *World, world_entity *Player,
                  world_entity *Source, float Damage, v2 Push,
                  status_effect Status, float StatusSeconds)
{
    if (!Player->IsPresent || Player->Hp <= 0.f)
    {
        return;
    }
    monster_affix_def *Affix = GetAffix(Source ? Source->EliteAffix : 0);
    Damage *= Affix->DamageScale;
    Player->Velocity.XY += Push;
    ApplyStatus(Player, Status, StatusSeconds);
    ApplyStatus(Player, Affix->OnHitStatus, Affix->OnHitStatusSeconds);
    float Dealt = Minimum(Damage, Player->Hp);
    DamageEntity(AppState, World, Player, Damage, Source);
    if (Source && Source->Type == EntityType_Monster &&
        Source->IsPresent && Affix->LifeSteal > 0.f)
    {
        Source->Hp = Minimum(Source->MaxHp, Source->Hp + Affix->LifeSteal * Dealt);
    }
}

internal void
HitPlayer(app_state *AppState, world *World, world_entity *Player,
          world_entity *Source, monster_ability *Ability, v2 Push)
{
    DealMonsterDamage(AppState, World, Player, Source, Ability->Damage, Push,
                      Ability->Status, Ability->StatusSeconds);
}

// NOTE(zoubir): the plain bite every monster has, off AttackInterval
internal void
MonsterBite(app_state *AppState, world *World, world_entity *Monster,
            world_entity *Player)
{
    monster_def *Def = GetMonsterDef(Monster->MonsterKind);
    DealMonsterDamage(AppState, World, Player, Monster, Def->AttackDamage,
                      V2(0.f), StatusEffect_None, 0.f);
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
