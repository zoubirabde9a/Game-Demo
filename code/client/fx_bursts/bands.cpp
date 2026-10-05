/* Filled bands for the bursts (fx_bursts.cpp): DrawArcBand, a band
   between two radii over a range of angles, and DrawWaveFront, the
   growing front of a ring or cone built from two of them. */

#define ARC_BAND_SEGMENTS_PER_TURN 48

inline u32
FxColor(float Alpha, u32 RGB)
{
    u32 Result = ((u32)(255.f * Clamp01(Alpha)) << 24) | RGB;
    return Result;
}

// NOTE(zoubir): a filled band round Centre between radius Inner and Outer,
// from angle From to To, shading from InnerColor on its inside edge to
// OuterColor on its outside one. The area effects are built from these:
// a whole turn is a ring, a slice of one a cone's front, an Inner of 0 a
// filled disc or wedge
internal void
DrawArcBand(render_context *RenderContext, v2 Centre, float From, float To,
            float Inner, float Outer, u32 InnerColor, u32 OuterColor,
            u32 Blend = RenderBlend_Additive)
{
    if (Outer <= 0.f || Outer <= Inner)
    {
        return;
    }
    Inner = Maximum(0.f, Inner);
    u32 Segments = (u32)(ARC_BAND_SEGMENTS_PER_TURN * Absolute(To - From) /
                         (2.f * Pi32)) + 3;
    v2 LastOut = Centre + GroundCircle(From, Outer);
    v2 LastIn = Centre + GroundCircle(From, Inner);
    for(u32 Index = 1; Index <= Segments; Index++)
    {
        float Angle = From + (To - From) * (float)Index / Segments;
        v2 Out = Centre + GroundCircle(Angle, Outer);
        v2 In = Centre + GroundCircle(Angle, Inner);
        DrawFilledQuad(RenderContext, LastIn, LastOut, Out, In,
                       InnerColor, OuterColor, OuterColor, InnerColor, Blend);
        LastOut = Out;
        LastIn = In;
    }
}

// NOTE(zoubir): a growing wave's front: a band whose outside edge is
// Front, glowing out to it, under a thin bright rim
internal void
DrawWaveFront(render_context *RenderContext, v2 Centre, float From, float To,
              float Front, float Thickness, float Alpha, u32 RGB)
{
    DrawArcBand(RenderContext, Centre, From, To, Front - Thickness, Front,
                FxColor(0.f, RGB), FxColor(0.55f * Alpha, RGB));
    DrawArcBand(RenderContext, Centre, From, To, Front - 2.5f, Front + 1.f,
                FxColor(Alpha, 0x00FFFFFF), FxColor(Alpha, 0x00FFFFFF),
                RenderBlend_Alpha);
}
