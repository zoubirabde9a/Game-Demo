/* Status motes: small particles round every unit with a running status
   effect (sim/status_effects.cpp), so anyone can see what is happening
   to it: embers rising off the burning, green bubbles off the poisoned,
   drops falling from the bleeding and the soaked, sparkles rising round
   whoever is healing, motes whirling round the hasted, a ring of roots
   at a rooted unit's feet and a dark swirl under one falling into a pit.
   Slowed and stunned have looks of their own (talent_fx.cpp, the stars
   in fx_bursts.cpp). One row per effect below; colours come from the
   effect's pip colour. Replicas carry statuses as bits, so it looks the
   same online. */

enum status_motion
{
    StatusMotion_None,
    // NOTE(zoubir): from the feet up past the head, fading
    StatusMotion_Rise,
    // NOTE(zoubir): from the chest down to the feet
    StatusMotion_Drip,
    // NOTE(zoubir): circling the body at waist height
    StatusMotion_Orbit,
    // NOTE(zoubir): a ring of motes on the ground, closing in
    StatusMotion_Ring,
};

struct status_fx_look
{
    status_motion Motion;
    u32 Count;
    // NOTE(zoubir): cycles a second
    float Speed;
    float Size;
};

global_variable status_fx_look StatusFxLooks[StatusEffect_Count] =
{
    {StatusMotion_None,  0, 0.f,  0.f},  // None
    {StatusMotion_Rise,  7, 1.3f, 3.f},  // Burning
    {StatusMotion_Rise,  4, 0.6f, 3.f},  // Poisoned
    {StatusMotion_None,  0, 0.f,  0.f},  // Slowed: frost, talent_fx.cpp
    {StatusMotion_None,  0, 0.f,  0.f},  // Stunned: stars, fx_bursts.cpp
    {StatusMotion_Drip,  3, 1.1f, 2.5f}, // Bleeding
    {StatusMotion_Rise,  6, 0.8f, 2.5f}, // Regenerating
    {StatusMotion_Orbit, 5, 1.6f, 2.5f}, // Hasted
    {StatusMotion_Ring,  8, 0.4f, 3.f},  // Rooted
    {StatusMotion_Drip,  4, 0.9f, 2.f},  // Soaked
    {StatusMotion_Ring, 10, 1.5f, 3.f},  // Falling
};

internal void
DrawStatusMotes(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    world *World = &AppState->World;
    float Clock = GetFxClock(AppState);
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent || Entity->Hp <= 0.f || !Entity->Collision ||
            (Entity->Type != EntityType_Player && Entity->Type != EntityType_Monster))
        {
            continue;
        }
        v3 HalfDims = Entity->Collision->TotalVolume.HalfDims;
        float Radius = Maximum(12.f, 1.1f * HalfDims.X);
        float Height = Maximum(24.f, 2.f * HalfDims.Z);
        v3 FeetAt = Entity->Position;
        v2 Feet = BurstToScreen(FeetAt, CameraOffset);
        for(u32 Effect = 1; Effect < StatusEffect_Count; Effect++)
        {
            status_fx_look *Look = &StatusFxLooks[Effect];
            if (Look->Motion == StatusMotion_None || Entity->StatusTimers[Effect] <= 0.f)
            {
                continue;
            }
            u32 RGB = (StatusTable[Effect].Color ? StatusTable[Effect].Color : 0xFF402030) &
                0x00FFFFFF;
            for(u32 Mote = 0; Mote < Look->Count; Mote++)
            {
                float Offset = BurstJitter(Mote, EntityIndex * 16 + Effect);
                float Life = fmodf(Look->Speed * Clock + Offset, 1.f);
                float Angle = 2.f * Pi32 * BurstJitter(Mote + 31, EntityIndex + Effect);
                v2 P = Feet;
                float Alpha = 1.f;
                switch(Look->Motion)
                {
                    case StatusMotion_Rise:
                    {
                        P += GroundCircle(Angle, 0.7f * Radius) - V2(0.f, (Height + 10.f) * Life);
                        Alpha = 1.f - Life;
                    } break;
                    case StatusMotion_Drip:
                    {
                        P += GroundCircle(Angle, 0.5f * Radius) -
                            V2(0.f, 0.8f * Height * (1.f - Life));
                        Alpha = 0.4f + 0.6f * Life;
                    } break;
                    case StatusMotion_Orbit:
                    {
                        float Turn = Angle + 2.f * Pi32 * Look->Speed * Clock;
                        P += GroundCircle(Turn, Radius) - V2(0.f, 0.5f * Height);
                        Alpha = 0.5f + 0.5f * Sin(Turn);
                    } break;
                    case StatusMotion_Ring:
                    {
                        float Turn = 2.f * Pi32 * ((float)Mote / (float)Look->Count) +
                            Look->Speed * Clock;
                        P += GroundCircle(Turn, Radius * (1.2f - 0.4f * Life));
                        Alpha = 0.6f + 0.4f * Life;
                    } break;
                    default: break;
                }
                DrawFxDot(RenderContext, P, 1.4f * Look->Size, FxColor(0.9f * Alpha, RGB));
            }
        }
    }
}
