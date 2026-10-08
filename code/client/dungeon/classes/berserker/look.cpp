/* Berserker look (client/dungeon/classes/berserker.cpp): where the great
   axe is in the Berserker's hands each frame, and what burns round it.

   The axe rests over the shoulder, bobbing with the breath and swaying
   with the walk. Each attack takes it from there and gives it back, read
   from the newest burst of the player's (role_fx.cpp) and its cast, so it
   plays the same online:
   - Cleave: pulled back past the start of the slice, whipped across it
     (fast out of the start, slowing into the end, where the crescent's
     lead is), held a moment in the follow-through, then eased back;
   - Execute: raised overhead through the wind-up, leaning back further
     and shaking as it fills, then brought down in front, left there a
     moment;
   - Leap: held overhead in the flight, then driven into the ground where
     it lands (Slam);
   - Whirlwind: held straight out, spinning round the body.

   Round the body: a red glow at the feet growing with Rage, embers rising
   off it past two thirds of the bar, eyes burning red; under Berserk a
   bigger, beating aura and steam rising off the shoulders. */

struct axe_pose
{
    v2 Grip;
    v2 Dir;
    float Length;
    float Side;
};

// NOTE(zoubir): A eased into B by T: the grip and length straight, the
// direction turning by its angle, Turn's way round (+1 or -1) or, with
// Turn 0, the short way
inline axe_pose
BlendAxePose(axe_pose A, axe_pose B, float T, float Turn = 0.f)
{
    T = Clamp01(T);
    float From = ATan2(A.Dir.Y, A.Dir.X);
    float Delta = ATan2(B.Dir.Y, B.Dir.X) - From;
    while(Delta > Pi32) Delta -= 2.f * Pi32;
    while(Delta < -Pi32) Delta += 2.f * Pi32;
    if (Turn * Delta < 0.f)
    {
        Delta += Turn * 2.f * Pi32;
    }
    float At = From + T * Delta;
    axe_pose Result;
    Result.Grip = A.Grip + T * (B.Grip - A.Grip);
    Result.Dir = V2(Cos(At), Sin(At));
    Result.Length = A.Length + T * (B.Length - A.Length);
    Result.Side = T < 0.5f ? A.Side : B.Side;
    return Result;
}

// NOTE(zoubir): the newest attack burst of the player in Slot still
// playing, and how far into it (T); 0 for none
internal role_burst *
NewestBerserkerAttack(app_state *AppState, u32 Slot, float *T)
{
    role_fx *Fx = GetRoleFx(AppState);
    float Clock = GetFxClock(AppState);
    role_burst *Result = 0;
    for(u32 Index = 0; Index < Fx->Count; Index++)
    {
        role_burst *Burst = &Fx->Bursts[Index];
        u32 Kind = (u32)Burst->Kind - SimBurst_BerserkerFirst;
        bool32 Attack = Kind == BerserkerBurst_Cleave || Kind == BerserkerBurst_CleaveBack ||
            Kind == BerserkerBurst_Execute || Kind == BerserkerBurst_Slam;
        float Age = (Clock - Burst->Start) / RoleBurstLife(Burst->Kind);
        if (Burst->Slot == Slot && Attack && Age >= 0.f && Age < 1.f &&
            (!Result || Burst->Start >= Result->Start))
        {
            Result = Burst;
            *T = Age;
        }
    }
    return Result;
}

// NOTE(zoubir): a swing across a slice round Centre: Along 0..1 across it
// (past either end for the wind-up and the follow-through), the axe
// Length long; its crescent goes on out to the reach the blow hits at
inline axe_pose
SwingAxePose(v2 Centre, float Angle, float Side, float Half, float Along, float Length)
{
    float At = Angle + Side * Half * (2.f * Along - 1.f);
    axe_pose Result;
    Result.Dir = V2(Cos(At), Sin(At));
    Result.Grip = Centre + 6.f * Result.Dir;
    Result.Length = Length;
    Result.Side = Side;
    return Result;
}

// NOTE(zoubir): how far a Cleave's blade is across its slice at T, the
// same for the axe (here) and the crescent (bursts.cpp)
inline float
CleaveAlong(float T)
{
    float Result;
    if (T < BERSERKER_SWING_WIND)
    {
        Result = -0.14f * SmoothStep01(T / BERSERKER_SWING_WIND);
    }
    else
    {
        float S = Clamp01((T - BERSERKER_SWING_WIND) / BERSERKER_SWING_SWEEP);
        Result = -0.14f + 1.2f * (1.f - (1.f - S) * (1.f - S) * (1.f - S));
    }
    return Result;
}

// NOTE(zoubir): the overhead angle for an axe raised facing Facing (+1
// right), and the chop from there toward Angle, down in front
inline float
OverheadAngle(float Facing)
{
    float Result = -0.5f * Pi32 - 0.45f * Facing;
    return Result;
}

inline float
ChopAngle(float From, float Angle, float Facing, float Done)
{
    float To = Angle + 0.55f * Facing;
    float Turn = To - From;
    while(Turn > Pi32) Turn -= 2.f * Pi32;
    while(Turn < -Pi32) Turn += 2.f * Pi32;
    float Result = From + Turn * Done;
    return Result;
}

internal axe_pose
BerserkerAxePose(app_state *AppState, player_slot *Slot, world_entity *Player, float Clock,
                 v3 CameraOffset)
{
    v2 Aim = GetPlayerAim(Player);
    float Facing = Aim.X < -0.1f ? -1.f : 1.f;
    float H = Player->Dimensions.Y;
    float Speed = Length(Player->Velocity.XY);
    float Walk = Minimum(1.f, Speed / 150.f);
    v2 Chest = RoleLookPoint(Player, 0.45f, CameraOffset);
    v2 Head = RoleLookPoint(Player, 0.8f, CameraOffset);
    v2 Feet = RoleLookPoint(Player, 0.f, CameraOffset);
    u32 SlotIndex = (u32)(Slot - AppState->Players);

    // NOTE(zoubir): at rest over the shoulder, the blade up behind the head
    axe_pose Rest;
    float Bob = 1.5f * Sin(2.2f * Clock + (float)SlotIndex) + 2.f * Walk * Sin(18.f * Clock);
    float Sway = 0.12f * Walk * Sin(9.f * Clock);
    float RestAngle = -0.5f * Pi32 - 0.55f * Facing + Sway;
    Rest.Dir = V2(Cos(RestAngle), Sin(RestAngle));
    Rest.Grip = Chest + V2(0.28f * Facing * Player->Dimensions.X, 3.f + Bob);
    Rest.Length = 0.62f * H;
    Rest.Side = -Facing;

    if (Player->CastSpell == PlayerSpell_BerserkerA)
    {
        // NOTE(zoubir): Whirlwind: straight out, spinning
        float Spin = -Facing * 21.f * Clock;
        axe_pose Pose;
        Pose.Dir = V2(Cos(Spin), Sin(Spin));
        Pose.Grip = Chest + 5.f * Pose.Dir;
        Pose.Length = BERSERKER_SWING_LENGTH * H;
        Pose.Side = Facing;
        float In = SmoothStep01((PlayerSpells[PlayerSpell_BerserkerA].CastTime - Player->CastLeft) / 0.12f);
        return BlendAxePose(Rest, Pose, In);
    }
    if (Player->CastSpell == PlayerSpell_BerserkerB)
    {
        // NOTE(zoubir): Execute's wind-up: up overhead, leaning back as it
        // fills, shaking near the end
        float Done = PlayerCastProgress(Player);
        float Up = SmoothStep01(Done / 0.35f);
        float At = OverheadAngle(Facing) - 0.4f * Facing * Done;
        axe_pose Pose;
        Pose.Dir = V2(Cos(At), Sin(At));
        float Shake = 1.6f * Done * Done;
        Pose.Grip = Head + V2(-0.1f * Facing * H + Shake * Sin(71.f * Clock), 4.f + Shake * Sin(53.f * Clock));
        Pose.Length = 0.66f * H;
        Pose.Side = Facing;
        return BlendAxePose(Rest, Pose, Up);
    }
    if (Slot->ClassFlags & BERSERKER_FLAG_LEAPING)
    {
        float At = OverheadAngle(Facing);
        axe_pose Pose;
        Pose.Dir = V2(Cos(At), Sin(At));
        Pose.Grip = Head + V2(-0.05f * Facing * H, 6.f);
        Pose.Length = 0.66f * H;
        Pose.Side = Facing;
        return Pose;
    }

    float T = 0.f;
    role_burst *Burst = NewestBerserkerAttack(AppState, SlotIndex, &T);
    if (!Burst)
    {
        return Rest;
    }
    u32 Kind = (u32)Burst->Kind - SimBurst_BerserkerFirst;
    axe_pose Pose = Rest;
    float Back = 0.f;
    float Turn = 0.f;
    if (Kind == BerserkerBurst_Cleave || Kind == BerserkerBurst_CleaveBack)
    {
        float Side = Kind == BerserkerBurst_Cleave ? 1.f : -1.f;
        v2 Centre = Feet - V2(0.f, BERSERKER_SWING_CHEST);
        Pose = SwingAxePose(Centre, Burst->Angle, Side, CLEAVE_HALF_ANGLE, CleaveAlong(T), BERSERKER_SWING_LENGTH * H);
        Back = SmoothStep01((T - 0.68f) / 0.32f);
        Turn = -Side;
    }
    else
    {
        // NOTE(zoubir): Execute and Slam: down from overhead in a blink,
        // left in the ground a moment
        float From = OverheadAngle(Facing);
        float Angle = Kind == BerserkerBurst_Slam ? (Facing > 0.f ? 0.f : Pi32) : Burst->Angle;
        float Down = 1.f - Square(1.f - Clamp01(T / 0.12f));
        float At = ChopAngle(From, Angle, Facing, Down);
        Pose.Dir = V2(Cos(At), Sin(At));
        Pose.Grip = Head + Down * (Chest + 8.f * V2(Cos(Angle), Sin(Angle)) - Head);
        Pose.Length = 0.7f * H;
        float Chop = ChopAngle(From, Angle, Facing, 1.f) - From;
        Pose.Side = Chop >= 0.f ? 1.f : -1.f;
        Back = SmoothStep01((T - 0.55f) / 0.45f);
        Turn = -Pose.Side;
    }
    axe_pose Result = BlendAxePose(Pose, Rest, Back, Turn);
    return Result;
}

internal void
DrawBerserkerAura(render_context *RenderContext, player_slot *Slot, world_entity *Player,
                  float Clock, v3 CameraOffset)
{
    float Rage = (float)Slot->ClassMeter / (float)BERSERKER_RAGE_MAX;
    bool32 Berserk = (Slot->ClassFlags & BERSERKER_FLAG_BERSERK) != 0;
    float Heat = Berserk ? 1.f : Rage;
    u32 Seed = Player->PlayerIndex;
    v2 Feet = RoleLookPoint(Player, 0.f, CameraOffset);
    float Beat = 0.5f + 0.5f * Sin((Berserk ? 9.f : 4.f) * Clock);
    float Pool = Player->Dimensions.X * (1.2f + 0.6f * Heat + (Berserk ? 0.5f + 0.2f * Beat : 0.f));
    DrawShaderQuad(RenderContext, Shader_Glow, Feet.X - 0.5f * Pool, Feet.Y - 0.22f * Pool, Pool,
                   0.44f * Pool, FxColor(0.12f + 0.4f * Heat + 0.1f * Beat * Heat, BERSERKER_CRIMSON_RGB),
                   RenderBlend_Additive);
    if (Berserk)
    {
        // NOTE(zoubir): a red haze round the whole body, beating
        v2 Body = RoleLookPoint(Player, 0.5f, CameraOffset);
        float Size = 1.9f * Player->Dimensions.Y * (0.9f + 0.1f * Beat);
        DrawShaderQuad(RenderContext, Shader_Glow, Body.X - 0.4f * Size, Body.Y - 0.5f * Size,
                       0.8f * Size, Size, FxColor(0.25f + 0.12f * Beat, BERSERKER_CRIMSON_RGB),
                       RenderBlend_Additive);
    }
    // NOTE(zoubir): heat rising off the body as Rage climbs
    u32 Embers = Berserk ? 10 : (Rage > 0.66f ? 5 : 0);
    v2 Shoulders = RoleLookPoint(Player, 0.6f, CameraOffset);
    for(u32 Ember = 0; Ember < Embers; Ember++)
    {
        float Rise = DungeonFxFraction(0.9f * Clock + 0.1f * (float)Ember + 0.37f * (float)Seed);
        float X = (BurstJitter(Ember + 16 * Seed, 501) - 0.5f) * 1.1f * Player->Dimensions.X;
        v2 P = Shoulders + V2(X + 3.f * Sin(5.f * Clock + (float)Ember), 10.f - 34.f * Rise);
        DrawFxDot(RenderContext, P, 2.5f - Rise, FxColor(0.95f * (1.f - Rise),
                                                          Ember & 1 ? BERSERKER_HOT_RGB : BERSERKER_CRIMSON_RGB));
    }
    // NOTE(zoubir): steam off the shoulders under Berserk, grey puffs
    // swelling as they rise
    for(u32 Puff = 0; Puff < (Berserk ? 6u : 0u); Puff++)
    {
        float Rise = DungeonFxFraction(0.6f * Clock + (float)Puff / 6.f);
        float Side = (Puff & 1) ? 1.f : -1.f;
        v2 P = Shoulders + V2(Side * (0.3f * Player->Dimensions.X + 8.f * Rise), -4.f - 30.f * Rise);
        float Size = 6.f + 14.f * Rise;
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - Size, P.Y - Size, 2.f * Size, 2.f * Size,
                       FxColor(0.35f * (1.f - Rise) * Clamp01(4.f * Rise), 0x00D8D8E0), RenderBlend_Alpha);
    }
}

// NOTE(zoubir): two red eyes under the brow once Rage runs high
internal void
DrawBerserkerEyes(render_context *RenderContext, player_slot *Slot, world_entity *Player,
                  float Clock, v3 CameraOffset)
{
    bool32 Berserk = (Slot->ClassFlags & BERSERKER_FLAG_BERSERK) != 0;
    float Rage = (float)Slot->ClassMeter / (float)BERSERKER_RAGE_MAX;
    float Glow = Berserk ? 1.f : Clamp01((Rage - 0.35f) / 0.4f);
    v2 Aim = GetPlayerAim(Player);
    if (Glow <= 0.f || Aim.Y < -0.6f)
    {
        return;
    }
    v2 Head = RoleLookPoint(Player, BERSERKER_EYE_HEIGHT, CameraOffset);
    float Look = Clamp01(Absolute(Aim.X)) * (Aim.X < 0.f ? -1.f : 1.f);
    v2 Mid = Head + V2(0.16f * Player->Dimensions.X * Look, 0.f);
    float Gap = 3.2f * (1.f - 0.45f * Absolute(Look));
    float Flicker = 0.85f + 0.15f * Sin(23.f * Clock + (float)Player->PlayerIndex);
    for(u32 Eye = 0; Eye < 2; Eye++)
    {
        v2 P = Mid + V2(Eye ? Gap : -Gap, 0.f);
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - 6.f, P.Y - 4.f, 12.f, 8.f,
                       FxColor(0.8f * Glow * Flicker, BERSERKER_CRIMSON_RGB), RenderBlend_Additive);
        DrawFxDot(RenderContext, P, 2.f, FxColor(Glow * Flicker, 0x006080FF));
    }
}
