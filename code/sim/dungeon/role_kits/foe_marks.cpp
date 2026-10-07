/* Foe marks (foe_marks.h), included by role_abilities.cpp before the
   kits: the table of what role spells leave on monsters. The striker's
   Searing stacks (striker.cpp) fade SEARING_SECONDS after the last; the
   tank's Sunder (tank.cpp) makes the monster take SUNDER_SHARE more from
   everyone for SUNDER_SECONDS. A row is used while either holds, and is
   dropped with its monster. */

inline bool32
FoeMarkUsed(foe_mark *Mark)
{
    bool32 Result = Mark->Stacks > 0 || Mark->SunderSeconds > 0.f;
    return Result;
}

// NOTE(zoubir): Monster's row in the table, 0 for none
internal foe_mark *
FindFoeMark(dungeon_run *Run, world *World, world_entity *Monster)
{
    u32 Slot = (u32)(Monster - World->Entities);
    for(u32 Index = 0; Index < MAX_FOE_MARKS; Index++)
    {
        foe_mark *Mark = &Run->Marks[Index];
        if (FoeMarkUsed(Mark) && Mark->Slot == Slot && Mark->Serial == Monster->MonsterSerial)
        {
            return Mark;
        }
    }
    return 0;
}

// NOTE(zoubir): Monster's row, made when it has none; a full table gives
// up the row whose marks run out first
internal foe_mark *
GetFoeMark(dungeon_run *Run, world *World, world_entity *Monster)
{
    foe_mark *Mark = FindFoeMark(Run, World, Monster);
    if (!Mark)
    {
        Mark = &Run->Marks[0];
        for(u32 Index = 0; Index < MAX_FOE_MARKS; Index++)
        {
            foe_mark *Row = &Run->Marks[Index];
            if (!FoeMarkUsed(Row))
            {
                Mark = Row;
                break;
            }
            if (Maximum(Row->Seconds, Row->SunderSeconds) <
                Maximum(Mark->Seconds, Mark->SunderSeconds))
            {
                Mark = Row;
            }
        }
        *Mark = {};
        Mark->Slot = (u32)(Monster - World->Entities);
        Mark->Serial = Monster->MonsterSerial;
    }
    return Mark;
}

// NOTE(zoubir): one more Searing stack on Monster, its fade starting again
internal void
AddSearing(dungeon_run *Run, world *World, world_entity *Monster)
{
    foe_mark *Mark = GetFoeMark(Run, World, Monster);
    Mark->Stacks = Minimum(Mark->Stacks + 1, (u32)SEARING_MOST);
    Mark->Seconds = SEARING_SECONDS;
}

// NOTE(zoubir): Monster takes Share more for Seconds, never cut shorter
// or weaker by a lesser sunder
internal void
AddSunder(dungeon_run *Run, world *World, world_entity *Monster, float Seconds, float Share)
{
    foe_mark *Mark = GetFoeMark(Run, World, Monster);
    Mark->SunderSeconds = Maximum(Mark->SunderSeconds, Seconds);
    Mark->SunderShare = Maximum(Mark->SunderShare, Share);
}

// NOTE(zoubir): from DungeonScaleDamage: what a player's hit on Monster
// is multiplied by for the marks on it
internal float
FoeMarkDamageScale(dungeon_run *Run, world *World, world_entity *Monster)
{
    foe_mark *Mark = FindFoeMark(Run, World, Monster);
    float Result = (Mark && Mark->SunderSeconds > 0.f) ? 1.f + Mark->SunderShare : 1.f;
    return Result;
}

// NOTE(zoubir): once a tick: marks fade, and go with their monster
internal void
UpdateFoeMarks(dungeon_run *Run, world *World, float DeltaTime)
{
    for(u32 Index = 0; Index < MAX_FOE_MARKS; Index++)
    {
        foe_mark *Mark = &Run->Marks[Index];
        if (!FoeMarkUsed(Mark))
        {
            continue;
        }
        Mark->Seconds -= DeltaTime;
        Mark->SunderSeconds = Maximum(0.f, Mark->SunderSeconds - DeltaTime);
        if (Mark->SunderSeconds <= 0.f)
        {
            Mark->SunderShare = 0.f;
        }
        if (Mark->Seconds <= 0.f)
        {
            Mark->Stacks = 0;
            Mark->Seconds = 0.f;
        }
        world_entity *Monster = FindMonsterBySerial(World, Mark->Slot, Mark->Serial);
        if (!Monster || Monster->Hp <= 0.f)
        {
            *Mark = {};
        }
    }
}
