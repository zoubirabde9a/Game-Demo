/* Encounters: the dungeon run's rooms coming alive, one at a time
   (docs/dungeon-plan.md). Included by sim_module.cpp after
   monster_population.cpp, whose spawning it uses; UpdateDungeon runs once
   a tick from SimulateTick.

   A room waits until a living player stands in it and the room before is
   cleared. Then its encounter starts: every row of the map's encounter
   table for the room spawns (crypt_encounters.cpp and the other levels), packs together on
   spots away from the party, a boss in the middle, each with health
   scaled to the party's size. The rest of the party is pulled in, and
   both of the room's gates close behind walls. The dead stay down while
   it lasts.

   When every monster of the encounter is dead the room is cleared: the
   gate onward opens and the dead stand up at the room's entrance. When
   every player is dead the party wipes: the monsters vanish, the room
   waits again, and everyone comes back at the checkpoint by its
   entrance gate, in the room before. Cleared rooms stay cleared. */

#include "threat.cpp"

// NOTE(zoubir): packs appear at least this far from every player
#define DUNGEON_PACK_DISTANCE 300.f
#define DUNGEON_PACK_SPREAD 70.f
#define DUNGEON_SPOT_TRIES 48
// NOTE(zoubir): the dead wait this long while a fight lasts, which no
// fight does; the fight's end sets the real wait
#define DUNGEON_DOWNED_SECONDS 1000000.f
#define DUNGEON_CLEAR_RESPAWN_SECONDS 1.f
#define DUNGEON_WIPE_RESPAWN_SECONDS 2.f
// NOTE(zoubir): how long a fight lasts with no living player in its room
#define DUNGEON_EMPTY_ROOM_SECONDS 2.f

inline u32
CountPartyPlayers(app_state *AppState, u32 *Standing)
{
    u32 Result = 0;
    *Standing = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Entity)
        {
            Result++;
            *Standing += IsDeadPlayer(Slot->Entity) ? 0 : 1;
        }
    }
    return Result;
}

// NOTE(zoubir): opens or closes one gate: walls on every one of its tiles
internal void
SetGateClosed(app_state *AppState, world *World, memory_arena *Arena,
              dungeon_run *Run, u32 Gate, bool32 Closed)
{
    u32 *Walls = Run->GateWalls[Gate];
    bool32 IsClosed = Walls[0] != 0;
    if (Closed == IsClosed)
    {
        return;
    }
    if (!Closed)
    {
        for(u32 Index = 0; Index < DUNGEON_GATE_TILES && Walls[Index]; Index++)
        {
            RemoveEntity(World, &World->Entities[Walls[Index] - 1]);
            Walls[Index] = 0;
        }
        return;
    }
    map_def *Map = GetMapDef((map_id)World->MapId);
    u32 Count = 0;
    for(i32 Y = 0; Y < (i32)Map->Height; Y++)
    {
        for(i32 X = 0; X < (i32)Map->Width; X++)
        {
            if (GateAtTile(World->MapId, X, Y) == Gate && Count < DUNGEON_GATE_TILES)
            {
                world_entity *Wall = AddWall(AppState, World, Arena,
                                             TileCenter(World, X, Y));
                Walls[Count++] = (u32)(Wall - World->Entities) + 1;
            }
        }
    }
}

// NOTE(zoubir): gate G joins room G + 1 to room G + 2. It is open once
// the room before it is cleared, unless the room after it is fighting
inline bool32
IsGateOpen(dungeon_run *Run, u32 Gate)
{
    bool32 Result = Run->RoomStates[Gate + 1] == RoomState_Cleared &&
        Run->RoomStates[Gate + 2] != RoomState_Fighting;
    return Result;
}

internal void
UpdateGates(app_state *AppState, world *World, memory_arena *Arena,
            dungeon_run *Run)
{
    for(u32 Gate = 0; Gate + 1 < Run->RoomCount; Gate++)
    {
        SetGateClosed(AppState, World, Arena, Run, Gate, !IsGateOpen(Run, Gate));
    }
    Run->GatesBuilt = true;
}

#include "pack_spots.cpp"

// NOTE(zoubir): one of a room's monsters near Spot; 0 when the run's
// list is full
internal world_entity *
SpawnFoe(app_state *AppState, world *World, memory_arena *Arena,
         dungeon_run *Run, v3 Spot, encounter_row *Row, float HealthScale)
{
    if (Run->FoeCount >= DUNGEON_MAX_FOES)
    {
        return 0;
    }
    entity_collision_volume_group *Volume =
        GetMonsterStats(Row->Kind)->FlyHeight > 0.f ?
        AppState->BatCollision : AppState->PlayerCollision;
    v3 Position = Spot;
    for(u32 Try = 0; Try < DUNGEON_SPOT_TRIES; Try++)
    {
        v3 Probe = Spot;
        if (Try > 0)
        {
            Probe.X += RandomBetween(&Run->Series, -DUNGEON_PACK_SPREAD, DUNGEON_PACK_SPREAD);
            Probe.Y += RandomBetween(&Run->Series, -DUNGEON_PACK_SPREAD, DUNGEON_PACK_SPREAD);
        }
        Probe = OnGround(World, Probe);
        if (IsSpawnSpotFree(AppState, World, Probe, Volume))
        {
            Position = Probe;
            break;
        }
    }
    world_entity *Monster = SpawnMonster(AppState, World, Arena, Position, Row->Kind);
    if (Row->Flags & Encounter_Elite)
    {
        ApplyEliteAffix(Monster, 1 + RandomChoice(&Run->Series, MonsterAffix_Count - 1));
    }
    Monster->MaxHp *= HealthScale;
    Monster->Hp = Monster->MaxHp;
    Monster->PaceScale = LevelFoePace(World->MapId);
    Run->FoeSlots[Run->FoeCount] = (u32)(Monster - World->Entities);
    Run->FoeSerials[Run->FoeCount] = Monster->MonsterSerial;
    Run->FoeCount++;
    return Monster;
}

// NOTE(zoubir): puts a living player at Position, as a respawn does
internal void
MovePlayerTo(app_state *AppState, world *World, memory_arena *Arena,
             world_entity *Player, v3 Position)
{
    v3 OldPosition = Player->Position;
    Player->Position = FindFreePlayerSpot(AppState, World, Position, Player);
    Player->Velocity = {};
    CheckAndChangeEntityChunk(AppState, World, Arena, OldPosition, Player);
}

internal void
StartEncounter(app_state *AppState, world *World, memory_arena *Arena,
               dungeon_run *Run, u32 Room)
{
    u32 Standing;
    u32 Players = CountPartyPlayers(AppState, &Standing);
    float HealthScale = DUNGEON_FOE_HEALTH * LevelFoeHealth(World->MapId) *
        PartyHealthScale(Players);
    Run->PartyDamage = DUNGEON_FOE_DAMAGE * PartyDamageScale(Players);
    Run->RoomStates[Room] = RoomState_Fighting;
    Run->FightingRoom = Room;
    StartMeter(AppState, Run, Room);
    Run->FoeCount = 0;
    Run->BossSlot = Run->BossSerial = Run->BossEventsFired = 0;

    u32 RowCount;
    encounter_row *Rows = GetEncounters(World->MapId, &RowCount);
    // NOTE(zoubir): a room of packs, no boss, takes the level's pack scale
    bool32 BossRoom = false;
    for(u32 RowIndex = 0; RowIndex < RowCount; RowIndex++)
    {
        BossRoom |= Rows[RowIndex].Room == Room && (Rows[RowIndex].Flags & Encounter_Boss);
    }
    if (!BossRoom)
    {
        HealthScale *= LevelPackScale(World->MapId);
    }
    // NOTE(zoubir): where the packs placed so far stand, to keep the next
    // apart from them
    v3 PackSpots[8];
    u32 PackSpotCount = 0;
    for(u32 Pack = 0; Pack < DUNGEON_MAX_FOES; Pack++)
    {
        bool32 PackUsed = false;
        v3 Spot = {};
        for(u32 RowIndex = 0; RowIndex < RowCount; RowIndex++)
        {
            encounter_row *Row = &Rows[RowIndex];
            if (Row->Room != Room || Row->Pack != Pack)
            {
                continue;
            }
            if (!PackUsed)
            {
                PackUsed = true;
                Spot = (Row->Flags & Encounter_Boss) ? Run->RoomMiddle[Room] :
                    PickPackSpotApart(World, Run, Room, PackSpots, PackSpotCount);
                if (PackSpotCount < ArrayCount(PackSpots))
                {
                    PackSpots[PackSpotCount++] = Spot;
                }
            }
            for(u32 Index = 0; Index < Row->Count; Index++)
            {
                world_entity *Foe = SpawnFoe(AppState, World, Arena, Run, Spot,
                                             Row, HealthScale);
                if (Foe && (Row->Flags & Encounter_Boss))
                {
                    Run->BossSlot = (u32)(Foe - World->Entities);
                    Run->BossSerial = Foe->MonsterSerial;
                }
            }
        }
    }

    // NOTE(zoubir): the party fights together: whoever is outside is
    // pulled in before the gates close, and a wipe brings everyone back
    // to the room's checkpoint
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (!Slot->Active || !Slot->Entity)
        {
            continue;
        }
        Slot->SpawnPosition = Run->RoomCheckpoint[Room];
        if (RoomAtPosition(World, Slot->Entity->Position.XY) == Room)
        {
            continue;
        }
        // NOTE(zoubir): the dead stay outside, but never on the gates'
        // tiles, where the walls are about to stand
        if (!IsDeadPlayer(Slot->Entity))
        {
            MovePlayerTo(AppState, World, Arena, Slot->Entity, Run->RoomEntry[Room]);
        }
        else if (RoomAtPosition(World, Slot->Entity->Position.XY) == 0)
        {
            MovePlayerTo(AppState, World, Arena, Slot->Entity, Run->RoomCheckpoint[Room]);
        }
    }
}

#include "fight_end.cpp"

// NOTE(zoubir): the next room may start once the one before is cleared
inline bool32
CanStartRoom(dungeon_run *Run, u32 Room)
{
    bool32 Result = Room >= 1 && Room <= Run->RoomCount &&
        Run->RoomStates[Room] == RoomState_Waiting &&
        (Room == 1 || Run->RoomStates[Room - 1] == RoomState_Cleared);
    return Result;
}

#include "role_abilities.cpp"
#include "revive.cpp"
#include "boss_scripts.cpp"

// NOTE(zoubir): a player thrown over a wall (a launch, a blast) lands on
// it or behind it, out of every room: back to the party's spot, so walls
// keep the run in order (.agents/issues/keep-edge-escape.md is the same
// escape on the duel maps). During a fight that spot is the room's
// entrance, not its checkpoint, which is behind the closed gate
internal void
RescueStrayPlayers(app_state *AppState, world *World, memory_arena *Arena,
                   dungeon_run *Run)
{
    map_def *Map = GetMapDef((map_id)World->MapId);
    float Tile = (float)World->TileWidth;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = Slot->Entity;
        if (!Slot->Active || !Player || IsDeadPlayer(Player) ||
            Player->Position.Z > Player->GroundZ + 1.f)
        {
            continue;
        }
        i32 X = (i32)floorf(Player->Position.X / Tile);
        i32 Y = (i32)floorf(Player->Position.Y / Tile);
        if (GetTerrainDef(TerrainAt(Map, X, Y))->Blocks)
        {
            v3 Spot = Run->FightingRoom ? Run->RoomEntry[Run->FightingRoom] :
                Slot->SpawnPosition;
            MovePlayerTo(AppState, World, Arena, Player, Spot);
        }
    }
}

// NOTE(zoubir): roles asked for this tick (player_input.Role): taken
// between fights, ignored during one
internal void
TakeRoleRequests(app_state *AppState, dungeon_run *Run)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        u32 Request = Slot->Input.Role;
        Slot->Input.Role = 0;
        if (Request && Slot->Active && !Run->FightingRoom)
        {
            SetPlayerRole(AppState, Slot, Request - 1);
        }
    }
}

// NOTE(zoubir): the fight as the HUD shows it (dungeon_run.Shown*)
internal void
UpdateShownFight(world *World, dungeon_run *Run)
{
    world_entity *Boss = Run->FightingRoom ? FightBoss(World, Run) : 0;
    Run->ShownBossKind = Boss ? Boss->MonsterKind : MonsterKind_Count;
    Run->ShownBossShare = (Boss && Boss->MaxHp > 0.f) ? Boss->Hp / Boss->MaxHp : 0.f;
    Run->ShownFoesLeft = Run->FightingRoom ? CountLiveFoes(World, Run) : 0;
}

// NOTE(zoubir): a cleared level stays cleared for DUNGEON_VICTORY_SECONDS,
// then the next level's map is built (StartNextRoundMap, setup.cpp;
// NextRunMap, levels.cpp): a new run, everyone in its first room keeping
// their role, level and talents
#define DUNGEON_VICTORY_SECONDS 20.f

internal void
UpdateRunEnd(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    if (NextRoomToClear(Run->RoomStates, Run->RoomCount))
    {
        Run->Seconds += DeltaTime;
        return;
    }
    bool32 WasWaiting = Run->VictorySeconds < DUNGEON_VICTORY_SECONDS;
    Run->VictorySeconds += DeltaTime;
    if (WasWaiting && Run->VictorySeconds >= DUNGEON_VICTORY_SECONDS)
    {
        AppState->RoundMapDue = true;
    }
}

// NOTE(zoubir): once a tick, from SimulateTick, before anyone moves
internal void
UpdateDungeon(app_state *AppState, memory_arena *Arena, float DeltaTime)
{
    dungeon_run *Run = AppState->Dungeon;
    if (!Run)
    {
        return;
    }
    world *World = &AppState->World;
    TakeRoleRequests(AppState, Run);
    UpdateThreat(&Run->Threat, DeltaTime);
    UpdateMeter(Run, DeltaTime);
    UpdateRoleEffects(AppState, Run, DeltaTime);
    UpdateRunTrees(AppState, Run, DeltaTime);
    RescueStrayPlayers(AppState, World, Arena, Run);
    if (!Run->FightingRoom)
    {
        for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
        {
            player_slot *Slot = &AppState->Players[SlotIndex];
            if (Slot->Active && Slot->Entity && !IsDeadPlayer(Slot->Entity))
            {
                u32 Room = RoomAtPosition(World, Slot->Entity->Position.XY);
                if (CanStartRoom(Run, Room))
                {
                    StartEncounter(AppState, World, Arena, Run, Room);
                    break;
                }
            }
        }
    }

    if (Run->FightingRoom && !UpdateFightEnd(AppState, World, Run, DeltaTime))
    {
        UpdateRevives(AppState, DeltaTime);
        RescueStrayFoes(AppState, World, Arena, Run);
        UpdateBossEvents(AppState, World, Arena, Run);
        for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
        {
            player_slot *Slot = &AppState->Players[SlotIndex];
            if (Slot->Active && Slot->Entity && IsDeadPlayer(Slot->Entity))
            {
                Slot->RespawnTimer = Maximum(Slot->RespawnTimer,
                                             DUNGEON_DOWNED_SECONDS);
            }
        }
    }
    UpdateGates(AppState, World, Arena, Run);
    UpdateShownFight(World, Run);
    UpdateRunEnd(AppState, Run, DeltaTime);
}
