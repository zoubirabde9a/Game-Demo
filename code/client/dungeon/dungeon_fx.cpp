/* Dungeon effects drawn over the world (sim/dungeon/): what the run
   changes on the ground and on the players, so it can be seen.

   - Closed gates: iron bars standing on every gate tile while its walls
     are up (encounters.cpp), glowing red while the room behind is being
     fought.
   - Sanctuaries: a healer's circle (role_abilities.cpp), a ring of gold
     on the ground with motes rising inside, fading as it runs out.
   - Shield Wall and Ward: a ring of pale plates round a tank behind
     Shield Wall, and a blue ring round anyone a healer's ward still
     covers.
   - Revives: a green ring round a downed player, filling as a healer
     stands over them (revive.cpp).

   It reads the run and the player slots: the simulation fills them
   offline, the snapshots online (client/dungeon/dungeon_net.cpp). */

#define GATE_BAR_HEIGHT 44.f
#define GATE_BARS_PER_TILE 4
#define GATE_IRON_RGB 0x00807870
#define GATE_SHADOW_RGB 0x00141210
#define GATE_GLOW_RGB 0x003040E0
#define SANCTUARY_RGB 0x0060E0F0
#define SANCTUARY_DOTS 28
#define SHIELD_WALL_RGB 0x00F0E0D0
#define WARD_RGB 0x00FFC070
#define REVIVE_RGB 0x0070F090

// NOTE(zoubir): a world point on the ground (Z up) in the overlay's space
inline v2
DungeonFxPoint(v3 World, v3 CameraOffset)
{
    v2 Result = BurstToScreen(World, CameraOffset);
    return Result;
}

// NOTE(zoubir): the part of X after the point, 0..1
inline float
DungeonFxFraction(float X)
{
    float Result = X - floorf(X);
    return Result;
}

// NOTE(zoubir): a ring of Dots round Centre on the ground, the share
// Filled of it lit (1 for all)
internal void
DrawDungeonRing(render_context *RenderContext, v3 Centre, float Radius, u32 Dots,
                float Filled, float Size, u32 Color, v3 CameraOffset)
{
    u32 Lit = (u32)(Filled * (float)Dots + 0.5f);
    for(u32 Dot = 0; Dot < Lit; Dot++)
    {
        float Angle = -0.5f * Pi32 + 2.f * Pi32 * (float)Dot / (float)Dots;
        v3 Point = Centre + V3(Radius * Cos(Angle), Radius * Sin(Angle), 0.f);
        DrawFxDot(RenderContext, DungeonFxPoint(Point, CameraOffset), Size, Color);
    }
}

internal void
DrawDungeonGates(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    float Clock = GetFxClock(AppState);
    float Tile = (float)World->TileWidth;
    for(u32 Gate = 0; Gate < DUNGEON_MAX_GATES; Gate++)
    {
        // NOTE(zoubir): the room on the far side is the one being fought
        bool32 Locked = Run->FightingRoom == Gate + 1 || Run->FightingRoom == Gate + 2;
        // NOTE(zoubir): bars stand across each tile from side to side; in
        // the three-quarter view a gate running north to south would put
        // bars spread along it on top of each other
        v3 Along = V3(1.f, 0.f, 0.f);
        for(u32 Index = 0; Index < DUNGEON_GATE_TILES && Run->GateWalls[Gate][Index]; Index++)
        {
            world_entity *Wall = &World->Entities[Run->GateWalls[Gate][Index] - 1];
            if (!Wall->IsPresent)
            {
                continue;
            }
            for(u32 Bar = 0; Bar < GATE_BARS_PER_TILE; Bar++)
            {
                float Offset = Tile * (((float)Bar + 0.5f) / GATE_BARS_PER_TILE - 0.5f);
                v3 Foot = Wall->Position + Offset * Along;
                v3 Top = Foot + V3(0.f, 0.f, GATE_BAR_HEIGHT);
                v2 A = DungeonFxPoint(Foot, CameraOffset);
                v2 B = DungeonFxPoint(Top, CameraOffset);
                DrawFxStreak(RenderContext, A, B, 7.f, FxColor(1.f, GATE_SHADOW_RGB),
                             FxColor(1.f, GATE_SHADOW_RGB), RenderBlend_Alpha);
                DrawFxStreak(RenderContext, A, B, 4.f, FxColor(1.f, GATE_IRON_RGB),
                             FxColor(1.f, GATE_IRON_RGB), RenderBlend_Alpha);
                if (Locked)
                {
                    float Pulse = 0.35f + 0.25f * Sin(4.f * Clock + (float)Index);
                    DrawFxStreak(RenderContext, A, B, 2.f, FxColor(Pulse, GATE_GLOW_RGB),
                                 FxColor(0.f, GATE_GLOW_RGB));
                }
            }
            // NOTE(zoubir): a crossbar near the top
            v3 Left = Wall->Position - (0.5f * Tile) * Along + V3(0.f, 0.f, GATE_BAR_HEIGHT - 8.f);
            v3 Right = Wall->Position + (0.5f * Tile) * Along + V3(0.f, 0.f, GATE_BAR_HEIGHT - 8.f);
            DrawFxStreak(RenderContext, DungeonFxPoint(Left, CameraOffset),
                         DungeonFxPoint(Right, CameraOffset), 3.f,
                         FxColor(1.f, GATE_IRON_RGB), FxColor(1.f, GATE_IRON_RGB),
                         RenderBlend_Alpha);
        }
    }
}

internal void
DrawSanctuaries(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    dungeon_run *Run = AppState->Dungeon;
    float Clock = GetFxClock(AppState);
    for(u32 Index = 0; Index < MAX_SANCTUARIES; Index++)
    {
        sanctuary *Zone = &Run->Sanctuaries[Index];
        if (Zone->Seconds <= 0.f)
        {
            continue;
        }
        float Fade = Clamp01(Zone->Seconds / 0.6f);
        DrawDungeonRing(RenderContext, Zone->Position, SANCTUARY_RADIUS, SANCTUARY_DOTS,
                        1.f, 3.f, FxColor(0.8f * Fade, SANCTUARY_RGB), CameraOffset);
        // NOTE(zoubir): motes rising inside, each on its own slow loop
        for(u32 Mote = 0; Mote < 10; Mote++)
        {
            float Seed = (float)Mote * 2.39996f;
            float Rise = DungeonFxFraction(0.5f * Clock + 0.1f * (float)Mote);
            float R = SANCTUARY_RADIUS * (0.2f + 0.7f * DungeonFxFraction(Seed * 0.37f));
            v3 Point = Zone->Position + V3(R * Cos(Seed), R * Sin(Seed), 30.f * Rise);
            DrawFxDot(RenderContext, DungeonFxPoint(Point, CameraOffset), 2.5f,
                      FxColor(Fade * (1.f - Rise), SANCTUARY_RGB));
        }
    }
}

internal void
DrawPartyMarks(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    float Clock = GetFxClock(AppState);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = Slot->Entity;
        if (!Slot->Active || !Player || !Player->IsPresent)
        {
            continue;
        }
        if (IsDeadPlayer(Player))
        {
            if (Slot->ReviveSeconds > 0.f)
            {
                DrawDungeonRing(RenderContext, Player->Position, 26.f, 20,
                                Clamp01(Slot->ReviveSeconds / REVIVE_SECONDS), 3.f,
                                FxColor(0.9f, REVIVE_RGB), CameraOffset);
            }
            continue;
        }
        if (Slot->ShieldWallSeconds > 0.f)
        {
            float Turn = 1.5f * Clock;
            for(u32 Plate = 0; Plate < 6; Plate++)
            {
                float Angle = Turn + 2.f * Pi32 * (float)Plate / 6.f;
                v3 Point = Player->Position + V3(22.f * Cos(Angle), 22.f * Sin(Angle), 14.f);
                DrawFxDot(RenderContext, DungeonFxPoint(Point, CameraOffset), 4.f,
                          FxColor(0.85f, SHIELD_WALL_RGB));
            }
        }
        if (Slot->WardAbsorb > 0.f)
        {
            DrawDungeonRing(RenderContext, Player->Position + V3(0.f, 0.f, 16.f), 20.f, 16,
                            Clamp01(Slot->WardAbsorb / WARD_ABSORB), 2.5f,
                            FxColor(0.8f, WARD_RGB), CameraOffset);
        }
    }
}

// NOTE(zoubir): from the screen pass, with the world overlays
internal void
DrawDungeonFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    if (!IsDungeon(AppState))
    {
        return;
    }
    DrawDungeonGates(RenderContext, AppState, CameraOffset);
    DrawSanctuaries(RenderContext, AppState, CameraOffset);
    DrawPartyMarks(RenderContext, AppState, CameraOffset);
}
