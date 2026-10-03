/* The real game behind game_api.h: the same simulation the client runs
   (code/sim), built without a window. Network slot N drives player slot N.

   Inputs arrive as held buttons; the simulation wants a move direction
   plus the buttons pressed this tick. A press is a button held now that
   was not held in the previous input. Presses from every input that
   arrives between two ticks are kept, so a tap shorter than a tick still
   fires once. */

#include "../app.cpp"

#define SIM_GAME_MEMORY Megabytes(64)

struct server_game
{
    app_state *AppState;
    memory_arena *Arena;
    u16 HeldButtons[NET_MAX_CLIENTS];
};

internal void
GameInit(server_game *Game)
{
    // app_state is large and the arena lives right after it, as in the client.
    void *Memory = calloc(1, SIM_GAME_MEMORY);
    Assert(Memory);
    app_state *AppState = (app_state *)Memory;
    InitializeArena(&AppState->MemoryArena, (memory_index *)(AppState + 1),
                    SIM_GAME_MEMORY - sizeof(app_state));
    SubArena(&AppState->ConstantsArena, &AppState->MemoryArena, Kilobytes(64));
    InitSimulation(AppState, &AppState->MemoryArena, &AppState->ConstantsArena);
    AppState->IsInitialized = true;

    *Game = {};
    Game->AppState = AppState;
    Game->Arena = &AppState->MemoryArena;
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
    app_state *AppState = Game->AppState;
    RemovePlayerFromSlot(AppState, &AppState->World, Slot);
    AddPlayerToSlot(AppState, &AppState->World, Game->Arena, Slot,
                    PlayerSpawnPosition(Slot));
    Game->HeldButtons[Slot] = 0;
}

internal void
GamePlayerLeft(server_game *Game, u32 Slot)
{
    RemovePlayerFromSlot(Game->AppState, &Game->AppState->World, Slot);
    Game->HeldButtons[Slot] = 0;
}

internal void
GameApplyInput(server_game *Game, u32 Slot, net_input *Input)
{
    player_slot *Player = &Game->AppState->Players[Slot];
    if (!Player->Active) return;

    u16 Held = Input->Buttons;
    u16 Pressed = Held & ~Game->HeldButtons[Slot];
    Game->HeldButtons[Slot] = Held;

    player_input *Out = &Player->Input;
    Out->Move = {};
    if (Held & NetButton_Left) Out->Move.X -= 1.f;
    if (Held & NetButton_Right) Out->Move.X += 1.f;
    if (Held & NetButton_Up) Out->Move.Y -= 1.f;
    if (Held & NetButton_Down) Out->Move.Y += 1.f;

    if (Pressed & NetButton_Sword) Out->Pressed |= PlayerButton_Attack;
    if (Pressed & NetButton_Fireball) Out->Pressed |= PlayerButton_Cast;
    if (Pressed & NetButton_Jump) Out->Pressed |= PlayerButton_Jump;
    if (Pressed & NetButton_Dash) Out->Pressed |= PlayerButton_Dash;
    if (Pressed & NetButton_Shockwave) Out->Pressed |= PlayerButton_Shockwave;
}

internal void
GameTick(server_game *Game, float Dt)
{
    app_state *AppState = Game->AppState;
    SimulateTick(AppState, Game->Arena, Dt);

    // Presses fire once; movement stays until the next input changes it.
    for (u32 Index = 0; Index < MAX_PLAYERS; ++Index)
    {
        AppState->Players[Index].Input.Pressed = 0;
    }
    // Sounds are for clients to play; the server has no speakers.
    AppState->Events = {};
}

// NOTE(zoubir): what the client needs, beyond the type, to pick the
// sprite: the monster's kind, or the shot's style (kept in its texture)
inline u8
SimGameVariant(world_entity *Entity)
{
    u8 Result = 0;
    if (Entity->Type == EntityType_Monster)
    {
        Result = (u8)Entity->MonsterKind;
    }
    else if (Entity->Type == EntityType_MonsterShot)
    {
        Result = (u8)Entity->Texture.Index;
    }
    return Result;
}

internal bool32
SimGameIsSent(world_entity *Entity)
{
    switch (Entity->Type)
    {
        case EntityType_Player:
        case EntityType_Monster:
        case EntityType_FireBall:
        case EntityType_Sword:
        case EntityType_Familiar:
        case EntityType_MonsterShot:
        case EntityType_MonsterHazard:
            return Entity->IsPresent;
        default:
            return false;
    }
}

// Monsters winding up or striking get an ability entry, so clients can
// draw the warning. Ready and recovering monsters have nothing to show.
internal void
SimGameWriteAbility(world_entity *Entity, u8 EntityIndex, net_snapshot *Out)
{
    if (Entity->Type != EntityType_Monster) return;
    if (Entity->AbilityPhase != AbilityPhase_Windup &&
        Entity->AbilityPhase != AbilityPhase_Active) return;
    if (Out->AbilityCount >= NET_MAX_SNAPSHOT_ABILITIES) return;

    net_ability_state *A = &Out->Abilities[Out->AbilityCount++];
    *A = {};
    A->EntityIndex = EntityIndex;
    A->Phase = (u8)Entity->AbilityPhase;
    A->Ability = (u8)Entity->AbilityIndex;
    A->TimeLeft = Entity->AbilityTimer;
    A->AimX = Entity->AbilityAim.X;
    A->AimY = Entity->AbilityAim.Y;
    A->PointCount = (u8)Minimum(Entity->AbilityPointCount, (u32)NET_MAX_ABILITY_POINTS);
    for (u32 Index = 0; Index < A->PointCount; ++Index)
    {
        A->PointX[Index] = Entity->AbilityPoints[Index].X;
        A->PointY[Index] = Entity->AbilityPoints[Index].Y;
    }
}

internal void
GameWriteSnapshot(server_game *Game, u32 ViewerSlot, net_snapshot *Out)
{
    world *World = &Game->AppState->World;
    Out->Count = 0;
    Out->AbilityCount = 0;

    // The viewer's own player goes first so it is never cut off by the
    // entity limit. TODO: when the limit is hit, prefer what is near the viewer.
    player_slot *Viewer = &Game->AppState->Players[ViewerSlot];
    world_entity *First = Viewer->Active ? Viewer->Entity : 0;

    for (u32 Pass = 0; Pass < 2; ++Pass)
    {
        for (u32 Index = 0; Index < World->EntityCount && Out->Count < NET_MAX_SNAPSHOT_ENTITIES; ++Index)
        {
            world_entity *Entity = &World->Entities[Index];
            if (!SimGameIsSent(Entity)) continue;
            if ((Pass == 0) != (Entity == First)) continue;

            net_entity_state *E = &Out->Entities[Out->Count++];
            E->Id = (u16)Index;
            E->Type = (u8)Entity->Type;
            E->Facing = (u8)Entity->AnimationState.LastAnimationDirection;
            E->Animation = (u8)Entity->AnimationState.CurrentType;
            E->Variant = SimGameVariant(Entity);
            E->Health = (i16)Entity->Hp;
            E->X = Entity->Position.X;
            E->Y = Entity->Position.Y;
            E->Z = Entity->Position.Z;
            E->VelX = Entity->Velocity.X;
            E->VelY = Entity->Velocity.Y;

            SimGameWriteAbility(Entity, (u8)(Out->Count - 1), Out);
        }
    }
}
