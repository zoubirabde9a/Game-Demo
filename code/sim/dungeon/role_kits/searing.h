/* Searing marks (role_kits/striker.cpp), the part dungeon.cpp needs
   before the rest: the marks a striker's hits leave on monsters, kept in
   the run as a small table by monster, like threat. A mark has 1 to
   SEARING_MOST stacks and fades SEARING_SECONDS after its last stack or
   refresh. Online the client keeps the same table from the snapshot,
   with Slot as its own entity index and Serial 0, to draw them. */

#define MAX_SEARING 16

struct searing_mark
{
    // NOTE(zoubir): the monster, as entity slot and MonsterSerial; Stacks
    // 0 is a free row
    u32 Slot;
    u32 Serial;
    u32 Stacks;
    float Seconds;
};
