/* Allies (role_abilities.cpp, before the kits): who a role spell that
   lands on a player goes to, and HealPlayer, the one way a role heals.
   The rules for picking are at the top of role_abilities.cpp. */

// NOTE(zoubir): the living player the client picked (player_input.Target:
// under the cursor or on the party frames), Self included when AllowSelf;
// else 0
inline world_entity *
CursorAlly(app_state *AppState, player_slot *Slot, world_entity *Self, bool32 AllowSelf)
{
    world *World = &AppState->World;
    u32 Index = Slot->Input.Target;
    world_entity *Result = 0;
    if (Index && Index - 1 < World->EntityCount)
    {
        world_entity *Unit = &World->Entities[Index - 1];
        if ((Unit != Self || AllowSelf) && Unit->IsPresent &&
            Unit->Type == EntityType_Player && !IsDeadPlayer(Unit))
        {
            Result = Unit;
        }
    }
    return Result;
}

// NOTE(zoubir): the living player within Range of From missing the most
// health, Skip left out (Self counts), or 0 when nobody is hurt
internal world_entity *
MostHurtAlly(app_state *AppState, v2 From, float Range, world_entity *Skip = 0)
{
    world_entity *Result = 0;
    float Worst = 0.f;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (Player && Player != Skip && Length(Player->Position.XY - From) <= Range)
        {
            float Missing = Player->MaxHp - Player->Hp;
            if (Missing > Worst)
            {
                Worst = Missing;
                Result = Player;
            }
        }
    }
    return Result;
}

// NOTE(zoubir): the living player within Range of Self nearest to it,
// Self left out, or 0
internal world_entity *
NearestOtherAlly(app_state *AppState, world_entity *Self, float Range)
{
    world_entity *Result = 0;
    float Best = Range;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        float Distance = Player ? Length(Player->Position.XY - Self->Position.XY) : 0.f;
        if (Player && Player != Self && Distance <= Best)
        {
            Best = Distance;
            Result = Player;
        }
    }
    return Result;
}

// NOTE(zoubir): who a healer's ally spell lands on (the rules at the top)
internal world_entity *
PickAllyFor(app_state *AppState, player_slot *Slot, world_entity *Player, float Range)
{
    world_entity *Result = CursorAlly(AppState, Slot, Player, true);
    if (!Result || Length(Result->Position.XY - Player->Position.XY) > Range)
    {
        Result = MostHurtAlly(AppState, Player->Position.XY, Range);
    }
    if (!Result)
    {
        Result = NearestOtherAlly(AppState, Player, Range);
    }
    if (!Result)
    {
        Result = Player;
    }
    return Result;
}

// NOTE(zoubir): Amount of health to Target from the healer in slot By;
// returns what it gave. The monsters fighting the party notice
internal float
HealPlayer(app_state *AppState, u32 By, world_entity *Target, float Amount)
{
    if (!Target || IsDeadPlayer(Target))
    {
        return 0.f;
    }
    float Given = Minimum(Amount * PartySustainScale(AppState->Dungeon),
                          Target->MaxHp - Target->Hp);
    Target->Hp += Given;
    CountMeterHealing(AppState, By, Given);
    dungeon_run *Run = AppState->Dungeon;
    if (Run && Given > 0.f && By < MAX_PLAYERS)
    {
        for(u32 Index = 0; Index < THREAT_ROWS; Index++)
        {
            threat_row *Row = &Run->Threat.Rows[Index];
            if (Row->Serial && FindMonsterBySerial(&AppState->World, Row->Slot, Row->Serial))
            {
                Row->Threat[By] += HEAL_THREAT_SHARE * Given;
            }
        }
    }
    return Given;
}
