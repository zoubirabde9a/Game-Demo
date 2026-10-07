/* Cast glow: the light of a cast drawn over the world (cast_fx.cpp has the
   state and the circle on the ground). While a player winds up a spell,
   a soft glow wraps the body, an orb grows in the hand it casts with,
   motes spiral into it from round the body and sparks rise off the rim
   of the circle. When the cast goes off the orb bursts: a flash, a ring
   thrown out from the feet and sparks flying from the hand. When it is
   cut short the orb sputters out in grey smoke.

   Every mote and spark is placed from the cast's age and its own index,
   so nothing is stored per particle and every machine draws the same. */

#define CAST_GLOW_MOTES 10
#define CAST_GLOW_SPARKS 8
#define CAST_GLOW_RAYS 10
// NOTE(zoubir): how far in front of the chest the hand is
#define CAST_GLOW_REACH 13.f

inline void
DrawCastLight(render_context *RenderContext, v2 Centre, float Size, u32 Color)
{
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.5f * Size,
                   Centre.Y - 0.5f * Size, Size, Size, Color, RenderBlend_Additive);
}

// NOTE(zoubir): the chest of a body with its feet at Feet, on screen
inline v2
CastChest(world_entity *Entity, v2 Feet)
{
    v2 Result = V2(Feet.X, Feet.Y - 0.32f * Entity->Dimensions.Y);
    return Result;
}

// NOTE(zoubir): where the hand is; looking up, the hand is behind the
// head, so it sits a little higher and narrower
inline v2
CastHand(world_entity *Entity, v2 Feet, v2 Direction)
{
    v2 Result = CastChest(Entity, Feet) +
        CAST_GLOW_REACH * V2(Direction.X, 0.6f * Direction.Y);
    return Result;
}

internal void
DrawWindingGlow(render_context *RenderContext, cast_fx_slot *Slot, cast_look *Look,
                world_entity *Entity, v2 Feet, float Clock)
{
    float P = Slot->Progress;
    float Open = Clamp01(Slot->Age / CAST_FX_OPEN_SECONDS);
    float Pulse = 0.5f + 0.5f * Sin(Clock * (8.f + 16.f * P));
    v2 Chest = CastChest(Entity, Feet);
    v2 Hand = CastHand(Entity, Feet, CastHandDirection(Entity));

    // NOTE(zoubir): the body glows from within, more as the cast fills
    DrawCastLight(RenderContext, Chest, 54.f + 22.f * P,
                  CastRGBA(Look->Color, 1.f, Open * (0.18f + 0.22f * P + 0.08f * Pulse)));

    // NOTE(zoubir): motes drawn in from round the body to the hand, faster
    // as the cast nears its end
    float Speed = 1.4f + 1.6f * P;
    for(u32 Index = 0; Index < CAST_GLOW_MOTES; Index++)
    {
        float Phase = Slot->Age * Speed + (float)Index / (float)CAST_GLOW_MOTES;
        float T = Phase - floorf(Phase);
        float Angle = 2.39996f * (float)Index + 2.5f * Slot->Age +
            (float)(u32)Phase * 1.3f;
        float Out = 4.f + 40.f * (1.f - T) * (1.f - T);
        v2 At = Hand + Out * V2(Cos(Angle), 0.65f * Sin(Angle));
        float Bright = Open * T * (0.5f + 0.5f * P);
        DrawCastLight(RenderContext, At, 5.f + 4.f * T, CastRGBA(Look->Color, 1.3f, Bright));
        DrawCastLight(RenderContext, At, 2.5f, CastRGBA(V3(1.f, 1.f, 1.f), 1.f, 0.7f * Bright));
    }

    // NOTE(zoubir): sparks rising off the rim of the circle on the ground
    for(u32 Index = 0; Index < CAST_GLOW_SPARKS; Index++)
    {
        float Phase = Slot->Age * 0.9f + (float)Index / (float)CAST_GLOW_SPARKS;
        float T = Phase - floorf(Phase);
        float Angle = 2.f * Pi32 * (float)Index / (float)CAST_GLOW_SPARKS +
            0.7f * (float)(u32)Phase + 0.4f * Slot->Age;
        float Rim = 0.85f * Look->Radius;
        v2 At = Feet + V2(Rim * Cos(Angle), CAST_SIGIL_FLATTEN * Rim * Sin(Angle)) -
            V2(0.f, 34.f * T * (0.6f + 0.6f * P));
        float Bright = Open * (1.f - T) * (0.35f + 0.65f * P);
        DrawCastLight(RenderContext, At, 6.f, CastRGBA(Look->Color, 1.2f, Bright));
    }

    // NOTE(zoubir): the orb in the hand: a coloured light and a white
    // core, swelling with the cast and throbbing hard at its end
    float Hot = Clamp01((P - 0.8f) / 0.2f);
    float Orb = (6.f + 18.f * P) * (1.f + 0.15f * Hot * Pulse);
    DrawCastLight(RenderContext, Hand, 2.6f * Orb,
                  CastRGBA(Look->Color, 1.f, Open * (0.45f + 0.35f * P)));
    DrawCastLight(RenderContext, Hand, Orb,
                  CastRGBA(Look->Color, 1.6f, Open * (0.7f + 0.3f * Pulse)));
    DrawCastLight(RenderContext, Hand, 0.45f * Orb,
                  CastRGBA(V3(1.f, 1.f, 1.f), 1.f, Open * (0.6f + 0.4f * P)));
}

internal void
DrawReleaseGlow(render_context *RenderContext, cast_fx_slot *Slot, cast_look *Look,
                world_entity *Entity, v2 Feet)
{
    float T = Clamp01(Slot->EndedAge / CAST_FX_RELEASE_SECONDS);
    float Left = 1.f - T;
    v2 Hand = CastHand(Entity, Feet, Slot->EndedHand);

    // NOTE(zoubir): a white flash in the hand that cools to the spell's
    // colour as it spreads
    float Flash = Clamp01(1.f - T / 0.35f);
    DrawCastLight(RenderContext, Hand, 40.f + 70.f * T,
                  CastRGBA(Look->Color, 1.4f, 0.9f * Left * Left));
    DrawCastLight(RenderContext, Hand, 26.f + 20.f * T,
                  CastRGBA(V3(1.f, 1.f, 1.f), 1.f, Flash));

    // NOTE(zoubir): a ring thrown out along the ground from the feet
    float Ring = 2.f * Look->Radius * (1.f + 2.2f * sqrtf(T));
    DrawShaderQuad(RenderContext, Shader_Ring, Feet.X - 0.5f * Ring,
                   Feet.Y - 0.5f * CAST_SIGIL_FLATTEN * Ring, Ring,
                   CAST_SIGIL_FLATTEN * Ring,
                   CastRGBA(Look->Color, 1.3f, Left * Left), RenderBlend_Additive);

    // NOTE(zoubir): sparks flying out of the hand, three dots each, the
    // head brightest
    for(u32 Index = 0; Index < CAST_GLOW_RAYS; Index++)
    {
        float Angle = 2.f * Pi32 * ((float)Index + 0.37f * (float)(Index & 3)) /
            (float)CAST_GLOW_RAYS;
        v2 Way = V2(Cos(Angle), 0.7f * Sin(Angle));
        float Reach = (18.f + 14.f * (float)(Index % 3)) * sqrtf(T) + 4.f;
        for(u32 Dot = 0; Dot < 3; Dot++)
        {
            v2 At = Hand + (Reach - 5.f * (float)Dot) * Way;
            float Bright = Left * (1.f - 0.3f * (float)Dot);
            DrawCastLight(RenderContext, At, 6.f - (float)Dot,
                          CastRGBA(Look->Color, 1.5f, Bright));
        }
    }
}

// NOTE(zoubir): the orb dies: grey puffs drifting up and a dim flicker
internal void
DrawFizzleGlow(render_context *RenderContext, cast_fx_slot *Slot,
               world_entity *Entity, v2 Feet)
{
    float T = Clamp01(Slot->EndedAge / CAST_FX_FIZZLE_SECONDS);
    float Left = 1.f - T;
    v2 Hand = CastHand(Entity, Feet, Slot->EndedHand);
    for(u32 Index = 0; Index < 5; Index++)
    {
        float Angle = 1.3f * (float)Index - 0.5f * Pi32;
        v2 At = Hand + V2(7.f * Cos(Angle) * T, -14.f * T * (0.6f + 0.1f * (float)Index));
        DrawShaderQuad(RenderContext, Shader_Glow, At.X - 7.f - 6.f * T, At.Y - 7.f - 6.f * T,
                       14.f + 12.f * T, 14.f + 12.f * T,
                       UI_RGBA(70, 70, 80, (u32)(150.f * Left)));
    }
    DrawCastLight(RenderContext, Hand, 12.f * Left,
                  CastRGBA(V3(0.7f, 0.7f, 0.8f), 1.f, 0.6f * Left * (0.5f + 0.5f * Sin(80.f * T))));
}

internal void
DrawCastGlows(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    cast_fx *Fx = AppState->CastFx;
    if (!Fx)
    {
        return;
    }
    world *World = &AppState->World;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        cast_fx_slot *Slot = Entity->IsPresent ? CastFxSlotOf(AppState, Entity) : 0;
        if (!Slot)
        {
            continue;
        }
        v2 Feet = Entity->Position.XY - CameraOffset.XY - V2(0.f, Entity->Position.Z);
        cast_look *Look = CastLookOf(Slot->Spell);
        if (Look)
        {
            DrawWindingGlow(RenderContext, Slot, Look, Entity, Feet, Fx->Clock);
        }
        else if ((Look = CastLookOf(Slot->EndedSpell)) != 0)
        {
            if (!Slot->Fizzled && Slot->EndedAge < CAST_FX_RELEASE_SECONDS)
            {
                DrawReleaseGlow(RenderContext, Slot, Look, Entity, Feet);
            }
            else if (Slot->Fizzled && Slot->EndedAge < CAST_FX_FIZZLE_SECONDS)
            {
                DrawFizzleGlow(RenderContext, Slot, Entity, Feet);
            }
        }
    }
}
