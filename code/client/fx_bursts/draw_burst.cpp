/* DrawBurst (fx_bursts.cpp): one burst, drawn by the shape its row in
   BurstLooks names. Each case is a few loops of square dots and filled
   bands (bands.cpp), timed by T, the burst's age as a share of its life. */

internal void
DrawBurst(render_context *RenderContext, fx_burst *Burst, v3 CameraOffset)
{
    burst_look *Look = &BurstLooks[Burst->Kind];
    burst_area Area = BurstArea(Burst->Kind);
    float T = Burst->Age / Area.Seconds;
    float EaseOut = 1.f - (1.f - T) * (1.f - T);
    u32 Alpha = (u32)(255.f * (1.f - T * T));
    u32 Color = (Alpha << 24) | Look->RGB;
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    switch (Look->Shape)
    {
        case BurstShape_Gather:
        {
            v2 Chest = Centre - V2(0.f, 22.f);
            float Radius = Area.Radius * (1.f - EaseOut);
            for(u32 Dot = 0; Dot < 12; Dot++)
            {
                float Angle = 2.f * Pi32 * Dot / 12.f + 3.f * T;
                DrawFxDot(RenderContext,
                          Chest + Radius * V2(Cos(Angle), Sin(Angle)),
                          2.f + 3.f * T, Color);
            }
        } break;

        case BurstShape_Ring:
        {
            float Front = Area.Radius * (0.3f + 0.7f * EaseOut);
            DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32, Front,
                          Maximum(3.f, 0.4f * Area.Radius * (1.f - T)),
                          1.f - T * T, Look->RGB);
        } break;

        case BurstShape_Cone:
        {
            // NOTE(zoubir): a wave racing out across the cone, with a
            // fainter one behind it; Area.HalfAngle is its area row's, or
            // a fixed spread for a look of its own
            float Half = Area.HalfAngle < Pi32 ? Area.HalfAngle : 1.2f;
            v2 Chest = Centre - V2(0.f, 16.f);
            for(u32 Wave = 0; Wave < 2; Wave++)
            {
                float Front = Area.Radius * EaseOut * (1.f - 0.25f * Wave);
                DrawWaveFront(RenderContext, Chest, Burst->Angle - Half,
                              Burst->Angle + Half, Front,
                              0.35f * Area.Radius * (1.f - 0.5f * T),
                              (1.f - T * T) * (1.f - 0.5f * Wave), Look->RGB);
            }
        } break;

        case BurstShape_Column:
        {
            // NOTE(zoubir): light under the dots: a pool on the ground and a
            // tall soft column, both added onto the world (glow.frag)
            float Pool = 2.2f * Area.Radius;
            DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.5f * Pool,
                           Centre.Y - 0.25f * Pool, Pool, 0.5f * Pool,
                           FxColor(0.7f * (1.f - T), Look->RGB), RenderBlend_Additive);
            float Glow = Clamp01(1.f - 2.f * T);
            if (Glow > 0.f)
            {
                float GlowHeight = 170.f * (0.4f + 0.6f * EaseOut);
                DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 22.f,
                               Centre.Y - GlowHeight, 44.f, GlowHeight + 10.f,
                               FxColor(0.8f * Glow, Look->RGB), RenderBlend_Additive);
            }
            DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32,
                          Area.Radius * (0.5f + 0.5f * EaseOut),
                          0.5f * Area.Radius * (1.f - T), 1.f - T * T,
                          Look->RGB);
            // NOTE(zoubir): a beam shooting up out of the ground, gone in
            // the first half
            float Beam = Clamp01(1.f - 2.f * T);
            for(u32 Dot = 0; Dot < 10 && Beam > 0.f; Dot++)
            {
                float Height = 110.f * EaseOut * (float)Dot / 9.f;
                DrawFxDot(RenderContext, Centre - V2(0.f, Height),
                          (8.f - 0.5f * Dot) * Beam, Color);
            }
            // NOTE(zoubir): sparks thrown up from inside the ring
            for(u32 Dot = 0; Dot < 18; Dot++)
            {
                float Angle = 2.f * Pi32 * BurstJitter(Dot, 1);
                float Out = Area.Radius * 0.8f * BurstJitter(Dot, 2);
                float Speed = 140.f + 180.f * BurstJitter(Dot, 3);
                float Height = Speed * Burst->Age - 300.f * Square(Burst->Age);
                v2 P = Centre + GroundCircle(Angle, Out) -
                    V2(0.f, Maximum(0.f, Height));
                DrawFxDot(RenderContext, P, 4.f, Color);
            }
        } break;

        case BurstShape_Spark:
        {
            for(u32 Dot = 0; Dot < 8; Dot++)
            {
                float Angle = 2.f * Pi32 * (Dot + 0.3f * BurstJitter(Dot, 7)) / 8.f;
                float Out = Area.Radius * EaseOut;
                v2 Direction = V2(Cos(Angle), Sin(Angle));
                DrawFxDot(RenderContext, Centre + Out * Direction,
                          5.f - 3.f * T, Color);
                DrawFxDot(RenderContext, Centre + 0.6f * Out * Direction,
                          3.f - 2.f * T, Color);
            }
        } break;

        case BurstShape_Slash:
        {
            // NOTE(zoubir): the cut runs across the direction of the hit,
            // widest early; sparks fly on along the hit
            v2 Along = V2(Cos(Burst->Angle), Sin(Burst->Angle));
            v2 Across = V2(-Along.Y, Along.X);
            float HalfLength = Area.Radius * (0.4f + 0.6f * EaseOut);
            for(u32 Dot = 0; Dot < 11; Dot++)
            {
                float Offset = ((float)Dot / 10.f - 0.5f) * 2.f * HalfLength;
                float Size = (6.f - 4.f * T) * (1.f - Absolute(Offset) / (HalfLength + 1.f));
                DrawFxDot(RenderContext, Centre + Offset * Across, Size + 1.f, Color);
            }
            for(u32 Dot = 0; Dot < 6; Dot++)
            {
                float Spread = (BurstJitter(Dot, 8) - 0.5f) * 1.2f;
                v2 Direction = V2(Cos(Burst->Angle + Spread), Sin(Burst->Angle + Spread));
                float Out = Area.Radius * 1.2f * EaseOut * (0.5f + 0.5f * BurstJitter(Dot, 9));
                DrawFxDot(RenderContext, Centre + Out * Direction, 4.f - 3.f * T, Color);
            }
        } break;

        case BurstShape_Arc:
        case BurstShape_ArcBack:
        {
            DrawSwordArc(RenderContext, Burst, Centre, Area.Radius, T);
        } break;

        case BurstShape_Skid:
        {
            for(u32 Dot = 0; Dot < 6; Dot++)
            {
                float Spread = (BurstJitter(Dot, 10) - 0.5f) * 1.4f;
                float Angle = Burst->Angle + Spread;
                float Out = Area.Radius * EaseOut * (0.5f + 0.5f * BurstJitter(Dot, 11));
                float Rise = 5.f * Sin(Pi32 * T) * BurstJitter(Dot, 12);
                DrawFxDot(RenderContext,
                          Centre + GroundCircle(Angle, Out) - V2(0.f, Rise),
                          6.f - 3.f * T, Color);
            }
        } break;

        case BurstShape_Death:
        {
            // NOTE(zoubir): a bright core for the first moment
            if (T < 0.25f)
            {
                DrawFxDot(RenderContext, Centre, 18.f * (1.f - 4.f * T) + 4.f,
                          Color);
            }
            for(u32 Dot = 0; Dot < 14; Dot++)
            {
                float Angle = 2.f * Pi32 * (Dot + BurstJitter(Dot, 13)) / 14.f;
                float Out = Area.Radius * EaseOut * (0.4f + 0.6f * BurstJitter(Dot, 14));
                float Rise = 26.f * T * (0.5f + BurstJitter(Dot, 15));
                v2 P = Centre + Out * V2(Cos(Angle), 0.6f * Sin(Angle)) -
                    V2(0.f, Rise);
                DrawFxDot(RenderContext, P, 5.f - 3.f * T, Color);
            }
        } break;

        case BurstShape_Mark:
        {
            // NOTE(zoubir): the outline holds still while the ground inside
            // it fills from the middle, filled as the cast goes off
            DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f,
                        Area.Radius * T, FxColor(0.1f, Look->RGB),
                        FxColor(0.35f * T, Look->RGB), RenderBlend_Alpha);
            DrawGroundRing(RenderContext, Centre, Area.Radius,
                           ((u32)(120.f + 120.f * T) << 24) | Look->RGB, 4.f);
        } break;

        case BurstShape_ConeMark:
        {
            // NOTE(zoubir): the cone's edges and its far arc, from its
            // area row
            u32 MarkColor = ((u32)(120.f + 120.f * T) << 24) | Look->RGB;
            float Half = Area.HalfAngle;
            DrawArcBand(RenderContext, Centre, Burst->Angle - Half,
                        Burst->Angle + Half, 0.f, Area.Radius * T,
                        FxColor(0.1f, Look->RGB), FxColor(0.35f * T, Look->RGB),
                        RenderBlend_Alpha);
            for(u32 Dot = 0; Dot < 9; Dot++)
            {
                float Along = (float)(Dot + 1) / 9.f;
                for(u32 Side = 0; Side < 2; Side++)
                {
                    float Angle = Burst->Angle + (Side ? Half : -Half);
                    DrawFxDot(RenderContext,
                              Centre + GroundCircle(Angle, Along * Area.Radius),
                              4.f, MarkColor);
                }
                float ArcAngle = Burst->Angle + Half * (2.f * (float)Dot / 8.f - 1.f);
                DrawFxDot(RenderContext,
                          Centre + GroundCircle(ArcAngle, Area.Radius), 4.f,
                          MarkColor);
            }
        } break;

        case BurstShape_Thrust:
        {
            // NOTE(zoubir): the point reaches the end in the first third
            // of the burst; the dots behind it are fainter and smaller
            v2 Along = V2(Cos(Burst->Angle), Sin(Burst->Angle));
            v2 Chest = Centre - V2(0.f, 16.f);
            float Lead = Minimum(1.f, 3.f * T);
            for(u32 Dot = 0; Dot < 12; Dot++)
            {
                float At = (float)Dot / 11.f;
                if (At > Lead)
                {
                    break;
                }
                float Strength = (1.f - T) * (0.4f + 0.6f * At / Lead);
                u32 DotColor = ((u32)(255.f * Strength) << 24) | Look->RGB;
                DrawFxDot(RenderContext, Chest + At * Area.Radius * Along,
                          2.f + 4.f * Strength, DotColor);
            }
        } break;

        case BurstShape_Spin:
        {
            // NOTE(zoubir): the centre runs on with the dash while two
            // blades turn twice around it, each trailing dots
            v2 Along = V2(Cos(Burst->Angle), Sin(Burst->Angle));
            v2 Middle = Centre - V2(0.f, 14.f) +
                2.f * Area.Radius * EaseOut * Along;
            for(u32 Blade = 0; Blade < 2; Blade++)
            {
                for(u32 Dot = 0; Dot < 7; Dot++)
                {
                    float Angle = 4.f * Pi32 * T + Pi32 * Blade - 0.2f * Dot;
                    float Strength = (1.f - T) * (1.f - Dot / 7.f);
                    u32 DotColor = ((u32)(255.f * Strength) << 24) | Look->RGB;
                    DrawFxDot(RenderContext,
                              Middle + GroundCircle(Angle, 0.8f * Area.Radius),
                              2.f + 3.f * Strength, DotColor);
                }
            }
        } break;

        case BurstShape_FrostNova:
        {
            DrawFrostNovaBurst(RenderContext, Centre, Area.Radius, T, Look->RGB);
        } break;

        case BurstShape_Vortex:
        {
            DrawGravityWellBurst(RenderContext, Centre, Area.Radius, T, Look->RGB);
        } break;

        case BurstShape_Shatter:
        {
            DrawWardBreakBurst(RenderContext, Centre, Area.Radius, T, Look->RGB);
        } break;

        case BurstShape_LevelUp:
        {
            DrawLevelUpBurst(RenderContext, Centre, Area.Radius, T, Look->RGB);
        } break;

        case BurstShape_Learned:
        {
            DrawTalentLearnedBurst(RenderContext, Centre, Area.Radius, T, Look->RGB);
        } break;

        case BurstShape_Puff:
        {
            for(u32 Dot = 0; Dot < 10; Dot++)
            {
                float Angle = 2.f * Pi32 * (Dot + BurstJitter(Dot, 4)) / 10.f;
                float Out = Area.Radius * EaseOut *
                    (0.6f + 0.4f * BurstJitter(Dot, 5));
                float Rise = 6.f * EaseOut * BurstJitter(Dot, 6);
                DrawFxDot(RenderContext,
                          Centre + GroundCircle(Angle, Out) - V2(0.f, Rise),
                          5.f - 3.f * T, Color);
            }
        } break;
    }
}
