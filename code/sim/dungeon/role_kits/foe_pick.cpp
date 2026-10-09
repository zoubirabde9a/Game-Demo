/* Foe pick (role_abilities.cpp, before the kits): which monster an attack
   spell goes for, Shield Throw (role_kits/tank.cpp) and Holy Fire
   (role_kits/healer.cpp), and where Shield Throw bounces next. A spell
   only reaches monsters in the caster's room, so it never wakes the
   room behind a gate. */

// NOTE(zoubir): the living monster nearest Point within Range of it, in
// Room, none of the Skip ones nor a Frost Mark (frost_tombs.h); 0 for none
internal world_entity *
NearestFoe(world *World, v2 Point, float Range, u32 Room, world_entity **Skip, u32 SkipCount)
{
    world_entity *Result = 0;
    float Best = Range;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            IsFrostMark(Monster) || RoomAtPosition(World, Monster->Position.XY) != Room)
        {
            continue;
        }
        bool32 Skipped = false;
        for(u32 Index = 0; Index < SkipCount; Index++)
        {
            Skipped |= Skip[Index] == Monster;
        }
        float Distance = Length(Monster->Position.XY - Point);
        if (!Skipped && Distance <= Best)
        {
            Best = Distance;
            Result = Monster;
        }
    }
    return Result;
}

// NOTE(zoubir): the monster an attack spell (Shield Throw, Holy Fire)
// goes for, in Player's room within Range of it: the one under the
// cursor, else the one nearest the cursor within ATTACK_PICK_RADIUS of
// it, else the one nearest Player; 0 for none
internal world_entity *
AttackTarget(app_state *AppState, player_slot *Slot, world_entity *Player, float Range)
{
    world *World = &AppState->World;
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    u32 Index = Slot->Input.Target;
    if (Index && Index - 1 < World->EntityCount)
    {
        world_entity *Unit = &World->Entities[Index - 1];
        if (Unit->IsPresent && Unit->Type == EntityType_Monster && Unit->Hp > 0.f &&
            !IsFrostMark(Unit) && RoomAtPosition(World, Unit->Position.XY) == Room &&
            Length(Unit->Position.XY - Player->Position.XY) <= Range)
        {
            return Unit;
        }
    }
    world_entity *Result = NearestFoe(World, AimPoint(Player), ATTACK_PICK_RADIUS, Room, 0, 0);
    if (!Result || Length(Result->Position.XY - Player->Position.XY) > Range)
    {
        Result = NearestFoe(World, Player->Position.XY, Range, Room, 0, 0);
    }
    return Result;
}
