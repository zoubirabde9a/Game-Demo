/* The real game behind game_api.h: the same simulation the client runs
   (code/sim), built without a window. Network slot N drives player slot N.

   Inputs arrive as held buttons; the simulation wants a move direction
   plus the buttons pressed this tick. A press is a button held now that
   was not held in the previous input. Presses from every input that
   arrives between two ticks are kept, so a tap shorter than a tick still
   fires once. */

#include "../app_sim.cpp" // the simulation without the client or UI
#include "event_relay.cpp"
#include "bots.cpp"

#define SIM_GAME_MEMORY Megabytes(64)
struct server_game
{
    app_state *AppState;
    memory_arena *Arena;
    u16 HeldButtons[NET_MAX_CLIENTS];
    u32 NameTurn; // which slot's name the next snapshots carry
    u32 LastInputTick[NET_MAX_CLIENTS]; // newest input applied per slot
    // Sounds and deaths on their way to the clients (event_relay.cpp).
    event_relay Relay;
    // How many bots to keep in slots no human uses, and their brains
    // (bots.cpp).
    u32 BotTarget;
    bot_brain Bots[MAX_PLAYERS];
};

internal void
GameInit(server_game *Game, u32 MapId)
{
    // app_state is large and the arena lives right after it, as in the client.
    void *Memory = calloc(1, SIM_GAME_MEMORY);
    Assert(Memory);
    app_state *AppState = (app_state *)Memory;
    InitializeArena(&AppState->MemoryArena, (memory_index *)(AppState + 1),
                    SIM_GAME_MEMORY - sizeof(app_state));
    SubArena(&AppState->ConstantsArena, &AppState->MemoryArena, Kilobytes(64));
    AppState->World.MapId = MapId;
    InitSimulation(AppState, &AppState->MemoryArena, &AppState->ConstantsArena);
    AppState->IsInitialized = true;

    *Game = {};
    Game->AppState = AppState;
    Game->Arena = &AppState->MemoryArena;
}

internal u32
GameContentId(server_game *Game)
{
    return SimContentId();
}

internal void
GameShutdown(server_game *Game)
{
    free(Game->AppState);
    *Game = {};
}

internal void
GamePlayerJoined(server_game *Game, u32 Slot)
{
    Game->Bots[Slot].Active = false;
    RelayJoined(&Game->Relay, Slot);
    app_state *AppState = Game->AppState;
    RemovePlayerFromSlot(AppState, &AppState->World, Slot);
    AddPlayerToSlot(AppState, &AppState->World, Game->Arena, Slot,
                    PlayerSpawnPosition(&AppState->World, Slot));
    Game->HeldButtons[Slot] = 0;
    Game->LastInputTick[Slot] = 0;
}

internal void
GamePlayerLeft(server_game *Game, u32 Slot)
{
    RemovePlayerFromSlot(Game->AppState, &Game->AppState->World, Slot);
    Game->HeldButtons[Slot] = 0;
    Game->LastInputTick[Slot] = 0;
}

internal void
GameApplyInput(server_game *Game, u32 Slot, net_input *Input)
{
    player_slot *Player = &Game->AppState->Players[Slot];
    if (!Player->Active) return;

    u16 Held = Input->Buttons;
    u16 Pressed = Held & ~Game->HeldButtons[Slot];
    Game->HeldButtons[Slot] = Held;

    Game->LastInputTick[Slot] = Input->Tick;
    player_input *Out = &Player->Input;
    Out->Move = {};
    if (Held & NetButton_Left) Out->Move.X -= 1.f;
    if (Held & NetButton_Right) Out->Move.X += 1.f;
    if (Held & NetButton_Up) Out->Move.Y -= 1.f;
    if (Held & NetButton_Down) Out->Move.Y += 1.f;
    Out->Aim = V2(Input->AimX, Input->AimY);

    if (Pressed & NetButton_Sword) Out->Pressed |= PlayerButton_Attack;
    if (Pressed & NetButton_Fireball) Out->Pressed |= PlayerButton_Cast;
    if (Pressed & NetButton_Jump) Out->Pressed |= PlayerButton_Jump;
    if (Pressed & NetButton_Dash) Out->Pressed |= PlayerButton_Dash;
    if (Pressed & NetButton_Shockwave) Out->Pressed |= PlayerButton_Shockwave;
    if (Pressed & NetButton_Blink) Out->Pressed |= PlayerButton_Blink;
}

// Whether another connected player already goes by Name (ignoring case).
internal bool32
SimGameNameTaken(server_game *Game, u32 Slot, char *Name)
{
    for (u32 Other = 0; Other < MAX_PLAYERS; ++Other)
    {
        player_slot *Player = &Game->AppState->Players[Other];
        if (Other == Slot || !Player->Active || !Player->Name[0]) continue;
        u32 Index = 0;
        for (; Name[Index] && Player->Name[Index]; ++Index)
        {
            char A = Name[Index], B = Player->Name[Index];
            if (A >= 'A' && A <= 'Z') A += 'a' - 'A';
            if (B >= 'A' && B <= 'Z') B += 'a' - 'A';
            if (A != B) break;
        }
        if (!Name[Index] && !Player->Name[Index]) return true;
    }
    return false;
}

// A name already in use gets " 2", " 3"... so the scoreboard and the kill
// feed can tell players apart; the base is shortened to make room.
internal void
GamePlayerNamed(server_game *Game, u32 Slot, char *Name)
{
    char *Out = Game->AppState->Players[Slot].Name;
    u32 Size = sizeof(Game->AppState->Players[Slot].Name);
    u32 Length = 0;
    for (; Name[Length] && Length + 1 < Size; ++Length)
    {
        Out[Length] = Name[Length];
    }
    Out[Length] = 0;
    for (u32 Number = 2; Out[0] && SimGameNameTaken(Game, Slot, Out) && Number <= MAX_PLAYERS; ++Number)
    {
        char Suffix[8];
        snprintf(Suffix, sizeof(Suffix), " %u", Number);
        u32 SuffixLength = (u32)strlen(Suffix);
        u32 Base = (Length + SuffixLength + 1 <= Size) ? Length : Size - 1 - SuffixLength;
        for (u32 Index = 0; Index <= SuffixLength; ++Index) Out[Base + Index] = Suffix[Index];
    }
}

internal void
GameTick(server_game *Game, float Dt)
{
    app_state *AppState = Game->AppState;
    SimulateTick(AppState, Game->Arena, Dt);
    Game->NameTurn = (Game->NameTurn + 1) % MAX_PLAYERS;

    // Presses fire once; movement stays until the next input changes it.
    for (u32 Index = 0; Index < MAX_PLAYERS; ++Index)
    {
        AppState->Players[Index].Input.Pressed = 0;
    }
    // Sounds and deaths are for clients; keep them for the snapshots.
    RelayKeep(&Game->Relay, &AppState->Events);
    AppState->Events = {};
}

// NOTE(zoubir): packing single entities into wire fields, and keeping
// the nearest; GameWriteSnapshot below assembles the snapshot
#include "sim_game/pack.cpp"

internal void
GameWriteSnapshot(server_game *Game, u32 ViewerSlot, net_snapshot *Out)
{
    world *World = &Game->AppState->World;
    Out->Count = 0;
    Out->FacingCount = 0;
    Out->InputTick = Game->LastInputTick[ViewerSlot];
    // The viewer's own cooldowns, for its HUD (sim/player_cooldowns.cpp).
    static_assert(PLAYER_COOLDOWN_COUNT == NET_COOLDOWN_COUNT, "one byte per cooldown");
    world_entity *Own = Game->AppState->Players[ViewerSlot].Entity;
    for (u32 Index = 0; Index < NET_COOLDOWN_COUNT; ++Index)
    {
        float Full;
        float *Seconds = Own ? PlayerCooldown(Own, Index, &Full) : 0;
        Out->Cooldowns[Index] = Seconds ? CooldownToByte(*Seconds, Full) : 0;
    }
    Out->AbilityCount = 0;

    // The viewer's own player goes first so it is never cut off by the
    // entity limit; the rest follow nearest first, so when more is going on
    // than fits, what gets left out is what is farthest away.
    player_slot *Viewer = &Game->AppState->Players[ViewerSlot];
    world_entity *First = Viewer->Active ? Viewer->Entity : 0;
    v2 Center = First ? First->Position.XY : V2(0.f);
    if (First && SimGameIsSent(First))
    {
        SimGameWriteEntity(First, (u16)First->ID, Out);
    }

    u32 Room = NET_MAX_SNAPSHOT_ENTITIES - Out->Count;
    sim_game_candidate Nearest[NET_MAX_SNAPSHOT_ENTITIES];
    u32 NearestCount = 0;
    for (u32 Index = 0; Index < World->EntityCount; ++Index)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity == First || !SimGameIsSent(Entity)) continue;
        sim_game_candidate Candidate = {Index, LengthSq(Entity->Position.XY - Center)};
        SimGameKeepNearest(Nearest, &NearestCount, Room, Candidate);
    }
    for (u32 Rank = 0; Rank < NearestCount; ++Rank)
    {
        u32 Index = Nearest[Rank].Index;
        SimGameWriteEntity(&World->Entities[Index], (u16)Index, Out);
    }

    // One connected player's name, the next one each tick.
    Out->NameSlot = NET_NO_NAME_SLOT;
    for (u32 Step = 0; Step < MAX_PLAYERS; ++Step)
    {
        u32 Slot = (Game->NameTurn + Step) % MAX_PLAYERS;
        player_slot *Player = &Game->AppState->Players[Slot];
        if (!Player->Active) continue;
        Out->NameSlot = (u8)Slot;
        for (u32 Index = 0; Index < NET_NAME_SIZE; ++Index) Out->Name[Index] = Player->Name[Index];
        break;
    }

    // Every connected player's score, so each client can show the scoreboard.
    Out->ScoreCount = 0;
    for (u32 Slot = 0; Slot < MAX_PLAYERS && Out->ScoreCount < NET_MAX_SNAPSHOT_SCORES; ++Slot)
    {
        player_slot *Player = &Game->AppState->Players[Slot];
        if (!Player->Active) continue;
        net_score *Score = &Out->Scores[Out->ScoreCount++];
        Score->Slot = (u8)Slot;
        Score->Kills = (u16)Player->Kills;
        Score->Deaths = (u16)Player->Deaths;
        Score->MonsterKills = (u16)Player->MonsterKills;
    }

    RelayWrite(&Game->Relay, ViewerSlot, First != 0, Center, Out);
}

internal void
GameListPlayers(server_game *Game, net_info_reply *Out)
{
    Out->NameCount = 0;
    for (u32 Slot = 0; Slot < MAX_PLAYERS && Out->NameCount < NET_MAX_SNAPSHOT_SCORES; ++Slot)
    {
        player_slot *Player = &Game->AppState->Players[Slot];
        if (!Player->Active) continue;
        char *Name = Out->Names[Out->NameCount++];
        if (Player->Name[0]) snprintf(Name, NET_NAME_SIZE, "%s", Player->Name);
        else snprintf(Name, NET_NAME_SIZE, "Player %u", Slot + 1);
    }
}

// Before each tick: keep BotTarget bots in the slots no human is connected
// to (bit N of ConnectedSlots set = a human has slot N), highest slots
// first so humans, who join the lowest free slot, rarely displace one;
// then give each bot this tick's input.
internal void
GameKeepBots(server_game *Game, u32 ConnectedSlots, float Dt)
{
    app_state *AppState = Game->AppState;
    u32 BotCount = 0;
    for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
    {
        if ((ConnectedSlots >> Slot) & 1) Game->Bots[Slot].Active = false;
        if (Game->Bots[Slot].Active) ++BotCount;
    }
    for (u32 Step = 0; Step < MAX_PLAYERS; ++Step)
    {
        u32 Slot = MAX_PLAYERS - 1 - Step;
        bool32 Human = (ConnectedSlots >> Slot) & 1;
        bot_brain *Bot = &Game->Bots[Slot];
        if (BotCount < Game->BotTarget && !Human && !Bot->Active &&
            !AppState->Players[Slot].Active)
        {
            GamePlayerJoined(Game, Slot);
            char Name[NET_NAME_SIZE];
            snprintf(Name, sizeof(Name), "Bot %u", Slot + 1);
            GamePlayerNamed(Game, Slot, Name);
            *Bot = {};
            Bot->Active = true;
            Bot->Random = 0x9e3779b9u * (Slot + 1) ^ Game->Relay.Tick;
            ++BotCount;
        }
        else if (BotCount > Game->BotTarget && Bot->Active)
        {
            GamePlayerLeft(Game, Slot);
            Bot->Active = false;
            --BotCount;
        }
    }
    for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
    {
        bot_brain *Bot = &Game->Bots[Slot];
        if (!Bot->Active) continue;
        world_entity *Self = AppState->Players[Slot].Entity;
        net_input Input = BotThink(Bot, AppState, Self, Game->Relay.Tick + 1, Dt);
        GameApplyInput(Game, Slot, &Input);
    }
}
