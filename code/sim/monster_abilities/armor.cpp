/* Front armour: hits from inside a shell's front arc are reduced
   (ModifyIncomingDamage, called by DamageEntity). */

// NOTE(zoubir): true when Source sits inside Target's armored front arc
internal bool32
IsInFrontArc(world_entity *Target, monster_def *Def, v2 SourcePosition)
{
    v2 ToSource = SourcePosition - Target->Position.XY;
    float Distance = Length(ToSource);
    if (Distance <= 0.f || LengthSq(Target->Direction) < 0.0001f)
    {
        return false;
    }
    float HalfArc = 0.5f * Def->FrontArcDegrees * (Pi32 / 180.f);
    bool32 Result = DotProduct((1.f / Distance) * ToSource, Target->Direction) >=
        Cos(HalfArc);
    return Result;
}

// NOTE(zoubir): DamageEntity runs every hit through this before taking
// health; monsters with a shell shrug off hits from the front
internal float
ModifyIncomingDamage(world_entity *Target, world_entity *Source, float Damage)
{
    float Result = Damage;
    if (Target->Type == EntityType_Monster && Target->Burrowed)
    {
        return 0.f;
    }
    if (Target->Type == EntityType_Monster && Source)
    {
        monster_def *Def = GetMonsterDef(Target->MonsterKind);
        if (Def->FrontArmor > 0.f &&
            IsInFrontArc(Target, Def, Source->Position.XY))
        {
            Result *= 1.f - Def->FrontArmor;
            Target->BlockFlash = BLOCK_FLASH_SECONDS;
        }
    }
    return Result;
}
