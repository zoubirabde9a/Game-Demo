/* Berserker axe art (client/dungeon/classes/berserker.cpp): the great axe
   and the hand axe drawn in screen space from a grip point and a
   direction, and the crescent a swing of the great axe leaves. Every
   look and burst of the class draws its axe through these, so the axe in
   the hands, in a swing and in a throw is the same one.

   Colours are 0x00BBGGRR, as the effects take them (FxColor). */

#define BERSERKER_CRIMSON_RGB 0x003232E1
#define BERSERKER_BLOOD_RGB 0x000C08A8
#define BERSERKER_DARK_RGB 0x00080650
#define BERSERKER_HOT_RGB 0x004090FF
#define BERSERKER_PALE_RGB 0x00C0D8FF
#define BERSERKER_STEEL_RGB 0x00C8C0B8
#define BERSERKER_IRON_RGB 0x00605850
#define BERSERKER_WOOD_RGB 0x00305078
#define BERSERKER_INK_RGB 0x00100C0C

// NOTE(zoubir): a solid colour for the axe's alpha-blended parts
inline u32
AxeInk(float Alpha, u32 RGB)
{
    u32 Result = FxColor(Alpha, RGB);
    return Result;
}

// NOTE(zoubir): Along and Across the axe's frame at Grip: P(A, B) is A
// along the haft and B across it, toward the blade's edge
struct axe_frame
{
    v2 Grip;
    v2 Along;
    v2 Across;
};

inline v2
AxePoint(axe_frame *Frame, float Along, float Across)
{
    v2 Result = Frame->Grip + Along * Frame->Along + Across * Frame->Across;
    return Result;
}

inline axe_frame
MakeAxeFrame(v2 Grip, v2 Dir, float Side)
{
    axe_frame Result;
    Result.Grip = Grip;
    Result.Along = NormalizeOr(Dir, V2(0.f, -1.f));
    Result.Across = Side * V2(-Result.Along.Y, Result.Along.X);
    return Result;
}

// NOTE(zoubir): the great axe: a long dark haft bound in leather, a
// pommel, and at its head a broad bearded blade with a back spike. Grip
// is where the hands hold it, Dir from there to the head, Length from the
// grip to the head, Side which way the edge faces (+1 or -1 across Dir).
// Heat 0..1 lights the blade's runes and edge with Rage; Alpha fades it
internal void
DrawGreatAxe(render_context *RenderContext, v2 Grip, v2 Dir, float Length, float Side,
             float Heat, float Alpha)
{
    if (Alpha <= 0.01f)
    {
        return;
    }
    axe_frame F = MakeAxeFrame(Grip, Dir, Side);
    float L = Length;
    v2 Butt = AxePoint(&F, -0.24f * L, 0.f);
    v2 Top = AxePoint(&F, 1.06f * L, 0.f);
    // NOTE(zoubir): the haft: an outline, the wood, a lit edge, leather
    // bands where the hands go
    DrawFxStroke(RenderContext, Butt, Top, 5.5f, 5.f, AxeInk(Alpha, BERSERKER_INK_RGB),
                 AxeInk(Alpha, BERSERKER_INK_RGB), RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Butt, Top, 3.2f, 3.f, AxeInk(Alpha, 0x00203858),
                 AxeInk(Alpha, BERSERKER_WOOD_RGB), RenderBlend_Alpha);
    DrawFxStroke(RenderContext, AxePoint(&F, -0.22f * L, -0.8f), AxePoint(&F, 1.f * L, -0.8f), 1.f,
                 1.f, AxeInk(0.6f * Alpha, 0x0070A0C8), AxeInk(0.6f * Alpha, 0x0070A0C8),
                 RenderBlend_Alpha);
    for(u32 Band = 0; Band < 3; Band++)
    {
        float At = -0.12f + 0.12f * (float)Band;
        DrawFxStroke(RenderContext, AxePoint(&F, At * L, -2.4f), AxePoint(&F, At * L, 2.4f), 2.2f, 2.2f,
                     AxeInk(Alpha, 0x00141C28), AxeInk(Alpha, 0x00141C28), RenderBlend_Alpha);
    }
    DrawFxDot(RenderContext, Butt, 5.f, AxeInk(Alpha, BERSERKER_INK_RGB));
    DrawFxDot(RenderContext, Butt, 3.f, AxeInk(Alpha, BERSERKER_IRON_RGB));

    // NOTE(zoubir): the blade, a crescent with a long beard: an outline a
    // little bigger, then steel dark at the haft and pale at the edge
    float W = 0.42f * L;
    v2 Socket0 = AxePoint(&F, 0.72f * L, 0.f);
    v2 Socket1 = AxePoint(&F, 1.02f * L, 0.f);
    v2 TopHorn = AxePoint(&F, 1.16f * L, 0.92f * W);
    v2 EdgeTop = AxePoint(&F, 1.05f * L, 1.f * W);
    v2 EdgeMid = AxePoint(&F, 0.86f * L, 1.06f * W);
    v2 EdgeLow = AxePoint(&F, 0.66f * L, 0.98f * W);
    v2 Beard = AxePoint(&F, 0.5f * L, 0.82f * W);
    v2 Neck = AxePoint(&F, 0.78f * L, 0.3f * W);
    v2 NeckTop = AxePoint(&F, 0.96f * L, 0.3f * W);
    float O = 1.6f;
    v2 Out = O * F.Across;
    v2 Up = O * F.Along;
    u32 Ink = AxeInk(Alpha, BERSERKER_INK_RGB);
    DrawFilledQuad(RenderContext, Socket0 - Up, Socket1 + Up, NeckTop + Up, Neck - Up, Ink, Ink, Ink, Ink,
                   RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Neck - Up, NeckTop + Up, TopHorn + Up + Out, EdgeMid + Out, Ink, Ink,
                   Ink, Ink, RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Neck - Up, EdgeMid + Out, EdgeLow + Out, Beard - Up + Out, Ink, Ink,
                   Ink, Ink, RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, NeckTop, TopHorn + Out, EdgeTop + Out, EdgeMid + Out, Ink, Ink, Ink,
                   Ink, RenderBlend_Alpha);
    u32 Dark = AxeInk(Alpha, BERSERKER_IRON_RGB);
    u32 Steel = AxeInk(Alpha, BERSERKER_STEEL_RGB);
    u32 Mid = AxeInk(Alpha, 0x00988C84);
    DrawFilledQuad(RenderContext, Socket0, Socket1, NeckTop, Neck, Dark, Dark, Mid, Mid,
                   RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Neck, NeckTop, TopHorn, EdgeMid, Mid, Mid, Steel, Steel,
                   RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Neck, EdgeMid, EdgeLow, Beard, Mid, Steel, Steel, Mid,
                   RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, NeckTop, TopHorn, EdgeTop, EdgeMid, Mid, Steel, Steel, Steel,
                   RenderBlend_Alpha);
    // NOTE(zoubir): the back spike, the other side of the haft
    v2 SpikeTip = AxePoint(&F, 0.9f * L, -0.42f * W);
    DrawFilledQuad(RenderContext, AxePoint(&F, 0.8f * L, -1.f), AxePoint(&F, 0.98f * L, -1.f),
                   SpikeTip + Up, SpikeTip - Up, Ink, Ink, Ink, Ink, RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, AxePoint(&F, 0.83f * L, -1.f), AxePoint(&F, 0.95f * L, -1.f),
                   SpikeTip, SpikeTip, Dark, Dark, Mid, Mid, RenderBlend_Alpha);
    // NOTE(zoubir): the edge, white along its curve, hot red with Rage
    u32 Edge = AxeInk(Alpha, 0x00FFFFFF);
    DrawFxStroke(RenderContext, TopHorn, EdgeTop, 1.6f, 1.8f, Edge, Edge, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, EdgeTop, EdgeMid, 1.8f, 2.f, Edge, Edge, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, EdgeMid, EdgeLow, 2.f, 1.8f, Edge, Edge, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, EdgeLow, Beard, 1.8f, 1.2f, Edge, Edge, RenderBlend_Alpha);
    if (Heat > 0.f)
    {
        // NOTE(zoubir): the edge glows red hot, a thin line inside it, and
        // a rune burns in the blade
        u32 Hot = FxColor(Alpha * Heat, BERSERKER_CRIMSON_RGB);
        v2 In = -1.2f * F.Across;
        DrawFxStroke(RenderContext, TopHorn + In, EdgeTop + In, 1.2f, 1.5f, Hot, Hot);
        DrawFxStroke(RenderContext, EdgeTop + In, EdgeMid + In, 1.5f, 1.8f, Hot, Hot);
        DrawFxStroke(RenderContext, EdgeMid + In, EdgeLow + In, 1.8f, 1.5f, Hot, Hot);
        DrawFxStroke(RenderContext, EdgeLow + In, Beard + In, 1.5f, 1.f, Hot, Hot);
        v2 Rune = AxePoint(&F, 0.86f * L, 0.6f * W);
        float Glow = 5.f + 6.f * Heat;
        DrawShaderQuad(RenderContext, Shader_Glow, EdgeMid.X - Glow, EdgeMid.Y - Glow, 2.f * Glow,
                       2.f * Glow, FxColor(0.5f * Alpha * Heat, BERSERKER_CRIMSON_RGB),
                       RenderBlend_Additive);
        u32 Burn = FxColor(Alpha * Heat, BERSERKER_HOT_RGB);
        DrawFxStroke(RenderContext, Rune - 2.f * F.Along - 2.f * F.Across, Rune + 2.f * F.Along + 2.f * F.Across,
                     1.2f, 1.2f, Burn, Burn, RenderBlend_Alpha);
        DrawFxStroke(RenderContext, Rune + 2.f * F.Along - 2.f * F.Across, Rune - 2.f * F.Along + 2.f * F.Across,
                     1.2f, 1.2f, Burn, Burn, RenderBlend_Alpha);
    }
}

// NOTE(zoubir): the hand axe it throws: a short haft and one small blade,
// turned by Spin (radians) round its middle at Centre
internal void
DrawHandAxe(render_context *RenderContext, v2 Centre, float Spin, float Size, float Alpha)
{
    v2 Dir = V2(Cos(Spin), Sin(Spin));
    axe_frame F = MakeAxeFrame(Centre - 0.45f * Size * Dir, Dir, 1.f);
    float L = Size;
    v2 Butt = AxePoint(&F, 0.f, 0.f);
    v2 Head = AxePoint(&F, 1.f * L, 0.f);
    u32 Ink = AxeInk(Alpha, BERSERKER_INK_RGB);
    DrawFxStroke(RenderContext, Butt, Head, 4.f, 4.f, Ink, Ink, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Butt, Head, 2.f, 2.f, AxeInk(Alpha, 0x00203858),
                 AxeInk(Alpha, BERSERKER_WOOD_RGB), RenderBlend_Alpha);
    v2 A = AxePoint(&F, 0.7f * L, 0.f);
    v2 B = AxePoint(&F, 1.02f * L, 0.f);
    v2 C = AxePoint(&F, 1.1f * L, 0.5f * L);
    v2 D = AxePoint(&F, 0.62f * L, 0.46f * L);
    v2 Out = 1.4f * F.Across;
    DrawFilledQuad(RenderContext, A - Out, B - Out, C + Out, D + Out, Ink, Ink, Ink, Ink,
                   RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, A, B, C, D, AxeInk(Alpha, BERSERKER_IRON_RGB),
                   AxeInk(Alpha, 0x00988C84), AxeInk(Alpha, BERSERKER_STEEL_RGB),
                   AxeInk(Alpha, BERSERKER_STEEL_RGB), RenderBlend_Alpha);
    DrawFxStroke(RenderContext, C, D, 1.6f, 1.6f, AxeInk(Alpha, 0x00FFFFFF), AxeInk(Alpha, 0x00FFFFFF),
                 RenderBlend_Alpha);
}

// NOTE(zoubir): the crescent of a great axe's swing round Centre, its
// outside edge at Radius: Tail to Lead are shares of the slice from Angle
// - Side * Half to Angle + Side * Half. Deep red inside a crimson body
// under a hot rim, thickest just behind the blade, thinning at both ends
internal void
DrawAxeCrescent(render_context *RenderContext, v2 Centre, float Angle, float Side, float Half,
                float Radius, float Width, float Tail, float Lead, float Alpha)
{
    if (Lead - Tail <= 0.001f || Alpha <= 0.f)
    {
        return;
    }
    u32 const Segments = 26;
    v2 LastOuter = {}, LastInner = {}, LastRim = {}, LastHalo = {};
    float LastStrength = 0.f;
    for(u32 Index = 0; Index <= Segments; Index++)
    {
        float S = (float)Index / (float)Segments;
        float Along = Tail + S * (Lead - Tail);
        float At = Angle + Side * Half * (2.f * Along - 1.f);
        v2 Out = V2(Cos(At), Sin(At));
        float Profile = Sin(Pi32 * powf(Clamp01(S), 1.7f));
        v2 Outer = Centre + Radius * Out;
        v2 Inner = Centre + (Radius - Width * Profile) * Out;
        v2 Rim = Centre + (Radius - 1.5f - 3.f * Profile) * Out;
        v2 Halo = Centre + (Radius + 2.f + 5.f * Profile) * Out;
        float Strength = Minimum(1.f, Alpha * (0.25f + 0.95f * S));
        if (Index > 0)
        {
            // NOTE(zoubir): dark blood under the light, so it reads on
            // pale stone, then the crimson body and the glow past the edge
            DrawFilledQuad(RenderContext, LastInner, LastOuter, Outer, Inner,
                           FxColor(0.f, BERSERKER_DARK_RGB), FxColor(0.55f * LastStrength, BERSERKER_DARK_RGB),
                           FxColor(0.55f * Strength, BERSERKER_DARK_RGB), FxColor(0.f, BERSERKER_DARK_RGB),
                           RenderBlend_Alpha);
            DrawFilledQuad(RenderContext, LastInner, LastOuter, Outer, Inner,
                           FxColor(0.f, BERSERKER_BLOOD_RGB), FxColor(LastStrength, BERSERKER_CRIMSON_RGB),
                           FxColor(Strength, BERSERKER_CRIMSON_RGB), FxColor(0.f, BERSERKER_BLOOD_RGB));
            DrawFilledQuad(RenderContext, LastOuter, LastHalo, Halo, Outer,
                           FxColor(0.6f * LastStrength, BERSERKER_CRIMSON_RGB), FxColor(0.f, BERSERKER_CRIMSON_RGB),
                           FxColor(0.f, BERSERKER_CRIMSON_RGB), FxColor(0.6f * Strength, BERSERKER_CRIMSON_RGB));
            DrawFilledQuad(RenderContext, LastRim, LastOuter, Outer, Rim,
                           FxColor(LastStrength, BERSERKER_PALE_RGB), FxColor(LastStrength, BERSERKER_PALE_RGB),
                           FxColor(Strength, BERSERKER_PALE_RGB), FxColor(Strength, BERSERKER_PALE_RGB));
        }
        LastOuter = Outer;
        LastInner = Inner;
        LastRim = Rim;
        LastHalo = Halo;
        LastStrength = Strength;
    }
}
