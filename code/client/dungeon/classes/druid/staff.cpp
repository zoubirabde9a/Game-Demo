/* Druid shapes (classes/druid.cpp): what the Druid's look and its
   effects are drawn from, in screen space: the gnarled staff with its
   glowing seed, the antlers, a leaf, a five-petal flower (the Bloom over
   its head, Regrowth), a falling star. Also when this Druid last cast
   something, read from the bursts in flight, so a remote Druid's staff
   moves the same as the local one's. */

#define DRUID_FX_LEAF_RGB 0x003CC8A5
#define DRUID_FX_PALE_RGB 0x00AAF5E1
#define DRUID_FX_DEEP_RGB 0x001E6E46
#define DRUID_FX_BARK_RGB 0x0028415F
#define DRUID_FX_BARK_LIGHT_RGB 0x00466E96
#define DRUID_FX_MOON_RGB 0x00FFC8AA
#define DRUID_FX_STAR_RGB 0x008CE1FF
#define DRUID_FX_PETAL_RGB 0x00BE96FF

// NOTE(zoubir): the youngest burst Index of the Druid in SlotIndex, how
// long ago it started; a big number for none
internal float
DruidBurstAge(app_state *AppState, u32 SlotIndex, u32 Index)
{
    role_fx *Fx = GetRoleFx(AppState);
    float Clock = GetFxClock(AppState);
    float Result = 1000.f;
    for(u32 Row = 0; Row < Fx->Count; Row++)
    {
        role_burst *Burst = &Fx->Bursts[Row];
        if (Burst->Kind == ClassBurst(SimBurst_DruidFirst, Index) && Burst->Slot == SlotIndex)
        {
            Result = Minimum(Result, Clock - Burst->Start);
        }
    }
    return Result;
}

// NOTE(zoubir): a rise to full over In, a hold, and a fall to nothing
// over Out at the end of T 0..1 of Life seconds
inline float
DruidEnvelope(float T, float Life, float In, float Out)
{
    float Seconds = T * Life;
    float Result = Minimum(Clamp01(Seconds / Maximum(0.01f, In)),
                           Clamp01((Life - Seconds) / Maximum(0.01f, Out)));
    return Result;
}

// NOTE(zoubir): a leaf at P pointing along Dir, Size long: two halves
// round a vein, Alpha over all of it
internal void
DrawDruidLeaf(render_context *RenderContext, v2 P, v2 Dir, float Size, float Alpha, u32 RGB)
{
    v2 Side = V2(-Dir.Y, Dir.X);
    v2 Tip = P + (0.5f * Size) * Dir;
    v2 Stem = P - (0.5f * Size) * Dir;
    v2 Wide = (0.28f * Size) * Side;
    u32 Fill = FxColor(Alpha, RGB);
    u32 Edge = FxColor(Alpha, DRUID_FX_DEEP_RGB);
    DrawFilledQuad(RenderContext, Stem, P + Wide, Tip, P - Wide, Edge, Fill, Fill, Fill,
                   RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Stem - (0.15f * Size) * Dir, Tip, 1.f, 0.5f,
                 FxColor(0.7f * Alpha, DRUID_FX_PALE_RGB), FxColor(0.2f * Alpha, DRUID_FX_PALE_RGB),
                 RenderBlend_Alpha);
}

// NOTE(zoubir): a five-petal flower at C, R across; Open 0..1 how far its
// petals are out, Alpha over all of it
internal void
DrawDruidFlower(render_context *RenderContext, v2 C, float R, float Open, float Spin, float Alpha,
                u32 RGB)
{
    for(u32 Petal = 0; Petal < 5; Petal++)
    {
        float A = Spin + 2.f * Pi32 * (float)Petal / 5.f;
        v2 Dir = V2(Cos(A), Sin(A));
        v2 Side = V2(-Dir.Y, Dir.X);
        float Reach = R * (0.35f + 0.65f * Open);
        v2 Tip = C + Reach * Dir;
        v2 Mid = C + (0.55f * Reach) * Dir;
        float Wide = 0.42f * R * (0.4f + 0.6f * Open);
        DrawFilledQuad(RenderContext, C, Mid + Wide * Side, Tip, Mid - Wide * Side,
                       FxColor(Alpha, DRUID_FX_PALE_RGB), FxColor(Alpha, RGB), FxColor(Alpha, RGB),
                       FxColor(Alpha, RGB), RenderBlend_Alpha);
    }
    DrawShaderQuad(RenderContext, Shader_Glow, C.X - 0.5f * R, C.Y - 0.5f * R, R, R,
                   FxColor(0.9f * Alpha, DRUID_FX_STAR_RGB), RenderBlend_Additive);
}

// NOTE(zoubir): an empty flower's place: a faint ring of five dots
internal void
DrawDruidBud(render_context *RenderContext, v2 C, float R, float Alpha)
{
    for(u32 Petal = 0; Petal < 5; Petal++)
    {
        float A = 2.f * Pi32 * (float)Petal / 5.f - 0.5f * Pi32;
        DrawFxDot(RenderContext, C + (0.5f * R) * V2(Cos(A), Sin(A)), 2.f,
                  FxColor(0.45f * Alpha, DRUID_FX_DEEP_RGB));
    }
}

// NOTE(zoubir): a star at P, R across, four long rays and a glow
internal void
DrawDruidStar(render_context *RenderContext, v2 P, float R, float Spin, float Alpha, u32 RGB)
{
    DrawShaderQuad(RenderContext, Shader_Glow, P.X - 1.6f * R, P.Y - 1.6f * R, 3.2f * R, 3.2f * R,
                   FxColor(0.7f * Alpha, RGB), RenderBlend_Additive);
    for(u32 Ray = 0; Ray < 4; Ray++)
    {
        float A = Spin + 0.5f * Pi32 * (float)Ray;
        float Long = (Ray % 2 ? 0.7f : 1.f) * R;
        DrawFxStroke(RenderContext, P, P + Long * V2(Cos(A), Sin(A)), 0.35f * R, 0.f,
                     FxColor(Alpha, 0x00FFFFFF), FxColor(0.f, RGB));
    }
}

// NOTE(zoubir): the staff, standing at Foot and leaning by Lean (radians
// from upright), Size tall, its knotted head holding a seed that glows
// Glow 0..1 in GlowRGB; returns where the seed is
internal v2
DrawDruidStaff(render_context *RenderContext, v2 Foot, float Lean, float Size, float Glow,
               u32 GlowRGB, float Clock)
{
    v2 Up = V2(Sin(Lean), -Cos(Lean));
    v2 Side = V2(-Up.Y, Up.X);
    // NOTE(zoubir): the shaft wanders a little, as a branch does
    v2 Points[6];
    for(u32 Knot = 0; Knot < 6; Knot++)
    {
        float Along = (float)Knot / 5.f;
        float Bend = 0.035f * Size * Sin(7.f * Along + 1.3f);
        Points[Knot] = Foot + (Along * Size) * Up + Bend * Side;
    }
    for(u32 Knot = 0; Knot < 5; Knot++)
    {
        float W = 3.6f - 0.5f * (float)Knot;
        DrawFxStroke(RenderContext, Points[Knot], Points[Knot + 1], W + 1.6f, W + 1.4f,
                     FxColor(1.f, DRUID_FX_BARK_RGB), FxColor(1.f, DRUID_FX_BARK_RGB), RenderBlend_Alpha);
        DrawFxStroke(RenderContext, Points[Knot] - 0.6f * Side, Points[Knot + 1] - 0.6f * Side,
                     0.6f * W, 0.6f * W, FxColor(0.8f, DRUID_FX_BARK_LIGHT_RGB),
                     FxColor(0.8f, DRUID_FX_BARK_LIGHT_RGB), RenderBlend_Alpha);
    }
    // NOTE(zoubir): the head: two branches curling round the seed
    v2 Top = Points[5];
    v2 Seed = Top + (0.09f * Size) * Up;
    for(float Sign = -1.f; Sign <= 1.f; Sign += 2.f)
    {
        v2 Last = Top;
        for(u32 Step = 1; Step <= 4; Step++)
        {
            // NOTE(zoubir): round the seed from below it to nearly over it
            float A = 0.85f * Pi32 * (float)Step / 4.f;
            float R = 0.09f * Size;
            v2 P = Seed - (R * Cos(A)) * Up + (Sign * R * Sin(A)) * Side;
            DrawFxStroke(RenderContext, Last, P, 2.6f - 0.4f * (float)Step, 2.2f - 0.4f * (float)Step,
                         FxColor(1.f, DRUID_FX_BARK_RGB), FxColor(1.f, DRUID_FX_BARK_RGB), RenderBlend_Alpha);
            Last = P;
        }
    }
    // NOTE(zoubir): a leaf or two sprouting from the knots
    DrawDruidLeaf(RenderContext, Points[3] + 5.f * Side, NormalizeOr(Side + 0.6f * Up, Side),
                  9.f, 1.f, DRUID_FX_LEAF_RGB);
    DrawDruidLeaf(RenderContext, Points[4] - 5.f * Side, NormalizeOr(-1.f * Side + 0.8f * Up, Side),
                  7.f, 1.f, DRUID_FX_LEAF_RGB);
    // NOTE(zoubir): the seed, always faintly alight
    float Pulse = 0.85f + 0.15f * Sin(3.f * Clock);
    float Light = 0.35f + 0.65f * Glow;
    float G = (7.f + 12.f * Glow) * Pulse;
    DrawShaderQuad(RenderContext, Shader_Glow, Seed.X - G, Seed.Y - G, 2.f * G, 2.f * G,
                   FxColor(0.8f * Light, GlowRGB), RenderBlend_Additive);
    DrawFxDot(RenderContext, Seed, 4.f + 2.f * Glow, FxColor(Light, 0x00FFFFFF));
    return Seed;
}

// NOTE(zoubir): antlers on the head at Crown, Size across, branching
// twice on each side, a leaf caught in them
internal void
DrawDruidAntlers(render_context *RenderContext, v2 Crown, float Size, float Facing)
{
    u32 Bone = FxColor(1.f, DRUID_FX_BARK_LIGHT_RGB);
    for(float Sign = -1.f; Sign <= 1.f; Sign += 2.f)
    {
        v2 Base = Crown + V2(Sign * 0.18f * Size, 0.f);
        v2 Mid = Base + V2(Sign * 0.22f * Size, -0.32f * Size);
        v2 Tip = Mid + V2(Sign * 0.08f * Size, -0.3f * Size);
        DrawFxStroke(RenderContext, Base, Mid, 2.6f, 2.f, Bone, Bone, RenderBlend_Alpha);
        DrawFxStroke(RenderContext, Mid, Tip, 2.f, 1.f, Bone, Bone, RenderBlend_Alpha);
        DrawFxStroke(RenderContext, Base + 0.5f * (Mid - Base), Base + 0.5f * (Mid - Base) +
                     V2(Sign * 0.2f * Size, -0.08f * Size), 1.6f, 0.8f, Bone, Bone, RenderBlend_Alpha);
        DrawFxStroke(RenderContext, Mid, Mid + V2(-Sign * 0.06f * Size, -0.2f * Size), 1.5f, 0.8f,
                     Bone, Bone, RenderBlend_Alpha);
    }
    DrawDruidLeaf(RenderContext, Crown + V2(Facing * 0.2f * Size, -0.12f * Size),
                  NormalizeOr(V2(Facing, -0.6f), V2(1.f, 0.f)), 0.32f * Size, 1.f, DRUID_FX_LEAF_RGB);
}
