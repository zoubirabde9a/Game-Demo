/* Mirror guard (dungeon.cpp): a monster holding up a mirror (a Reflect
   ability in its Active time, sim/monster_ability_kinds.cpp) takes nothing
   from a player's blow and turns part of it back on that player. Called
   from DungeonScaleDamage next to the boss wards. Clients draw the mirror
   from the ability phase and index in the snapshot
   (client/dungeon/starless_fx.cpp), so nothing new goes on the wire. */

// NOTE(zoubir): the monster holds a mirror up through a Reflect's Active
// time; a hit on it then is turned back (MirrorTurnsBack)
internal monster_ability *
GetRaisedMirror(world_entity *Entity)
{
    monster_ability *Result = 0;
    if (Entity->Type == EntityType_Monster && Entity->AbilityPhase == AbilityPhase_Active)
    {
        monster_def *Def = GetMonsterDef(Entity->MonsterKind);
        if (Entity->AbilityIndex < Def->AbilityCount &&
            Def->Abilities[Entity->AbilityIndex].Kind == MonsterAbility_Reflect)
        {
            Result = &Def->Abilities[Entity->AbilityIndex];
        }
    }
    return Result;
}

// NOTE(zoubir): a mirror is raised or about to be: for bots, which stop
// hitting it in time (server/bots.cpp)
inline bool32
IsRaisingMirror(world_entity *Entity)
{
    bool32 Result = GetRaisedMirror(Entity) != 0;
    if (!Result && Entity->Type == EntityType_Monster &&
        Entity->AbilityPhase == AbilityPhase_Windup)
    {
        monster_def *Def = GetMonsterDef(Entity->MonsterKind);
        Result = Entity->AbilityIndex < Def->AbilityCount &&
            Def->Abilities[Entity->AbilityIndex].Kind == MonsterAbility_Reflect;
    }
    return Result;
}

// NOTE(zoubir): from DungeonScaleDamage: a blow on a raised mirror does
// nothing to the monster; Spread of it, at most Damage, lands on the
// player who struck instead. The striker is its own source, so the
// level's and the party's scaling do not grow it, only the striker's
// role does. Returns whether the mirror took the blow
internal bool32
MirrorTurnsBack(app_state *AppState, world_entity *Target, world_entity *Striker,
                float Damage)
{
    monster_ability *Mirror = GetRaisedMirror(Target);
    if (!Mirror)
    {
        return false;
    }
    Target->BlockFlash = 0.2f;
    v3 Chest = Target->Position;
    Chest.Z += 16.f;
    EmitBurst(&AppState->Events, SimBurst_Blocked, SIM_NOBODY, Chest);
    float Back = Minimum(Mirror->Damage, Mirror->Spread * Damage);
    if (Striker && Striker->IsPresent && Striker->Hp > 0.f && Back > 0.f)
    {
        v3 Hurt = Striker->Position;
        Hurt.Z += 14.f;
        EmitBurst(&AppState->Events, SimBurst_MonsterHit, SIM_NOBODY, Hurt);
        DamageEntity(AppState, &AppState->World, Striker, Back, Striker);
    }
    return true;
}
