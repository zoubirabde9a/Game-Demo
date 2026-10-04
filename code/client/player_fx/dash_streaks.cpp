/* Dash streaks: while a player's dash flag is on (DashFlash locally, the
   PLAYER_FLASH_DASH bit the server sends for replicas; dash and blink both
   set it), dots are laid every DASH_STREAK_SPACING units along the path
   it covered since the last frame, so a blink's jump or a replica's step
   between snapshots reads as one line. Each dot shrinks and fades over
   DASH_STREAK_SECONDS.

   Afterimages: every DASH_GHOST_INTERVAL of a dash the player's sprite,
   on the frame it shows then, is left behind as a tinted ghost that
   fades over DASH_GHOST_SECONDS. */

// NOTE(zoubir): a full blink lays 40 dots, a dash about 15
#define MAX_DASH_DOTS 256
#define DASH_STREAK_SPACING 8.f
#define DASH_STREAK_SECONDS 0.25f
#define DASH_STREAK_RGB 0x00FFF0D0
#define MAX_DASH_GHOSTS 24
#define DASH_GHOST_INTERVAL 0.03f
#define DASH_GHOST_SECONDS 0.22f
// NOTE(zoubir): 0x00BBGGRR, a cold blue so ghosts read apart from bodies
#define DASH_GHOST_RGB 0x00FFE0C0

struct dash_ghost
{
    v3 Position;
    v2 Dimensions;
    v4 Uvs;
    asset_id Texture;
    float Age;
};

struct dash_dot
{
    v2 Position;
    float Age;
};

struct dash_streaks
{
    dash_dot Dots[MAX_DASH_DOTS];
    u32 Count;
    // NOTE(zoubir): where each slot's player was last frame, while dashing
    bool32 WasOn[MAX_PLAYERS];
    v2 Last[MAX_PLAYERS];
    dash_ghost Ghosts[MAX_DASH_GHOSTS];
    u32 GhostCount;
    // NOTE(zoubir): seconds until each slot's player leaves its next ghost
    float GhostTimer[MAX_PLAYERS];
};

// NOTE(zoubir): a full pool skips the ghost; the next one is 0.03 s away
inline void
AddDashGhost(dash_streaks *Fx, world_entity *Player)
{
    if (Fx->GhostCount < MAX_DASH_GHOSTS && Player->Texture.Type)
    {
        dash_ghost *Ghost = &Fx->Ghosts[Fx->GhostCount++];
        Ghost->Position = Player->Position;
        Ghost->Dimensions = Player->Dimensions;
        Ghost->Uvs = Player->Uvs;
        Ghost->Texture = Player->Texture;
        Ghost->Age = 0.f;
    }
}

inline void
AddDashDot(dash_streaks *Fx, v2 Position)
{
    if (Fx->Count < MAX_DASH_DOTS)
    {
        dash_dot *Dot = &Fx->Dots[Fx->Count++];
        Dot->Position = Position;
        Dot->Age = 0.f;
    }
}

inline bool32
IsDashShowing(world_entity *Player)
{
    bool32 Result = Player->DashFlash > 0.f ||
        (Player->AbilityIndex & PLAYER_FLASH_DASH);
    return Result;
}

internal void
UpdateDashStreaks(dash_streaks *Fx, app_state *AppState, float DeltaTime)
{
    for(u32 Index = 0; Index < Fx->Count;)
    {
        dash_dot *Dot = &Fx->Dots[Index];
        Dot->Age += DeltaTime;
        if (Dot->Age >= DASH_STREAK_SECONDS)
        {
            *Dot = Fx->Dots[--Fx->Count];
        }
        else
        {
            Index++;
        }
    }

    for(u32 Index = 0; Index < Fx->GhostCount;)
    {
        dash_ghost *Ghost = &Fx->Ghosts[Index];
        Ghost->Age += DeltaTime;
        if (Ghost->Age >= DASH_GHOST_SECONDS)
        {
            *Ghost = Fx->Ghosts[--Fx->GhostCount];
        }
        else
        {
            Index++;
        }
    }

    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = Slot->Entity;
        bool32 On = Slot->Active && Player && Player->IsPresent &&
            IsDashShowing(Player);
        if (On)
        {
            v2 Now = Player->Position.XY;
            v2 From = Fx->WasOn[SlotIndex] ? Fx->Last[SlotIndex] : Now;
            float Distance = Length(Now - From);
            u32 Steps = (u32)(Distance / DASH_STREAK_SPACING);
            Steps = Steps > 32 ? 32 : Steps;
            for(u32 Step = 1; Step <= Steps; Step++)
            {
                AddDashDot(Fx, Lerp2(From, (float)Step / (float)(Steps + 1), Now));
            }
            // NOTE(zoubir): a dash stopped short (by a monster, a wall) stays
            // put, and a dot a frame there piled up into a white block; one
            // that goes nowhere at all (a blink into a unit) leaves none
            if (Distance >= 0.5f * DASH_STREAK_SPACING)
            {
                AddDashDot(Fx, Now);
            }
            Fx->Last[SlotIndex] = Now;
            Fx->GhostTimer[SlotIndex] -= DeltaTime;
            if (Fx->GhostTimer[SlotIndex] <= 0.f)
            {
                AddDashGhost(Fx, Player);
                Fx->GhostTimer[SlotIndex] = DASH_GHOST_INTERVAL;
            }
        }
        else
        {
            Fx->GhostTimer[SlotIndex] = 0.f;
        }
        Fx->WasOn[SlotIndex] = On;
    }
}

// NOTE(zoubir): the sprite as it was, placed as DrawEntity places it
internal void
DrawDashGhosts(render_context *RenderContext, app_state *AppState,
               dash_streaks *Fx, v3 CameraOffset)
{
    assets *Assets = &AppState->Assets;
    for(u32 Index = 0; Index < Fx->GhostCount; Index++)
    {
        dash_ghost *Ghost = &Fx->Ghosts[Index];
        loaded_texture *Texture = GetTexture(Assets, AppState->OpenGL, AppState,
                                             Ghost->Texture);
        if (!Texture)
        {
            continue;
        }
        zas_texture_info *Info = &GetAssetInfo(Assets, Ghost->Texture)->Texture;
        v2 P = Ghost->Position.XY - CameraOffset.XY -
            Info->Origin * Ghost->Dimensions;
        P.Y -= Ghost->Position.Z;
        float Life = 1.f - Ghost->Age / DASH_GHOST_SECONDS;
        u32 Color = ((u32)(210.f * Life) << 24) | DASH_GHOST_RGB;
        // NOTE(zoubir): just under a player standing in the same spot
        float Ground = TerrainHeightAt(&AppState->World, Ghost->Position.X,
                                       Ghost->Position.Y);
        BeginBatch(RenderContext, Texture->ID,
                   StandingSortKey(Ghost->Position.Y, Ground) - 0.5f,
                   RenderContext->TextureProgram);
        RenderQuadTexture(RenderContext, P.X, P.Y, Ghost->Dimensions.X,
                          Ghost->Dimensions.Y, Ghost->Uvs, Color,
                          Ghost->Position.Z);
        EndBatch(RenderContext);
    }
}

// NOTE(zoubir): a wide soft dot with a bright core, at hip height so the
// streak runs through the body rather than under it
internal void
DrawDashStreaks(render_context *RenderContext, dash_streaks *Fx,
                v3 CameraOffset)
{
    for(u32 Index = 0; Index < Fx->Count; Index++)
    {
        dash_dot *Dot = &Fx->Dots[Index];
        float Life = 1.f - Dot->Age / DASH_STREAK_SECONDS;
        v2 P = Dot->Position - CameraOffset.XY - V2(0.f, 12.f);
        u32 Soft = ((u32)(150.f * Life) << 24) | DASH_STREAK_RGB;
        u32 Core = ((u32)(230.f * Life) << 24) | DASH_STREAK_RGB;
        DrawFxDot(RenderContext, P, 5.f + 10.f * Life, Soft);
        DrawFxDot(RenderContext, P, 2.f + 3.f * Life, Core);
    }
}
