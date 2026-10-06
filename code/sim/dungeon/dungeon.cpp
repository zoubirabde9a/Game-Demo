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

struct dungeon_run
{
    u32 RoomCount;
};

// NOTE(zoubir): whether the world being played is a dungeon run
inline bool32
IsDungeon(app_state *AppState)
{
    bool32 Result = AppState->Dungeon != 0;
    return Result;
}

#include "roles.cpp"

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
        AppState->Dungeon->RoomCount = CountRooms(AppState->World.MapId);
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
        Result *= GetRoleDef(AppState->Players[Target->PlayerIndex].Role)->DamageTaken;
    }
    player_slot *Attacker = DungeonAttackerSlot(AppState, Source);
    if (Attacker && Target->Type == EntityType_Monster)
    {
        Result *= GetRoleDef(Attacker->Role)->DamageDealt;
    }
    return Result;
}
