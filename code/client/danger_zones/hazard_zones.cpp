/* Hazard zones (danger_zones.cpp): what burns or snares a player standing
   on it after a blow lands (Web Nova, Anvil Drop, Hellfire Cleave, Magma
   Dive, Cinder Cleave and the rest of the MonsterHazard entities). The
   hazard hurts within a circle of its ability's Radius, but its sprite is
   a squashed blob, so the true circle is painted on the ground under it
   through the danger zone shader, its fill drawing in toward the middle
   as the hazard runs out.

   Snapshots do not carry how long a hazard has left, so it is timed from
   when this client first saw it, HazardSeconds of its ability; online that
   can be a snapshot late. */

#define HAZARD_ZONE_STRENGTH 0.55f
// NOTE(zoubir): fades out over its last this-many seconds
#define HAZARD_ZONE_FADE 0.4f

internal void
DrawHazardZones(render_context *RenderContext, world *World, danger_zones *Zones,
                v3 CameraOffset, float Now)
{
    for(u32 Slot = 0; Slot < DANGER_TRACKED; Slot++)
    {
        world_entity *Hazard = &World->Entities[Slot];
        if (Slot >= World->EntityCount || !Hazard->IsPresent ||
            Hazard->Type != EntityType_MonsterHazard || Hazard->MonsterKind >= MonsterKind_Count)
        {
            Zones->HazardId[Slot] = 0;
            continue;
        }
        if (Zones->HazardId[Slot] != Hazard->ID)
        {
            Zones->HazardId[Slot] = Hazard->ID;
            Zones->HazardBorn[Slot] = Now;
        }
        monster_def *Def = GetMonsterDef(Hazard->MonsterKind);
        if (Hazard->AbilityIndex >= Def->AbilityCount)
        {
            continue;
        }
        monster_ability *Ability = &Def->Abilities[Hazard->AbilityIndex];
        float Lasts = Maximum(Ability->HazardSeconds, 0.01f);
        float Left = Lasts - (Now - Zones->HazardBorn[Slot]);
        if (Ability->Radius <= 0.f || Left <= 0.f)
        {
            continue;
        }
        float Strength = HAZARD_ZONE_STRENGTH * Clamp01(Left / HAZARD_ZONE_FADE);
        DrawDangerDisc(RenderContext, World, CameraOffset, Hazard->Position.XY, Ability->Radius,
                       Clamp01(Left / Lasts), DANGER_PALETTE_HARM, 0, Strength);
    }
}
