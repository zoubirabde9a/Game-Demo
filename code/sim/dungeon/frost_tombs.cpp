/* Frost tombs (boss_scripts.cpp): Vaelith the Everwinter Queen
   (sim/monsters/rift_everwinter.cpp) freezes the party one player at a
   time, and the rest of it breaks them out (docs/dungeon-rift.md).

   While two or more of the party stand in her room, TOMB_FIRST_SECONDS
   into the fight and then every TOMB_EVERY_SECONDS, she marks one of
   them who is not the player she is after: a Frost Mark
   (sim/monsters/rift_ice_tomb.cpp) hangs over their head and follows
   them, its ring closing through its Frostbind's windup. When it
   closes, every player inside it freezes, so the marked player runs from the
   party and the party from them.

   A frozen player stands in an Ice Tomb: stunned, held where they froze
   and out of reach of every other blow. Breaking the tomb frees them.
   Left standing through its Shatter's windup (the cast bar over the
   block), the tomb shatters: the player inside loses
   TOMB_SHATTER_SHARE of their health and Vaelith heals TOMB_HEAL_SHARE
   of hers. When she dies, or the fight ends, the marks and tombs go and
   everyone inside walks free.

   Marks and tombs are monsters, so their rings, cast bars, health and the
   frozen player's shield go to every client in the snapshot as they are,
   and nothing new goes on the wire. */

#define TOMB_FIRST_SECONDS 20.f
#define TOMB_EVERY_SECONDS 30.f
// NOTE(zoubir): the mark's and the tomb's slams end this close to their
// windup's end, before the slam would land (UpdateBossEvents runs before
// the monsters move, a tick before they would trigger)
#define TOMB_SLACK_SECONDS 0.034f
#define TOMB_SHATTER_SHARE 0.4f
#define TOMB_HEAL_SHARE 0.04f
// NOTE(zoubir): a moment out of reach for a player just freed, and how
// long each tick's hold lasts, so a player the script lets go is free at
// once even if it never runs again
#define TOMB_FREED_SHIELD_SECONDS 0.5f
#define TOMB_HOLD_SECONDS 0.25f

// NOTE(zoubir): Player stops being held and may move again
internal void
ThawPlayer(app_state *AppState, world_entity *Player)
{
    if (Player && !IsDeadPlayer(Player))
    {
        Player->StatusTimers[StatusEffect_Stunned] = 0.f;
        Player->SpawnShield = Maximum(Player->SpawnShield, TOMB_FREED_SHIELD_SECONDS);
        EmitBurst(&AppState->Events, SimBurst_Spawn, (u8)Player->PlayerIndex, Player->Position);
    }
}

// NOTE(zoubir): Entity moved to Position, telling the chunk lists
internal void
PlaceFrostEntity(app_state *AppState, world *World, memory_arena *Arena,
                 world_entity *Entity, v3 Position)
{
    v3 OldPosition = Entity->Position;
    Entity->Position = Position;
    Entity->Velocity = {};
    CheckAndChangeEntityChunk(AppState, World, Arena, OldPosition, Entity);
}

// NOTE(zoubir): the mark and every tomb go, and everyone inside is freed
internal void
ReleaseFrostTombs(app_state *AppState, world *World, frost_tombs *Tombs)
{
    world_entity *Mark = FindMonsterBySerial(World, Tombs->MarkSlot, Tombs->MarkSerial);
    if (Mark)
    {
        RemoveEntity(World, Mark);
    }
    Tombs->MarkSerial = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Tomb = FindMonsterBySerial(World, Tombs->TombSlots[SlotIndex],
                                                 Tombs->TombSerials[SlotIndex]);
        if (Tombs->TombSerials[SlotIndex])
        {
            ThawPlayer(AppState, LivingPlayerInSlot(AppState, SlotIndex));
        }
        if (Tomb)
        {
            RemoveEntity(World, Tomb);
        }
        Tombs->TombSerials[SlotIndex] = 0;
    }
}

// NOTE(zoubir): a monster of Kind at Position with its slam ready to
// start the tick it is made, and its slot and serial for the run
internal world_entity *
SpawnFrostMonster(app_state *AppState, world *World, memory_arena *Arena,
                  v3 Position, monster_kind Kind)
{
    world_entity *Result = SpawnMonster(AppState, World, Arena, Position, Kind);
    Result->AbilityCooldowns[0] = 0.f;
    return Result;
}

// NOTE(zoubir): the player in SlotIndex freezes in a new tomb
internal void
EntombPlayer(app_state *AppState, world *World, memory_arena *Arena,
             dungeon_run *Run, u32 SlotIndex, float HealthScale)
{
    frost_tombs *Tombs = &Run->FrostTombs;
    world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
    if (!Player || Tombs->TombSerials[SlotIndex])
    {
        return;
    }
    // NOTE(zoubir): a hair in front of the player, so the block's pane
    // draws over them
    v3 Spot = Player->Position;
    Spot.Y += 1.f;
    world_entity *Tomb = SpawnFrostMonster(AppState, World, Arena, Spot, MonsterKind_IceTomb);
    Tomb->MaxHp *= HealthScale;
    Tomb->Hp = Tomb->MaxHp;
    AddCollisionRule(AppState, Arena, Tomb->ID, Player->ID, false);
    Tombs->TombSlots[SlotIndex] = (u32)(Tomb - World->Entities);
    Tombs->TombSerials[SlotIndex] = Tomb->MonsterSerial;
    Player->Velocity = {};
    ApplyStatus(Player, StatusEffect_Stunned, TOMB_HOLD_SECONDS);
    EmitBurst(&AppState->Events, SimBurst_Spawn, SIM_NOBODY, Tomb->Position);
}

// NOTE(zoubir): the living players in the fight's room, into Out
// (MAX_PLAYERS long) by slot; returns how many
internal u32
FrostCandidates(app_state *AppState, dungeon_run *Run, u32 *Out)
{
    u32 Count = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (Player &&
            RoomAtPosition(&AppState->World, Player->Position.XY) == Run->FightingRoom)
        {
            Out[Count++] = SlotIndex;
        }
    }
    return Count;
}

// NOTE(zoubir): a mark on one of the party Vaelith is not after; false
// when there is nobody to mark
internal bool32
StartFrostMark(app_state *AppState, world *World, memory_arena *Arena,
               dungeon_run *Run, world_entity *Boss)
{
    frost_tombs *Tombs = &Run->FrostTombs;
    u32 Slots[MAX_PLAYERS];
    u32 Count = FrostCandidates(AppState, Run, Slots);
    if (Count < 2)
    {
        return false;
    }
    world_entity *Held = PickThreatTarget(AppState, &Run->Threat, Boss);
    u32 Victims[MAX_PLAYERS];
    u32 VictimCount = 0;
    for(u32 Index = 0; Index < Count; Index++)
    {
        if (AppState->Players[Slots[Index]].Entity != Held)
        {
            Victims[VictimCount++] = Slots[Index];
        }
    }
    u32 Victim = Victims[RandomChoice(&Run->Series, VictimCount)];
    world_entity *Player = AppState->Players[Victim].Entity;
    v3 Spot = Player->Position;
    Spot.Z += FROST_MARK_HEIGHT;
    world_entity *Mark = SpawnFrostMonster(AppState, World, Arena, Spot, MonsterKind_FrostMark);
    Tombs->MarkSlot = (u32)(Mark - World->Entities);
    Tombs->MarkSerial = Mark->MonsterSerial;
    Tombs->MarkVictim = Victim + 1;
    return true;
}

// NOTE(zoubir): the mark follows its player; when its ring closes, every
// living player in it freezes
internal void
UpdateFrostMark(app_state *AppState, world *World, memory_arena *Arena,
                dungeon_run *Run, float HealthScale)
{
    frost_tombs *Tombs = &Run->FrostTombs;
    world_entity *Mark = FindMonsterBySerial(World, Tombs->MarkSlot, Tombs->MarkSerial);
    world_entity *Victim = Tombs->MarkVictim ?
        LivingPlayerInSlot(AppState, Tombs->MarkVictim - 1) : 0;
    if (!Mark || !Victim)
    {
        if (Mark)
        {
            RemoveEntity(World, Mark);
        }
        Tombs->MarkSerial = 0;
        return;
    }
    v3 Over = Victim->Position;
    Over.Z = Mark->Position.Z;
    PlaceFrostEntity(AppState, World, Arena, Mark, Over);
    if (Mark->AbilityPhase != AbilityPhase_Windup ||
        Mark->AbilityTimer > TOMB_SLACK_SECONDS)
    {
        return;
    }
    float Radius = GetMonsterDef(MonsterKind_FrostMark)->Abilities[0].Radius;
    u32 Slots[MAX_PLAYERS];
    u32 Count = FrostCandidates(AppState, Run, Slots);
    for(u32 Index = 0; Index < Count; Index++)
    {
        world_entity *Player = AppState->Players[Slots[Index]].Entity;
        if (Length(Player->Position.XY - Mark->Position.XY) <= Radius)
        {
            EntombPlayer(AppState, World, Arena, Run, Slots[Index], HealthScale);
        }
    }
    RemoveEntity(World, Mark);
    Tombs->MarkSerial = 0;
}

// NOTE(zoubir): each tomb holds its player until it breaks, or shatters
// on them when its countdown ends
internal void
UpdateIceTombs(app_state *AppState, world *World, memory_arena *Arena,
               dungeon_run *Run, world_entity *Boss)
{
    frost_tombs *Tombs = &Run->FrostTombs;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (!Tombs->TombSerials[SlotIndex])
        {
            continue;
        }
        world_entity *Tomb = FindMonsterBySerial(World, Tombs->TombSlots[SlotIndex],
                                                 Tombs->TombSerials[SlotIndex]);
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (!Tomb || Tomb->Hp <= 0.f || !Player)
        {
            // NOTE(zoubir): broken: the player inside walks out
            if (Tomb && Tomb->Hp > 0.f)
            {
                RemoveEntity(World, Tomb);
            }
            ThawPlayer(AppState, Player);
            Tombs->TombSerials[SlotIndex] = 0;
            continue;
        }
        if (Tomb->AbilityPhase == AbilityPhase_Windup &&
            Tomb->AbilityTimer <= TOMB_SLACK_SECONDS)
        {
            // NOTE(zoubir): past the shield the tomb kept up, as only the
            // ice was holding off the blow; set straight on the health,
            // the way the countdown promised, and through DamageEntity only
            // when it kills, so the death counts
            EmitBurst(&AppState->Events, SimBurst_Impact, SIM_NOBODY, ChestOf(Player));
            Player->SpawnShield = 0.f;
            float Loss = TOMB_SHATTER_SHARE * Player->MaxHp;
            if (Player->Hp > Loss)
            {
                Player->Hp -= Loss;
            }
            else
            {
                DamageEntity(AppState, World, Player, Player->MaxHp, Tomb);
            }
            Boss->Hp = Minimum(Boss->MaxHp, Boss->Hp + TOMB_HEAL_SHARE * Boss->MaxHp);
            EmitBurst(&AppState->Events, SimBurst_Spawn, SIM_NOBODY, Boss->Position);
            RemoveEntity(World, Tomb);
            Player->StatusTimers[StatusEffect_Stunned] = 0.f;
            Tombs->TombSerials[SlotIndex] = 0;
            continue;
        }
        v3 Inside = Tomb->Position;
        Inside.Y -= 1.f;
        PlaceFrostEntity(AppState, World, Arena, Player, Inside);
        ApplyStatus(Player, StatusEffect_Stunned, TOMB_HOLD_SECONDS);
        Player->SpawnShield = Maximum(Player->SpawnShield, TOMB_HOLD_SECONDS);
    }
}

// NOTE(zoubir): once a tick while a fight lasts, from UpdateBossEvents;
// Boss is the fight's boss, 0 for none
internal void
UpdateFrostTombs(app_state *AppState, world *World, memory_arena *Arena,
                 dungeon_run *Run, world_entity *Boss)
{
    frost_tombs *Tombs = &Run->FrostTombs;
    bool32 Fighting = Boss && Boss->Hp > 0.f && Boss->MonsterKind == MonsterKind_Everwinter;
    if (!Fighting || Tombs->BossSerial != Boss->MonsterSerial)
    {
        ReleaseFrostTombs(AppState, World, Tombs);
        *Tombs = {};
        if (Fighting)
        {
            Tombs->BossSerial = Boss->MonsterSerial;
            Tombs->NextSeconds = Run->Seconds + TOMB_FIRST_SECONDS;
        }
        return;
    }
    u32 Standing;
    float HealthScale = LevelFoeHealth(World->MapId) *
        PartyHealthScale(CountPartyPlayers(AppState, &Standing));
    UpdateIceTombs(AppState, World, Arena, Run, Boss);
    if (Tombs->MarkSerial)
    {
        UpdateFrostMark(AppState, World, Arena, Run, HealthScale);
        return;
    }
    bool32 AnyTomb = false;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        AnyTomb |= Tombs->TombSerials[SlotIndex] != 0;
    }
    if (!AnyTomb && Run->Seconds >= Tombs->NextSeconds &&
        StartFrostMark(AppState, World, Arena, Run, Boss))
    {
        Tombs->NextSeconds = Run->Seconds + TOMB_EVERY_SECONDS;
    }
}
