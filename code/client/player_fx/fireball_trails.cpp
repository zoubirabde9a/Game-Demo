/* Fireball trails: every fireball leaves an ember each frame where it is,
   at the height it flies; each ember shrinks, cools from yellow to red
   and fades over FIREBALL_TRAIL_SECONDS. Read from the fireball entities
   themselves, so replicas trail the same way online. */

#define MAX_FIREBALL_EMBERS 256
#define FIREBALL_TRAIL_SECONDS 0.22f

struct fireball_ember
{
    v2 Position;
    float Age;
};

struct fireball_trails
{
    fireball_ember Embers[MAX_FIREBALL_EMBERS];
    u32 Count;
};

internal void
UpdateFireBallTrails(fireball_trails *Fx, app_state *AppState, float DeltaTime)
{
    for(u32 Index = 0; Index < Fx->Count;)
    {
        fireball_ember *Ember = &Fx->Embers[Index];
        Ember->Age += DeltaTime;
        if (Ember->Age >= FIREBALL_TRAIL_SECONDS)
        {
            *Ember = Fx->Embers[--Fx->Count];
        }
        else
        {
            Index++;
        }
    }

    world *World = &AppState->World;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *FireBall = &World->Entities[Index];
        if (FireBall->IsPresent && FireBall->Type == EntityType_FireBall &&
            Fx->Count < MAX_FIREBALL_EMBERS)
        {
            fireball_ember *Ember = &Fx->Embers[Fx->Count++];
            // NOTE(zoubir): drawn where the sprite is, lifted by its height
            Ember->Position = FireBall->Position.XY - V2(0.f, FireBall->Position.Z);
            Ember->Age = 0.f;
        }
    }
}

internal void
DrawFireBallTrails(render_context *RenderContext, fireball_trails *Fx,
                   v3 CameraOffset)
{
    for(u32 Index = 0; Index < Fx->Count; Index++)
    {
        fireball_ember *Ember = &Fx->Embers[Index];
        float Life = 1.f - Ember->Age / FIREBALL_TRAIL_SECONDS;
        // NOTE(zoubir): yellow when fresh, red as it cools (0xAABBGGRR)
        u32 Green = (u32)(60.f + 170.f * Life);
        u32 Color = ((u32)(220.f * Life) << 24) | (40u << 16) | (Green << 8) | 255u;
        DrawFxDot(RenderContext, Ember->Position - CameraOffset.XY,
                  2.f + 5.f * Life, Color);
    }
}
