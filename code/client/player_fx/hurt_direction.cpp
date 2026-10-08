/* Hurt direction: when the local player takes a hit, a red arc flares on
   the ground round them on the side the blow came from, so a shot from
   off screen or behind still tells them where to look. Read from health
   and the hit's direction (HitAngle, the way the hit threw the player,
   so the blow came from the opposite side), which snapshots carry, so
   it shows the same online. Damage over time has no hit and no arc.
   Entry point: UpdateHurtDirection and DrawHurtDirection from
   player_fx.cpp. */

#define HURT_ARC_SECONDS 0.6f
#define HURT_ARC_RADIUS 34.f
#define HURT_ARC_WIDTH 14.f
// NOTE(zoubir): half the arc's spread, in radians
#define HURT_ARC_HALF 0.6f
#define HURT_ARC_RGB 0x003030FF

struct hurt_direction
{
    float LastHp;
    u32 LastID;
    // NOTE(zoubir): where the blow came from, and 1 when it landed,
    // fading to 0
    float From;
    float Flare;
};

internal void
UpdateHurtDirection(hurt_direction *Hurt, app_state *AppState, float DeltaTime)
{
    world_entity *Local = GetLocalPlayer(AppState);
    Hurt->Flare = Maximum(0.f, Hurt->Flare - DeltaTime / HURT_ARC_SECONDS);
    if (!Local || !Local->IsPresent)
    {
        Hurt->LastID = 0;
        return;
    }
    // NOTE(zoubir): a new body (a respawn, a map change) is not a hit
    if (Hurt->LastID != Local->ID + 1)
    {
        Hurt->LastID = Local->ID + 1;
        Hurt->LastHp = Local->Hp;
        return;
    }
    if (Local->Hp < Hurt->LastHp && Local->HitFresh > 0.f)
    {
        Hurt->From = Local->HitAngle + Pi32;
        Hurt->Flare = 1.f;
    }
    Hurt->LastHp = Local->Hp;
}

internal void
DrawHurtDirection(render_context *RenderContext, app_state *AppState,
                  hurt_direction *Hurt, v3 CameraOffset)
{
    world_entity *Local = GetLocalPlayer(AppState);
    if (!Local || !Local->IsPresent || Hurt->Flare <= 0.f)
    {
        return;
    }
    // NOTE(zoubir): it bursts out a little as it fades
    float Grow = 1.f - Hurt->Flare;
    float Outer = HURT_ARC_RADIUS + 10.f * Grow + HURT_ARC_WIDTH;
    float Inner = Outer - HURT_ARC_WIDTH;
    v3 Feet = Local->Position;
    Feet.Z = Local->GroundZ;
    v2 Centre = BurstToScreen(Feet, CameraOffset);
    float Alpha = Hurt->Flare * Hurt->Flare;
    DrawArcBand(RenderContext, Centre, Hurt->From - HURT_ARC_HALF, Hurt->From + HURT_ARC_HALF,
                Inner, Outer, FxColor(0.f, HURT_ARC_RGB), FxColor(Alpha, HURT_ARC_RGB),
                RenderBlend_Alpha);
    DrawArcBand(RenderContext, Centre, Hurt->From - 0.6f * HURT_ARC_HALF,
                Hurt->From + 0.6f * HURT_ARC_HALF, Outer - 2.f, Outer,
                FxColor(Alpha, 0x00C0C0FF), FxColor(Alpha, 0x00C0C0FF), RenderBlend_Alpha);
}
