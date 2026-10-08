/* Duelist effects (sim/dungeon/role_kits/duelist.cpp), included by
   classes/class_fx.cpp: how a Duelist looks in a run, its bursts
   (SimBurst_DuelistFirst on, their rows in duelist_bursts.inc) and what its
   spells leave in the world.

   The look: a fencer en garde, the rapier held out at chest height point
   forward toward the aim, the off hand raised behind. The blade follows
   the class's bursts and cast: a Thrust snaps it out along the stab and
   back, a Lunge drives it out low and long, Heartseeker draws it back
   through the wind-up, rose light gathering at the point, then drives it
   through; a counter is a quick thrust at the foe. On guard (Riposte) the
   blade is held across in front with a shimmering arc before it, gold
   once it has parried. Tempo lights the blade rose, more with every
   stack, and at full Tempo a glint runs up it; small diamonds over the
   head show the stacks to the party. Perfect Form leaves rose afterimages
   behind the Duelist and petals drifting off it. Everything is read from
   the slot's ClassMeter and ClassFlags, its cast and its bursts, all of
   which reach every client. The bursts are duelist/bursts.cpp; the shapes
   duelist/shapes.cpp. */

// NOTE(zoubir): the rapier's length, a share of the body's height
#define DUELIST_RAPIER 0.78f

#include "duelist/shapes.cpp"
#include "duelist/bursts.cpp"

struct duelist_pose
{
    v2 Grip;
    v2 Dir;
};

inline duelist_pose
BlendDuelistPose(duelist_pose A, duelist_pose B, float T)
{
    duelist_pose Result;
    Result.Grip = A.Grip + Clamp01(T) * (B.Grip - A.Grip);
    Result.Dir = DuelistNormal(A.Dir + Clamp01(T) * (B.Dir - A.Dir));
    return Result;
}

// NOTE(zoubir): where the rapier is Lag seconds ago (the afterimages run
// late), from the player's aim, walk, cast, flags and bursts
internal duelist_pose
DuelistPoseAt(app_state *AppState, player_slot *Slot, world_entity *Player, float Clock, v2 Body,
              float Lag)
{
    u32 SlotIndex = Player->PlayerIndex;
    v2 Aim = DuelistNormal(GetPlayerAim(Player));
    float W = Player->Dimensions.X;
    float Speed = Length(Player->Velocity.XY);
    float Walk = Minimum(1.f, Speed / 150.f);
    float Breathe = 1.f * Sin(2.2f * Clock + (float)SlotIndex);
    // NOTE(zoubir): en garde: the sword hand out in front at chest
    // height, the point a little up, bobbing with the step
    duelist_pose Result;
    Result.Grip = Body + (0.22f * W) * Aim + V2(0.f, -2.f + Breathe + Walk * 2.f * Sin(10.f * Clock));
    Result.Dir = DuelistNormal(Aim + V2(0.f, -0.22f));

    // NOTE(zoubir): on guard: the blade across in front, point up and
    // out to the side the off hand is not
    if (Slot->ClassFlags & DUELIST_FLAG_GUARD)
    {
        float Facing = Aim.X < -0.1f ? -1.f : 1.f;
        duelist_pose Guard;
        Guard.Grip = Body + (0.18f * W) * Aim + V2(-Facing * 0.12f * W, 4.f);
        Guard.Dir = DuelistNormal(V2(Facing * 0.45f, -1.f) + 0.25f * Aim);
        float In = DuelistEase(DuelistBurstAge(AppState, SlotIndex, DuelistBurst_Guard) / 0.08f);
        Result = BlendDuelistPose(Result, Guard, In);
    }

    // NOTE(zoubir): Heartseeker's wind-up: the point drawn back along the
    // aim, the hand pulled in, a tremble as it fills
    if (Player->CastSpell == PlayerSpell_DuelistB)
    {
        float Done = PlayerCastProgress(Player);
        float Shake = 1.2f * Done * Sin(45.f * Clock);
        duelist_pose Draw;
        Draw.Grip = Body - (0.25f * W) * Aim + V2(Shake, -4.f);
        Draw.Dir = Aim;
        Result = BlendDuelistPose(Result, Draw, DuelistEase(Done * 3.f));
    }

    // NOTE(zoubir): the newest stab of any kind drives the blade out
    // along its angle and back
    u32 Stabs[4] = {DuelistBurst_Thrust, DuelistBurst_Lunge, DuelistBurst_Heartseeker, DuelistBurst_Counter};
    float Lengths[4] = {0.2f, 0.32f, 0.36f, 0.25f};
    float Reaches[4] = {0.55f, 0.8f, 0.9f, 0.6f};
    role_burst *Newest = 0;
    u32 Which = 0;
    for(u32 Index = 0; Index < ArrayCount(Stabs); Index++)
    {
        role_burst *Burst = DuelistNewestBurst(AppState, SlotIndex, Stabs[Index]);
        if (Burst && (!Newest || Burst->Start > Newest->Start))
        {
            Newest = Burst;
            Which = Index;
        }
    }
    if (Newest)
    {
        float Age = Clock - Newest->Start - Lag;
        float Hold = DuelistHold(Age, Lengths[Which]);
        if (Hold > 0.f)
        {
            v2 Along = V2(Cos(Newest->Angle), Sin(Newest->Angle));
            // NOTE(zoubir): a burst on the foe points from the Duelist to it
            if (Stabs[Which] == DuelistBurst_Heartseeker || Stabs[Which] == DuelistBurst_Counter)
            {
                v3 At = DuelistBurstPlace(Newest->Position);
                v2 To = At.XY - Player->Position.XY;
                if (LengthSq(To) > 1.f)
                {
                    Along = DirectionTo(To);
                }
            }
            duelist_pose Out;
            Out.Grip = Body + (Reaches[Which] * W) * Along + V2(0.f, Which == 1 ? 6.f : 0.f);
            Out.Dir = Along;
            Result = BlendDuelistPose(Result, Out, Hold);
        }
    }
    return Result;
}

// NOTE(zoubir): a living Duelist's look, over its sprite, every frame
internal void
DrawDuelistLook(render_context *RenderContext, app_state *AppState, player_slot *Slot,
                world_entity *Player, float Clock, v3 CameraOffset)
{
    u32 SlotIndex = Player->PlayerIndex;
    v2 Aim = DuelistNormal(GetPlayerAim(Player));
    float Facing = Aim.X < -0.1f ? -1.f : 1.f;
    float W = Player->Dimensions.X;
    float H = Player->Dimensions.Y;
    v2 Feet = RoleLookPoint(Player, 0.f, CameraOffset);
    v2 Body = RoleLookPoint(Player, 0.45f, CameraOffset);
    u32 Tempo = Minimum((u32)Slot->ClassMeter, (u32)DUELIST_MOST_TEMPO);
    bool32 Full = (Slot->ClassFlags & DUELIST_FLAG_TEMPO) != 0;
    bool32 Form = (Slot->ClassFlags & DUELIST_FLAG_FORM) != 0;
    bool32 Guard = (Slot->ClassFlags & DUELIST_FLAG_GUARD) != 0;
    bool32 Parried = (Slot->ClassFlags & DUELIST_FLAG_PARRIED) != 0;
    float Glow = (float)Tempo / (float)DUELIST_MOST_TEMPO;
    if (Full)
    {
        Glow = 0.85f + 0.15f * Sin(7.f * Clock);
    }

    // NOTE(zoubir): Perfect Form: afterimages a beat behind, petals off
    // the shoulders
    if (Form)
    {
        v2 Back = V2(-Facing, 0.f);
        for(u32 Image = 1; Image <= 2; Image++)
        {
            float Lag = 0.07f * (float)Image;
            float Sway = 3.f * Sin(4.f * Clock + (float)Image);
            v2 Offset = (13.f * (float)Image) * Back + V2(0.f, Sway);
            float Fade = 0.7f - 0.2f * (float)Image;
            DrawDuelistGhost(RenderContext, Feet + Offset, 0.9f * W, 0.95f * H, Facing, Fade);
            duelist_pose Echo = DuelistPoseAt(AppState, Slot, Player, Clock, Body + Offset, Lag);
            DrawDuelistRapier(RenderContext, Echo.Grip, Echo.Dir, DUELIST_RAPIER * H, 1.f, Fade);
        }
        for(u32 Petal = 0; Petal < 6; Petal++)
        {
            float Phase = DungeonFxFraction(0.45f * Clock + 0.167f * (float)Petal + 0.3f * (float)SlotIndex);
            float X = (BurstJitter(Petal + 8 * SlotIndex, 751) - 0.5f) * 1.4f * W;
            v2 P = Body + V2(X + 8.f * Sin(3.f * Clock + (float)Petal) - Facing * 30.f * Phase,
                             -0.3f * H + 0.9f * H * Phase);
            DrawDuelistPetal(RenderContext, P, 6.f, 3.f * Clock + (float)Petal, (1.f - Phase) * Minimum(1.f, 5.f * Phase));
        }
    }

    // NOTE(zoubir): the guard's shimmering arc in front, gold once it
    // has parried
    if (Guard)
    {
        float A = ATan2(Aim.Y, Aim.X);
        float Flicker = 0.75f + 0.25f * Sin(30.f * Clock);
        u32 RGB = Parried ? DUELIST_GOLD_RGB : DUELIST_PALE_RGB;
        DrawArcBand(RenderContext, Body, A - 1.f, A + 1.f, 0.55f * H, 0.7f * H, FxColor(0.f, RGB),
                    FxColor(0.55f * Flicker, RGB));
        DrawArcBand(RenderContext, Body, A - 1.f, A + 1.f, 0.66f * H, 0.72f * H,
                    FxColor(0.8f * Flicker, 0x00FFFFFF), FxColor(0.f, 0x00FFFFFF));
        for(u32 Glint = 0; Glint < 3; Glint++)
        {
            float G = A - 1.f + 2.f * DungeonFxFraction(1.8f * Clock + 0.33f * (float)Glint);
            v2 P = Body + (0.66f * H) * V2(Cos(G), Sin(G));
            DrawShaderQuad(RenderContext, Shader_Glow, P.X - 6.f, P.Y - 6.f, 12.f, 12.f,
                           FxColor(0.8f, 0x00FFFFFF), RenderBlend_Additive);
        }
    }

    // NOTE(zoubir): the off hand raised behind, as a fencer holds it
    v2 Shoulder = RoleLookPoint(Player, 0.62f, CameraOffset);
    v2 Off = Shoulder + V2(-Facing * 0.34f * W, -0.08f * H + 1.f * Sin(2.2f * Clock));
    DrawFxStroke(RenderContext, Shoulder + V2(-Facing * 0.15f * W, 0.f), Off, 2.6f, 2.f,
                 FxColor(0.9f, 0x00140A14), FxColor(0.9f, 0x00140A14), RenderBlend_Alpha);
    DrawDuelistDisc(RenderContext, Off, 2.6f, FxColor(0.9f, 0x00140A14));
    DrawDuelistDisc(RenderContext, Off, 1.8f, FxColor(1.f, 0x00F0E8F8));

    // NOTE(zoubir): Heartseeker's wind-up gathers rose light at the point
    duelist_pose Pose = DuelistPoseAt(AppState, Slot, Player, Clock, Body, 0.f);
    float Length = DUELIST_RAPIER * H;
    if (Player->CastSpell == PlayerSpell_DuelistB)
    {
        float Done = PlayerCastProgress(Player);
        Glow = Maximum(Glow, Done);
        v2 Point = Pose.Grip + Length * Pose.Dir;
        float S = 8.f + 16.f * Done;
        DrawShaderQuad(RenderContext, Shader_Glow, Point.X - S, Point.Y - S, 2.f * S, 2.f * S,
                       FxColor(0.4f + 0.5f * Done, DUELIST_RGB), RenderBlend_Additive);
        for(u32 Mote = 0; Mote < 5; Mote++)
        {
            float A = 2.f * Pi32 * (float)Mote / 5.f + 3.f * Clock;
            float R = 24.f * (1.f - DungeonFxFraction(2.f * Clock + 0.2f * (float)Mote));
            DrawFxDot(RenderContext, Point + R * V2(Cos(A), Sin(A)), 2.5f, FxColor(0.9f, DUELIST_PALE_RGB));
        }
    }
    DrawDuelistRapier(RenderContext, Pose.Grip, Pose.Dir, Length, Glow, 1.f);
    if (Full)
    {
        float Run = DungeonFxFraction(1.4f * Clock);
        v2 P = Pose.Grip + (Length * (0.15f + 0.85f * Run)) * Pose.Dir;
        DrawFxStroke(RenderContext, P - V2(5.f, 0.f), P + V2(5.f, 0.f), 1.5f, 1.5f,
                     FxColor(0.f, 0x00FFFFFF), FxColor(0.95f, 0x00FFFFFF));
        DrawFxStroke(RenderContext, P - V2(0.f, 5.f), P + V2(0.f, 5.f), 1.5f, 1.5f,
                     FxColor(0.95f, 0x00FFFFFF), FxColor(0.f, 0x00FFFFFF));
    }

    // NOTE(zoubir): the Tempo stacks over the head, for the party to read;
    // at five they pulse, while they fade they flicker
    if (Tempo > 0)
    {
        bool32 Fading = (Slot->ClassFlags & DUELIST_FLAG_FADING) != 0;
        float Flicker = Fading ? 0.55f + 0.45f * Sin(14.f * Clock) : 1.f;
        float Pulse = Full ? 0.5f + 0.5f * Sin(8.f * Clock) : 0.f;
        v2 Head = RoleLookPoint(Player, 1.f, CameraOffset) + V2(0.f, -16.f);
        float Gap = 10.f;
        for(u32 Stack = 0; Stack < DUELIST_MOST_TEMPO; Stack++)
        {
            v2 P = Head + V2(Gap * ((float)Stack - 2.f), 0.f);
            DrawDuelistDiamond(RenderContext, P, 11.f, Stack < Tempo ? 1.f : 0.f, Pulse, Flicker);
        }
    }
}

// NOTE(zoubir): every frame in a run, over the world: nothing lasts of
// the Duelist's spells past their bursts
internal void
DrawDuelistFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
}
