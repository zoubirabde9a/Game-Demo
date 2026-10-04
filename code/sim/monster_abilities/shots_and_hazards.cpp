/* What abilities leave in the world: shots (single or fanned volleys)
   and lingering ground hazards, and how each updates. */

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
    Shot->EliteAffix = Owner->EliteAffix;
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
// embers). Anyone standing in it keeps getting the ability's status. On a
// bounded map the centre is kept on the map: a patch wholly past the edge
// was in no chunk, so nothing could touch, see or remove it (the toad's
// bile barrage aimed there, .agents/issues/toad-hazard-off-map.md)
internal world_entity *
AddMonsterHazard(app_state *AppState, world *World, memory_arena *Arena,
                 world_entity *Owner, monster_ability *Ability, v2 Center)
{
    if (!World->Unbounded)
    {
        float Width = (float)(World->NumTilesX * World->TileWidth);
        float Height = (float)(World->NumTilesY * World->TileHeight);
        Center.X = Minimum(Width, Maximum(0.f, Center.X));
        Center.Y = Minimum(Height, Maximum(0.f, Center.Y));
    }
    world_entity *Hazard = AddEntity(AppState, World, Arena,
                                     EntityType_MonsterHazard,
                                     V3(Center.X, Center.Y, 0.f),
                                     AppState->FireBallCollision);
    Hazard->MonsterKind = Owner->MonsterKind;
    Hazard->AbilityIndex = Owner->AbilityIndex;
    Hazard->EliteAffix = Owner->EliteAffix;
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
        // NOTE(zoubir): a player jumping over the patch is not touched
        if (Player->IsPresent && Player->Type == EntityType_Player &&
            Player->Hp > 0.f && !IsClearOfGround(Player) &&
            Length(Player->Position.XY - Hazard->Position.XY) <= Ability->Radius)
        {
            ApplyStatus(Player, Ability->Status, Ability->StatusSeconds);
        }
    }
}

// NOTE(zoubir): a shot flies straight until it runs out of time, hits a
// wall, or comes within its ability's Radius of a player who is not
// jumping over it (IsAboveShot)
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
            Player->Hp > 0.f && !IsAboveShot(Player, Shot) &&
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
