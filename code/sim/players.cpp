/* Player slots: joining, finding players, respawning them, and the
   player's aim. */

// NOTE(zoubir): V as a unit vector, or Fallback when V is too short to
// have a direction
inline v2
NormalizeOr(v2 V, v2 Fallback)
{
    float LengthSquared = LengthSq(V);
    v2 Result = LengthSquared > 0.0001f ?
        V * (1.f / SquareRoot(LengthSquared)) : Fallback;
    return Result;
}

// NOTE(zoubir): the player's aim; before any cursor input, the way it last
// walked (and Right for a player that never moved)
inline v2
GetPlayerAim(world_entity *Player)
{
    v2 Result = Player->Aim;
    if (LengthSq(Result) < 0.0001f)
    {
        Result = NormalizeOr(Player->Direction, V2(1.f, 0.f));
    }
    return Result;
}

// NOTE(zoubir): in monster_population.cpp, included after this file
internal bool32
IsSpawnSpotFree(app_state *AppState, world *World, v3 Position,
                entity_collision_volume_group *Volume);

// NOTE(zoubir): nearest spot to Desired on rings of 24 units that is inside
// the arena and clear, standing on the ground there (on top of raised
// ground); Desired itself when every ring is full
internal v3
FindFreeSpotAround(app_state *AppState, world *World, v3 Desired,
                   entity_collision_volume_group *Volume)
{
    float Width = (float)(World->NumTilesX * World->TileWidth);
    float Height = (float)(World->NumTilesY * World->TileHeight);
    float Margin = 2.f * (float)World->TileWidth;
    for(u32 Ring = 1; Ring <= 12; Ring++)
    {
        float Radius = 24.f * Ring;
        u32 Steps = 8 * Ring;
        for(u32 Step = 0; Step < Steps; Step++)
        {
            float Angle = 2.f * Pi32 * (float)Step / (float)Steps;
            v3 Spot = OnGround(World, Desired + V3(Radius * Cos(Angle),
                                                   Radius * Sin(Angle), 0.f));
            if (!World->Unbounded &&
                (Spot.X < Margin || Spot.X > Width - Margin ||
                 Spot.Y < Margin || Spot.Y > Height - Margin))
            {
                continue;
            }
            if (IsSpawnSpotFree(AppState, World, Spot, Volume))
            {
                return Spot;
            }
        }
    }
    return Desired;
}

// NOTE(zoubir): Desired if a player fits there, else the nearest free spot
// around it. A spawn point inside a tree, or a monster standing on it,
// would otherwise leave the player stuck inside something. Self is the
// player being placed, whose own body must not count as in the way, or 0.
internal v3
FindFreePlayerSpot(app_state *AppState, world *World, v3 Desired,
                   world_entity *Self)
{
    // NOTE(zoubir): hide Self from the overlap checks during the search only
    bool32 SelfWasPresent = Self ? Self->IsPresent : false;
    if (Self)
    {
        Self->IsPresent = false;
    }
    v3 Result = OnGround(World, Desired);
    entity_collision_volume_group *Volume = AppState->PlayerCollision;
    if (!IsSpawnSpotFree(AppState, World, Result, Volume))
    {
        Result = FindFreeSpotAround(AppState, World, Result, Volume);
    }
    if (Self)
    {
        Self->IsPresent = SelfWasPresent;
    }
    return Result;
}

// NOTE(zoubir): puts a new player entity in SlotIndex, at SpawnPosition
// or the nearest free spot to it; returns it
internal world_entity *
AddPlayerToSlot(app_state *AppState, world *World, memory_arena *Arena,
                u32 SlotIndex, v3 SpawnPosition)
{
    Assert(SlotIndex < MAX_PLAYERS);
    player_slot *Slot = &AppState->Players[SlotIndex];
    *Slot = {};
    Slot->Active = true;
    Slot->Level = 1;
    Slot->SpawnPosition = SpawnPosition;
    Slot->Entity = AddPlayer(AppState, World, Arena,
                             FindFreePlayerSpot(AppState, World, SpawnPosition, 0));
    Slot->Entity->PlayerIndex = SlotIndex;
    return Slot->Entity;
}

// NOTE(zoubir): the slot is free again; the entity leaves the world
internal void
RemovePlayerFromSlot(app_state *AppState, world *World, u32 SlotIndex)
{
    Assert(SlotIndex < MAX_PLAYERS);
    player_slot *Slot = &AppState->Players[SlotIndex];
    if (Slot->Active && Slot->Entity)
    {
        RemoveEntity(World, Slot->Entity);
    }
    *Slot = {};
}

// NOTE(zoubir): where each slot appears and respawns: the map's spawn
// tiles in turn, players beyond the map's count stepping round the same
// tiles a little further out
internal v3
PlayerSpawnPosition(world *World, u32 SlotIndex)
{
    Assert(SlotIndex < MAX_PLAYERS);
    map_def *Map = GetMapDef((map_id)World->MapId);
    float TileSize = World->TileWidth ? (float)World->TileWidth : (float)ARENA_TILE_SIZE;
    v3 Result = V3(0.5f * TileSize, 0.5f * TileSize, 0.f);
    if (Map->Kind == MapKind_Infinite)
    {
        // NOTE(zoubir): a ring inside the clearing kept open at the origin
        float Angle = 2.f * Pi32 * (float)SlotIndex / (float)MAX_PLAYERS;
        Result.X += 72.f * Cos(Angle);
        Result.Y += 72.f * Sin(Angle);
    }
    else if (Map->SpawnCount > 0)
    {
        u32 Spawn = SlotIndex % Map->SpawnCount;
        u32 Lap = SlotIndex / Map->SpawnCount;
        Result.X = ((float)Map->SpawnX[Spawn] + 0.5f) * TileSize + 48.f * (float)Lap;
        Result.Y = ((float)Map->SpawnY[Spawn] + 0.5f) * TileSize;
    }
    return Result;
}

inline player_slot *
GetPlayerSlot(app_state *AppState, world_entity *PlayerEntity)
{
    Assert(PlayerEntity->Type == EntityType_Player);
    Assert(PlayerEntity->PlayerIndex < MAX_PLAYERS);
    player_slot *Result = &AppState->Players[PlayerEntity->PlayerIndex];
    return Result;
}

inline world_entity *
GetLocalPlayer(app_state *AppState)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    world_entity *Result = Slot->Active ? Slot->Entity : 0;
    return Result;
}

// NOTE(zoubir): closest live player on the ground plane, or 0
internal world_entity *
FindNearestPlayer(app_state *AppState, v2 Position, float *OutDistance)
{
    world_entity *Result = 0;
    float BestDistance = 1e30f;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Entity && Slot->Entity->IsPresent &&
            !IsDeadPlayer(Slot->Entity))
        {
            float Distance = Length(Slot->Entity->Position.XY - Position);
            if (Distance < BestDistance)
            {
                BestDistance = Distance;
                Result = Slot->Entity;
            }
        }
    }
    if (OutDistance)
    {
        *OutDistance = BestDistance;
    }
    return Result;
}

// NOTE(zoubir): the shield a respawn gives (progression/talents.cpp,
// included after this)
inline float RespawnShieldSeconds(player_slot *Slot);

// NOTE(zoubir): the player entity is never removed. While dead it waits
// out RespawnTimer (deaths are counted in DamageEntity), then goes back to
// its slot's spawn point with full health, so slot pointers never dangle.
// Returns true while the player is still dead.
internal bool32
UpdateDeadPlayer(player_slot *Slot, world *World, memory_arena *Arena,
                 app_state *AppState, float DeltaTime)
{
    world_entity *Player = Slot->Entity;
    if (Player->Hp > 0.f)
    {
        return false;
    }

    Slot->RespawnTimer -= DeltaTime;
    if (Slot->RespawnTimer > 0.f)
    {
        return true;
    }

    v3 OldPosition = Player->Position;
    Player->Position = FindFreePlayerSpot(AppState, World, Slot->SpawnPosition, Player);
    Player->Velocity = {};
    Player->Hp = Player->MaxHp;
    Slot->RespawnTimer = 0.f;
    Slot->DelayedInputCount = 0;
    // NOTE(zoubir): a cast or a slam cut short by death does not go off
    // at the spawn point
    CancelPlayerCast(Player);
    Player->PendingLandArea = 0;
    Player->SpawnShield = RespawnShieldSeconds(Slot);
    CheckAndChangeEntityChunk(AppState, World, Arena,
                              OldPosition, Player);
    EmitBurst(&AppState->Events, SimBurst_Spawn, (u8)Player->PlayerIndex,
              Player->Position);
    return false;
}
