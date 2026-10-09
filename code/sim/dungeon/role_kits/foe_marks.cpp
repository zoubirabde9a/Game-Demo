/* Foe marks (foe_marks.h), included by role_abilities.cpp before the
   kits: the table of what role spells leave on monsters. The striker's
   Searing stacks (striker.cpp) burn, and run out SEARING_SECONDS after
   the last (UpdateSearing explodes them then); the
   tank's Sunder (tank.cpp) makes the monster take SUNDER_SHARE more from
   everyone for SUNDER_SECONDS. A row is used while either holds, and is
   dropped with its monster. */

inline bool32
FoeMarkUsed(foe_mark *Mark)
{
    bool32 Result = Mark->Stacks > 0 || Mark->SunderSeconds > 0.f || Mark->WeakenSeconds > 0.f;
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
            if (Maximum(Maximum(Row->Seconds, Row->SunderSeconds), Row->WeakenSeconds) <
                Maximum(Maximum(Mark->Seconds, Mark->SunderSeconds), Mark->WeakenSeconds))
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

// NOTE(zoubir): Stacks more Searing on Monster from the striker in slot
// By, its clock starting again
internal void
AddSearing(dungeon_run *Run, world *World, world_entity *Monster, u32 Stacks = 1, u32 By = 0)
{
    foe_mark *Mark = GetFoeMark(Run, World, Monster);
    Mark->Stacks = Minimum(Mark->Stacks + Stacks, (u32)SEARING_MOST);
    Mark->Seconds = SEARING_SECONDS;
    Mark->SearBy = By;
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
// NOTE(zoubir): Monster weakened for Seconds: it deals Share less
// (FoeWeakenScale), the stronger of what it had and this
internal void
WeakenFoe(dungeon_run *Run, world *World, world_entity *Monster, float Seconds, float Share)
{
    foe_mark *Mark = GetFoeMark(Run, World, Monster);
    Mark->WeakenSeconds = Maximum(Mark->WeakenSeconds, Seconds);
    Mark->WeakenShare = Maximum(Mark->WeakenShare, Share);
}

// NOTE(zoubir): the share of its hit a weakened monster Source still
// deals, from DungeonScaleDamage; 1 for anything else
internal float
FoeWeakenScale(dungeon_run *Run, world *World, world_entity *Source)
{
    foe_mark *Mark = (Source && Source->Type == EntityType_Monster) ? FindFoeMark(Run, World, Source) : 0;
    float Result = (Mark && Mark->WeakenSeconds > 0.f) ? 1.f - Mark->WeakenShare : 1.f;
    return Result;
}

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
        Mark->WeakenSeconds = Maximum(0.f, Mark->WeakenSeconds - DeltaTime);
        if (Mark->WeakenSeconds <= 0.f)
        {
            Mark->WeakenShare = 0.f;
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
