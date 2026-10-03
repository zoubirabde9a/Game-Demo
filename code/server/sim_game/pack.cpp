/* Snapshot packing: how one server entity becomes the fields of a
   net_entity_state (the monster kind or shot style in Variant, status
   effects as bits, facing and ability details for front-armoured and
   winding-up monsters), which entities are sent at all, and keeping the
   nearest ones when there are more than fit. The static_asserts fail the
   build when an enum outgrows its bits on the wire. Included by
   sim_game.cpp, whose GameWriteSnapshot puts a snapshot together. */

// NOTE(zoubir): what the client needs beyond the type: the monster's
// kind or the shot's style (kept in its texture) to pick the sprite, the
// player's slot to match it with the scoreboard
inline u8
SimGameVariant(world_entity *Entity)
{
    u8 Result = 0;
    if (Entity->Type == EntityType_Monster)
    {
        Result = (u8)Entity->MonsterKind;
    }
    else if (Entity->Type == EntityType_Player)
    {
        Result = (u8)Entity->PlayerIndex;
    }
    else if (Entity->Type == EntityType_MonsterHazard)
    {
        // NOTE(zoubir): with Ability, the client finds the hazard's look
        // and size in that monster kind's ability table
        Result = (u8)Entity->MonsterKind;
    }
    else if (Entity->Type == EntityType_MonsterShot)
    {
        Result = (u8)Entity->Texture.Index;
    }
    return Result;
}

// NOTE(zoubir): the protocol packs these into a few bits each
static_assert(AnimationDirection_Count <= 4, "Facing is 2 bits on the wire");
static_assert(AnimationType_Count <= 16, "Animation is 4 bits on the wire");
static_assert(MonsterAffix_Count <= 8, "Affix is 3 bits on the wire");
static_assert(StatusEffect_Count - 1 <= 3, "Status is 3 bits on the wire");
static_assert(MAX_MONSTER_ABILITIES <= 4, "Ability is 2 bits on the wire");
static_assert(EntityType_Count <= 64, "Type is 6 bits on the wire");

// NOTE(zoubir): bit N set while status effect N + 1 is running
inline u8
SimGameStatusBits(world_entity *Entity)
{
    u8 Result = 0;
    for (u32 Effect = 1; Effect < StatusEffect_Count; ++Effect)
    {
        if (Entity->StatusTimers[Effect] > 0.f) Result |= (u8)(1 << (Effect - 1));
    }
    return Result;
}

// NOTE(zoubir): front-armoured monsters send which way they face, as a
// whole turn in 256 steps, so clients draw the shell on the right side
internal void
SimGameWriteFacing(world_entity *Entity, u8 EntityIndex, net_snapshot *Out)
{
    if (Entity->Type != EntityType_Monster ||
        Out->FacingCount >= NET_MAX_SNAPSHOT_FACINGS ||
        GetMonsterDef(Entity->MonsterKind)->FrontArmor <= 0.f ||
        LengthSq(Entity->Direction) < 0.0001f)
    {
        return;
    }
    float Turns = ATan2(Entity->Direction.Y, Entity->Direction.X) / (2.f * Pi32);
    net_facing *Facing = &Out->Facings[Out->FacingCount++];
    Facing->EntityIndex = EntityIndex;
    Facing->Angle = (u8)(RoundFloatToI32(Turns * 256.f) & 255);
}

internal bool32
SimGameIsSent(world_entity *Entity)
{
    switch (Entity->Type)
    {
        case EntityType_Player:
        case EntityType_Monster:
        case EntityType_FireBall:
        case EntityType_Sword:
        case EntityType_Familiar:
        case EntityType_MonsterShot:
        case EntityType_MonsterHazard:
            return Entity->IsPresent;
        default:
            return false;
    }
}

// Monsters winding up or striking get an ability entry, so clients can
// draw the warning. Ready and recovering monsters have nothing to show.
internal void
SimGameWriteAbility(world_entity *Entity, u8 EntityIndex, net_snapshot *Out)
{
    if (Entity->Type != EntityType_Monster) return;
    if (Entity->AbilityPhase != AbilityPhase_Windup &&
        Entity->AbilityPhase != AbilityPhase_Active) return;
    if (Out->AbilityCount >= NET_MAX_SNAPSHOT_ABILITIES) return;

    net_ability_state *A = &Out->Abilities[Out->AbilityCount++];
    *A = {};
    A->EntityIndex = EntityIndex;
    A->Phase = (u8)Entity->AbilityPhase;
    A->Ability = (u8)Entity->AbilityIndex;
    A->TimeLeft = Entity->AbilityTimer;
    A->AimX = Entity->AbilityAim.X;
    A->AimY = Entity->AbilityAim.Y;
    A->PointCount = (u8)Minimum(Entity->AbilityPointCount, (u32)NET_MAX_ABILITY_POINTS);
    for (u32 Index = 0; Index < A->PointCount; ++Index)
    {
        A->PointX[Index] = Entity->AbilityPoints[Index].X;
        A->PointY[Index] = Entity->AbilityPoints[Index].Y;
    }
}

// NOTE(zoubir): one entity's record, plus its ability windup and facing
// entries, which point back at it by its index in the snapshot
internal void
SimGameWriteEntity(world_entity *Entity, u16 Id, net_snapshot *Out)
{
    net_entity_state *E = &Out->Entities[Out->Count++];
    E->Id = Id;
    E->Type = (u8)Entity->Type;
    E->Facing = (u8)Entity->AnimationState.LastAnimationDirection;
    E->Animation = (u8)Entity->AnimationState.CurrentType;
    E->Variant = SimGameVariant(Entity);
    E->Affix = (u8)Entity->EliteAffix;
    E->Status = SimGameStatusBits(Entity);
    E->Ability = (u8)Entity->AbilityIndex;
    E->Flash = (Entity->Type == EntityType_Monster && Entity->PhaseFlash > 0.f) ? 1 : 0;
    if (Entity->Type == EntityType_Player)
    {
        E->Ability = (u8)((Entity->ShockwaveFlash > 0.f ? PLAYER_FLASH_SHOCKWAVE : 0) |
                          (Entity->DashFlash > 0.f ? PLAYER_FLASH_DASH : 0));
    }
    E->Health = (i16)Entity->Hp;
    E->X = Entity->Position.X;
    E->Y = Entity->Position.Y;
    E->Z = Entity->Position.Z;
    E->VelX = Entity->Velocity.X;
    E->VelY = Entity->Velocity.Y;

    SimGameWriteAbility(Entity, (u8)(Out->Count - 1), Out);
    SimGameWriteFacing(Entity, (u8)(Out->Count - 1), Out);
}

struct sim_game_candidate
{
    u32 Index;
    float DistanceSq;
};

// NOTE(zoubir): keeps the Room nearest candidates in List, nearest first
// (insertion, so ties keep entity order)
internal void
SimGameKeepNearest(sim_game_candidate *List, u32 *Count, u32 Room,
                   sim_game_candidate Candidate)
{
    if (Room == 0) return;
    if (*Count == Room && Candidate.DistanceSq >= List[*Count - 1].DistanceSq) return;
    u32 At = (*Count < Room) ? (*Count)++ : *Count - 1;
    while (At > 0 && List[At - 1].DistanceSq > Candidate.DistanceSq)
    {
        List[At] = List[At - 1];
        --At;
    }
    List[At] = Candidate;
}
