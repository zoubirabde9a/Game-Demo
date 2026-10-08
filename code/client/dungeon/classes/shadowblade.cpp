/* Shadowblade effects (sim/dungeon/role_kits/shadowblade.cpp), included by
   classes/class_fx.cpp: how a Shadowblade looks in a run, its bursts
   (SimBurst_ShadowbladeFirst on, their rows in shadowblade_bursts.inc) and what its
   spells leave in the world.

   The look: a dagger in each hand held point down, a shadow smoking off
   the shoulders, and the combo points as gems over the head. The daggers
   follow the class's bursts: Twin Strike swings one hand then the other
   across the cut, Fan of Knives
   crouches with the daggers crossed then flings both arms out, and
   Eviscerate draws both back then stabs in turn. A critical strike
   waiting lights the blades acid green; Shadow Dance puts a shadow beside
   the player that strikes a moment after it. Everything is read from the
   slot's ClassMeter and ClassFlags, its cast and its bursts, all of which
   reach every client. The bursts are shadowblade/bursts.cpp; the shapes
   shadowblade/shapes.cpp. */

// NOTE(zoubir): a dagger's length, a share of the body's height
#define SHADOWBLADE_DAGGER 0.27f

#include "shadowblade/shapes.cpp"
#include "shadowblade/bursts.cpp"

struct shadowblade_hands
{
    v2 Grip[2];
    v2 Dir[2];
    // NOTE(zoubir): 0 hides a dagger (the thrown one, for a moment)
    float Shown[2];
};

// NOTE(zoubir): where the two daggers are Lag seconds ago (the clone runs
// late), from the player's aim, walk, cast and the class's bursts
internal shadowblade_hands
ShadowbladeHandsAt(app_state *AppState, player_slot *Slot, world_entity *Player, float Clock,
                   v2 Body, float Lag)
{
    shadowblade_hands Result = {};
    u32 SlotIndex = Player->PlayerIndex;
    v2 Aim = ShadowbladeNormal(GetPlayerAim(Player));
    float Facing = Aim.X < -0.1f ? -1.f : 1.f;
    float W = Player->Dimensions.X;
    float Speed = Length(Player->Velocity.XY);
    float Walk = Minimum(1.f, Speed / 150.f);
    float Breathe = 1.2f * Sin(2.4f * Clock + (float)SlotIndex);
    for(u32 Hand = 0; Hand < 2; Hand++)
    {
        // NOTE(zoubir): at rest: the front hand low and out, its blade
        // pointing down and back along the forearm; the back hand nearer
        float Side = Hand == 0 ? Facing : -Facing;
        float Swing = Walk * 3.f * Sin(10.f * Clock + Pi32 * (float)Hand);
        Result.Grip[Hand] = Body + V2(Side * (Hand == 0 ? 0.3f : 0.24f) * W, (Hand == 0 ? 1.f : 3.f) + Breathe + Swing);
        Result.Dir[Hand] = ShadowbladeNormal(V2(-Side * 0.55f + 0.25f * Facing * Walk, 1.f));
        Result.Shown[Hand] = 1.f;
    }

    // NOTE(zoubir): Twin Strike: the front hand cuts across, then the back
    // one the other way, the cuts crossing in front
    float Twin = ShadowbladeBurstAge(AppState, SlotIndex, ShadowbladeBurst_TwinStrike) - Lag;
    for(u32 Hand = 0; Hand < 2; Hand++)
    {
        float Age = Twin - (Hand ? 0.1f : 0.f);
        float Hold = ShadowbladeHold(Age, 0.3f);
        if (Hold <= 0.f)
        {
            continue;
        }
        float Sweep = ShadowbladeEase(Age / 0.12f);
        float From = Hand ? -1.2f : 1.2f;
        float Theta = From - 2.f * From * Sweep;
        v2 Reach = (0.32f * W + 6.f) * ShadowbladeRotate(Aim, Theta);
        v2 Grip = Body + V2(0.f, -2.f) + Reach;
        v2 Dir = ShadowbladeRotate(Aim, 0.5f * Theta + (Hand ? 0.3f : -0.3f));
        Result.Grip[Hand] = ShadowbladeLerp(Result.Grip[Hand], Grip, Hold);
        Result.Dir[Hand] = ShadowbladeNormal(ShadowbladeLerp(Result.Dir[Hand], Dir, Hold));
    }

    // NOTE(zoubir): Fan of Knives: crouched with the daggers crossed while
    // it winds up, then both arms flung wide
    float FanAge = ShadowbladeBurstAge(AppState, SlotIndex, ShadowbladeBurst_Fan) - Lag;
    bool32 Crouch = Player->CastSpell == PlayerSpell_ShadowbladeA;
    bool32 Draw = Player->CastSpell == PlayerSpell_ShadowbladeB;
    float Fling = ShadowbladeHold(FanAge, 0.35f);
    for(u32 Hand = 0; Hand < 2; Hand++)
    {
        float Side = Hand == 0 ? 1.f : -1.f;
        if (Crouch)
        {
            float In = ShadowbladeEase(PlayerCastProgress(Player) * 3.f);
            v2 Grip = Body + V2(Side * 0.08f * W, 8.f);
            v2 Dir = ShadowbladeNormal(V2(-Side * 1.f, -0.5f));
            Result.Grip[Hand] = ShadowbladeLerp(Result.Grip[Hand], Grip, In);
            Result.Dir[Hand] = ShadowbladeNormal(ShadowbladeLerp(Result.Dir[Hand], Dir, In));
        }
        else if (Fling > 0.f)
        {
            v2 Grip = Body + V2(Side * (0.7f * W + 6.f), -6.f);
            v2 Dir = ShadowbladeNormal(V2(Side, -0.35f));
            Result.Grip[Hand] = ShadowbladeLerp(Result.Grip[Hand], Grip, Fling);
            Result.Dir[Hand] = ShadowbladeNormal(ShadowbladeLerp(Result.Dir[Hand], Dir, Fling));
        }
    }

    // NOTE(zoubir): Eviscerate: both drawn back and up while it winds up,
    // then quick stabs in turn along the burst's angle
    float Evis = ShadowbladeBurstAge(AppState, SlotIndex, ShadowbladeBurst_Eviscerate) - Lag;
    float Stab = ShadowbladeHold(Evis, 0.4f);
    for(u32 Hand = 0; Hand < 2; Hand++)
    {
        float Side = Hand == 0 ? Facing : -Facing;
        if (Draw)
        {
            float In = ShadowbladeEase(PlayerCastProgress(Player) * 2.f);
            v2 Grip = Body - 0.3f * W * Aim + V2(Side * 0.3f * W, -10.f);
            v2 Dir = ShadowbladeNormal(Aim + V2(0.f, -0.6f));
            Result.Grip[Hand] = ShadowbladeLerp(Result.Grip[Hand], Grip, In);
            Result.Dir[Hand] = ShadowbladeNormal(ShadowbladeLerp(Result.Dir[Hand], Dir, In));
        }
        else if (Stab > 0.f)
        {
            float Beat = Sin(Pi32 * Clamp01(Evis / 0.3f) * 5.f + Pi32 * (float)Hand);
            v2 Grip = Body + (0.3f + 0.35f * Maximum(0.f, Beat)) * W * Aim + V2(Side * 0.18f * W, -2.f);
            Result.Grip[Hand] = ShadowbladeLerp(Result.Grip[Hand], Grip, Stab);
            Result.Dir[Hand] = ShadowbladeNormal(ShadowbladeLerp(Result.Dir[Hand], Aim, Stab));
        }
    }
    return Result;
}

internal void
DrawShadowbladeHands(render_context *RenderContext, shadowblade_hands *Hands, float Size,
                     float Glow, float Alpha)
{
    for(u32 Hand = 2; Hand > 0; Hand--)
    {
        if (Hands->Shown[Hand - 1] > 0.f)
        {
            DrawShadowbladeDagger(RenderContext, Hands->Grip[Hand - 1], Hands->Dir[Hand - 1], Size,
                                  Glow, Alpha * Hands->Shown[Hand - 1]);
        }
    }
}

// NOTE(zoubir): a living Shadowblade's look, over its sprite, every frame
internal void
DrawShadowbladeLook(render_context *RenderContext, app_state *AppState, player_slot *Slot,
            world_entity *Player, float Clock, v3 CameraOffset)
{
    u32 SlotIndex = Player->PlayerIndex;
    v2 Aim = ShadowbladeNormal(GetPlayerAim(Player));
    float W = Player->Dimensions.X;
    float H = Player->Dimensions.Y;
    v2 Feet = RoleLookPoint(Player, 0.f, CameraOffset);
    v2 Body = RoleLookPoint(Player, 0.38f, CameraOffset);
    bool32 Dance = (Slot->ClassFlags & SHADOWBLADE_FLAG_DANCE) != 0;
    bool32 Crit = (Slot->ClassFlags & SHADOWBLADE_FLAG_CRIT) != 0;
    v2 Velocity = Player->Velocity.XY;
    float Speed = Length(Velocity);

    // NOTE(zoubir): just arrived from a Shadowstep: everything fades in
    float Step = ShadowbladeBurstAge(AppState, SlotIndex, ShadowbladeBurst_Step);
    float Alpha = Step < 0.2f ? 0.3f + 0.7f * ShadowbladeEase(Step / 0.2f) : 1.f;

    // NOTE(zoubir): afterimages behind it when it moves fast
    if (Speed > 260.f)
    {
        v2 Back = (-1.f / Speed) * Velocity;
        for(u32 Image = 1; Image <= 3; Image++)
        {
            float Fade = 0.35f * Clamp01((Speed - 260.f) / 200.f) * (1.f - 0.28f * (float)Image);
            DrawShadowbladeSilhouette(RenderContext, Feet + (14.f * (float)Image) * Back, 0.9f * W,
                                      H, Fade);
        }
    }

    // NOTE(zoubir): Shadow Dance's clone, behind the shoulder, a beat late
    if (Dance)
    {
        v2 Side = V2(-Aim.Y, Aim.X);
        float Sway = 3.f * Sin(3.f * Clock);
        v2 Offset = -22.f * Aim + (12.f + Sway) * Side;
        float Flicker = 0.75f + 0.15f * Sin(17.f * Clock) + 0.1f * Sin(5.3f * Clock);
        DrawShadowbladeSilhouette(RenderContext, Feet + Offset, 0.95f * W, 0.96f * H, 0.8f * Flicker);
        shadowblade_hands Clone = ShadowbladeHandsAt(AppState, Slot, Player, Clock, Body + Offset, 0.09f);
        DrawShadowbladeHands(RenderContext, &Clone, SHADOWBLADE_DAGGER * H, 1.f, 0.5f * Flicker);
        // NOTE(zoubir): wisps rising off the clone
        for(u32 Wisp = 0; Wisp < 4; Wisp++)
        {
            float Rise = DungeonFxFraction(0.9f * Clock + 0.25f * (float)Wisp);
            v2 P = Feet + Offset + V2((BurstJitter(Wisp, 301) - 0.5f) * W, -H * (0.3f + 0.8f * Rise));
            DrawFxDot(RenderContext, P, 3.f, FxColor(0.6f * (1.f - Rise), SHADOWBLADE_RGB));
        }
    }

    // NOTE(zoubir): the shadow smoking off the shoulders and trailing the
    // way it came, more of it on the move
    v2 Shoulders = RoleLookPoint(Player, 0.7f, CameraOffset);
    v2 Trail = Speed > 20.f ? (-1.f / Speed) * Velocity : V2(-0.3f * (Aim.X < 0.f ? -1.f : 1.f), 0.f);
    float Drift = 10.f + 22.f * Minimum(1.f, Speed / 200.f);
    for(u32 Wisp = 0; Wisp < 9; Wisp++)
    {
        float Phase = DungeonFxFraction(0.8f * Clock + 0.111f * (float)Wisp + 0.37f * (float)SlotIndex);
        float X = (BurstJitter(Wisp + 16 * SlotIndex, 311) - 0.5f) * 0.9f * W;
        v2 P = Shoulders + V2(X + 3.f * Sin(5.f * Clock + (float)Wisp), 2.f) +
            (Drift * Phase) * Trail + V2(0.f, -16.f * Phase);
        float Size = (5.f + 5.f * Phase) * (Wisp % 3 == 0 ? 1.2f : 1.f);
        float Fade = Alpha * (1.f - Phase) * Minimum(1.f, 4.f * Phase);
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - Size, P.Y - Size, 2.f * Size, 2.f * Size,
                       FxColor(0.7f * Fade, SHADOWBLADE_DEEP_RGB), RenderBlend_Alpha);
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - 0.7f * Size, P.Y - 0.7f * Size, 1.4f * Size,
                       1.4f * Size, FxColor(0.45f * Fade, SHADOWBLADE_RGB), RenderBlend_Additive);
    }

    // NOTE(zoubir): the daggers, lit acid green while a critical strike
    // waits, a glint running up the edge
    float Glow = Crit ? 0.65f + 0.35f * Sin(9.f * Clock) : 0.f;
    shadowblade_hands Hands = ShadowbladeHandsAt(AppState, Slot, Player, Clock, Body, 0.f);
    DrawShadowbladeHands(RenderContext, &Hands, SHADOWBLADE_DAGGER * H, Glow, Alpha);
    if (Crit)
    {
        for(u32 Hand = 0; Hand < 2; Hand++)
        {
            float Run = DungeonFxFraction(1.6f * Clock + 0.5f * (float)Hand);
            v2 P = Hands.Grip[Hand] + (SHADOWBLADE_DAGGER * H * (0.2f + 0.8f * Run)) * Hands.Dir[Hand];
            DrawFxStroke(RenderContext, P - V2(5.f, 0.f), P + V2(5.f, 0.f), 1.5f, 1.5f,
                         FxColor(0.f, 0x00FFFFFF), FxColor(0.9f, 0x00FFFFFF));
            DrawFxStroke(RenderContext, P - V2(0.f, 5.f), P + V2(0.f, 5.f), 1.5f, 1.5f,
                         FxColor(0.9f, 0x00FFFFFF), FxColor(0.f, 0x00FFFFFF));
        }
    }

    // NOTE(zoubir): the combo points over the head, for the whole party
    // to read; at five they pulse, while they fade they flicker
    u32 Points = Slot->ClassMeter;
    if (Points > 0)
    {
        bool32 Fading = (Slot->ClassFlags & SHADOWBLADE_FLAG_FADING) != 0;
        float Full = Points >= SHADOWBLADE_MOST_POINTS ? 0.5f + 0.5f * Sin(8.f * Clock) : 0.f;
        float Flicker = Fading ? 0.55f + 0.45f * Sin(14.f * Clock) : 1.f;
        v2 Head = RoleLookPoint(Player, 1.f, CameraOffset) + V2(0.f, -16.f);
        float Gap = 10.f;
        for(u32 Gem = 0; Gem < SHADOWBLADE_MOST_POINTS; Gem++)
        {
            v2 P = Head + V2(Gap * ((float)Gem - 2.f), -2.f * Full * (Gem % 2 ? 1.f : 0.f));
            DrawShadowbladeGem(RenderContext, P, 7.f, Gem < Points ? 1.f : 0.f, 0.f, Full,
                               Flicker * Alpha);
        }
    }
}

// NOTE(zoubir): every frame in a run, over the world: poison dripping off
// the monsters its daggers cut. Poisoned shows in every client's copy of
// the monster, so the drips do too
internal void
DrawShadowbladeFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    bool32 Party = false;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        Party |= AppState->Players[SlotIndex].Active &&
            AppState->Players[SlotIndex].Role == PlayerRole_Shadowblade;
    }
    if (!Party)
    {
        return;
    }
    world *World = &AppState->World;
    float Clock = GetFxClock(AppState);
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            !HasStatus(Monster, StatusEffect_Poisoned))
        {
            continue;
        }
        DrawShadowbladePoison(RenderContext, Monster, Clock, EntityIndex, CameraOffset);
    }
}
