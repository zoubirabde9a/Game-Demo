/* Soak test: the real game with 8 players mashing random buttons and
   joining and leaving at random, for several simulated minutes per seed.
   After every tick it checks the world's bookkeeping, so a rule that
   corrupts state (an entity moved without updating its chunks, a
   projectile never removed, a player slot pointing at a dead entity) is
   caught on the tick it happens instead of as a crash minutes later.

   Usage: soak_tests [minutes-per-seed] [seed-count] [map | K/N]
   (default 3 and 4); the default runs end with the random-frame-time
   run and an all-bots run. A map name runs every seed on that map only. K/N
   runs only every Nth of the default runs, starting at the Kth (0-based),
   so N programs given 0/N .. N-1/N cover exactly the default runs, with
   the same seeds, at the same time; test.bat does that.
   Run by test.bat. On Linux, build with -fsanitize=address for memory errors. */

#include <stdio.h>
#include <stdlib.h>
#include "../server/server.cpp"

global_variable int TestFailures;
global_variable u32 CurrentSeed;
global_variable u32 CurrentTick;

// Stops at the first failure: later checks would only repeat it.
#define Require(Expression) if (!(Expression)) { Fail(#Expression, __LINE__); return false; }

internal void
Fail(const char *Expression, int Line)
{
    TestFailures++;
    printf("  FAILED soak_tests.cpp(%d): %s (seed %u, tick %u, %.1f s in)\n",
           Line, Expression, CurrentSeed, CurrentTick, CurrentTick / 60.0f);
}

struct soak_random { u32 State; };

internal u32
NextRandom(soak_random *R)
{
    u32 X = R->State;
    X ^= X << 13; X ^= X >> 17; X ^= X << 5;
    R->State = X;
    return X;
}

internal bool32
Chance(soak_random *R, u32 OneIn) { return NextRandom(R) % OneIn == 0; }

internal bool32
IsFinite(float F) { return F == F && F < 1e30f && F > -1e30f; }

// Every present entity must be listed once in each chunk its collision box
// touches and nowhere else; nothing else may be listed.
internal bool32
CheckChunks(world *World)
{
    static u16 Listed[ArrayCount(((world *)0)->Entities)];
    memset(Listed, 0, sizeof(Listed));

    for (world_chunk *Chunk = World->FirstChunk; Chunk; Chunk = Chunk->NextInWorld)
    {
        Require(FindChunk(World, Chunk->ChunkX, Chunk->ChunkY, Chunk->ChunkZ) == Chunk);
        for (world_entity_chunk *Block = &Chunk->FirstEntityChunk; Block; Block = Block->Next)
        {
            Require(Block->EntityCount <= ArrayCount(Block->Entities));
            for (u32 Index = 0; Index < Block->EntityCount; ++Index)
            {
                world_entity *Entity = Block->Entities[Index];
                u32 EntityIndex = (u32)(Entity - World->Entities);
                Require(EntityIndex < World->EntityCount);
                Require(Entity->IsPresent);
                Listed[EntityIndex]++;
            }
        }
    }

    for (u32 Index = 0; Index < World->EntityCount; ++Index)
    {
        world_entity *Entity = &World->Entities[Index];
        if (!Entity->IsPresent || !Entity->Collision)
        {
            Require(Listed[Index] == 0);
            continue;
        }
        entity_collision_volume *Total = &Entity->Collision->TotalVolume;
        rectangle3 Box = RectCenterHalfDims(Entity->Position + Total->Offset, Total->HalfDims);
        chunk_range Range = GetChunkRange(World, Box);
        u32 Expected = (u32)((Range.MaxX - Range.MinX + 1) * (Range.MaxY - Range.MinY + 1) *
                             (Range.MaxZ - Range.MinZ + 1));
        // NOTE(zoubir): wholly off a bounded map, an entity is in no chunk:
        // nothing can hit it or see it, and it would never be cleaned up
        if (Expected == 0)
        {
            printf("  entity %u (type %d, kind %d, ability %u) at (%.1f, %.1f) is off the map, in no chunk\n",
                   Index, (int)Entity->Type, (int)Entity->MonsterKind, Entity->AbilityIndex,
                   Entity->Position.X, Entity->Position.Y);
        }
        Require(Expected > 0);
        if (Listed[Index] != Expected)
        {
            printf("  entity %u (type %d) at (%.1f, %.1f, %.1f) is in %u chunks, its box covers %u\n",
                   Index, (int)Entity->Type, Entity->Position.X, Entity->Position.Y,
                   Entity->Position.Z, Listed[Index], Expected);
        }
        Require(Listed[Index] == Expected);
    }
    return true;
}

// How far two entities' collision shapes overlap, along the axis where
// they overlap least: the distance one would need to move to be clear.
// 0 when they do not touch. Shapes may have several boxes (trees).
internal float
Penetration(world_entity *A, world_entity *B)
{
    float Deepest = 0;
    for (u32 IndexA = 0; IndexA < A->Collision->VolumesCount; ++IndexA)
    for (u32 IndexB = 0; IndexB < B->Collision->VolumesCount; ++IndexB)
    {
        entity_collision_volume *VA = &A->Collision->Volumes[IndexA];
        entity_collision_volume *VB = &B->Collision->Volumes[IndexB];
        v3 CenterA = A->Position + VA->Offset;
        v3 CenterB = B->Position + VB->Offset;
        float Least = 1e30f;
        for (u32 Axis = 0; Axis < 3; ++Axis)
        {
            float Distance = CenterA.Data[Axis] - CenterB.Data[Axis];
            if (Distance < 0) Distance = -Distance;
            float Gap = VA->HalfDims.Data[Axis] + VB->HalfDims.Data[Axis] - Distance;
            if (Gap < Least) Least = Gap;
        }
        if (Least > Deepest) Deepest = Least;
    }
    return Deepest;
}

internal bool32
IsMover(world_entity *E)
{
    return E->IsPresent && E->Collision && !IsDeadPlayer(E) &&
           (E->Type == EntityType_Player || E->Type == EntityType_Monster);
}

global_variable u32 UnitOverlapTicks;  // ticks where two units overlapped
global_variable float DeepestUnitOverlap;

// Units must never end a tick inside something the rules say blocks them.
// Overlap with walls, trees and rocks fails at once; overlap between units
// is counted (pushes and spawns can briefly cause it) and reported.
internal bool32
CheckCollisions(server_game *Game)
{
    app_state *AppState = Game->AppState;
    world *World = &AppState->World;
    const float Tolerance = 0.5f;
    bool32 UnitOverlapThisTick = false;

    for (u32 IndexA = 0; IndexA < World->EntityCount; ++IndexA)
    {
        world_entity *A = &World->Entities[IndexA];
        if (!IsMover(A)) continue;
        for (u32 IndexB = 0; IndexB < World->EntityCount; ++IndexB)
        {
            world_entity *B = &World->Entities[IndexB];
            if (B == A || !B->IsPresent || !B->Collision || IsDeadPlayer(B)) continue;
            bool32 Solid = B->Type == EntityType_StaticObject || B->Type == EntityType_Tiled;
            if (!Solid && !(IsMover(B) && IndexB > IndexA)) continue;
            if (!CanCollide(AppState, A->Type, B->Type) || !CanCollide(AppState, A, B)) continue;

            float Depth = Penetration(A, B);
            if (Depth <= Tolerance) continue;
            if (Solid)
            {
                printf("  entity %u (type %d) at (%.1f, %.1f, %.1f) is %.1f deep in entity %u (type %d) at (%.1f, %.1f)\n",
                       IndexA, (int)A->Type, A->Position.X, A->Position.Y, A->Position.Z, Depth,
                       IndexB, (int)B->Type, B->Position.X, B->Position.Y);
                Require(Depth <= Tolerance);
            }
            UnitOverlapThisTick = true;
            if (!getenv("SOAK_VERBOSE"))
            {
                printf("  units %u (type %d) and %u (type %d) overlap %.1f at (%.0f, %.0f)\n",
                       IndexA, (int)A->Type, IndexB, (int)B->Type, Depth, A->Position.X, A->Position.Y);
                Require(Depth <= Tolerance);
            }
            if (Depth > DeepestUnitOverlap) DeepestUnitOverlap = Depth;
            if (getenv("SOAK_VERBOSE"))
            {
                printf("    tick %u: entity %u (type %d kind %d phase %d ability %u) and %u (type %d kind %d phase %d ability %u) overlap %.1f at (%.0f, %.0f)\n",
                       CurrentTick, IndexA, (int)A->Type, (int)A->MonsterKind, (int)A->AbilityPhase, A->AbilityIndex,
                       IndexB, (int)B->Type, (int)B->MonsterKind, (int)B->AbilityPhase, B->AbilityIndex,
                       Depth, A->Position.X, A->Position.Y);
            }
        }
    }
    // On infinite maps walls and props are terrain stand-ins, not entities,
    // and on every map raised ground is.
    {
        for (u32 IndexA = 0; IndexA < World->EntityCount; ++IndexA)
        {
            world_entity *A = &World->Entities[IndexA];
            if (!IsMover(A) || !CanCollide(AppState, A->Type, EntityType_StaticObject)) continue;
            entity_collision_volume *Total = &A->Collision->TotalVolume;
            rectangle3 Box = RectCenterHalfDims(A->Position + Total->Offset, Total->HalfDims);
            world_entity *Terrain[64];
            u32 Count = GatherTerrainColliders(World, Box, Terrain, 0, ArrayCount(Terrain));
            for (u32 Index = 0; Index < Count; ++Index)
            {
                float Depth = Penetration(A, Terrain[Index]);
                if (Depth <= Tolerance) continue;
                printf("  entity %u (type %d kind %d) at (%.1f, %.1f, %.1f) is %.1f deep in terrain at (%.1f, %.1f)\n",
                       IndexA, (int)A->Type, (int)A->MonsterKind, A->Position.X, A->Position.Y,
                       A->Position.Z, Depth, Terrain[Index]->Position.X, Terrain[Index]->Position.Y);
                Require(Depth <= Tolerance);
            }
        }
    }
    if (UnitOverlapThisTick) UnitOverlapTicks++;
    return true;
}

internal bool32
CheckWorld(server_game *Game)
{
    if (!CheckCollisions(Game)) return false;
    app_state *AppState = Game->AppState;
    world *World = &AppState->World;
    float Width = (float)(World->NumTilesX * World->TileWidth);
    float Height = (float)(World->NumTilesY * World->TileHeight);

    // Growth here means something is spawned faster than it is removed.
    Require(World->EntityCount < ArrayCount(World->Entities) / 2);
    u32 Present = 0;
    for (u32 Index = 0; Index < World->EntityCount; ++Index)
    {
        world_entity *Entity = &World->Entities[Index];
        if (!Entity->IsPresent) continue;
        ++Present;
        bool32 Inside = World->Unbounded ||
                        (Entity->Position.X >= -64.0f && Entity->Position.X <= Width + 64.0f &&
                         Entity->Position.Y >= -64.0f && Entity->Position.Y <= Height + 64.0f);
        if (!Inside)
        {
            printf("  entity %u type %d kind %d at (%.1f, %.1f, %.1f) moving (%.1f, %.1f), arena %.0f x %.0f\n",
                   Index, (int)Entity->Type, (int)Entity->MonsterKind, Entity->Position.X, Entity->Position.Y,
                   Entity->Position.Z, Entity->Velocity.X, Entity->Velocity.Y, Width, Height);
        }
        Require(IsFinite(Entity->Position.X) && IsFinite(Entity->Position.Y) && IsFinite(Entity->Position.Z));
        Require(IsFinite(Entity->Velocity.X) && IsFinite(Entity->Velocity.Y) && IsFinite(Entity->Velocity.Z));
        Require(Inside);
        Require(Entity->Position.Z >= -0.01f);
        Require(IsFinite(Entity->Hp));
    }
    Require(Present < 1000);

    // Free IDs must name removed entities, each once.
    static u8 Free[ArrayCount(((world *)0)->Entities)];
    memset(Free, 0, sizeof(Free));
    for (u32 Index = 0; Index < World->FreeEntityCount; ++Index)
    {
        u32 Id = World->FreeEntityIDs[Index];
        Require(Id < World->EntityCount);
        Require(!World->Entities[Id].IsPresent);
        Require(Free[Id] == 0);
        Free[Id] = 1;
    }

    for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; ++SlotIndex)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (!Slot->Active) continue;
        Require(Slot->Entity != 0);
        Require(Slot->Entity->IsPresent);
        Require(Slot->Entity->Type == EntityType_Player);
        Require(Slot->Entity->PlayerIndex == SlotIndex);
        Require(Slot->Entity->Hp <= Slot->Entity->MaxHp);
    }

    return CheckChunks(World);
}

// The buttons a player holds change now and then, like a real player.
internal u16
RandomButtons(soak_random *R, u16 Held, u32 Heading)
{
    if (!Chance(R, 8)) return Held;
    u16 Result = 0;
    u32 Dir = NextRandom(R) % 9; // 8 directions or standing still
    // On infinite maps each player leans toward its own heading, so the
    // group spreads out and the world has to follow them all.
    if (Heading < 8 && Chance(R, 2)) Dir = Heading;
    if (Dir == 0 || Dir == 1 || Dir == 7) Result |= NetButton_Left;
    if (Dir == 3 || Dir == 4 || Dir == 5) Result |= NetButton_Right;
    if (Dir == 1 || Dir == 2 || Dir == 3) Result |= NetButton_Up;
    if (Dir == 5 || Dir == 6 || Dir == 7) Result |= NetButton_Down;
    if (Chance(R, 3)) Result |= NetButton_Sword;
    if (Chance(R, 3)) Result |= NetButton_Fireball;
    if (Chance(R, 6)) Result |= NetButton_Jump;
    if (Chance(R, 6)) Result |= NetButton_Dash;
    if (Chance(R, 10)) Result |= NetButton_Shockwave;
    if (Chance(R, 10)) Result |= NetButton_Blink;
    if (Chance(R, 10)) Result |= NetButton_Push;
    if (Chance(R, 12)) Result |= NetButton_Launch;
    if (Chance(R, 8)) Result |= NetButton_Slam;
    return Result;
}

internal bool32
SoakOneSeed(u32 Seed, u32 Minutes, u32 MapId)
{
    CurrentSeed = Seed;
    soak_random R = {Seed * 2654435761u + 1};
    static server_game Game;
    GameInit(&Game, MapId);
    bool32 Unbounded = Game.AppState->World.Unbounded;
    float FarthestApart = 0.f;

    bool32 Joined[MAX_PLAYERS] = {};
    u16 Held[MAX_PLAYERS] = {};
    u32 InputTick = 0;
    u32 Ticks = Minutes * 60 * SERVER_TICK_RATE;
    u32 MaxEntities = 0;
    bool32 Ok = true;

    for (CurrentTick = 0; CurrentTick < Ticks && Ok; ++CurrentTick)
    {
        for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
        {
            // Mostly everyone plays; now and then someone drops or reconnects.
            if (!Joined[Slot] && Chance(&R, 120)) { GamePlayerJoined(&Game, Slot); Joined[Slot] = true; }
            else if (Joined[Slot] && Chance(&R, 3000)) { GamePlayerLeft(&Game, Slot); Joined[Slot] = false; Held[Slot] = 0; }
            if (!Joined[Slot]) continue;

            Held[Slot] = RandomButtons(&R, Held[Slot], Unbounded ? Slot : 9);
            net_input Input = {++InputTick, Held[Slot], 0, 0};
            GameApplyInput(&Game, Slot, &Input);
        }
        GameTick(&Game, 1.0f / SERVER_TICK_RATE);

        static net_snapshot Snapshot;
        if (CurrentTick % SERVER_SNAPSHOT_INTERVAL == 0)
        {
            GameWriteSnapshot(&Game, NextRandom(&R) % MAX_PLAYERS, &Snapshot);
        }
        Ok = CheckWorld(&Game);
        if (Game.AppState->World.EntityCount > MaxEntities) MaxEntities = Game.AppState->World.EntityCount;
        for (u32 A = 0; A < MAX_PLAYERS; ++A)
        for (u32 B = A + 1; B < MAX_PLAYERS; ++B)
        {
            player_slot *SA = &Game.AppState->Players[A];
            player_slot *SB = &Game.AppState->Players[B];
            if (!SA->Active || !SB->Active || !SA->Entity || !SB->Entity) continue;
            float Apart = Length(SA->Entity->Position.XY - SB->Entity->Position.XY);
            if (Apart > FarthestApart) FarthestApart = Apart;
        }
    }

    u32 Kills = 0, Deaths = 0;
    for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
    {
        Kills += Game.AppState->Players[Slot].Kills + Game.AppState->Players[Slot].MonsterKills;
        Deaths += Game.AppState->Players[Slot].Deaths;
    }
    printf("  %s seed %u: %u min, %s, peak %u entity slots, %u chunks, players up to %.0f apart, %u kills and %u deaths among current players\n",
           GetMapDef((map_id)MapId)->Name, Seed, Minutes, Ok ? "ok" : "stopped", MaxEntities,
           Game.AppState->World.ChunkCount, FarthestApart, Kills, Deaths);
    printf("    units overlapped on %u of %u ticks, deepest %.1f units\n",
           UnitOverlapTicks, CurrentTick, DeepestUnitOverlap);
    UnitOverlapTicks = 0;
    DeepestUnitOverlap = 0;
    GameShutdown(&Game);
    return Ok;
}

// The soak's first find: fireballs had no collision rule with walls, so a
// player at the arena's edge shot them out of the map.
internal void
TestFireballBurstsOnWall()
{
    static server_game Game;
    GameInit(&Game);
    GamePlayerJoined(&Game, 0); // spawns about 300 units below the top wall
    world *World = &Game.AppState->World;
    float WallBottom = (float)World->TileHeight; // the border wall is one tile thick

    u32 Tick = 0;
    for (u32 Frame = 0; Frame < 10; ++Frame) // face up
    {
        net_input Up = {++Tick, NetButton_Up, 0, 0};
        GameApplyInput(&Game, 0, &Up);
        GameTick(&Game, 1.0f / SERVER_TICK_RATE);
    }
    net_input Cast = {++Tick, NetButton_Up | NetButton_Fireball, 0, 0};
    GameApplyInput(&Game, 0, &Cast);

    bool32 SawFireball = false;
    bool32 WentThrough = false;
    for (u32 Frame = 0; Frame < 3 * SERVER_TICK_RATE; ++Frame)
    {
        net_input Stand = {++Tick, 0, 0, 0};
        GameApplyInput(&Game, 0, &Stand);
        GameTick(&Game, 1.0f / SERVER_TICK_RATE);
        for (u32 Index = 0; Index < World->EntityCount; ++Index)
        {
            world_entity *Entity = &World->Entities[Index];
            if (!Entity->IsPresent || Entity->Type != EntityType_FireBall) continue;
            SawFireball = true;
            if (Entity->Position.Y < WallBottom - 9.0f) WentThrough = true;
        }
    }
    CurrentTick = Tick;
    if (!SawFireball) Fail("SawFireball", __LINE__);
    if (WentThrough) Fail("!WentThrough", __LINE__);
    printf("  fireball at the wall: %s\n", SawFireball && !WentThrough ? "ok" : "FAILED");
    GameShutdown(&Game);
}

// Units that start a tick inside a tree or inside each other end it apart.
internal void
TestOverlapsAreSeparated()
{
    static server_game Game;
    GameInit(&Game);
    world *World = &Game.AppState->World;
    for (u32 Index = 0; Index < World->EntityCount; ++Index) // no monsters in the way
    {
        if (World->Entities[Index].Type == EntityType_Monster) RemoveEntity(World, &World->Entities[Index]);
    }
    Game.AppState->Monsters->Target = 0;
    GamePlayerJoined(&Game, 0);
    GamePlayerJoined(&Game, 1);
    world_entity *P0 = Game.AppState->Players[0].Entity;
    world_entity *P1 = Game.AppState->Players[1].Entity;

    world_entity *Tree = 0;
    for (u32 Index = 0; Index < World->EntityCount && !Tree; ++Index)
    {
        world_entity *E = &World->Entities[Index];
        if (E->IsPresent && E->Type == EntityType_StaticObject && E->Collision == Game.AppState->TreeCollision) Tree = E;
    }

    // Player 0 put inside the tree's trunk: one tick later it is out.
    v3 Before = P0->Position;
    P0->Position = Tree->Position + V3(3.f, 2.f, 0.f);
    CheckAndChangeEntityChunk(Game.AppState, World, Game.Arena, Before, P0);
    if (!(Penetration(P0, Tree) > 0.5f)) Fail("player starts inside the tree", __LINE__);
    GameTick(&Game, 1.0f / SERVER_TICK_RATE);
    bool32 OutOfTree = Penetration(P0, Tree) <= 0.5f;

    // Player 0 put almost on top of player 1: one tick later they are apart.
    Before = P0->Position;
    P0->Position = P1->Position + V3(4.f, 1.f, 0.f);
    CheckAndChangeEntityChunk(Game.AppState, World, Game.Arena, Before, P0);
    if (!(Penetration(P0, P1) > 0.5f)) Fail("players start overlapping", __LINE__);
    GameTick(&Game, 1.0f / SERVER_TICK_RATE);
    bool32 Apart = Penetration(P0, P1) <= 0.5f;

    if (!OutOfTree) Fail("OutOfTree", __LINE__);
    if (!Apart) Fail("Apart", __LINE__);
    printf("  separation: %s\n", OutOfTree && Apart ? "ok" : "FAILED");
    GameShutdown(&Game);
}

// Usage: soak_tests [minutes] [seeds] [map]. With no map, the seeds play
// the Old Arena and one more seed plays each other map, so a hand-made map
// is soaked as well as the infinite ones.
// NOTE(zoubir): the bare simulation (no server) under random play with a
// random frame time, 4 players for 3 minutes. Seed 5 used to crash after
// about 8000 ticks on a chunk bug. Moved here from sim_tests.cpp, where
// it took 18 of its 24 seconds; here it runs as one more soak part.
internal void
TestRandomPlaySoak()
{
    app_state *AppState = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(48);
    memory_arena Arena, Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)),
                    Megabytes(1));
    InitSimulation(AppState, &Arena, &Constants);
    for(u32 Slot = 0; Slot < 4; Slot++)
    {
        AddPlayerToSlot(AppState, &AppState->World, &Arena, Slot,
                        PlayerSpawnPosition(&AppState->World, Slot));
    }
    random_series Series = Seed(5);
    u32 Ticks = 60 * 180;
    for(u32 Tick = 0; Tick < Ticks; Tick++)
    {
        for(u32 Slot = 0; Slot < 4; Slot++)
        {
            player_input *Input = &AppState->Players[Slot].Input;
            if (RandomChoice(&Series, 20) == 0)
            {
                Input->Move.X = (float)RandomChoice(&Series, 3) - 1.f;
                Input->Move.Y = (float)RandomChoice(&Series, 3) - 1.f;
            }
            Input->Pressed = RandomChoice(&Series, 10) == 0 ?
                (1u << RandomChoice(&Series, 5)) : 0;
        }
        SimulateTick(AppState, &Arena,
                     RandomBetween(&Series, 0.005f, 0.05f));
        AppState->Events.Count = 0;
    }
    if (CountLiveMonsters(&AppState->World) == 0) Fail("monsters still alive", __LINE__);
    free(Arena.Base);
    free(Constants.Base);
    free(AppState);
}

// NOTE(zoubir): eight server bots (server/bots.cpp) and nobody else, for
// a simulated minute, with the same world checks as the soak: the bots'
// brains drive every player, so a bot that walks somewhere odd or keeps
// pressing something the rules do not expect shows up here.
internal void
SoakBots(u32 Minutes)
{
    CurrentSeed = 0;
    static server_game Game;
    GameInit(&Game, MapId_Arena);
    Game.BotTarget = MAX_PLAYERS;
    bool32 Ok = true;
    u32 Ticks = Minutes * 60 * SERVER_TICK_RATE;
    float Dt = 1.0f / SERVER_TICK_RATE;
    for (CurrentTick = 0; CurrentTick < Ticks && Ok; ++CurrentTick)
    {
        GameKeepBots(&Game, 0, Dt);
        GameTick(&Game, Dt);
        static net_snapshot Snapshot;
        if (CurrentTick % SERVER_SNAPSHOT_INTERVAL == 0)
        {
            GameWriteSnapshot(&Game, CurrentTick % MAX_PLAYERS, &Snapshot);
        }
        Ok = CheckWorld(&Game);
    }
    u32 Kills = 0, MonsterKills = 0, Deaths = 0, Bots = 0;
    for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
    {
        Kills += Game.AppState->Players[Slot].Kills;
        MonsterKills += Game.AppState->Players[Slot].MonsterKills;
        Deaths += Game.AppState->Players[Slot].Deaths;
        Bots += Game.Bots[Slot].Active ? 1 : 0;
    }
    if (Bots != MAX_PLAYERS) Fail("all eight bots still playing", __LINE__);
    printf("  bots: %u min, %s, %u bots, %u player kills, %u monster kills, %u deaths\n",
           Minutes, Ok ? "ok" : "stopped", Bots, Kills, MonsterKills, Deaths);
    GameShutdown(&Game);
}

int
main(int ArgCount, char **Args)
{
    u32 Minutes = ArgCount > 1 ? (u32)atoi(Args[1]) : 3;
    u32 Seeds = ArgCount > 2 ? (u32)atoi(Args[2]) : 4;
    map_id OnlyMap = MapId_Count;
    u32 Part = 0, Parts = 1;
    if (ArgCount > 3)
    {
        if (sscanf(Args[3], "%u/%u", &Part, &Parts) != 2 || Parts == 0 || Part >= Parts)
        {
            Part = 0;
            Parts = 1;
            OnlyMap = FindMapByName(Args[3], MapId_Count);
        }
    }

    // The default runs: every seed on the arena, then one more seed on
    // each other map. A part runs every Parts-th of them.
    u32 RunSeed[64];
    u32 RunMap[64];
    u32 RunCount = 0;
    for (u32 Seed = 1; Seed <= Seeds && RunCount < 64; ++Seed)
    {
        RunSeed[RunCount] = Seed;
        RunMap[RunCount++] = (OnlyMap == MapId_Count) ? MapId_Arena : OnlyMap;
    }
    if (OnlyMap == MapId_Count)
    {
        for (u32 MapIndex = 0; MapIndex < MapId_Count && RunCount < 64; ++MapIndex)
        {
            if (MapIndex == MapId_Arena) continue;
            RunSeed[RunCount] = Seeds + 1;
            RunMap[RunCount++] = MapIndex;
        }
    }

    if (Part == 0)
    {
        TestFireballBurstsOnWall();
        TestOverlapsAreSeparated();
    }
    if (Parts > 1)
    {
        printf("soak part %u/%u: %u seeds x %u simulated minutes, 8 players\n",
               Part, Parts, Seeds, Minutes);
    }
    else
    {
        printf("soak: %u seeds x %u simulated minutes, 8 players\n", Seeds, Minutes);
    }
    for (u32 Run = Part; Run < RunCount; Run += Parts)
    {
        SoakOneSeed(RunSeed[Run], Minutes, RunMap[Run]);
    }
    // The random-frame-time regression is one more run after the others.
    if (OnlyMap == MapId_Count && RunCount % Parts == Part)
    {
        printf("  random play, 4 players, 3 min of random frame times\n");
        TestRandomPlaySoak();
    }
    // Then eight bots, one more run.
    if (OnlyMap == MapId_Count && (RunCount + 1) % Parts == Part)
    {
        SoakBots(Minutes);
    }
    printf("soak tests: %s\n", TestFailures ? "FAILED" : "all seeds passed");
    return TestFailures ? 1 : 0;
}
