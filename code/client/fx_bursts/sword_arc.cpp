/* The sword's swing (BurstShape_Arc and BurstShape_ArcBack in
   fx_bursts.cpp): the blade itself sweeping across the slice the cut
   hits, and the crescent of light its edge leaves behind. */

// NOTE(zoubir): a swing, T going 0..1 over the burst:
// - the blade (a steel edge, a bronze guard and a dark grip) whips from
//   one side of the slice to the other in the first SWING_SWEEP, fast
//   out of the start and slowing as it arrives, then hangs at the end of
//   its follow-through and fades;
// - its edge leaves a crescent at the reach the cut hits at: pointed at
//   both ends, thickest a little behind the blade, with a white-hot rim
//   on the outside and a few thin streaks of motion inside it. The tail
//   chases the blade round and the crescent thins away;
// - a flare rides the blade's tip while it moves, and once it stops a
//   few sparks fly on from the end of the cut.
// The finisher is wider and gold, with a second crescent inside the
// first. Drawn round the chest over the
// slice the sim hits (IsInSwordSlice)
#define SWING_SWEEP 0.32f
#define SWING_SEGMENTS 28
#define SWING_CHEST 12.f
// NOTE(zoubir): the blade, from the hand outward, as shares of the reach
#define SWING_GRIP 0.06f
#define SWING_GUARD 0.16f
#define SWING_BLADE 0.6f

// NOTE(zoubir): the crescent's thickness at S, 0 at the tail to 1 at the
// tip: nothing at either end, most about two thirds of the way along
inline float
SwingProfile(float S)
{
    float Result = Sin(Pi32 * powf(Clamp01(S), 1.6f));
    return Result;
}

// NOTE(zoubir): one crescent between Tail and Lead (0..1 across the
// slice), its outside edge at Radius, at most Width deep, at Alpha
internal void
DrawSwingCrescent(render_context *RenderContext, v2 Centre, float Angle,
                  float Side, float HalfAngle, float Radius, float Width,
                  float Tail, float Lead, float Alpha, u32 RGB)
{
    if (Lead - Tail <= 0.001f || Alpha <= 0.f)
    {
        return;
    }
    v2 LastOuter = {}, LastInner = {}, LastRim = {}, LastHalo = {};
    u32 LastBody = 0, LastRimColor = 0;
    for(u32 Index = 0; Index <= SWING_SEGMENTS; Index++)
    {
        float S = (float)Index / SWING_SEGMENTS;
        float Along = Tail + S * (Lead - Tail);
        float At = Angle + Side * HalfAngle * (2.f * Along - 1.f);
        v2 Out = V2(Cos(At), Sin(At));
        float Profile = SwingProfile(S);
        v2 Outer = Centre + Radius * Out;
        v2 Inner = Centre + (Radius - Width * Profile) * Out;
        v2 Rim = Centre + (Radius - 1.f - 2.5f * Profile) * Out;
        v2 Halo = Centre + (Radius + 1.f + 3.f * Profile) * Out;
        // NOTE(zoubir): brighter toward the blade
        float Strength = Alpha * (0.2f + 0.8f * S);
        u32 Body = FxColor(0.9f * Strength, RGB);
        u32 RimColor = FxColor(Strength * Minimum(1.f, 2.f * Profile + 0.3f),
                               0x00FFFFFF);
        if (Index > 0)
        {
            DrawFilledQuad(RenderContext, LastInner, LastOuter, Outer, Inner,
                           FxColor(0.f, RGB), LastBody, Body, FxColor(0.f, RGB),
                           RenderBlend_Additive);
            DrawFilledQuad(RenderContext, LastOuter, LastHalo, Halo, Outer,
                           LastBody, FxColor(0.f, RGB), FxColor(0.f, RGB), Body,
                           RenderBlend_Additive);
            DrawFilledQuad(RenderContext, LastRim, LastOuter, Outer, Rim,
                           LastRimColor, LastRimColor, RimColor, RimColor);
        }
        LastOuter = Outer;
        LastInner = Inner;
        LastRim = Rim;
        LastHalo = Halo;
        LastBody = Body;
        LastRimColor = RimColor;
    }

    // NOTE(zoubir): streaks of motion inside the crescent, each a little
    // behind the one outside it
    for(u32 Streak = 0; Streak < 3; Streak++)
    {
        float Depth = 0.18f + 0.12f * Streak;
        float StreakLead = Lead - (0.04f + 0.05f * Streak) * (Lead - Tail);
        float StreakTail = Tail + (0.3f + 0.1f * Streak) * (Lead - Tail);
        if (StreakLead <= StreakTail)
        {
            continue;
        }
        v2 Last = {};
        for(u32 Index = 0; Index <= 12; Index++)
        {
            float S = (float)Index / 12.f;
            float Along = StreakTail + S * (StreakLead - StreakTail);
            float At = Angle + Side * HalfAngle * (2.f * Along - 1.f);
            v2 P = Centre + (Radius - Depth * Width * 1.6f) * V2(Cos(At), Sin(At));
            if (Index > 0)
            {
                float Fade = Alpha * 0.5f * (1.f - 0.25f * Streak);
                DrawFxStroke(RenderContext, Last, P, 1.5f * (S - 1.f / 12.f),
                             1.5f * S, FxColor(Fade * (S - 1.f / 12.f), 0x00FFFFFF),
                             FxColor(Fade * S, 0x00FFFFFF));
            }
            Last = P;
        }
    }
}

// NOTE(zoubir): the sword itself, pointing along Out from the hand, at
// Alpha
internal void
DrawSwingBlade(render_context *RenderContext, v2 Centre, v2 Out,
               float Radius, float Alpha, bool32 Finisher)
{
    if (Alpha <= 0.f)
    {
        return;
    }
    v2 Across = V2(-Out.Y, Out.X);
    v2 Grip = Centre + SWING_GRIP * Radius * Out;
    v2 Guard = Centre + SWING_GUARD * Radius * Out;
    v2 Tip = Centre + SWING_BLADE * Radius * Out;
    u32 A = (u32)(255.f * Clamp01(Alpha)) << 24;
    // NOTE(zoubir): a dark outline first, so the steel reads on pale
    // stone; then the blade, darker steel on one half and light on the
    // other, so it reads as a ridged edge, with a white line on the side
    // that cuts; then the grip and the guard across it, outlined too
    v2 Point = Tip + 3.f * Out;
    DrawFxStroke(RenderContext, Grip - 1.5f * Out, Guard, 6.f, 6.f,
                 A | 0x00101418, A | 0x00101418, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Guard, Point + 1.5f * Out, 10.f, 2.f,
                 A | 0x00101418, A | 0x00101418, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Guard - 1.5f * Across, Point - 1.5f * Across,
                 3.f, 0.f, A | (Finisher ? 0x003078B0 : 0x00807068),
                 A | (Finisher ? 0x0050A0D0 : 0x00A09088), RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Guard + 1.5f * Across, Point, 3.f, 0.f,
                 A | (Finisher ? 0x0090E0FF : 0x00F0E8E0),
                 A | 0x00FFFFFF, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Guard + 2.6f * Across, Point, 1.f, 0.f,
                 A | 0x00FFFFFF, A | 0x00FFFFFF, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Grip, Guard, 3.f, 3.f, A | 0x00284060,
                 A | 0x00385880, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Guard - 7.f * Across, Guard + 7.f * Across,
                 5.5f, 5.5f, A | 0x00101418, A | 0x00101418, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Guard - 6.f * Across, Guard + 6.f * Across,
                 3.f, 3.f, A | 0x003C90D0, A | 0x0070C8F8, RenderBlend_Alpha);
}

internal void
DrawSwordArc(render_context *RenderContext, fx_burst *Burst, v2 Feet,
             float Radius, float T)
{
    burst_look *Look = &BurstLooks[Burst->Kind];
    bool32 Finisher = Burst->Kind == SimBurst_SwingArcFinisher;
    float HalfAngle = Finisher ? SWORD_FINISHER_HALF_ANGLE : SWORD_HALF_ANGLE;
    float Side = Look->Shape == BurstShape_Arc ? 1.f : -1.f;
    float Width = (Finisher ? 0.5f : 0.42f) * Radius;
    v2 Centre = Feet - V2(0.f, SWING_CHEST);

    // NOTE(zoubir): fast out of the start, slowing as it arrives
    float Sweep = Clamp01(T / SWING_SWEEP);
    float Lead = 1.f - (1.f - Sweep) * (1.f - Sweep) * (1.f - Sweep);
    float Tail = Minimum(Lead, Square(Clamp01((T - 0.1f) / 0.75f)));
    float Fade = 1.f - Square(Clamp01((T - SWING_SWEEP) / (1.f - SWING_SWEEP)));
    float TipAngle = Burst->Angle + Side * HalfAngle * (2.f * Lead - 1.f);
    v2 TipOut = V2(Cos(TipAngle), Sin(TipAngle));

    if (Finisher)
    {
        DrawSwingCrescent(RenderContext, Centre, Burst->Angle, Side, HalfAngle,
                          0.72f * Radius, 0.6f * Width,
                          Maximum(0.f, Tail - 0.08f), Maximum(0.f, Lead - 0.12f),
                          0.45f * Fade, Look->RGB);
    }
    DrawSwingCrescent(RenderContext, Centre, Burst->Angle, Side, HalfAngle,
                      Radius, Width, Tail, Lead, Fade, Look->RGB);

    // NOTE(zoubir): the blade hangs at the end of its follow-through a
    // moment before it fades
    float BladeAlpha = 1.f - Clamp01((T - SWING_SWEEP - 0.1f) / 0.3f);
    DrawSwingBlade(RenderContext, Centre, TipOut, Radius, BladeAlpha, Finisher);

    // NOTE(zoubir): a flare riding the tip while it moves
    if (Sweep < 1.f)
    {
        v2 Tip = Centre + Radius * TipOut;
        float Flare = (Finisher ? 40.f : 28.f) * (1.f - 0.4f * Sweep);
        DrawShaderQuad(RenderContext, Shader_Glow, Tip.X - 0.5f * Flare,
                       Tip.Y - 0.5f * Flare, Flare, Flare,
                       FxColor(1.f - 0.5f * Sweep, Look->RGB), RenderBlend_Additive);
        DrawFxDot(RenderContext, Tip, Finisher ? 4.f : 3.f, 0xFFFFFFFF);
    }

    // NOTE(zoubir): sparks thrown on from the end of the cut, along it,
    // each a short streak pointing the way it flies
    float SparkT = Clamp01((T - SWING_SWEEP) / (1.f - SWING_SWEEP));
    if (T >= SWING_SWEEP && SparkT < 1.f)
    {
        float EndAngle = Burst->Angle + Side * HalfAngle;
        v2 Out = V2(Cos(EndAngle), Sin(EndAngle));
        v2 End = Centre + Radius * Out;
        v2 Onward = Side * V2(-Out.Y, Out.X);
        u32 Count = Finisher ? 9 : 5;
        float Ease = 1.f - Square(1.f - SparkT);
        for(u32 Spark = 0; Spark < Count; Spark++)
        {
            float Speed = 0.45f * Radius * (0.5f + 0.9f * BurstJitter(Spark, 16));
            float Spread = (BurstJitter(Spark, 17) - 0.3f) * 1.1f;
            v2 Direction = NormalizeOr(Onward + Spread * Out, Onward);
            v2 P = End + Speed * Ease * Direction;
            float Trail = 7.f * (1.f - SparkT);
            u32 Color = Spark & 1 ? 0x00FFFFFF : Look->RGB;
            DrawFxStroke(RenderContext, P - Trail * Direction, P, 0.f,
                         2.5f - 1.5f * SparkT, FxColor(0.f, Color),
                         FxColor(1.f - SparkT, Color));
        }
    }
}
