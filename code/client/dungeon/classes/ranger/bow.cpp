/* Ranger bow (classes/ranger.cpp): the shapes the Ranger's look and its
   effects are drawn from, in screen space: the longbow (limbs that bend
   as the string is drawn, the string and a nocked arrow), a flying arrow
   with its head, shaft, fletching and trail, and the quiver. Also when
   this Ranger last loosed a shot, read from the bursts in flight, so a
   remote Ranger's bow snaps the same as the local one's. */

#define RANGER_FX_TEAL_RGB 0x00AAD25A
#define RANGER_FX_PALE_RGB 0x00E6FFBE
#define RANGER_FX_DEEP_RGB 0x0064781E
#define RANGER_FX_WOOD_RGB 0x0028486E
#define RANGER_FX_WOOD_LIGHT_RGB 0x004E86B8
#define RANGER_FX_WOOD_DARK_RGB 0x00141E2C
#define RANGER_FX_STRING_RGB 0x00D2E6EB
#define RANGER_FX_STEEL_RGB 0x00E0DCD2
#define RANGER_FX_GOLD_RGB 0x0096E6FF

// NOTE(zoubir): the youngest burst Index of the Ranger in SlotIndex, how
// long ago it started; a big number for none
internal float
RangerBurstAge(app_state *AppState, u32 SlotIndex, u32 Index)
{
    role_fx *Fx = GetRoleFx(AppState);
    float Clock = GetFxClock(AppState);
    float Result = 1000.f;
    for(u32 Row = 0; Row < Fx->Count; Row++)
    {
        role_burst *Burst = &Fx->Bursts[Row];
        if (Burst->Kind == ClassBurst(SimBurst_RangerFirst, Index) && Burst->Slot == SlotIndex)
        {
            Result = Minimum(Result, Clock - Burst->Start);
        }
    }
    return Result;
}

// NOTE(zoubir): whether a burst of the same kind and Ranger as Burst
// started after it, which takes over from it (a mark or a trap sent again)
internal bool32
RangerBurstSuperseded(app_state *AppState, role_burst *Burst)
{
    role_fx *Fx = GetRoleFx(AppState);
    bool32 Result = false;
    for(u32 Row = 0; Row < Fx->Count; Row++)
    {
        role_burst *Other = &Fx->Bursts[Row];
        Result |= Other != Burst && Other->Kind == Burst->Kind && Other->Slot == Burst->Slot &&
            Other->Start > Burst->Start;
    }
    return Result;
}

// NOTE(zoubir): the aim as a screen direction
inline v2
RangerScreenAim(world_entity *Player)
{
    v2 Result = NormalizeOr(GetPlayerAim(Player), V2(1.f, 0.f));
    return Result;
}

// NOTE(zoubir): where the bow hand is on screen
inline v2
RangerBowGrip(world_entity *Player, v3 CameraOffset)
{
    v2 Result = RoleLookPoint(Player, 0.42f, CameraOffset) +
        (0.36f * Player->Dimensions.X) * RangerScreenAim(Player);
    return Result;
}

// NOTE(zoubir): a ring of teal light round Centre, its outside edge at
// Radius, Width thick, brightest at the rim
internal void
RangerRing(render_context *RenderContext, v2 Centre, float Radius, float Width, float Alpha, u32 RGB)
{
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Radius - Width, Radius,
                FxColor(0.f, RGB), FxColor(0.8f * Alpha, RGB));
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Radius - 1.5f, Radius + 0.5f,
                FxColor(0.7f * Alpha, RANGER_FX_PALE_RGB), FxColor(0.7f * Alpha, RANGER_FX_PALE_RGB));
}

// NOTE(zoubir): a solid stroke from A to B, Width wide, one colour
inline void
RangerStroke(render_context *RenderContext, v2 A, v2 B, float Width, u32 Color)
{
    DrawFxStroke(RenderContext, A, B, Width, Width, Color, Color, RenderBlend_Alpha);
}

// NOTE(zoubir): an arrow flying with its point at Head along Dir, Length
// long, Alpha seen; a Trail of light behind it that long, Glow on the
// head (0 for none). Big is how heavy it is, 1 for a plain arrow
internal void
DrawRangerArrow(render_context *RenderContext, v2 Head, v2 Dir, float Length, float Alpha,
                float Trail, float Glow, float Big, u32 TrailRGB)
{
    v2 Side = V2(-Dir.Y, Dir.X);
    v2 Tail = Head - Length * Dir;
    if (Trail > 0.f)
    {
        DrawFxStreak(RenderContext, Tail - Trail * Dir, Head - 0.3f * Length * Dir, 5.f * Big,
                     FxColor(0.f, TrailRGB), FxColor(0.45f * Alpha, TrailRGB));
        DrawFxStreak(RenderContext, Tail - 0.5f * Trail * Dir, Tail, 1.6f * Big,
                     FxColor(0.f, 0x00FFFFFF), FxColor(0.5f * Alpha, RANGER_FX_PALE_RGB));
    }
    if (Glow > 0.f)
    {
        float G = 18.f * Big * (0.6f + 0.4f * Glow);
        DrawShaderQuad(RenderContext, Shader_Glow, Head.X - G, Head.Y - G, 2.f * G, 2.f * G,
                       FxColor(0.8f * Glow * Alpha, TrailRGB), RenderBlend_Additive);
    }
    // NOTE(zoubir): a dark edge first, so the shaft reads on light stone
    RangerStroke(RenderContext, Tail, Head - 4.f * Big * Dir, 3.2f * Big,
                 FxColor(0.7f * Alpha, RANGER_FX_WOOD_DARK_RGB));
    RangerStroke(RenderContext, Tail, Head - 4.f * Big * Dir, 1.6f * Big,
                 FxColor(Alpha, RANGER_FX_WOOD_LIGHT_RGB));
    // NOTE(zoubir): the steel head, a leaf blade
    v2 Base = Head - 8.f * Big * Dir;
    u32 Steel = FxColor(Alpha, RANGER_FX_STEEL_RGB);
    u32 Shade = FxColor(Alpha, 0x00908880);
    DrawFilledQuad(RenderContext, Head, Base + 3.4f * Big * Side, Base, Base, Steel, Steel, Shade,
                   Shade, RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Head, Base - 3.4f * Big * Side, Base, Base, Steel, Shade, Shade,
                   Shade, RenderBlend_Alpha);
    // NOTE(zoubir): fletching, two teal vanes swept back at the tail
    u32 Vane = FxColor(Alpha, RANGER_FX_TEAL_RGB);
    u32 VaneTip = FxColor(Alpha, RANGER_FX_PALE_RGB);
    for(float Sign = -1.f; Sign <= 1.f; Sign += 2.f)
    {
        v2 Front = Tail + 8.f * Big * Dir;
        v2 Back = Tail + 1.f * Big * Dir;
        v2 Out = Tail - 1.5f * Big * Dir + Sign * 4.f * Big * Side;
        DrawFilledQuad(RenderContext, Front, Back, Out, Out, Vane, Vane, VaneTip, VaneTip,
                       RenderBlend_Alpha);
    }
}

// NOTE(zoubir): a longbow held at Grip, shooting along Dir (a screen
// direction), Size from tip to tip; Draw 0..1 pulls the string back and
// bends the limbs, Nock with an arrow on the string (Heavy, a Piercing
// Shot's), Quiver 0..1 shakes the string after a release
internal void
DrawRangerBow(render_context *RenderContext, v2 Grip, v2 Dir, float Size, float Draw,
              bool32 Nock, float Heavy, float Quiver, float Clock)
{
    v2 Side = V2(-Dir.Y, Dir.X);
    float Half = 0.5f * Size;
    float Bend = 0.16f + 0.14f * Draw;
    // NOTE(zoubir): the limbs, each a few segments curving back toward the
    // archer at the tips
    v2 Points[9];
    for(u32 Index = 0; Index < 9; Index++)
    {
        float T = -1.f + 2.f * (float)Index / 8.f;
        Points[Index] = Grip + (Half * T) * Side - (Bend * Half * T * T) * Dir;
    }
    u32 Dark = FxColor(0.85f, RANGER_FX_WOOD_DARK_RGB);
    u32 Wood = FxColor(1.f, RANGER_FX_WOOD_RGB);
    u32 Light = FxColor(1.f, RANGER_FX_WOOD_LIGHT_RGB);
    for(u32 Index = 0; Index < 8; Index++)
    {
        float Mid = 1.f - Absolute(-1.f + 2.f * ((float)Index + 0.5f) / 8.f);
        float Width = 2.f + 2.2f * Mid;
        RangerStroke(RenderContext, Points[Index], Points[Index + 1], Width + 1.8f, Dark);
        RangerStroke(RenderContext, Points[Index], Points[Index + 1], Width, Wood);
        // NOTE(zoubir): light along the belly of the limb
        RangerStroke(RenderContext, Points[Index] + 0.6f * Dir, Points[Index + 1] + 0.6f * Dir,
                     0.35f * Width, Light);
    }
    // NOTE(zoubir): a teal-wrapped grip and pale horn tips
    RangerStroke(RenderContext, Grip - 3.5f * Side, Grip + 3.5f * Side, 5.f,
                 FxColor(1.f, RANGER_FX_DEEP_RGB));
    RangerStroke(RenderContext, Grip - 2.f * Side, Grip + 2.f * Side, 2.f,
                 FxColor(1.f, RANGER_FX_TEAL_RGB));
    DrawFxDot(RenderContext, Points[0], 3.f, FxColor(1.f, RANGER_FX_STRING_RGB));
    DrawFxDot(RenderContext, Points[8], 3.f, FxColor(1.f, RANGER_FX_STRING_RGB));

    // NOTE(zoubir): the string, from tip to tip through the nock, which a
    // draw pulls back and a release leaves shivering
    float Rest = Bend * Half;
    float Pull = Rest + Draw * 0.55f * Size;
    float Shiver = Quiver * 3.f * Sin(70.f * Clock);
    v2 NockPoint = Grip - Pull * Dir + Shiver * Dir;
    u32 String = FxColor(0.95f, RANGER_FX_STRING_RGB);
    DrawFxStroke(RenderContext, Points[0], NockPoint, 1.2f, 1.2f, String, String, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, NockPoint, Points[8], 1.2f, 1.2f, String, String, RenderBlend_Alpha);
    if (Nock)
    {
        float Length = 0.78f * Size + 0.3f * Size * Heavy;
        v2 Head = NockPoint + Length * Dir;
        DrawRangerArrow(RenderContext, Head, Dir, Length, 1.f, 0.f, 0.f, 1.f + 0.4f * Heavy,
                        RANGER_FX_TEAL_RGB);
    }
}

// NOTE(zoubir): a quiver slung across the back at Centre, leaning by
// Lean, three fletched arrows standing out of it
internal void
DrawRangerQuiver(render_context *RenderContext, v2 Centre, float Size, float Lean)
{
    v2 Up = V2(Sin(Lean), -Cos(Lean));
    v2 Side = V2(-Up.Y, Up.X);
    float W = 0.2f * Size;
    v2 Top = Centre + (0.5f * Size) * Up;
    v2 Bottom = Centre - (0.5f * Size) * Up;
    // NOTE(zoubir): the fletchings first, the quiver's mouth over them
    for(u32 Arrow = 0; Arrow < 3; Arrow++)
    {
        float Across = (-0.55f + 0.55f * (float)Arrow) * W;
        v2 Root = Top + Across * Side;
        v2 Tip = Root + (0.28f * Size) * Up + 0.12f * Across * Side;
        RangerStroke(RenderContext, Root, Tip, 1.4f, FxColor(1.f, RANGER_FX_WOOD_LIGHT_RGB));
        u32 Vane = FxColor(1.f, Arrow == 1 ? RANGER_FX_PALE_RGB : RANGER_FX_TEAL_RGB);
        DrawFilledQuad(RenderContext, Tip, Tip - (0.16f * Size) * Up + 2.6f * Side,
                       Tip - (0.18f * Size) * Up, Tip - (0.16f * Size) * Up - 2.6f * Side,
                       Vane, Vane, Vane, Vane, RenderBlend_Alpha);
    }
    u32 Dark = FxColor(0.9f, RANGER_FX_WOOD_DARK_RGB);
    u32 Leather = FxColor(1.f, 0x00305A86);
    u32 LeatherShade = FxColor(1.f, 0x00203C5A);
    DrawFilledQuad(RenderContext, Top - (W + 1.5f) * Side, Top + (W + 1.5f) * Side,
                   Bottom + (0.8f * W + 1.5f) * Side, Bottom - (0.8f * W + 1.5f) * Side,
                   Dark, Dark, Dark, Dark, RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Top - W * Side, Top + W * Side, Bottom + 0.8f * W * Side,
                   Bottom - 0.8f * W * Side, Leather, LeatherShade, LeatherShade, Leather,
                   RenderBlend_Alpha);
    // NOTE(zoubir): a teal band round its mouth and a strap buckle
    RangerStroke(RenderContext, Top - W * Side - 1.5f * Up, Top + W * Side - 1.5f * Up, 2.4f,
                 FxColor(1.f, RANGER_FX_TEAL_RGB));
    DrawFxDot(RenderContext, Centre, 2.6f, FxColor(1.f, 0x0060C0E0));
}
