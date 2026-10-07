/* World lights: the coloured light that players' lanterns (on dark maps),
   bodies under a glowing status (on fire, poisoned...), wall torches,
   fireballs, monster shots, kunai, bursts (a hit, a nova, a level up) and
   lava throw on the ground and the bodies near them. Gathered once a frame
   into a short list in window pixels; the world grade shader
   (build/shaders/fx/world_grade.frag) brightens and tints every pixel
   within reach of each one, so a fireball lights the grass it flies over.

   Moving things come first, then bursts, then torches, then lava; past
   WORLD_LIGHTS_MAX a new light replaces the one that matters least
   (LightWorth: weak, small, far from the middle of the screen). A burst
   flashes in its own colour (fx_bursts.cpp) and fades over its life; dust
   and targeting marks give no light. Lava is summed over blocks of
   WORLD_LIGHT_LAVA_BLOCK tiles aligned to the map, so a block's light stays
   put while the camera moves and only its edge ones come and go. */

#define WORLD_LIGHTS_MAX 32
#define WORLD_LIGHT_LAVA_BLOCK 3

struct world_lights
{
    u32 Count;
    // NOTE(zoubir): per light, x and y in window pixels from the bottom
    // left (as gl_FragCoord), radius in window pixels, strength
    float Spot[4 * WORLD_LIGHTS_MAX];
    // NOTE(zoubir): per light, r g b and a flicker (0 steady, 1 fire)
    float Color[4 * WORLD_LIGHTS_MAX];
    // NOTE(zoubir): how much each light matters (LightWorth); when the list
    // is full a new light takes the place of the least one, if it is worth
    // more
    float Worth[WORLD_LIGHTS_MAX];
    // NOTE(zoubir): the middle of the window, in the same pixels as Spot
    v2 Middle;
};

// NOTE(zoubir): bright, wide lights near the middle of the screen matter
// most; one far out at the edge, little
inline float
LightWorth(world_lights *Lights, float X, float Y, float Radius, float Strength)
{
    float Away = Length(V2(X, Y) - Lights->Middle);
    float Result = Strength * Radius / (Radius + Away);
    return Result;
}

struct world_light_look
{
    float Radius;   // world units
    float Strength;
    v3 Color;
    float Flicker;
};

// NOTE(zoubir): one row per kind of thing that gives light
global_variable world_light_look FireballLight = {130.f, 0.8f, {1.0f, 0.55f, 0.22f}, 1.f};
global_variable world_light_look MonsterShotLight = {90.f, 0.7f, {1.0f, 0.45f, 0.55f}, 0.5f};
global_variable world_light_look KunaiLight = {45.f, 0.35f, {0.70f, 0.85f, 1.0f}, 0.f};
// NOTE(zoubir): a burst lights this many times its own radius, at least
// that of a BURST_LIGHT_MIN_RADIUS one, starting at this strength
#define BURST_LIGHT_REACH 2.2f
#define BURST_LIGHT_MIN_RADIUS 30.f
#define BURST_LIGHT_STRENGTH 0.9f
// NOTE(zoubir): a lantern, carried by every living player on a map whose
// mood asks for one (map_moods.cpp); its strength comes from the mood
global_variable world_light_look LanternLight = {170.f, 1.f, {1.0f, 0.78f, 0.50f}, 0.4f};
// NOTE(zoubir): the light a body gives off while a status runs on it, in
// status_effect order (sim/status_effects.cpp); strength 0 for none. Fire
// burns like a torch, and its flicker of 1 also shimmers with heat
// (world_grade.frag)
global_variable world_light_look StatusLights[StatusEffect_Count] =
{
    {},                                          // None
    {95.f, 0.7f, {1.0f, 0.5f, 0.18f}, 1.f},      // Burning
    {70.f, 0.35f, {0.45f, 1.0f, 0.35f}, 0.3f},   // Poisoned
    {},                                          // Slowed: frost, talent_fx.cpp
    {},                                          // Stunned: stars, fx_bursts.cpp
    {},                                          // Bleeding
    {70.f, 0.3f, {0.6f, 1.0f, 0.6f}, 0.f},       // Regenerating
    {80.f, 0.45f, {1.0f, 0.95f, 0.5f}, 0.f},     // Hasted
    {},                                          // Rooted
    {},                                          // Soaked
    {},                                          // Falling
};
static_assert(ArrayCount(StatusLights) == StatusEffect_Count, "one light per status");
// NOTE(zoubir): a torch on a wall (wall_torches.cpp); its strength comes
// from the map's mood
global_variable world_light_look TorchLight = {150.f, 1.f, {1.0f, 0.62f, 0.28f}, 0.6f};
global_variable world_light_look LavaLight = {175.f, 0.75f, {1.0f, 0.42f, 0.12f}, 1.f};

internal void
AddWorldLight(world_lights *Lights, v3 CameraOffset, float Zoom,
              float WindowHeight, v2 WorldXY, float Z,
              world_light_look Look, float StrengthScale)
{
    float X = (WorldXY.X - CameraOffset.X) * Zoom;
    float Y = WindowHeight - (WorldXY.Y - CameraOffset.Y - Z) * Zoom;
    float Radius = Look.Radius * Zoom;
    float Strength = Look.Strength * StrengthScale;
    float Worth = LightWorth(Lights, X, Y, Radius, Strength);
    u32 Slot = Lights->Count;
    if (Slot >= WORLD_LIGHTS_MAX)
    {
        Slot = 0;
        for(u32 Other = 1; Other < WORLD_LIGHTS_MAX; Other++)
        {
            if (Lights->Worth[Other] < Lights->Worth[Slot])
            {
                Slot = Other;
            }
        }
        if (Lights->Worth[Slot] >= Worth)
        {
            return;
        }
    }
    else
    {
        Lights->Count++;
    }
    Lights->Worth[Slot] = Worth;
    u32 Index = 4 * Slot;
    Lights->Spot[Index + 0] = X;
    Lights->Spot[Index + 1] = Y;
    Lights->Spot[Index + 2] = Radius;
    Lights->Spot[Index + 3] = Strength;
    Lights->Color[Index + 0] = Look.Color.X;
    Lights->Color[Index + 1] = Look.Color.Y;
    Lights->Color[Index + 2] = Look.Color.Z;
    Lights->Color[Index + 3] = Look.Flicker;
}

// NOTE(zoubir): View is the window in world units (GetWorldView); lights
// past its edge by more than their radius are left out
// NOTE(zoubir): Zoom is framebuffer pixels per world unit and WindowHeight
// the framebuffer's height: the shader works in gl_FragCoord, which on a
// display scaled past 100% has more pixels than the window says
internal void
GatherWorldLights(app_state *AppState, v3 CameraOffset, app_window *View,
                  float Zoom, float WindowHeight, world_lights *Lights)
{
    Lights->Count = 0;
    Lights->Middle = 0.5f * V2((float)View->Width * Zoom, WindowHeight);
    world *World = &AppState->World;
    float Margin = 160.f;
    v2 Min = CameraOffset.XY - V2(Margin, Margin);
    v2 Max = CameraOffset.XY + V2((float)View->Width + Margin,
                                  (float)View->Height + Margin);

    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent)
        {
            continue;
        }
        world_light_look *Look = 0;
        float Scale = 1.f;
        switch(Entity->Type)
        {
            case EntityType_Player:
            {
                // NOTE(zoubir): and on any map at night (weather.cpp)
                Scale = Maximum(MoodFor(World->MapId)->Lantern,
                                0.8f * (1.f - Daylight(AppState)));
                Look = (Scale > 0.f && !IsDeadPlayer(Entity)) ? &LanternLight : 0;
            } break;
            case EntityType_FireBall: Look = &FireballLight; break;
            case EntityType_MonsterShot: Look = &MonsterShotLight; break;
            case EntityType_Kunai: Look = &KunaiLight; break;
            default: break;
        }
        v2 P = Entity->Position.XY;
        if (Look && P.X > Min.X && P.X < Max.X && P.Y > Min.Y && P.Y < Max.Y)
        {
            // NOTE(zoubir): halfway down to the ground, since most of
            // what it lights is the ground under it
            AddWorldLight(Lights, CameraOffset, Zoom, WindowHeight, P,
                          0.5f * Entity->Position.Z, *Look, Scale);
        }
        bool32 Unit = Entity->Type == EntityType_Player || Entity->Type == EntityType_Monster;
        if (Unit && !IsDeadPlayer(Entity) &&
            P.X > Min.X && P.X < Max.X && P.Y > Min.Y && P.Y < Max.Y)
        {
            for(u32 Effect = 1; Effect < StatusEffect_Count; Effect++)
            {
                if (StatusLights[Effect].Strength > 0.f &&
                    HasStatus(Entity, (status_effect)Effect))
                {
                    AddWorldLight(Lights, CameraOffset, Zoom, WindowHeight, P,
                                  Entity->Position.Z + 12.f, StatusLights[Effect], 1.f);
                }
            }
        }
    }

    fx_bursts *Fx = AppState->FxBursts;
    for(u32 Index = 0; Fx && Index < Fx->Count; Index++)
    {
        fx_burst *Burst = &Fx->Bursts[Index];
        burst_shape Shape = BurstLooks[Burst->Kind].Shape;
        if (Shape == BurstShape_Puff || Shape == BurstShape_Skid ||
            Shape == BurstShape_Mark || Shape == BurstShape_ConeMark)
        {
            continue;
        }
        burst_area Area = BurstArea(Burst->Kind);
        float Left = 1.f - Burst->Age / Area.Seconds;
        v2 P = Burst->Position.XY;
        if (Left <= 0.f || P.X < Min.X || P.X > Max.X || P.Y < Min.Y || P.Y > Max.Y)
        {
            continue;
        }
        // NOTE(zoubir): 0x00BBGGRR, brought up so its brightest channel
        // is 1: a pale burst lights as much as a vivid one
        u32 RGB = BurstLooks[Burst->Kind].RGB;
        v3 Color = V3((float)(RGB & 0xFF), (float)((RGB >> 8) & 0xFF),
                      (float)((RGB >> 16) & 0xFF));
        Color *= 1.f / Maximum(1.f, Maximum(Color.X, Maximum(Color.Y, Color.Z)));
        world_light_look Look = {BURST_LIGHT_REACH * Maximum(Area.Radius, BURST_LIGHT_MIN_RADIUS),
                                 BURST_LIGHT_STRENGTH, Color, 0.f};
        AddWorldLight(Lights, CameraOffset, Zoom, WindowHeight, P,
                      0.5f * Burst->Position.Z, Look, Left * Left);
    }

    if (World->TileWidth == 0 || World->TileMap.Texture.Type != AssetType_TerrainAtlas)
    {
        return;
    }
    map_def *Map = GetMapDef((map_id)World->MapId);
    i32 Tile = (i32)World->TileWidth;
    float Torches = MoodFor(World->MapId)->Torches;
    for(i32 Y = FloorDiv((i32)floorf(Min.Y), Tile); Torches > 0.f &&
            Y <= FloorDiv((i32)floorf(Max.Y), Tile); Y++)
    {
        for(i32 X = FloorDiv((i32)floorf(Min.X), Tile); X <= FloorDiv((i32)floorf(Max.X), Tile); X++)
        {
            float Lift;
            if (WallTorchAt(AppState, Map, X, Y, &Lift))
            {
                // NOTE(zoubir): the light sits at the foot of the wall, out in
                // front of it, so it falls on the floor and not the wall top
                AddWorldLight(Lights, CameraOffset, Zoom, WindowHeight,
                              V2(((float)X + 0.5f) * Tile, (float)(Y + 1) * Tile + 10.f),
                              0.f, TorchLight, Torches);
            }
        }
    }
    i32 Block = WORLD_LIGHT_LAVA_BLOCK;
    i32 MinBlockX = FloorDiv(FloorDiv((i32)floorf(Min.X), Tile), Block);
    i32 MinBlockY = FloorDiv(FloorDiv((i32)floorf(Min.Y), Tile), Block);
    i32 MaxBlockX = FloorDiv(FloorDiv((i32)floorf(Max.X), Tile), Block);
    i32 MaxBlockY = FloorDiv(FloorDiv((i32)floorf(Max.Y), Tile), Block);
    for(i32 BlockY = MinBlockY; BlockY <= MaxBlockY; BlockY++)
    {
        for(i32 BlockX = MinBlockX; BlockX <= MaxBlockX; BlockX++)
        {
            v2 Sum = {};
            u32 LavaTiles = 0;
            for(i32 Y = BlockY * Block; Y < (BlockY + 1) * Block; Y++)
            {
                for(i32 X = BlockX * Block; X < (BlockX + 1) * Block; X++)
                {
                    if (!World->Unbounded &&
                        (X < 0 || Y < 0 || X >= (i32)World->NumTilesX ||
                         Y >= (i32)World->NumTilesY))
                    {
                        continue;
                    }
                    if (CachedTile(AppState, Map, X, Y)->Kind == TerrainKind_Lava)
                    {
                        Sum += V2((X + 0.5f) * Tile, (Y + 0.5f) * Tile);
                        LavaTiles++;
                    }
                }
            }
            if (LavaTiles)
            {
                float Share = (float)LavaTiles / (float)(Block * Block);
                AddWorldLight(Lights, CameraOffset, Zoom, WindowHeight,
                              Sum * (1.f / (float)LavaTiles), 0.f, LavaLight,
                              0.4f + 0.6f * Share);
            }
        }
    }
}
