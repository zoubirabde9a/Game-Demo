/* Frost tombs (frost_tombs.cpp), the part dungeon.cpp needs before the
   rest: the state a dungeon_run keeps for Vaelith's ice tombs, and the
   test DungeonScaleDamage and the spells' foe pick use to leave her
   Frost Mark alone. */

struct frost_tombs
{
    // NOTE(zoubir): the fight these belong to, as the boss's MonsterSerial;
    // a new one (a new fight, or the same after a wipe) starts them again
    u32 BossSerial;
    // NOTE(zoubir): Run->Seconds when the next mark may come
    float NextSeconds;
    // NOTE(zoubir): the mark following a player, as slot and MonsterSerial
    // (0 serial for none), and that player's slot + 1
    u32 MarkSlot;
    u32 MarkSerial;
    u32 MarkVictim;
    // NOTE(zoubir): by player slot, the tomb holding that player, as slot
    // and MonsterSerial (0 serial for none)
    u32 TombSlots[MAX_PLAYERS];
    u32 TombSerials[MAX_PLAYERS];
};

// NOTE(zoubir): Vaelith's Frost Mark: nothing hurts it and no spell picks
// it as a target
inline bool32
IsFrostMark(world_entity *Entity)
{
    bool32 Result = Entity->Type == EntityType_Monster &&
        Entity->MonsterKind == MonsterKind_FrostMark;
    return Result;
}
