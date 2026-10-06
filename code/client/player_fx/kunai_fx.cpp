/* Kunai: each kunai (sim/player_abilities/kunai.cpp) drawn as a small
   steel blade pointing where it flies, with a dark wrapped grip and a
   ring at the back, over a pale streak of motes it leaves behind that
   fade over KUNAI_TRAIL_SECONDS. Read from the kunai entities
   themselves, so replicas look the same online. The entity has no
   sprite; this is all of its look.

   DrawKunaiTargetMark puts four corner brackets round the unit a kunai
   thrown now would go for (KunaiTargetFor, the cursor's pick): bright
   when the key is ready, dim while it cools down, none when the cursor
   is on no enemy. */

#define MAX_KUNAI_MOTES 192
#define KUNAI_TRAIL_SECONDS 0.16f

struct kunai_mote
{
    v2 Position;
    float Age;
};

struct kunai_trails
{
    kunai_mote Motes[MAX_KUNAI_MOTES];
    u32 Count;
};

// NOTE(zoubir): where the kunai shows: its ground point lifted by its height
inline v2
KunaiDrawPoint(world_entity *Kunai)
{
    v2 Result = Kunai->Position.XY - V2(0.f, Kunai->Position.Z);
    return Result;
}

internal void
UpdateKunaiTrails(kunai_trails *Fx, app_state *AppState, float DeltaTime)
{
    for(u32 Index = 0; Index < Fx->Count;)
    {
        kunai_mote *Mote = &Fx->Motes[Index];
        Mote->Age += DeltaTime;
        if (Mote->Age >= KUNAI_TRAIL_SECONDS)
        {
            *Mote = Fx->Motes[--Fx->Count];
        }
        else
        {
            Index++;
        }
    }
    world *World = &AppState->World;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Kunai = &World->Entities[Index];
        if (Kunai->IsPresent && Kunai->Type == EntityType_Kunai &&
            Fx->Count < MAX_KUNAI_MOTES)
        {
            kunai_mote *Mote = &Fx->Motes[Fx->Count++];
            Mote->Position = KunaiDrawPoint(Kunai);
            Mote->Age = 0.f;
        }
    }
}

// NOTE(zoubir): a four-cornered shape in one colour; P are offsets along
// Dir and across it (Side) from Centre
inline void
DrawKunaiPart(render_context *RenderContext, v2 Centre, v2 Dir, v2 Side,
              v2 P0, v2 P1, v2 P2, v2 P3, u32 Color)
{
    DrawFilledQuad(RenderContext,
                   Centre + P0.X * Dir + P0.Y * Side, Centre + P1.X * Dir + P1.Y * Side,
                   Centre + P2.X * Dir + P2.Y * Side, Centre + P3.X * Dir + P3.Y * Side,
                   Color, Color, Color, Color);
}

internal void
DrawKunai(render_context *RenderContext, world_entity *Kunai, v3 CameraOffset)
{
    v2 Centre = KunaiDrawPoint(Kunai) - CameraOffset.XY;
    v2 Dir = NormalizeOr(Kunai->Velocity.XY, V2(1.f, 0.f));
    v2 Side = V2(-Dir.Y, Dir.X);
    // NOTE(zoubir): 0xAABBGGRR. A dark rim first so it reads on grass and
    // stone, then the steel, the edge's shine, the grip and the ring
    u32 Rim = 0xD0201810;
    u32 Steel = 0xFFEBDCD2;
    u32 Shine = 0xFFFFFFFF;
    u32 Grip = 0xFF30309A;
    DrawKunaiPart(RenderContext, Centre, Dir, Side,
                  V2(11.f, 0.f), V2(1.f, 5.f), V2(-2.f, 0.f), V2(1.f, -5.f), Rim);
    DrawKunaiPart(RenderContext, Centre, Dir, Side,
                  V2(-1.f, 2.f), V2(-9.f, 2.f), V2(-9.f, -2.f), V2(-1.f, -2.f), Rim);
    DrawKunaiPart(RenderContext, Centre, Dir, Side,
                  V2(10.f, 0.f), V2(1.f, 3.8f), V2(-1.f, 0.f), V2(1.f, -3.8f), Steel);
    DrawKunaiPart(RenderContext, Centre, Dir, Side,
                  V2(10.f, 0.f), V2(1.f, 1.2f), V2(0.f, 0.f), V2(1.f, 0.f), Shine);
    DrawKunaiPart(RenderContext, Centre, Dir, Side,
                  V2(-1.f, 1.f), V2(-7.f, 1.f), V2(-7.f, -1.f), V2(-1.f, -1.f), Grip);
    DrawKunaiPart(RenderContext, Centre, Dir, Side,
                  V2(-7.f, 0.f), V2(-9.f, 2.f), V2(-11.f, 0.f), V2(-9.f, -2.f), Steel);
}

#define KUNAI_MARK_HALF 17.f
#define KUNAI_MARK_ARM 7.f
// NOTE(zoubir): a unit's middle sits about this far above its feet
#define KUNAI_MARK_LIFT 14.f

internal void
DrawKunaiTargetMark(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    world_entity *Player = GetLocalPlayer(AppState);
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    if (!Player || !Player->IsPresent || IsDeadPlayer(Player) ||
        !AbilityLevel(Slot, PlayerButton_Kunai))
    {
        return;
    }
    world_entity *Target = KunaiTargetFor(&AppState->World, Player);
    if (!Target)
    {
        return;
    }
    bool32 Ready = Player->ActionCooldowns[PlayerAction_Kunai] <= 0.f;
    u32 Color = Ready ? 0xF0FFF0D8 : 0x80B0A8A0;
    u32 Rim = Ready ? 0xC0201810 : 0x60201810;
    v2 Centre = Target->Position.XY - V2(0.f, Target->Position.Z + KUNAI_MARK_LIFT) -
        CameraOffset.XY;
    float H = KUNAI_MARK_HALF;
    float A = KUNAI_MARK_ARM;
    for(u32 Corner = 0; Corner < 4; Corner++)
    {
        float SX = (Corner & 1) ? 1.f : -1.f;
        float SY = (Corner & 2) ? 1.f : -1.f;
        v2 Tip = Centre + V2(SX * H, SY * H);
        v2 Across = V2(-SX * A, 0.f);
        v2 Down = V2(0.f, -SY * A);
        for(u32 Pass = 0; Pass < 2; Pass++)
        {
            float W = Pass ? 1.f : 2.f;
            u32 C = Pass ? Color : Rim;
            DrawFilledQuad(RenderContext, Tip + V2(0.f, -W * SY), Tip + Across + V2(0.f, -W * SY),
                           Tip + Across + V2(0.f, W * SY), Tip + V2(0.f, W * SY), C, C, C, C);
            DrawFilledQuad(RenderContext, Tip + V2(-W * SX, 0.f), Tip + Down + V2(-W * SX, 0.f),
                           Tip + Down + V2(W * SX, 0.f), Tip + V2(W * SX, 0.f), C, C, C, C);
        }
    }
}

internal void
DrawKunaiFx(render_context *RenderContext, app_state *AppState, kunai_trails *Fx,
            v3 CameraOffset)
{
    for(u32 Index = 0; Index < Fx->Count; Index++)
    {
        kunai_mote *Mote = &Fx->Motes[Index];
        float Life = 1.f - Mote->Age / KUNAI_TRAIL_SECONDS;
        u32 Color = ((u32)(170.f * Life) << 24) | 0x00FFF0D8;
        DrawFxDot(RenderContext, Mote->Position - CameraOffset.XY, 1.f + 2.5f * Life, Color);
    }
    world *World = &AppState->World;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Kunai = &World->Entities[Index];
        if (Kunai->IsPresent && Kunai->Type == EntityType_Kunai)
        {
            DrawKunai(RenderContext, Kunai, CameraOffset);
        }
    }
}
