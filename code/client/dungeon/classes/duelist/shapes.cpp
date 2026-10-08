/* Duelist shapes (classes/duelist.cpp, before its look and bursts): the
   pieces every Duelist effect is drawn from: the rapier, a thin piercing
   line, a Tempo diamond, a heart, a fencer's afterimage and rose petals.
   All in screen space, from the fx clock. */

#define DUELIST_RGB 0x00AA6EF0
#define DUELIST_PALE_RGB 0x00E6D2FF
#define DUELIST_DEEP_RGB 0x0050185A
#define DUELIST_STEEL_RGB 0x00F8F2EE
#define DUELIST_GOLD_RGB 0x0060D2FF

// NOTE(zoubir): seconds since the newest burst Index of the Duelist in
// slot SlotIndex, or a big number when none is playing
internal float
DuelistBurstAge(app_state *AppState, u32 SlotIndex, u32 Index)
{
    role_fx *Fx = GetRoleFx(AppState);
    float Clock = GetFxClock(AppState);
    float Result = 100.f;
    sim_burst Kind = ClassBurst(SimBurst_DuelistFirst, Index);
    for(u32 Burst = 0; Burst < Fx->Count; Burst++)
    {
        role_burst *Row = &Fx->Bursts[Burst];
        if (Row->Kind == Kind && Row->Slot == SlotIndex)
        {
            Result = Minimum(Result, Clock - Row->Start);
        }
    }
    return Result;
}

// NOTE(zoubir): the newest burst Index of the Duelist in slot SlotIndex,
// 0 when none is playing
internal role_burst *
DuelistNewestBurst(app_state *AppState, u32 SlotIndex, u32 Index)
{
    role_fx *Fx = GetRoleFx(AppState);
    role_burst *Result = 0;
    sim_burst Kind = ClassBurst(SimBurst_DuelistFirst, Index);
    for(u32 Burst = 0; Burst < Fx->Count; Burst++)
    {
        role_burst *Row = &Fx->Bursts[Burst];
        if (Row->Kind == Kind && Row->Slot == SlotIndex && (!Result || Row->Start > Result->Start))
        {
            Result = Row;
        }
    }
    return Result;
}

inline float
DuelistEase(float T)
{
    float C = Clamp01(T);
    float Result = C * C * (3.f - 2.f * C);
    return Result;
}

// NOTE(zoubir): how much of an action of Length seconds, Age into it,
// still holds the pose: a snap in, then back out over its second half
inline float
DuelistHold(float Age, float Length)
{
    float Result = 0.f;
    if (Age >= 0.f && Age < Length)
    {
        Result = Minimum(Clamp01(Age / 0.03f), 1.f - DuelistEase((Age - 0.5f * Length) / (0.5f * Length)));
    }
    return Result;
}

inline v2
DuelistNormal(v2 V)
{
    v2 Result = LengthSq(V) > 0.0001f ? DirectionTo(V) : V2(1.f, 0.f);
    return Result;
}

inline v2
DuelistRotate(v2 V, float Angle)
{
    float C = Cos(Angle);
    float S = Sin(Angle);
    v2 Result = V2(C * V.X - S * V.Y, S * V.X + C * V.Y);
    return Result;
}

inline void
DrawDuelistDisc(render_context *RenderContext, v2 C, float Radius, u32 Color,
                u32 Blend = RenderBlend_Alpha)
{
    DrawArcBand(RenderContext, C, 0.f, 2.f * Pi32, 0.f, Radius, Color, Color, Blend);
}

// NOTE(zoubir): a rapier whose grip is at Grip, its blade Length long
// along Dir (a unit vector): a long thin steel blade with a dark outline,
// a gold cup hilt with a swept knuckle bow, a rose grip and pommel. Glow
// (0..1, Tempo) runs rose light up the blade; Alpha fades it all
internal void
DrawDuelistRapier(render_context *RenderContext, v2 Grip, v2 Dir, float Length, float Glow,
                  float Alpha)
{
    v2 Side = V2(-Dir.Y, Dir.X);
    v2 Base = Grip + (0.12f * Length) * Dir;
    v2 Tip = Grip + Length * Dir;
    u32 Dark = FxColor(0.9f * Alpha, 0x00140A14);
    // NOTE(zoubir): the blade: an outline, then a bright edge over a
    // greyer back, tapering to the point
    DrawFxStroke(RenderContext, Base, Tip + 1.5f * Dir, 4.6f, 1.6f, Dark, Dark, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Base, Tip, 2.6f, 0.6f, FxColor(Alpha, 0x00B0A4A8),
                 FxColor(Alpha, 0x00D8D0D0), RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Base - 0.5f * Side, Tip - 0.2f * Side, 1.2f, 0.4f,
                 FxColor(Alpha, DUELIST_STEEL_RGB), FxColor(Alpha, 0x00FFFFFF), RenderBlend_Alpha);
    if (Glow > 0.f)
    {
        DrawFxStroke(RenderContext, Base, Tip + 3.f * Dir, 10.f, 3.f,
                     FxColor(0.7f * Glow * Alpha, DUELIST_RGB), FxColor(0.35f * Glow * Alpha, DUELIST_PALE_RGB));
        v2 Mid = Base + (0.5f * Length) * Dir;
        DrawShaderQuad(RenderContext, Shader_Glow, Mid.X - 0.7f * Length, Mid.Y - 0.7f * Length,
                       1.4f * Length, 1.4f * Length, FxColor(0.3f * Glow * Alpha, DUELIST_RGB),
                       RenderBlend_Additive);
    }
    // NOTE(zoubir): the knuckle bow sweeping from the cup back to the
    // pommel, and the cup itself
    v2 Cup = Grip + (0.1f * Length) * Dir;
    v2 Pommel = Grip - (0.09f * Length) * Dir;
    v2 Bow = Grip + (0.03f * Length) * Dir + (0.075f * Length) * Side;
    DrawFxStroke(RenderContext, Cup + 0.03f * Length * Side, Bow, 2.4f, 2.4f, Dark, Dark, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Bow, Pommel, 2.4f, 2.4f, Dark, Dark, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Cup + 0.03f * Length * Side, Bow, 1.2f, 1.2f,
                 FxColor(Alpha, DUELIST_GOLD_RGB), FxColor(Alpha, DUELIST_GOLD_RGB), RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Bow, Pommel, 1.2f, 1.2f, FxColor(Alpha, DUELIST_GOLD_RGB),
                 FxColor(Alpha, 0x0030A0E0), RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Cup - (0.06f * Length) * Side, Cup + (0.06f * Length) * Side,
                 4.5f, 4.5f, Dark, Dark, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Cup - (0.05f * Length) * Side, Cup + (0.05f * Length) * Side,
                 3.f, 3.f, FxColor(Alpha, 0x0080E8FF), FxColor(Alpha, 0x0020A0E0), RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Pommel, Cup, 2.8f, 2.8f, FxColor(Alpha, DUELIST_DEEP_RGB),
                 FxColor(Alpha, DUELIST_RGB), RenderBlend_Alpha);
    DrawDuelistDisc(RenderContext, Pommel, 2.2f, Dark);
    DrawDuelistDisc(RenderContext, Pommel, 1.5f, FxColor(Alpha, DUELIST_GOLD_RGB));
}

// NOTE(zoubir): a thin piercing line from From to To, revealed up to
// Reveal (0..1) from From, Width at its widest, a white core in a halo
internal void
DrawDuelistLine(render_context *RenderContext, v2 From, v2 To, float Reveal, float Width,
                float Alpha, u32 RGB)
{
    v2 End = From + Clamp01(Reveal) * (To - From);
    DrawFxStroke(RenderContext, From, End, 0.4f * Width, 3.f * Width, FxColor(0.f, RGB),
                 FxColor(0.5f * Alpha, RGB));
    DrawFxStroke(RenderContext, From, End, 0.2f, Width, FxColor(0.f, 0x00FFFFFF),
                 FxColor(Alpha, 0x00FFFFFF));
}

// NOTE(zoubir): a Tempo diamond at P, Size tall: a thin dark-rimmed
// outline, filled rose by Fill (0..1, from the bottom); Flash brightens it
internal void
DrawDuelistDiamond(render_context *RenderContext, v2 P, float Size, float Fill, float Flash,
                   float Alpha)
{
    float H = 0.5f * Size;
    float W = 0.27f * Size;
    v2 Top = P + V2(0.f, -H);
    v2 Right = P + V2(W, 0.f);
    v2 Bottom = P + V2(0.f, H);
    v2 Left = P + V2(-W, 0.f);
    u32 Dark = FxColor(0.85f * Alpha, 0x00140A14);
    if (Fill > 0.f)
    {
        float F = Clamp01(Fill);
        u32 Lit = FxColor(Alpha, DUELIST_RGB);
        u32 Low = FxColor(Alpha, 0x00701E8C);
        // NOTE(zoubir): filled from the bottom point up to the Fill line
        float Y = H - 2.f * H * F;
        float Half = W * (1.f - fabsf(Y) / H);
        if (Y >= 0.f)
        {
            DrawFilledQuad(RenderContext, Bottom, P + V2(-Half, Y), P + V2(Half, Y), Bottom,
                           Low, Lit, Lit, Low, RenderBlend_Alpha);
        }
        else
        {
            DrawFilledQuad(RenderContext, Bottom, Left, Right, Bottom, Low, Lit, Lit, Low,
                           RenderBlend_Alpha);
            DrawFilledQuad(RenderContext, Left, P + V2(-Half, Y), P + V2(Half, Y), Right,
                           Lit, Lit, Lit, Lit, RenderBlend_Alpha);
        }
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - 1.6f * Size, P.Y - 1.6f * Size, 3.2f * Size,
                       3.2f * Size, FxColor((0.25f + 0.5f * Flash) * F * Alpha, DUELIST_RGB),
                       RenderBlend_Additive);
    }
    float Line = Maximum(1.2f, 0.09f * Size);
    v2 Corners[5] = {Top, Right, Bottom, Left, Top};
    for(u32 Edge = 0; Edge < 4; Edge++)
    {
        DrawFxStroke(RenderContext, Corners[Edge], Corners[Edge + 1], Line + 1.f, Line + 1.f, Dark, Dark,
                     RenderBlend_Alpha);
    }
    u32 Rim = FxColor(Alpha, Fill > 0.f ? 0x00F0E0FF : 0x00A08098);
    for(u32 Edge = 0; Edge < 4; Edge++)
    {
        DrawFxStroke(RenderContext, Corners[Edge], Corners[Edge + 1], Line, Line, Rim, Rim,
                     RenderBlend_Alpha);
    }
}

// NOTE(zoubir): half of a diamond breaking off: Right picks the half, Age
// seconds since the break, falling and turning away
internal void
DrawDuelistShard(render_context *RenderContext, v2 P, float Size, bool32 Right, float Age,
                 float Alpha)
{
    float Way = Right ? 1.f : -1.f;
    float H = 0.5f * Size;
    float W = 0.27f * Size;
    v2 Drift = V2(Way * 40.f * Age, -30.f * Age + 260.f * Age * Age);
    float Turn = Way * 3.f * Age;
    v2 C = P + Drift + V2(Way * 0.12f * Size, 0.f);
    v2 A = C + DuelistRotate(V2(-Way * 0.12f * Size, -H), Turn);
    v2 B = C + DuelistRotate(V2(Way * (W - 0.12f * Size), 0.f), Turn);
    v2 D = C + DuelistRotate(V2(-Way * 0.12f * Size, H), Turn);
    // NOTE(zoubir): the crack is a jag down the middle
    v2 J = C + DuelistRotate(V2(-Way * 0.2f * Size, 0.05f * Size), Turn);
    u32 Fill = FxColor(0.85f * Alpha, DUELIST_RGB);
    u32 Rim = FxColor(Alpha, 0x00F0E0FF);
    DrawFilledQuad(RenderContext, A, B, D, J, Fill, Fill, Fill, Fill, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, A, B, 1.4f, 1.4f, Rim, Rim, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, B, D, 1.4f, 1.4f, Rim, Rim, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, D, J, 1.2f, 1.2f, FxColor(Alpha, 0x00FFFFFF), FxColor(Alpha, 0x00FFFFFF),
                 RenderBlend_Alpha);
    DrawFxStroke(RenderContext, J, A, 1.2f, 1.2f, FxColor(Alpha, 0x00FFFFFF), FxColor(Alpha, 0x00FFFFFF),
                 RenderBlend_Alpha);
}

// NOTE(zoubir): a heart centred on C, Size across, in RGB with a pale
// heart inside and a glow round it
internal void
DrawDuelistHeart(render_context *RenderContext, v2 C, float Size, u32 RGB, float Alpha)
{
    if (Alpha <= 0.01f)
    {
        return;
    }
    DrawShaderQuad(RenderContext, Shader_Glow, C.X - 1.2f * Size, C.Y - 1.2f * Size, 2.4f * Size,
                   2.4f * Size, FxColor(0.4f * Alpha, RGB), RenderBlend_Additive);
    for(u32 Layer = 0; Layer < 3; Layer++)
    {
        // NOTE(zoubir): a dark rim, the rose heart, a pale shine in it
        float S = Layer == 0 ? Size + 3.f : (Layer == 1 ? Size : 0.45f * Size);
        u32 Color = Layer == 0 ? FxColor(0.8f * Alpha, 0x00200A20) :
            FxColor(Alpha, Layer == 1 ? RGB : DUELIST_PALE_RGB);
        v2 At = Layer == 2 ? C + V2(-0.12f * Size, -0.1f * Size) : C;
        float R = 0.27f * S;
        DrawDuelistDisc(RenderContext, At + V2(-0.24f * S, -0.12f * S), R, Color);
        DrawDuelistDisc(RenderContext, At + V2(0.24f * S, -0.12f * S), R, Color);
        DrawFilledQuad(RenderContext, At + V2(-0.5f * S, -0.06f * S), At + V2(0.f, 0.02f * S),
                       At + V2(0.5f * S, -0.06f * S), At + V2(0.f, 0.48f * S), Color, Color, Color, Color,
                       RenderBlend_Alpha);
    }
}

// NOTE(zoubir): a fencer's afterimage standing at Feet, Height tall,
// side-on toward Facing (+1 right, -1 left): a rose body, the lead leg
// out, the off arm up behind, the blade arm out in front
internal void
DrawDuelistGhost(render_context *RenderContext, v2 Feet, float Width, float Height, float Facing,
                 float Alpha)
{
    if (Alpha <= 0.01f)
    {
        return;
    }
    v2 Hip = Feet - V2(0.f, 0.45f * Height);
    v2 Neck = Feet - V2(0.f, 0.75f * Height);
    v2 Head = Feet - V2(0.f, 0.86f * Height);
    u32 Body = FxColor(0.55f * Alpha, DUELIST_RGB);
    u32 Rim = FxColor(0.8f * Alpha, DUELIST_PALE_RGB);
    DrawShaderQuad(RenderContext, Shader_Glow, Hip.X - 0.9f * Width, Hip.Y - 0.6f * Height, 1.8f * Width,
                   1.3f * Height, FxColor(0.45f * Alpha, DUELIST_RGB), RenderBlend_Additive);
    DrawFxStroke(RenderContext, Hip, Neck, 0.38f * Width, 0.3f * Width, Body, Body);
    DrawFxStroke(RenderContext, Hip, Feet + V2(Facing * 0.42f * Width, 0.f), 0.16f * Width, 0.1f * Width,
                 Body, Body);
    DrawFxStroke(RenderContext, Hip, Feet + V2(-Facing * 0.3f * Width, 0.f), 0.16f * Width, 0.1f * Width,
                 Body, Body);
    DrawFxStroke(RenderContext, Neck, Neck + V2(Facing * 0.55f * Width, 0.08f * Height), 0.12f * Width,
                 0.08f * Width, Rim, Rim);
    DrawFxStroke(RenderContext, Neck, Neck + V2(-Facing * 0.35f * Width, -0.12f * Height), 0.12f * Width,
                 0.08f * Width, Body, Body);
    DrawDuelistDisc(RenderContext, Head, 0.17f * Width, Rim, RenderBlend_Additive);
}

// NOTE(zoubir): a rose petal at P, Size long, turned by Angle
inline void
DrawDuelistPetal(render_context *RenderContext, v2 P, float Size, float Angle, float Alpha)
{
    v2 Dir = V2(Cos(Angle), Sin(Angle));
    v2 Side = V2(-Dir.Y, Dir.X);
    u32 Rim = FxColor(Alpha, DUELIST_RGB);
    u32 Heart = FxColor(Alpha, DUELIST_PALE_RGB);
    DrawFilledQuad(RenderContext, P - (0.5f * Size) * Dir, P + (0.3f * Size) * Side,
                   P + (0.5f * Size) * Dir, P - (0.3f * Size) * Side, Rim, Heart, Rim, Rim,
                   RenderBlend_Alpha);
}
