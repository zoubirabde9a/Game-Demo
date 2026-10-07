/* dungeon: the co-op mode (docs/dungeon-plan.md). Players pick a role
   (tank, healer, damage) and clear a dungeon's rooms together. Every
   dungeon rule asks IsDungeon first, so the duel game runs exactly as it
   did when the dungeon is not being played.

   The run's state is AppState->Dungeon (dungeon_fields.inc), 0 when the
   map is not a dungeon; each
   player's role is in player_slot (dungeon_slot_fields.inc). The rest of
   the simulation calls in through a few hooks: DungeonScaleDamage
   (DamageEntity, entity.cpp), ApplyRoleToPlayer (players.cpp) and
   StartDungeonRun (setup.cpp).

   Included by sim_module.cpp after players.cpp: it reads player slots
   and map defs, and nothing else in sim/ depends on it but those hooks. */

#include "rooms.cpp"

enum room_state
{
    RoomState_Waiting,
    RoomState_Fighting,
    RoomState_Cleared,
};

#define DUNGEON_MAX_FOES 32
#define DUNGEON_GATE_TILES 8
// NOTE(zoubir): tiles from a gate to where the party is put either side
#define DUNGEON_ENTRY_DEPTH 4.f

#define THREAT_ROWS 64
#define TAUNT_SECONDS 4.f
// NOTE(zoubir): a taunt also lifts the taunter's threat to this much
// above the top, so the monster stays on them once it runs out
#define TAUNT_THREAT_LEAD 1.1f

struct threat_row
{
    u32 Slot;
    u32 Serial;
    float Threat[MAX_PLAYERS];
    // NOTE(zoubir): the player slot + 1 that taunted it (0 for none), and
    // the seconds left of it
    u32 TauntedBy;
    float TauntSeconds;
};

struct threat_table
{
    threat_row Rows[THREAT_ROWS];
};

// NOTE(zoubir): a healer's Sanctuary on the ground (role_abilities.cpp)
#define MAX_SANCTUARIES 8
struct sanctuary
{
    v3 Position;
    float Seconds;
    u32 By;
};

struct dungeon_run
{
    u32 RoomCount;
    // NOTE(zoubir): by room, 1 for the first (encounters.cpp)
    u8 RoomStates[DUNGEON_MAX_ROOMS + 1];
    // NOTE(zoubir): where a room's fight pulls the party in, where it
    // comes back after a wipe (in the room before, by the gate), and the
    // room's middle, where its boss stands
    v3 RoomEntry[DUNGEON_MAX_ROOMS + 1];
    v3 RoomCheckpoint[DUNGEON_MAX_ROOMS + 1];
    v3 RoomMiddle[DUNGEON_MAX_ROOMS + 1];
    // NOTE(zoubir): the room being fought, 0 for none, and its monsters,
    // as entity slot and MonsterSerial (slots are reused)
    u32 FightingRoom;
    u32 FoeCount;
    u32 FoeSlots[DUNGEON_MAX_FOES];
    u32 FoeSerials[DUNGEON_MAX_FOES];
    // NOTE(zoubir): the wall entities closing each gate, as slot + 1
    // (0 for none). Built on the first tick, so only a world that
    // simulates has them
    bool32 GatesBuilt;
    u32 GateWalls[DUNGEON_MAX_GATES][DUNGEON_GATE_TILES];
    u32 Wipes;
    random_series Series;
    // NOTE(zoubir): who each monster attacks (threat.cpp)
    threat_table Threat;
    sanctuary Sanctuaries[MAX_SANCTUARIES];
};

// NOTE(zoubir): whether the world being played is a dungeon run
inline bool32
IsDungeon(app_state *AppState)
{
    bool32 Result = AppState->Dungeon != 0;
    return Result;
}

#include "roles.cpp"

// NOTE(zoubir): share of a hit a tank takes behind Shield Wall
// (role_abilities.cpp)
#define SHIELD_WALL_SCALE 0.4f

// NOTE(zoubir): in threat.cpp, included by encounters.cpp later
internal void AddThreat(threat_table *Table, world *World, world_entity *Monster,
                       u32 PlayerSlot, float Amount);

// NOTE(zoubir): the world was just built for its map in Arena
// (InitSimulation, RebuildWorldForMap): a dungeon map starts a fresh run
// there, any other leaves none
internal void
StartDungeonRun(app_state *AppState, memory_arena *Arena)
{
    AppState->Dungeon = 0;
    if (GetMapDef((map_id)AppState->World.MapId)->Dungeon)
    {
        AppState->Dungeon = AllocateStruct(Arena, dungeon_run);
        ZeroSize(AppState->Dungeon, sizeof(dungeon_run));
        dungeon_run *Run = AppState->Dungeon;
        world *World = &AppState->World;
        Run->RoomCount = Minimum(CountRooms(World->MapId), (u32)DUNGEON_MAX_ROOMS);
        Run->Series = Seed(World->MapId * 7919 + 17);
        for(u32 Room = 1; Room <= Run->RoomCount; Room++)
        {
            v2 Middle = GateOrRoomMiddle(World->MapId, 0, Room);
            Run->RoomMiddle[Room] = NearestRoomTile(World, Room, Middle);
            Run->RoomEntry[Room] = Run->RoomMiddle[Room];
            Run->RoomCheckpoint[Room] = Run->RoomMiddle[Room];
            if (Room >= 2)
            {
                // NOTE(zoubir): a few tiles in from the gate on each side,
                // so a party put there has room round the spot
                v2 Gate = GateOrRoomMiddle(World->MapId, Room - 2, 0);
                v2 Before = GateOrRoomMiddle(World->MapId, 0, Room - 1);
                Run->RoomEntry[Room] =
                    NearestRoomTile(World, Room, Gate + DUNGEON_ENTRY_DEPTH * DirectionTo(Middle - Gate));
                Run->RoomCheckpoint[Room] =
                    NearestRoomTile(World, Room - 1, Gate + DUNGEON_ENTRY_DEPTH * DirectionTo(Before - Gate));
            }
        }
    }
}

// NOTE(zoubir): the slot behind a hit: the player itself, or the owner of
// its sword, fireball or kunai; 0 for monsters and the world
inline player_slot *
DungeonAttackerSlot(app_state *AppState, world_entity *Source)
{
    player_slot *Result = 0;
    if (Source && Source->Type == EntityType_Player &&
        Source->PlayerIndex < MAX_PLAYERS)
    {
        Result = &AppState->Players[Source->PlayerIndex];
    }
    else if (Source && Source->HasOwner && Source->OwnerSlot < MAX_PLAYERS)
    {
        Result = &AppState->Players[Source->OwnerSlot];
    }
    return Result;
}

// NOTE(zoubir): from DamageEntity: a player's role changes what it takes
// and what it deals. Outside a dungeon run the damage is left alone
internal float
DungeonScaleDamage(app_state *AppState, world_entity *Target,
                   world_entity *Source, float Damage)
{
    float Result = Damage;
    if (!IsDungeon(AppState))
    {
        return Result;
    }
    if (Target->Type == EntityType_Player && Target->PlayerIndex < MAX_PLAYERS)
    {
        player_slot *Slot = &AppState->Players[Target->PlayerIndex];
        Result *= GetRoleDef(Slot->Role)->DamageTaken;
        if (Slot->ShieldWallSeconds > 0.f)
        {
            Result *= SHIELD_WALL_SCALE;
        }
        // NOTE(zoubir): a healer's ward takes what it can
        float Absorbed = Minimum(Result, Slot->WardAbsorb);
        Slot->WardAbsorb -= Absorbed;
        Result -= Absorbed;
    }
    player_slot *Attacker = DungeonAttackerSlot(AppState, Source);
    if (Attacker && Target->Type == EntityType_Monster)
    {
        role_def *Role = GetRoleDef(Attacker->Role);
        Result *= Role->DamageDealt;
        AddThreat(&AppState->Dungeon->Threat, &AppState->World, Target,
                  (u32)(Attacker - AppState->Players), Result * Role->ThreatScale);
    }
    return Result;
}
