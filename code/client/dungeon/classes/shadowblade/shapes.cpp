/* Shadowblade shapes (classes/shadowblade.cpp, before its look and bursts):
   the pieces every Shadowblade effect is drawn from: a dagger, a curved
   cut, a body-shaped shadow for afterimages and the clone, smoke puffs
   and the combo-point gems. All in screen space, from the fx clock. */

#define SHADOWBLADE_RGB 0x00FF6EAA
#define SHADOWBLADE_PALE_RGB 0x00FFC8E6
#define SHADOWBLADE_DEEP_RGB 0x005A143C
#define SHADOWBLADE_ACID_RGB 0x0046FF96
#define SHADOWBLADE_STEEL_RGB 0x00F4E8E0
#define SHADOWBLADE_SMOKE_RGB 0x00301A26

// NOTE(zoubir): seconds since the newest burst Index of the Shadowblade in
// slot SlotIndex, or a big number when none is playing
internal float
ShadowbladeBurstAge(app_state *AppState, u32 SlotIndex, u32 Index)
{
    role_fx *Fx = GetRoleFx(AppState);
    float Clock = GetFxClock(AppState);
    float Result = 100.f;
    sim_burst Kind = ClassBurst(SimBurst_ShadowbladeFirst, Index);
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

inline float
ShadowbladeEase(float T)
{
    float C = Clamp01(T);
    float Result = C * C * (3.f - 2.f * C);
    return Result;
}

// NOTE(zoubir): how much of an action of Length seconds, Age into it,
// still holds the pose: a quick blend in, then back out over its end
inline float
ShadowbladeHold(float Age, float Length)
{
    float Result = 0.f;
    if (Age >= 0.f && Age < Length)
    {
        Result = Minimum(Clamp01(Age / 0.04f), 1.f - ShadowbladeEase((Age - 0.6f * Length) / (0.4f * Length)));
    }
    return Result;
}

inline v2
ShadowbladeLerp(v2 A, v2 B, float T)
{
    v2 Result = A + T * (B - A);
    return Result;
}

inline v2
ShadowbladeNormal(v2 V)
{
    v2 Result = LengthSq(V) > 0.0001f ? DirectionTo(V) : V2(0.f, 1.f);
    return Result;
}

inline v2
ShadowbladeRotate(v2 V, float Angle)
{
    float C = Cos(Angle);
    float S = Sin(Angle);
    v2 Result = V2(C * V.X - S * V.Y, S * V.X + C * V.Y);
    return Result;
}

// NOTE(zoubir): a dagger whose grip is at Grip, its blade Length long
// along Dir (a unit vector): a dark outline, a steel blade lit along one
// edge, a violet crossguard and an acid-green pommel stone. Glow lights
// the edge (a critical strike waiting), Alpha fades it all
internal void
DrawShadowbladeDagger(render_context *RenderContext, v2 Grip, v2 Dir, float Length,
                      float Glow, float Alpha)
{
    v2 Side = V2(-Dir.Y, Dir.X);
    float W = 0.15f * Length;
    v2 Base = Grip + (0.18f * Length) * Dir;
    v2 Tip = Grip + Length * Dir;
    v2 Belly = Base + (0.45f * Length) * Dir;
    u32 Dark = FxColor(0.9f * Alpha, 0x00140A12);
    // NOTE(zoubir): the outline, one pixel out round the blade
    DrawFilledQuad(RenderContext, Base - (W + 1.5f) * Side, Belly - (W + 1.2f) * Side,
                   Tip + 2.f * Dir, Base + (W + 1.5f) * Side, Dark, Dark, Dark, Dark,
                   RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Base + (W + 1.5f) * Side, Belly + (0.4f * W + 1.2f) * Side,
                   Tip + 2.f * Dir, Tip + 2.f * Dir, Dark, Dark, Dark, Dark, RenderBlend_Alpha);
    // NOTE(zoubir): the blade: a bright edge and a darker back
    u32 Edge = FxColor(Alpha, SHADOWBLADE_STEEL_RGB);
    u32 Back = FxColor(Alpha, 0x00A08C90);
    DrawFilledQuad(RenderContext, Base - W * Side, Belly - W * Side, Tip, Base,
                   Edge, Edge, Edge, Back, RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Base, Tip, Belly + 0.4f * W * Side, Base + W * Side,
                   Back, Back, Back, Back, RenderBlend_Alpha);
    // NOTE(zoubir): a violet sheen down the fuller
    DrawFxStroke(RenderContext, Base + 0.1f * Length * Dir, Tip - 0.15f * Length * Dir,
                 0.5f * W, 0.f, FxColor(0.55f * Alpha, SHADOWBLADE_RGB),
                 FxColor(0.f, SHADOWBLADE_RGB));
    if (Glow > 0.f)
    {
        DrawFxStroke(RenderContext, Base, Tip + 2.f * Dir, 3.f * W, 0.5f * W,
                     FxColor(0.7f * Glow * Alpha, SHADOWBLADE_ACID_RGB),
                     FxColor(0.3f * Glow * Alpha, SHADOWBLADE_ACID_RGB));
        DrawShaderQuad(RenderContext, Shader_Glow, Belly.X - 1.2f * Length, Belly.Y - 1.2f * Length,
                       2.4f * Length, 2.4f * Length, FxColor(0.35f * Glow * Alpha, SHADOWBLADE_ACID_RGB),
                       RenderBlend_Additive);
    }
    // NOTE(zoubir): the guard across the grip, the wrapped grip, the stone
    v2 Guard = Grip + (0.16f * Length) * Dir;
    DrawFxStroke(RenderContext, Guard - (1.9f * W) * Side, Guard + (1.9f * W) * Side,
                 0.32f * Length, 0.32f * Length, Dark, Dark, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Guard - (1.6f * W) * Side, Guard + (1.6f * W) * Side,
                 0.18f * Length, 0.18f * Length, FxColor(Alpha, 0x00C05A8C),
                 FxColor(Alpha, 0x00FF9AD2), RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Grip - (0.14f * Length) * Dir, Guard, 0.2f * Length,
                 0.2f * Length, FxColor(Alpha, 0x00281420), FxColor(Alpha, 0x00281420),
                 RenderBlend_Alpha);
    v2 Pommel = Grip - (0.17f * Length) * Dir;
    DrawFxDot(RenderContext, Pommel, 0.14f * Length + 1.5f, FxColor(Alpha, 0x00142814));
    DrawFxDot(RenderContext, Pommel, 0.14f * Length, FxColor(Alpha, SHADOWBLADE_ACID_RGB));
}

// NOTE(zoubir): a curved cut, from From to To bowing out by Bow (a share
// of its length, sideways), Width at its widest, drawn up to Reveal
// (0..1 of its length) as a cutting blade would, its tail fading
internal void
DrawShadowbladeCut(render_context *RenderContext, v2 From, v2 To, float Bow, float Width,
                   float Reveal, float Alpha, u32 RGB)
{
    v2 Along = To - From;
    v2 Side = V2(-Along.Y, Along.X);
    u32 Steps = 12;
    v2 Last = From;
    for(u32 Step = 1; Step <= Steps; Step++)
    {
        float T = (float)Step / (float)Steps;
        if (T > Reveal + 0.001f)
        {
            break;
        }
        float LastT = (float)(Step - 1) / (float)Steps;
        v2 P = From + T * Along + (4.f * Bow * T * (1.f - T)) * Side;
        // NOTE(zoubir): thin at both ends, fat in the middle, and brightest
        // just behind the blade's edge
        float WA = Width * Sin(Pi32 * LastT);
        float WB = Width * Sin(Pi32 * T);
        float Near = 1.f - Clamp01((Reveal - T) * 1.6f);
        DrawFxStroke(RenderContext, Last, P, 2.4f * WA, 2.4f * WB,
                     FxColor(0.3f * Alpha, RGB), FxColor(0.3f * Alpha, RGB), RenderBlend_Alpha);
        DrawFxStroke(RenderContext, Last, P, WA, WB, FxColor(Alpha * (0.5f + 0.5f * Near), 0x00FFF4FF),
                     FxColor(Alpha * (0.5f + 0.5f * Near), 0x00FFF4FF));
        Last = P;
    }
}

// NOTE(zoubir): a person-shaped shadow standing at Feet, Height tall: a
// soft body, shoulders and a head, dark with a violet rim; for the
// afterimages and Shadow Dance's clone
internal void
DrawShadowbladeSilhouette(render_context *RenderContext, v2 Feet, float Width, float Height,
                          float Alpha)
{
    if (Alpha <= 0.01f)
    {
        return;
    }
    v2 Head = Feet - V2(0.f, 0.82f * Height);
    v2 Chest = Feet - V2(0.f, 0.5f * Height);
    DrawShaderQuad(RenderContext, Shader_Glow, Chest.X - 0.9f * Width, Chest.Y - 0.65f * Height,
                   1.8f * Width, 1.3f * Height, FxColor(0.8f * Alpha, SHADOWBLADE_RGB),
                   RenderBlend_Additive);
    DrawShaderQuad(RenderContext, Shader_Glow, Chest.X - 0.6f * Width, Chest.Y - 0.45f * Height,
                   1.2f * Width, 0.95f * Height, FxColor(0.85f * Alpha, SHADOWBLADE_DEEP_RGB),
                   RenderBlend_Alpha);
    DrawShaderQuad(RenderContext, Shader_Glow, Head.X - 0.32f * Width, Head.Y - 0.32f * Width,
                   0.64f * Width, 0.64f * Width, FxColor(0.9f * Alpha, SHADOWBLADE_DEEP_RGB),
                   RenderBlend_Alpha);
    // NOTE(zoubir): two eyes catching the light
    DrawFxDot(RenderContext, Head + V2(-0.07f * Width, 0.f), 2.f, FxColor(Alpha, SHADOWBLADE_PALE_RGB));
    DrawFxDot(RenderContext, Head + V2(0.07f * Width, 0.f), 2.f, FxColor(Alpha, SHADOWBLADE_PALE_RGB));
}

// NOTE(zoubir): a soft puff of smoke, dark over the ground with a faint
// violet light in it
inline void
DrawShadowbladePuff(render_context *RenderContext, v2 P, float Size, float Alpha)
{
    DrawShaderQuad(RenderContext, Shader_Glow, P.X - Size, P.Y - 0.8f * Size, 2.f * Size,
                   1.6f * Size, FxColor(0.75f * Alpha, SHADOWBLADE_SMOKE_RGB), RenderBlend_Alpha);
    DrawShaderQuad(RenderContext, Shader_Glow, P.X - 0.6f * Size, P.Y - 0.6f * Size, 1.2f * Size,
                   1.f * Size, FxColor(0.18f * Alpha, SHADOWBLADE_RGB), RenderBlend_Additive);
}

// NOTE(zoubir): a combo point's gem at P, Size across: a dark socket, then
// when Lit a violet diamond with a pale facet; Flash brightens it for a
// fresh point, Full for five
internal void
DrawShadowbladeGem(render_context *RenderContext, v2 P, float Size, float Lit, float Flash,
                   float Full, float Alpha)
{
    float H = 0.5f * Size;
    u32 Socket = FxColor(0.8f * Alpha, 0x00140A12);
    DrawFilledQuad(RenderContext, P + V2(0.f, -H - 1.5f), P + V2(H + 1.5f, 0.f),
                   P + V2(0.f, H + 1.5f), P + V2(-H - 1.5f, 0.f), Socket, Socket, Socket, Socket,
                   RenderBlend_Alpha);
    u32 Empty = FxColor(0.5f * Alpha, 0x004A2A44);
    DrawFilledQuad(RenderContext, P + V2(0.f, -0.7f * H), P + V2(0.7f * H, 0.f),
                   P + V2(0.f, 0.7f * H), P + V2(-0.7f * H, 0.f), Empty, Empty, Empty, Empty,
                   RenderBlend_Alpha);
    if (Lit <= 0.f)
    {
        return;
    }
    float L = Lit * Alpha;
    u32 Top = FxColor(L, Full > 0.f ? 0x00FFE0F4 : SHADOWBLADE_PALE_RGB);
    u32 Bottom = FxColor(L, Full > 0.f ? SHADOWBLADE_RGB : 0x00C0408A);
    float S = H * (0.8f + 0.2f * Lit);
    DrawFilledQuad(RenderContext, P + V2(0.f, -S), P + V2(S, 0.f), P + V2(0.f, S), P + V2(-S, 0.f),
                   Top, Top, Bottom, Top, RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, P + V2(-0.15f * S, -0.6f * S), P + V2(0.25f * S, -0.2f * S),
                   P + V2(-0.05f * S, 0.05f * S), P + V2(-0.4f * S, -0.25f * S),
                   FxColor(0.85f * L, 0x00FFFFFF), FxColor(0.2f * L, 0x00FFFFFF),
                   FxColor(0.f, 0x00FFFFFF), FxColor(0.4f * L, 0x00FFFFFF), RenderBlend_Additive);
    float Glow = 0.35f + 0.6f * Flash + 0.3f * Full;
    DrawShaderQuad(RenderContext, Shader_Glow, P.X - 2.f * Size, P.Y - 2.f * Size, 4.f * Size,
                   4.f * Size, FxColor(Glow * L, Full > 0.f ? 0x00FF90E0 : SHADOWBLADE_RGB),
                   RenderBlend_Additive);
}
