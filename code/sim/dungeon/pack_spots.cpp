/* Pack spots (encounters.cpp): where a room's packs stand when its fight
   starts, and where monsters joining a fight (a boss's adds) climb out.
   Packs stand apart and out of the party's reach where the room has the
   space, so a party pulls them one at a time. */

// NOTE(zoubir): where a room's packs stand at the start of its fight: past
// the farthest a monster notices a player (420, imps and spiders) from
// the party and the room's entrance, and apart from each other, so the
// party pulls them one at a time instead of the whole room at once. A
// room too small for that gets the next row down, the last being the
// old rule
#define DUNGEON_PACK_RULES 3
global_variable float DungeonPackFromParty[DUNGEON_PACK_RULES] = {460.f, 380.f, DUNGEON_PACK_DISTANCE};
global_variable float DungeonPackApart[DUNGEON_PACK_RULES] = {300.f, 200.f, 0.f};

// NOTE(zoubir): a spot in Room for a pack: open ground at least FromParty
// from the party and the room's entrance, and at least Apart from each of
// Taken; 0 in *Found when none turns up
internal v3
TryPackSpot(world *World, dungeon_run *Run, u32 Room, float FromParty, float Apart,
            v3 *Taken, u32 TakenCount, bool32 *Found)
{
    map_def *Map = GetMapDef((map_id)World->MapId);
    float Tile = (float)World->TileWidth;
    *Found = false;
    for(u32 Try = 0; Try < DUNGEON_SPOT_TRIES; Try++)
    {
        i32 X = (i32)RandomChoice(&Run->Series, Map->Width);
        i32 Y = (i32)RandomChoice(&Run->Series, Map->Height);
        v3 Spot = V3(((float)X + 0.5f) * Tile, ((float)Y + 0.5f) * Tile, 0.f);
        if (RoomAtTile(World->MapId, X, Y) != Room || !IsOpenTile(Map, X, Y) ||
            !IsFarFromPlayers(World, Spot.XY, FromParty) ||
            Length(Spot.XY - Run->RoomEntry[Room].XY) < FromParty)
        {
            continue;
        }
        bool32 Clear = true;
        for(u32 Index = 0; Index < TakenCount && Clear; Index++)
        {
            Clear = Length(Spot.XY - Taken[Index].XY) >= Apart;
        }
        if (Clear)
        {
            *Found = true;
            return Spot;
        }
    }
    return Run->RoomMiddle[Room];
}

// NOTE(zoubir): a spot for a room's next pack, by the first of the
// DungeonPack rules the room has space for; the room's middle when none
internal v3
PickPackSpotApart(world *World, dungeon_run *Run, u32 Room, v3 *Taken, u32 TakenCount)
{
    v3 Result = Run->RoomMiddle[Room];
    for(u32 Rule = 0; Rule < DUNGEON_PACK_RULES; Rule++)
    {
        bool32 Found;
        v3 Spot = TryPackSpot(World, Run, Room, DungeonPackFromParty[Rule],
                              DungeonPackApart[Rule], Taken, TakenCount, &Found);
        if (Found)
        {
            Result = Spot;
            break;
        }
    }
    return Result;
}

// NOTE(zoubir): a spot in Room for monsters joining a fight (a boss's
// adds): open ground away from the party; the room's middle when none
internal v3
PickPackSpot(world *World, dungeon_run *Run, u32 Room)
{
    bool32 Found;
    v3 Result = TryPackSpot(World, Run, Room, DUNGEON_PACK_DISTANCE, 0.f, 0, 0, &Found);
    return Result;
}
