/* Threat (dungeon.cpp): in a dungeon run a monster attacks the player
   with the most threat on it, not the nearest one, so a tank can hold
   it. Damage on a monster makes threat for the player behind it, times
   the role's ThreatScale (roles.cpp: the tank's is four times the
   others'). A taunt puts the taunter on top for TAUNT_SECONDS whatever
   the numbers say. A monster nobody has hurt yet goes for the nearest
   player, as outside the dungeon. Once it has a target it keeps it until
   someone passes their threat by THREAT_PULL_NEAR (THREAT_PULL_FAR from
   out of its reach), and chases it however far (sim/update.cpp). A burn
   or bleed makes threat for whoever left it (DungeonStatusThreat), and a
   player who dies loses all theirs.

   The table is a fixed set of rows in the run, one per monster that has
   threat, keyed by entity slot and MonsterSerial: a row whose monster
   died is simply reused. The monster code asks through DungeonPickTarget
   (FindMonsterTarget, sim/monster_abilities/hits.cpp). The rows live in
   dungeon_run (dungeon.cpp); this file is included by encounters.cpp,
   after the monster population it reads. */

// NOTE(zoubir): the row for Monster; with Make, a fresh one when it has
// none (taking a dead monster's row, or the quietest one when all live)
internal threat_row *
FindThreatRow(threat_table *Table, world *World, world_entity *Monster,
              bool32 Make)
{
    u32 Slot = (u32)(Monster - World->Entities);
    threat_row *Free = 0;
    for(u32 Index = 0; Index < THREAT_ROWS; Index++)
    {
        threat_row *Row = &Table->Rows[Index];
        if (Row->Serial == Monster->MonsterSerial && Row->Slot == Slot)
        {
            return Row;
        }
        if (!Free && (!Row->Serial || !FindMonsterBySerial(World, Row->Slot, Row->Serial)))
        {
            Free = Row;
        }
    }
    if (!Make || !Free)
    {
        return 0;
    }
    ZeroSize(Free, sizeof(*Free));
    Free->Slot = Slot;
    Free->Serial = Monster->MonsterSerial;
    return Free;
}

internal void
AddThreat(threat_table *Table, world *World, world_entity *Monster,
          u32 PlayerSlot, float Amount)
{
    if (!Monster->MonsterSerial || PlayerSlot >= MAX_PLAYERS || Amount <= 0.f)
    {
        return;
    }
    threat_row *Row = FindThreatRow(Table, World, Monster, true);
    if (Row)
    {
        Row->Threat[PlayerSlot] += Amount;
    }
}

// NOTE(zoubir): every monster within Radius of Taunter attacks it for
// TAUNT_SECONDS and keeps the lead after
internal u32
TauntAround(app_state *AppState, threat_table *Table, world_entity *Taunter,
            float Radius)
{
    world *World = &AppState->World;
    u32 Result = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster ||
            Length(Monster->Position.XY - Taunter->Position.XY) > Radius)
        {
            continue;
        }
        threat_row *Row = FindThreatRow(Table, World, Monster, true);
        if (!Row)
        {
            continue;
        }
        float Top = 1.f;
        for(u32 PlayerSlot = 0; PlayerSlot < MAX_PLAYERS; PlayerSlot++)
        {
            Top = Maximum(Top, Row->Threat[PlayerSlot]);
        }
        Row->Threat[Taunter->PlayerIndex] = Top * TAUNT_THREAT_LEAD;
        Row->TauntedBy = Taunter->PlayerIndex + 1;
        Row->TauntSeconds = TAUNT_SECONDS;
        Result++;
    }
    return Result;
}

inline world_entity *
LivingPlayerInSlot(app_state *AppState, u32 PlayerSlot)
{
    world_entity *Result = 0;
    player_slot *Slot = &AppState->Players[PlayerSlot];
    if (Slot->Active && Slot->Entity && Slot->Entity->IsPresent &&
        !IsDeadPlayer(Slot->Entity))
    {
        Result = Slot->Entity;
    }
    return Result;
}

// NOTE(zoubir): a challenger this close to the monster needs only
// THREAT_PULL_NEAR over the one it is after, this many times its bite
#define THREAT_NEAR_REACH 1.5f

// NOTE(zoubir): the player Monster should attack by threat: its taunter,
// else the one it is after (HeldBy) unless another living player has
// passed them by the pull margin, else the one with the most threat.
// 0 when it has none, and the caller falls back to the nearest player
internal world_entity *
PickThreatTarget(app_state *AppState, threat_table *Table, world_entity *Monster)
{
    world *World = &AppState->World;
    threat_row *Row = FindThreatRow(Table, World, Monster, false);
    if (!Row)
    {
        return 0;
    }
    if (Row->TauntedBy)
    {
        world_entity *Taunter = LivingPlayerInSlot(AppState, Row->TauntedBy - 1);
        if (Taunter)
        {
            return Taunter;
        }
    }
    world_entity *Result = 0;
    u32 TopSlot = 0;
    float Best = 0.f;
    for(u32 PlayerSlot = 0; PlayerSlot < MAX_PLAYERS; PlayerSlot++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, PlayerSlot);
        if (Player && Row->Threat[PlayerSlot] > Best)
        {
            Best = Row->Threat[PlayerSlot];
            Result = Player;
            TopSlot = PlayerSlot;
        }
    }
    world_entity *Held = Row->HeldBy ? LivingPlayerInSlot(AppState, Row->HeldBy - 1) : 0;
    if (Result && Held && TopSlot != Row->HeldBy - 1)
    {
        float Reach = THREAT_NEAR_REACH * GetMonsterStats(Monster->MonsterKind)->AttackRange;
        float Pull = Length(Result->Position.XY - Monster->Position.XY) <= Reach ?
            THREAT_PULL_NEAR : THREAT_PULL_FAR;
        if (Best <= Pull * Row->Threat[Row->HeldBy - 1])
        {
            Result = Held;
        }
    }
    return Result;
}

// NOTE(zoubir): once a tick: taunts run down, a dead player's threat is
// wiped everywhere, and each monster's row remembers who it is after now
internal void
UpdateThreat(app_state *AppState, threat_table *Table, float DeltaTime)
{
    world *World = &AppState->World;
    for(u32 Index = 0; Index < THREAT_ROWS; Index++)
    {
        threat_row *Row = &Table->Rows[Index];
        if (!Row->Serial)
        {
            continue;
        }
        Row->TauntSeconds = Maximum(0.f, Row->TauntSeconds - DeltaTime);
        if (Row->TauntSeconds <= 0.f)
        {
            Row->TauntedBy = 0;
        }
        for(u32 PlayerSlot = 0; PlayerSlot < MAX_PLAYERS; PlayerSlot++)
        {
            player_slot *Slot = &AppState->Players[PlayerSlot];
            if (Slot->Entity && IsDeadPlayer(Slot->Entity))
            {
                Row->Threat[PlayerSlot] = 0.f;
            }
        }
        world_entity *Monster = FindMonsterBySerial(World, Row->Slot, Row->Serial);
        world_entity *Target = Monster ? PickThreatTarget(AppState, Table, Monster) : 0;
        Row->HeldBy = Target ? Target->PlayerIndex + 1 : 0;
    }
}

// NOTE(zoubir): from UpdateStatusEffects (sim/status_effects.cpp): a burn
// or bleed the player in PlayerSlot left on Monster took Lost health
internal void
DungeonStatusThreat(app_state *AppState, world_entity *Monster, u32 PlayerSlot,
                    float Lost)
{
    if (!IsDungeon(AppState) || PlayerSlot >= MAX_PLAYERS || !(Lost > 0.f))
    {
        return;
    }
    player_slot *Attacker = &AppState->Players[PlayerSlot];
    AddThreat(&AppState->Dungeon->Threat, &AppState->World, Monster, PlayerSlot,
              Lost * GetRoleDef(Attacker->Role)->ThreatScale * RunThreatScale(Attacker));
}

// NOTE(zoubir): from FindMonsterTarget (sim/monster_abilities/hits.cpp):
// in a dungeon run, the player Monster attacks by threat, with its
// distance; 0 outside a run or when nobody has threat on it
internal world_entity *
DungeonPickTarget(app_state *AppState, world_entity *Monster, float *DistanceOut)
{
    if (!IsDungeon(AppState))
    {
        return 0;
    }
    world_entity *Result = PickThreatTarget(AppState, &AppState->Dungeon->Threat, Monster);
    if (Result && DistanceOut)
    {
        *DistanceOut = Length(Result->Position.XY - Monster->Position.XY);
    }
    return Result;
}

// NOTE(zoubir): from UpdateMonster (sim/update.cpp): true when Monster
// attacks someone by threat, and so chases them however far they are
internal bool32
ChasesByThreat(app_state *AppState, world_entity *Monster)
{
    return DungeonPickTarget(AppState, Monster, 0) != 0;
}
