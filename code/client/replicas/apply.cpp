/* Applying a snapshot: one entity's state (position, velocity, health,
   facing, animation, elite affix, status pips, ability, last hit) onto
   its replica, front-armoured monsters' facings, monsters' wind-ups (for
   their warnings), players' casts (for their cast bars), and every
   player's score. Included by replicas.cpp,
   whose SyncReplicas calls these on each new snapshot. */

internal void
ApplyStateToReplica(app_state *AppState, memory_arena *Arena,
                    world_entity *Replica, net_entity_state *State)
{
    v3 OldPosition = Replica->Position;
    Replica->Position = V3(State->X, State->Y, State->Z);
    Replica->Velocity = V3(State->VelX, State->VelY, State->VelZ);
    // NOTE(zoubir): an elite monster has more health and its own tint; the
    // simulation sets both in ApplyEliteAffix, so the replica does too
    // when it first shows the affix (it was made plain). Health comes
    // from the snapshot right after.
    if (Replica->Type == EntityType_Monster &&
        Replica->EliteAffix == MonsterAffix_None &&
        State->Affix != MonsterAffix_None && State->Affix < MonsterAffix_Count)
    {
        ApplyEliteAffix(Replica, State->Affix);
    }
    Replica->Hp = (float)State->Health;
    if (State->Facing < AnimationDirection_Count)
    {
        Replica->AnimationDirection = (animation_direction)State->Facing;
    }
    if (State->Animation < AnimationType_Count)
    {
        Replica->AnimationType = (animation_type)State->Animation;
    }
    Replica->EliteAffix = State->Affix;
    Replica->AbilityIndex = State->Ability;
    // NOTE(zoubir): status pips blink under 1 s left; the client does not
    // know the real time left, so active effects read as 1.5 s
    for(u32 Effect = 1; Effect < StatusEffect_Count; Effect++)
    {
        Replica->StatusTimers[Effect] =
            (State->Status & (1 << (Effect - 1))) ? 1.5f : 0.f;
    }
    // NOTE(zoubir): the last hit while it is fresh (sim/hit.cpp); the
    // real time left is not sent, so fresh reads as HIT_FRESH_SECONDS.
    // The hit-pause runs down between snapshots (advance.cpp)
    Replica->HitFresh = State->Hit ? HIT_FRESH_SECONDS : 0.f;
    Replica->HitStop = 0.001f * (float)State->HitStop;
    Replica->HitAngle = (float)State->HitAngle * (2.f * Pi32 / 256.f);
    Replica->HitBySlot = State->HitBy;
    Replica->HitThrown = State->HitThrown;
    // NOTE(zoubir): keeps the chunk lists right so RemoveEntity finds it
    CheckAndChangeEntityChunk(AppState, &AppState->World, Arena,
                              OldPosition, Replica);
}

// NOTE(zoubir): the replica of the snapshot's entity at EntityIndex, 0
// when it has none
inline world_entity *
SnapshotReplica(world *World, replica_table *Table, net_snapshot *Snapshot,
                u32 EntityIndex)
{
    u16 Id = Snapshot->Entities[EntityIndex].Id;
    world_entity *Result = (Id < MAX_REPLICAS && Table->LocalIndexPlusOne[Id]) ?
        &World->Entities[Table->LocalIndexPlusOne[Id] - 1] : 0;
    return Result;
}

// NOTE(zoubir): front-armoured monsters' facing, a whole turn in 256
// steps, back into the unit Direction their shell is drawn from
internal void
ApplySnapshotFacings(world *World, replica_table *Table, net_snapshot *Snapshot)
{
    for(u32 Index = 0; Index < Snapshot->FacingCount; Index++)
    {
        net_facing *Facing = &Snapshot->Facings[Index];
        world_entity *Replica =
            SnapshotReplica(World, Table, Snapshot, Facing->EntityIndex);
        if (Replica)
        {
            float Angle = (float)Facing->Angle * (2.f * Pi32 / 256.f);
            Replica->Direction = V2(Cos(Angle), Sin(Angle));
        }
    }
}

// NOTE(zoubir): player slots mirror the server's: a slot is active while
// the server lists its score, and points at that player's replica. The
// local slot stays active so the camera always has someone to follow.
internal void
ApplySnapshotScores(app_state *AppState, net_snapshot *Snapshot)
{
    bool32 Listed[MAX_PLAYERS] = {};
    for(u32 Index = 0; Index < Snapshot->ScoreCount; Index++)
    {
        net_score *Score = &Snapshot->Scores[Index];
        if (Score->Slot < MAX_PLAYERS)
        {
            player_slot *Slot = &AppState->Players[Score->Slot];
            Listed[Score->Slot] = true;
            Slot->Active = true;
            Slot->Kills = Score->Kills;
            Slot->Deaths = Score->Deaths;
            Slot->MonsterKills = Score->MonsterKills;
            Slot->Level = Score->Level;
            Slot->WardReady = Score->Ward;
        }
    }
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (!Listed[SlotIndex] && SlotIndex != AppState->LocalPlayerIndex)
        {
            AppState->Players[SlotIndex].Active = false;
            AppState->Players[SlotIndex].Entity = 0;
        }
    }
}

// NOTE(zoubir): monsters winding up or using an ability, so their warnings
// (aim lines, target circles, where a burrower comes up) are drawn online
// as offline. Every monster in the snapshot starts as not using one; the
// listed ones get the phase, ability, time left, aim and target points.
// Burrowed is not sent: a burrow ability being used means burrowed.
internal void
ApplySnapshotAbilities(world *World, replica_table *Table, net_snapshot *Snapshot)
{
    for(u32 Index = 0; Index < Snapshot->Count; Index++)
    {
        world_entity *Replica = SnapshotReplica(World, Table, Snapshot, Index);
        if (Replica && Replica->Type == EntityType_Monster)
        {
            Replica->AbilityPhase = AbilityPhase_Ready;
            Replica->AbilityPointCount = 0;
            Replica->Burrowed = false;
        }
    }
    for(u32 Index = 0; Index < Snapshot->AbilityCount; Index++)
    {
        net_ability_state *State = &Snapshot->Abilities[Index];
        world_entity *Replica =
            SnapshotReplica(World, Table, Snapshot, State->EntityIndex);
        if (!Replica)
        {
            continue;
        }
        monster_def *Def = (Replica->Type == EntityType_Monster) ?
            GetMonsterDef(Replica->MonsterKind) : 0;
        if (!Def || State->Ability >= Def->AbilityCount ||
            (State->Phase != AbilityPhase_Windup && State->Phase != AbilityPhase_Active))
        {
            continue;
        }
        Replica->AbilityPhase = (ability_phase)State->Phase;
        Replica->AbilityIndex = State->Ability;
        Replica->AbilityTimer = State->TimeLeft;
        Replica->AbilityAim = V2(State->AimX, State->AimY);
        Replica->AbilityPointCount = Minimum((u32)State->PointCount, (u32)MAX_ABILITY_POINTS);
        for(u32 Point = 0; Point < Replica->AbilityPointCount; Point++)
        {
            Replica->AbilityPoints[Point] = V2(State->PointX[Point], State->PointY[Point]);
        }
        Replica->Burrowed = (State->Phase == AbilityPhase_Active &&
                             Def->Abilities[State->Ability].Kind == MonsterAbility_Burrow);
    }
}

// NOTE(zoubir): players winding up a spell (sim/player_casts.cpp), so
// their cast bars are drawn online as offline. Every player in the
// snapshot starts as not casting; the listed ones get the spell and the
// time left. The local player's own cast is predicted (prediction.cpp),
// which puts its own back over this
internal void
ApplySnapshotCasts(world *World, replica_table *Table, net_snapshot *Snapshot)
{
    for(u32 Index = 0; Index < Snapshot->Count; Index++)
    {
        world_entity *Replica = SnapshotReplica(World, Table, Snapshot, Index);
        if (Replica && Replica->Type == EntityType_Player)
        {
            CancelPlayerCast(Replica);
        }
    }
    for(u32 Index = 0; Index < Snapshot->CastCount; Index++)
    {
        net_player_cast *Cast = &Snapshot->Casts[Index];
        world_entity *Replica =
            SnapshotReplica(World, Table, Snapshot, Cast->EntityIndex);
        if (!Replica || Replica->Type != EntityType_Player ||
            Cast->Spell == PlayerSpell_None || Cast->Spell >= PlayerSpell_Count)
        {
            continue;
        }
        Replica->CastSpell = Cast->Spell;
        Replica->CastLeft = PlayerSpells[Cast->Spell].CastTime *
            (1.f - (float)Cast->Done / 255.f);
    }
}

// NOTE(zoubir): the local player's experience and talents, which only the
// server changes. Before ApplyOwnTimers: the ranks set the cooldowns'
// full lengths, and prediction unlocks and levels abilities from them
internal void
ApplyOwnProgression(player_slot *Own, net_snapshot *Snapshot)
{
    Own->Xp = Snapshot->Xp;
    for(u32 Index = 0; Index < NET_TALENT_COUNT; Index++)
    {
        Own->Ranks[Index] = Snapshot->TalentRanks[Index];
    }
}

// NOTE(zoubir): the local player's own cooldowns, which only the server
// runs, put where the HUD reads them, and its stagger, which only the
// server's hits start; prediction replays both from here
internal void
ApplyOwnTimers(app_state *AppState, world_entity *Local, net_snapshot *Snapshot)
{
    for(u32 Index = 0; Index < PLAYER_COOLDOWN_COUNT; Index++)
    {
        float Full;
        float *Seconds = PlayerCooldown(AppState, Local, Index, &Full);
        if (Seconds) *Seconds = CooldownFromByte(Snapshot->Cooldowns[Index], Full);
    }
    Local->Stagger = CooldownFromByte(Snapshot->Stagger, PlayerStats.StaggerSeconds);
}

// NOTE(zoubir): a player replica's slot on the server is its Variant; the
// slot shows it. The server does not send respawn timers: it starts its
// own at the death tick, so the client starts the same one when it sees
// the death, at most a snapshot late.
internal void
BindPlayerSlot(app_state *AppState, world_entity *Replica,
               net_entity_state *State, bool32 WasDead)
{
    if (State->Type != EntityType_Player || State->Variant >= MAX_PLAYERS)
    {
        return;
    }
    player_slot *Slot = &AppState->Players[State->Variant];
    Slot->Active = true;
    Slot->Entity = Replica;
    Replica->PlayerIndex = State->Variant;
    if (!WasDead && IsDeadPlayer(Replica))
    {
        Slot->RespawnTimer = PLAYER_RESPAWN_SECONDS;
    }
}

// NOTE(zoubir): a boss's enrage burst: start it when the snapshot's bit
// comes on; it then plays out on its own (advance.cpp)
internal void
StartEnrageBurst(replica_table *Table, world_entity *Replica,
                 net_entity_state *State)
{
    if (State->Flash && !Table->Flashing[State->Id] &&
        Replica->Type == EntityType_Monster)
    {
        Replica->PhaseFlash = ENRAGE_FLASH_SECONDS;
    }
    Table->Flashing[State->Id] = State->Flash;
}
