/* Player abilities that act on other entities (monsters and other
   players, never the user). UpdatePlayer decides when
   one fires (key + cooldown); the effect itself lives here. */

#define SHOCKWAVE_RADIUS 90.f
#define SHOCKWAVE_DAMAGE 40.f
#define SHOCKWAVE_KNOCKBACK 500.f
// NOTE(zoubir): how long the ring stays on screen
#define SHOCKWAVE_FLASH_SECONDS 0.25f

// NOTE(zoubir): hits every monster within SHOCKWAVE_RADIUS on the ground
// plane and throws the survivors away from Source. Returns monsters hit.
internal u32
TriggerShockwave(app_state *AppState, world *World, world_entity *Source)
{
    u32 HitCount = 0;
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Target = &World->Entities[EntityIndex];
        if (!Target->IsPresent || Target == Source ||
            (Target->Type != EntityType_Monster &&
             Target->Type != EntityType_Player))
        {
            continue;
        }

        v2 Away = Target->Position.XY - Source->Position.XY;
        float Distance = Length(Away);
        if (Distance > SHOCKWAVE_RADIUS)
        {
            continue;
        }

        HitCount++;
        if (!DamageEntity(AppState, World, Target, SHOCKWAVE_DAMAGE, Source) &&
            Distance > 0.f)
        {
            Target->Velocity.XY += (SHOCKWAVE_KNOCKBACK / Distance) * Away;
        }
    }
    Source->ShockwaveFlash = SHOCKWAVE_FLASH_SECONDS;
    return HitCount;
}
