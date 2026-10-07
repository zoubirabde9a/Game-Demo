/* Threat (dungeon.cpp): in a dungeon run a monster attacks the player
   with the most threat on it, not the nearest one, so a tank can hold
   it. Damage on a monster makes threat for the player behind it, times
   the role's ThreatScale (roles.cpp: the tank's is four times the
   others'). A taunt puts the taunter on top for TAUNT_SECONDS whatever
   the numbers say. A monster nobody has hurt yet goes for the nearest
   player, as outside the dungeon.

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

internal void
UpdateThreat(threat_table *Table, float DeltaTime)
{
    for(u32 Index = 0; Index < THREAT_ROWS; Index++)
    {
        threat_row *Row = &Table->Rows[Index];
        Row->TauntSeconds = Maximum(0.f, Row->TauntSeconds - DeltaTime);
        if (Row->TauntSeconds <= 0.f)
        {
            Row->TauntedBy = 0;
        }
    }
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

// NOTE(zoubir): the player Monster should attack by threat: its taunter,
// else the living player with the most threat on it. 0 when it has none,
// and the caller falls back to the nearest player
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
    float Best = 0.f;
    for(u32 PlayerSlot = 0; PlayerSlot < MAX_PLAYERS; PlayerSlot++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, PlayerSlot);
        if (Player && Row->Threat[PlayerSlot] > Best)
        {
            Best = Row->Threat[PlayerSlot];
            Result = Player;
        }
    }
    return Result;
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
