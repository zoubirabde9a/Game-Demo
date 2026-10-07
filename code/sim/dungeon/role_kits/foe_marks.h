/* Foe marks (role_kits/foe_marks.cpp), the part dungeon.cpp needs before
   the rest: what role spells leave on monsters, kept in the run as a
   small table by monster, like threat: the striker's Searing stacks
   (striker.cpp) and the tank's Sunder (tank.cpp). Online the client
   keeps the same table from the snapshot, with Slot as its own entity
   index and Serial 0, to draw them. */

#define MAX_FOE_MARKS 16

struct foe_mark
{
    // NOTE(zoubir): the monster, as entity slot and MonsterSerial; a row
    // with no stacks and no sunder is free (FoeMarkUsed)
    u32 Slot;
    u32 Serial;
    // NOTE(zoubir): Searing stacks, 0 to SEARING_MOST, and the seconds
    // until they fade
    u32 Stacks;
    float Seconds;
    // NOTE(zoubir): seconds the monster stays sundered, and the share more
    // it takes meanwhile
    float SunderSeconds;
    float SunderShare;
};
