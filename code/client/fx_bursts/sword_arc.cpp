/* The sword's swing (BurstShape_Arc and BurstShape_ArcBack in
   fx_bursts.cpp): a crescent the blade's tip leaves across the slice the
   cut hits. */

// NOTE(zoubir): a sword's sweep (BurstShape_Arc): the blade's tip races
// across the slice in the first SWING_SWEEP of the burst, easing out,
// leaving a crescent behind it: a glow thickest at the tip that fades
// toward the middle of the swing and in to the swinger, and a bright rim
// on the outside, where the cut hits. Once the tip is across, the tail
// runs after it and the crescent thins away; sparks fly off the far end.
// Drawn at chest height over the slice the sim hits (IsInSwordSlice)
#define SWING_SWEEP 0.45f
#define SWING_SEGMENTS 20
#define SWING_CHEST 12.f

internal void
DrawSwordArc(render_context *RenderContext, fx_burst *Burst, v2 Feet,
             float Radius, float T)
{
    burst_look *Look = &BurstLooks[Burst->Kind];
    bool32 Finisher = Burst->Kind == SimBurst_SwingArcFinisher;
    float HalfAngle = Finisher ? SWORD_FINISHER_HALF_ANGLE : SWORD_HALF_ANGLE;
    float Side = Look->Shape == BurstShape_Arc ? 1.f : -1.f;
    float Width = (Finisher ? 0.7f : 0.55f) * Radius;
    v2 Centre = Feet - V2(0.f, SWING_CHEST);

    float Sweep = Clamp01(T / SWING_SWEEP);
    float Lead = 1.f - Square(1.f - Sweep);
    float Tail = Square(Clamp01((T - 0.2f) / 0.8f));
    float Fade = 1.f - Square(Clamp01((T - SWING_SWEEP) / (1.f - SWING_SWEEP)));
    if (Lead - Tail <= 0.001f)
    {
        return;
    }

    // NOTE(zoubir): Along 0..1 across the slice, in the swing's direction
    v2 Outer[SWING_SEGMENTS + 1];
    v2 Inner[SWING_SEGMENTS + 1];
    v2 RimIn[SWING_SEGMENTS + 1];
    v2 RimOut[SWING_SEGMENTS + 1];
    float Strength[SWING_SEGMENTS + 1];
    for(u32 Index = 0; Index <= SWING_SEGMENTS; Index++)
    {
        float U = (float)Index / SWING_SEGMENTS;
        float Along = Tail + U * (Lead - Tail);
        float Angle = Burst->Angle + Side * HalfAngle * (2.f * Along - 1.f);
        v2 Out = V2(Cos(Angle), Sin(Angle));
        // NOTE(zoubir): the blade's tip swells out a little while it moves
        float Swell = 1.f + 0.06f * U * (1.f - Sweep);
        float Thickness = Width * (0.2f + 0.8f * U) * (0.4f + 0.6f * Fade);
        Outer[Index] = Centre + Radius * Swell * Out;
        Inner[Index] = Centre + (Radius * Swell - Thickness) * Out;
        RimOut[Index] = Centre + (Radius * Swell + 1.5f) * Out;
        RimIn[Index] = Centre + (Radius * Swell - 3.f) * Out;
        Strength[Index] = Fade * (0.25f + 0.75f * U);
    }
    for(u32 Index = 0; Index < SWING_SEGMENTS; Index++)
    {
        u32 A = (u32)(220.f * Strength[Index]);
        u32 B = (u32)(220.f * Strength[Index + 1]);
        DrawFilledQuad(RenderContext, Inner[Index], Outer[Index],
                       Outer[Index + 1], Inner[Index + 1],
                       Look->RGB, (A << 24) | Look->RGB,
                       (B << 24) | Look->RGB, Look->RGB,
                       RenderBlend_Additive);
        u32 RimA = (u32)(255.f * Strength[Index]);
        u32 RimB = (u32)(255.f * Strength[Index + 1]);
        DrawFilledQuad(RenderContext, RimIn[Index], RimOut[Index],
                       RimOut[Index + 1], RimIn[Index + 1],
                       (RimA << 24) | 0x00FFFFFF, (RimA << 24) | 0x00FFFFFF,
                       (RimB << 24) | 0x00FFFFFF, (RimB << 24) | 0x00FFFFFF);
    }

    // NOTE(zoubir): a flare on the tip while it sweeps
    v2 Tip = Outer[SWING_SEGMENTS];
    if (Sweep < 1.f)
    {
        float Flare = (Finisher ? 8.f : 6.f) * (1.f - 0.5f * Sweep);
        DrawFxDot(RenderContext, Tip, Flare, 0xFFFFFFFF);
    }

    // NOTE(zoubir): sparks thrown on from the end of the sweep, along it
    float SparkT = Clamp01((T - SWING_SWEEP) / (1.f - SWING_SWEEP));
    if (T >= SWING_SWEEP && SparkT < 1.f)
    {
        float EndAngle = Burst->Angle + Side * HalfAngle;
        v2 End = Centre + Radius * V2(Cos(EndAngle), Sin(EndAngle));
        v2 Onward = Side * V2(-Sin(EndAngle), Cos(EndAngle));
        v2 Out = V2(Cos(EndAngle), Sin(EndAngle));
        u32 Count = Finisher ? 8 : 5;
        for(u32 Spark = 0; Spark < Count; Spark++)
        {
            float Speed = 0.35f * Radius * (0.6f + 0.8f * BurstJitter(Spark, 16));
            float Spread = (BurstJitter(Spark, 17) - 0.3f) * 0.9f;
            v2 Direction = NormalizeOr(Onward + Spread * Out, Onward);
            float Ease = 1.f - Square(1.f - SparkT);
            u32 Alpha = (u32)(255.f * (1.f - SparkT));
            DrawFxDot(RenderContext, End + Speed * Ease * Direction,
                      4.f - 2.f * SparkT, (Alpha << 24) | Look->RGB);
        }
    }
}
