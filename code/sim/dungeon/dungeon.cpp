/* dungeon: the co-op mode (docs/dungeon-plan.md). Players pick a role
   (tank, healer, damage) and clear a dungeon's rooms together. Every
   dungeon rule asks IsDungeon first, so the duel game runs exactly as it
   did when the dungeon is not being played.

   The run's state is AppState->Dungeon (dungeon_fields.inc), 0 when the
   map is not a dungeon; each
   player's role is in player_slot (dungeon_slot_fields.inc). The rest of
   the simulation calls in through a few hooks: DungeonScaleDamage
   and CountMeterDamage (DamageEntity, entity.cpp), ApplyRoleToPlayer (players.cpp) and
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

// NOTE(zoubir): a healer's Sanctuary on the ground (role_kits/healer.cpp)
#define MAX_SANCTUARIES 8
struct sanctuary
{
    v3 Position;
    float Seconds;
    u32 By;
    float Radius;
    float HealPerSecond;
};

// NOTE(zoubir): the damage role's Inferno (role_kits/striker.cpp): the
// seconds until the meteor lands, then the seconds the ground burns
#define MAX_INFERNOS 8
struct inferno
{
    v3 Position;
    float Delay;
    float Seconds;
    float Radius;
    float TickTimer;
    u32 By;
};

// NOTE(zoubir): the damage role's Giant Fireball in flight
// (role_kits/striker.cpp): where it is, its velocity, how far it may still
// fly, the room it was cast in and who cast it; Distance 0 is a free one
#define MAX_GIANT_FIREBALLS 4
struct giant_fireball
{
    v3 Position;
    v2 Velocity;
    float Distance;
    u32 Room;
    u32 By;
};

#include "boss_clock.h"
#include "role_kits/foe_marks.h"

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
    // NOTE(zoubir): seconds the room being fought has had no living
    // player in it (UpdateFightEnd, fight_end.cpp)
    float EmptySeconds;
    u32 FoeCount;
    u32 FoeSlots[DUNGEON_MAX_FOES];
    u32 FoeSerials[DUNGEON_MAX_FOES];
    // NOTE(zoubir): the fight's boss, 0 serial for none, and which of its
    // scripted events have happened, one bit per BossEvents row
    // (boss_scripts.cpp)
    u32 BossSlot;
    u32 BossSerial;
    u32 BossEventsFired;
    boss_clock Clock;
    // NOTE(zoubir): the wall entities closing each gate, as slot + 1
    // (0 for none). Built on the first tick, so only a world that
    // simulates has them
    bool32 GatesBuilt;
    u32 GateWalls[DUNGEON_MAX_GATES][DUNGEON_GATE_TILES];
    u32 Wipes;
    // NOTE(zoubir): what the monsters' hits are multiplied by in the room
    // being fought, set when it starts for the party's size
    // (PartyDamageScale, party_scaling.cpp); 0 between fights, read as 1
    float PartyDamage;
    random_series Series;
    // NOTE(zoubir): who each monster attacks (threat.cpp)
    threat_table Threat;
    sanctuary Sanctuaries[MAX_SANCTUARIES];
    inferno Infernos[MAX_INFERNOS];
    giant_fireball GiantFireballs[MAX_GIANT_FIREBALLS];
    // NOTE(zoubir): the striker's marks on monsters (role_kits/striker.cpp)
    foe_mark Marks[MAX_FOE_MARKS];
    // NOTE(zoubir): what the HUD shows of the fight: the boss's kind
    // (MonsterKind_Count for none) and share of health, and the monsters
    // left. UpdateDungeon sets them; online the snapshot does
    // (client/dungeon/dungeon_net.cpp), as the client has no fight to read
    u32 ShownBossKind;
    float ShownBossShare;
    u32 ShownFoesLeft;
    // NOTE(zoubir): seconds the run has lasted, until its last room is
    // cleared; then the seconds since, until a new run starts
    // (UpdateRunEnd, encounters.cpp)
    float Seconds;
    float VictorySeconds;
    // NOTE(zoubir): the meter's fight (meter.cpp): its room (0 before the
    // first), its length, and how many fights have started, which tells
    // an online client the meter started again
    u32 MeterRoom;
    float MeterSeconds;
    u32 MeterFight;
    // NOTE(zoubir): what each later class keeps per run
    // (role_kits/<class>.h)
    ranger_run Ranger;
    shadowblade_run Shadowblade;
    stormcaller_run Stormcaller;
    duelist_run Duelist;
    frostmage_run FrostMage;
    druid_run Druid;
};

#include "gate_crossing.cpp"

// NOTE(zoubir): the room the party has to clear next, 0 when all are
inline u32
NextRoomToClear(u8 *RoomStates, u32 RoomCount)
{
    for(u32 Room = 1; Room <= RoomCount; Room++)
    {
        if (RoomStates[Room] != RoomState_Cleared)
        {
            return Room;
        }
    }
    return 0;
}

// NOTE(zoubir): whether the world being played is a dungeon run
inline bool32
IsDungeon(app_state *AppState)
{
    bool32 Result = AppState->Dungeon != 0;
    return Result;
}

#include "roles.cpp"
#include "role_talents.cpp"
// NOTE(zoubir): every class spell's numbers, here so the damage hooks
// below can read them too
#include "role_kits/role_numbers.h"

// NOTE(zoubir): share of a hit a tank takes behind Shield Wall
// (role_abilities.cpp)
#define SHIELD_WALL_SCALE 0.4f
// NOTE(zoubir): while a healer's ward holds, the warded ally deals this
// much more (role_kits/healer.cpp): a ward on the striker is damage, a
// ward on the tank is safety
#define WARD_EMPOWER_SHARE 0.12f

#include "party_scaling.cpp"

// NOTE(zoubir): in threat.cpp and role_kits/striker.cpp, included by
// encounters.cpp later
internal void AddThreat(threat_table *Table, world *World, world_entity *Monster,
                       u32 PlayerSlot, float Amount);
internal void OnRoleKill(player_slot *Attacker);
// NOTE(zoubir): the second tree's kill and hurt effects (run_tree/run_effects.cpp)
internal void OnRunKill(app_state *AppState, player_slot *Attacker);
internal void OnRunHurt(app_state *AppState, player_slot *Slot, world_entity *Source, float Damage);
internal float FoeMarkDamageScale(dungeon_run *Run, world *World, world_entity *Monster);
internal void OnRoleHit(app_state *AppState, player_slot *Attacker, world_entity *Target,
                        world_entity *Source, float Damage);
internal float GuardianAngelSave(app_state *AppState, world_entity *Ally, player_slot *AllySlot,
                                 float Damage);
internal float TankRefusesToFall(app_state *AppState, player_slot *Slot, world_entity *Player,
                               float Damage);
internal float FireguardTakes(app_state *AppState, player_slot *Slot, world_entity *Target,
                              float Damage);

// NOTE(zoubir): the world was just built for its map in Arena
// (InitSimulation, RebuildWorldForMap): a dungeon map starts a fresh run
// there, any other leaves none
internal void
StartDungeonRun(app_state *AppState, memory_arena *Arena)
{
    AppState->Dungeon = 0;
    if (!GetMapDef((map_id)AppState->World.MapId)->Dungeon)
    {
        AppState->DungeonRoomsCleared = 0;
    }
    else
    {
        AppState->Dungeon = AllocateStruct(Arena, dungeon_run);
        ZeroSize(AppState->Dungeon, sizeof(dungeon_run));
        dungeon_run *Run = AppState->Dungeon;
        world *World = &AppState->World;
        Run->RoomCount = Minimum(CountRooms(World->MapId), (u32)DUNGEON_MAX_ROOMS);
        Run->Series = Seed(World->MapId * 7919 + 17);
        Run->ShownBossKind = MonsterKind_Count;
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

#include "meter.cpp"
#include "boss_wards.cpp"
#include "mirror_guard.cpp"

// NOTE(zoubir): from DamageEntity and ApplyHit: in a dungeon run a player
// never hurts, shoves or stuns another; outside one this is never true
internal bool32
IsFriendlyFire(app_state *AppState, world_entity *Target, world_entity *Source)
{
    bool32 Result = false;
    if (IsDungeon(AppState) && Target->Type == EntityType_Player)
    {
        player_slot *Attacker = DungeonAttackerSlot(AppState, Source);
        Result = Attacker && Attacker->Entity != Target;
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
        Result *= GetRoleDef(Slot->Role)->DamageTaken * RoleTalentTakenScale(Slot, Target);
        // NOTE(zoubir): a bigger party's fight hits harder, the tank less
        // so (PartySustainScale)
        dungeon_run *Run = AppState->Dungeon;
        bool32 FromPlayer = Source && Source->Type == EntityType_Player;
        if (Run->FightingRoom && !FromPlayer)
        {
            Result *= Slot->Role == PlayerRole_Tank ? PartySustainScale(Run) : RunPartyDamage(Run);
            Result *= RunBossDamage(Run);
            // NOTE(zoubir): and a deeper level harder still (levels.cpp)
            Result *= LevelFoeDamage(AppState->World.MapId);
            if (!Run->BossSerial)
            {
                Result *= LevelPackScale(AppState->World.MapId);
            }
        }
        Result *= BossClockDamageScale(&Run->Clock, Run->FightingRoom, Run->BossSerial);
        if (Slot->ShieldWallSeconds > 0.f)
        {
            Result *= SHIELD_WALL_SCALE;
        }
        // NOTE(zoubir): a tank's Shield Slam rallied them
        if (Slot->RallySeconds > 0.f)
        {
            Result *= 1.f - Slot->RallyShare;
        }
        // NOTE(zoubir): a striker's Fireguard takes what it can, then a
        // healer's ward
        Result = FireguardTakes(AppState, Slot, Target, Result);
        float Absorbed = Minimum(Result, Slot->WardAbsorb);
        Slot->WardAbsorb -= Absorbed;
        Result -= Absorbed;
        // NOTE(zoubir): a healer's Guardian Angel catches a falling ally
        Result = GuardianAngelSave(AppState, Target, Slot, Result);
        // NOTE(zoubir): and a tank's Unbroken the blow that still downs it
        // (role_kits/tank.cpp)
        Result = TankRefusesToFall(AppState, Slot, Target, Result);
        OnRunHurt(AppState, Slot, Source, Result);
    }
    player_slot *Attacker = DungeonAttackerSlot(AppState, Source);
    // NOTE(zoubir): a boss behind its pylons takes nothing, burns
    // included (boss_wards.cpp); a blow shows Blocked
    if (IsWardedBoss(AppState, Target))
    {
        if (Attacker)
        {
            WardDeflects(AppState, Target);
        }
        return 0.f;
    }
    // NOTE(zoubir): a raised mirror takes nothing and turns the blow back
    // on its striker (mirror_guard.cpp)
    if (Attacker && MirrorTurnsBack(AppState, Target, Attacker->Entity, Result))
    {
        return 0.f;
    }
    if (Attacker && Target->Type == EntityType_Monster)
    {
        role_def *Role = GetRoleDef(Attacker->Role);
        Result *= Role->DamageDealt * RoleTalentDealtScale(Attacker, Target);
        Result *= RunDealtScale(AppState, Attacker, Target);
        // NOTE(zoubir): a sundered monster takes more (role_kits/tank.cpp),
        // and a warded ally deals more (role_kits/healer.cpp)
        Result *= FoeMarkDamageScale(AppState->Dungeon, &AppState->World, Target);
        if (Attacker->WardAbsorb > 0.f)
        {
            Result *= 1.f + Attacker->WardEmpower;
        }
        // NOTE(zoubir): a striker's Combustion (role_kits/striker.cpp)
        if (Attacker->CombustSeconds > 0.f)
        {
            Result *= 1.f + COMBUSTION_SHARE;
        }
        AddThreat(&AppState->Dungeon->Threat, &AppState->World, Target,
                  (u32)(Attacker - AppState->Players),
                  Result * Role->ThreatScale * RunThreatScale(Attacker));
        OnRoleHit(AppState, Attacker, Target, Source, Result);
        if (Result >= Target->Hp)
        {
            OnRoleKill(Attacker);
            OnRunKill(AppState, Attacker);
        }
    }
    return Result;
}
