/* Frost Mage shapes (classes/frostmage.cpp): what the Frost Mage's look
   and effects are drawn from, in screen space: the ice palette, a shard
   of ice (a long diamond, lit on one face), a snowflake, a frost ring, the
   staff with its crystal head, and how long ago this mage's latest burst
   of a kind started, read from the bursts in flight, so a remote mage
   looks the same as the local one. */

#define FROST_FX_ICE_RGB 0x00FFD796
#define FROST_FX_PALE_RGB 0x00FFF4E0
#define FROST_FX_DEEP_RGB 0x00D2782D
#define FROST_FX_GLOW_RGB 0x00FFE678
#define FROST_FX_STAFF_RGB 0x00463A30
#define FROST_FX_STAFF_LIGHT_RGB 0x00826E5C

// NOTE(zoubir): the youngest burst Index of the mage in SlotIndex, how
// long ago it started; a big number for none
internal float
FrostBurstAge(app_state *AppState, u32 SlotIndex, u32 Index)
{
    role_fx *Fx = GetRoleFx(AppState);
    float Clock = GetFxClock(AppState);
    float Result = 1000.f;
    for(u32 Row = 0; Row < Fx->Count; Row++)
    {
        role_burst *Burst = &Fx->Bursts[Row];
        if (Burst->Kind == ClassBurst(SimBurst_FrostMageFirst, Index) && Burst->Slot == SlotIndex)
        {
            Result = Minimum(Result, Clock - Burst->Start);
        }
    }
    return Result;
}

// NOTE(zoubir): a shard of ice with its point at Tip along Dir, Length
// long and Width across at its widest; Alpha seen. One face lit, one in
// shade, a bright edge down the middle
internal void
DrawIceShard(render_context *RenderContext, v2 Tip, v2 Dir, float Length, float Width, float Alpha)
{
    v2 Side = V2(-Dir.Y, Dir.X);
    v2 Back = Tip - Length * Dir;
    v2 Mid = Tip - 0.62f * Length * Dir;
    v2 Left = Mid + 0.5f * Width * Side;
    v2 Right = Mid - 0.5f * Width * Side;
    u32 Lit = FxColor(0.9f * Alpha, FROST_FX_PALE_RGB);
    u32 Ice = FxColor(0.85f * Alpha, FROST_FX_ICE_RGB);
    u32 Shade = FxColor(0.85f * Alpha, FROST_FX_DEEP_RGB);
    DrawFilledQuad(RenderContext, Tip, Left, Back, Back, Lit, Ice, Ice, Ice, RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Tip, Right, Back, Back, Ice, Shade, Shade, Shade, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, Tip, Back, 1.2f, 0.4f, FxColor(Alpha, FROST_FX_PALE_RGB),
                 FxColor(0.3f * Alpha, FROST_FX_ICE_RGB));
}

// NOTE(zoubir): a six-armed snowflake at C, Radius out, turned Turn
internal void
DrawSnowflake(render_context *RenderContext, v2 C, float Radius, float Turn, float Alpha, u32 RGB)
{
    for(u32 Arm = 0; Arm < 6; Arm++)
    {
        float A = Turn + Pi32 * (float)Arm / 3.f;
        v2 Dir = V2(Cos(A), Sin(A));
        v2 End = C + Radius * Dir;
        DrawFxStroke(RenderContext, C, End, 0.16f * Radius, 0.08f * Radius, FxColor(Alpha, RGB),
                     FxColor(0.6f * Alpha, RGB));
        v2 Fork = C + 0.6f * Radius * Dir;
        for(float Sign = -1.f; Sign <= 1.f; Sign += 2.f)
        {
            float B = A + Sign * 0.7f;
            DrawFxStroke(RenderContext, Fork, Fork + 0.32f * Radius * V2(Cos(B), Sin(B)), 0.1f * Radius,
                         0.05f * Radius, FxColor(0.8f * Alpha, RGB), FxColor(0.4f * Alpha, RGB));
        }
    }
}

// NOTE(zoubir): a ring of frost round Centre, its outside edge at Radius,
// Width thick, brightest at the rim
internal void
FrostRing(render_context *RenderContext, v2 Centre, float Radius, float Width, float Alpha)
{
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Maximum(0.f, Radius - Width), Radius,
                FxColor(0.f, FROST_FX_ICE_RGB), FxColor(0.8f * Alpha, FROST_FX_ICE_RGB));
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Maximum(0.f, Radius - 1.5f), Radius + 0.5f,
                FxColor(0.7f * Alpha, FROST_FX_PALE_RGB), FxColor(0.7f * Alpha, FROST_FX_PALE_RGB));
}

// NOTE(zoubir): a soft glow at P, Size across each way
inline void
FrostGlow(render_context *RenderContext, v2 P, float Size, float Alpha, u32 RGB)
{
    DrawShaderQuad(RenderContext, Shader_Glow, P.X - Size, P.Y - Size, 2.f * Size, 2.f * Size,
                   FxColor(Alpha, RGB), RenderBlend_Additive);
}

// NOTE(zoubir): the aim as a screen direction
inline v2
FrostScreenAim(world_entity *Player)
{
    v2 Result = NormalizeOr(GetPlayerAim(Player), V2(1.f, 0.f));
    return Result;
}

// NOTE(zoubir): where the staff's crystal is on screen: held out on the
// aim side, above the hand
inline v2
FrostStaffHead(world_entity *Player, v3 CameraOffset)
{
    v2 Aim = FrostScreenAim(Player);
    float Side = Aim.X < -0.1f ? -1.f : 1.f;
    v2 Result = RoleLookPoint(Player, 0.95f, CameraOffset) +
        V2(Side * 0.42f * Player->Dimensions.X, 0.f) + (0.12f * Player->Dimensions.X) * Aim;
    return Result;
}

// NOTE(zoubir): the staff from the floor up to its crystal Head, leaning
// with the walk; Charge 0..1 lights the crystal for a cast
internal void
DrawFrostStaff(render_context *RenderContext, v2 Head, float Length, float Lean, float Charge,
               float Clock)
{
    v2 Up = NormalizeOr(V2(Lean, -1.f), V2(0.f, -1.f));
    v2 Foot = Head - Length * Up;
    DrawFxStroke(RenderContext, Foot, Head, 3.4f, 2.6f, FxColor(1.f, FROST_FX_STAFF_RGB),
                 FxColor(1.f, FROST_FX_STAFF_LIGHT_RGB), RenderBlend_Alpha);
    // NOTE(zoubir): the crystal's cradle, two prongs round its base
    v2 Side = V2(-Up.Y, Up.X);
    for(float Sign = -1.f; Sign <= 1.f; Sign += 2.f)
    {
        DrawFxStroke(RenderContext, Head - 4.f * Up, Head + 3.f * Up + 4.f * Sign * Side, 2.f, 1.f,
                     FxColor(1.f, FROST_FX_STAFF_LIGHT_RGB), FxColor(1.f, FROST_FX_STAFF_LIGHT_RGB),
                     RenderBlend_Alpha);
    }
    float Pulse = 0.5f + 0.5f * Sin(3.f * Clock);
    FrostGlow(RenderContext, Head + 6.f * Up, 12.f + 14.f * Charge, 0.35f + 0.1f * Pulse + 0.5f * Charge,
              FROST_FX_GLOW_RGB);
    DrawIceShard(RenderContext, Head + 14.f * Up, Up, 16.f, 8.f, 1.f);
}
