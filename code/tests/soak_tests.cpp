/* Soak test: the real game with 8 players mashing random buttons and
   joining and leaving at random, for several simulated minutes per seed.
   After every tick it checks the world's bookkeeping, so a rule that
   corrupts state (an entity moved without updating its chunks, a
   projectile never removed, a player slot pointing at a dead entity) is
   caught on the tick it happens instead of as a crash minutes later.

   Usage: soak_tests [minutes-per-seed] [seed-count]   (default 3 and 4)
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

    for (u32 X = 0; X < CHUNK_MAX_X; ++X)
    for (u32 Y = 0; Y < CHUNK_MAX_Y; ++Y)
    for (u32 Z = 0; Z < CHUNK_MAX_Z; ++Z)
    {
        for (world_entity_chunk *Block = &World->Chunks[X][Y][Z].FirstEntityChunk; Block; Block = Block->Next)
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
        u32 MinX, MinY, MinZ, MaxX, MaxY, MaxZ;
        GetChunksFromBox(World, Box, &MinX, &MinY, &MinZ, &MaxX, &MaxY, &MaxZ);
        u32 Expected = (MaxX - MinX + 1) * (MaxY - MinY + 1) * (MaxZ - MinZ + 1);
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

internal bool32
CheckWorld(server_game *Game)
{
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
        bool32 Inside = Entity->Position.X >= -64.0f && Entity->Position.X <= Width + 64.0f &&
                        Entity->Position.Y >= -64.0f && Entity->Position.Y <= Height + 64.0f;
        if (!Inside)
        {
            printf("  entity %u type %d kind %d at (%.1f, %.1f, %.1f) moving (%.1f, %.1f), arena %.0f x %.0f\n",
                   Index, (int)Entity->Type, (int)Entity->MonsterKind, Entity->Position.X, Entity->Position.Y,
                   Entity->Position.Z, Entity->Velocity.X, Entity->Velocity.Y, Width, Height);
        }
        Require(IsFinite(Entity->Position.X) && IsFinite(Entity->Position.Y) && IsFinite(Entity->Position.Z));
        Require(IsFinite(Entity->Velocity.X) && IsFinite(Entity->Velocity.Y) && IsFinite(Entity->Velocity.Z));
        Require(Entity->Position.X >= -64.0f && Entity->Position.X <= Width + 64.0f);
        Require(Entity->Position.Y >= -64.0f && Entity->Position.Y <= Height + 64.0f);
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
RandomButtons(soak_random *R, u16 Held)
{
    if (!Chance(R, 8)) return Held;
    u16 Result = 0;
    u32 Dir = NextRandom(R) % 9; // 8 directions or standing still
    if (Dir == 0 || Dir == 1 || Dir == 7) Result |= NetButton_Left;
    if (Dir == 3 || Dir == 4 || Dir == 5) Result |= NetButton_Right;
    if (Dir == 1 || Dir == 2 || Dir == 3) Result |= NetButton_Up;
    if (Dir == 5 || Dir == 6 || Dir == 7) Result |= NetButton_Down;
    if (Chance(R, 3)) Result |= NetButton_Sword;
    if (Chance(R, 3)) Result |= NetButton_Fireball;
    if (Chance(R, 6)) Result |= NetButton_Jump;
    if (Chance(R, 6)) Result |= NetButton_Dash;
    if (Chance(R, 10)) Result |= NetButton_Shockwave;
    return Result;
}

internal bool32
SoakOneSeed(u32 Seed, u32 Minutes)
{
    CurrentSeed = Seed;
    soak_random R = {Seed * 2654435761u + 1};
    static server_game Game;
    GameInit(&Game);

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

            Held[Slot] = RandomButtons(&R, Held[Slot]);
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
    }

    u32 Kills = 0, Deaths = 0;
    for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
    {
        Kills += Game.AppState->Players[Slot].Kills + Game.AppState->Players[Slot].MonsterKills;
        Deaths += Game.AppState->Players[Slot].Deaths;
    }
    printf("  seed %u: %u min, %s, peak %u entity slots, %u kills and %u deaths among current players\n",
           Seed, Minutes, Ok ? "ok" : "stopped", MaxEntities, Kills, Deaths);
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

int
main(int ArgCount, char **Args)
{
    u32 Minutes = ArgCount > 1 ? (u32)atoi(Args[1]) : 3;
    u32 Seeds = ArgCount > 2 ? (u32)atoi(Args[2]) : 4;
    TestFireballBurstsOnWall();
    printf("soak: %u seeds x %u simulated minutes, 8 players\n", Seeds, Minutes);
    for (u32 Seed = 1; Seed <= Seeds; ++Seed)
    {
        SoakOneSeed(Seed, Minutes);
    }
    printf("soak tests: %s\n", TestFailures ? "FAILED" : "all seeds passed");
    return TestFailures ? 1 : 0;
}
