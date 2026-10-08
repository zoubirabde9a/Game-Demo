/* Windup glow (draw_entities.cpp): a monster winding up an attack glows
   round its edge in that attack's colour, so a player watching the
   monster and not the ground still sees the blow coming. The glow
   brightens and beats faster as the windup runs out, and a boss's is
   wider. The colours are the ones the cast tells over its head use
   (monster_cast_tells.cpp), one per kind of attack.

   Read only from AbilityPhase, AbilityIndex and AbilityTimer, which
   snapshots carry, so it looks the same online. */

// NOTE(zoubir): world units of glow round the sprite, past the outline
#define WINDUP_GLOW_WIDTH 2.f
#define WINDUP_GLOW_BOSS_WIDTH 3.f

// NOTE(zoubir): one color per ability kind, so the same color means the
// same kind of danger on every monster
inline u32
CastTellColor(monster_ability_kind Kind, u32 Alpha)
{
    u32 Result;
    switch(Kind)
    {
        case MonsterAbility_Slam:   Result = UI_RGBA(255, 140,  50, Alpha); break;
        case MonsterAbility_Charge: Result = UI_RGBA(255,  70,  60, Alpha); break;
        case MonsterAbility_Mortar: Result = UI_RGBA(255, 190,  60, Alpha); break;
        case MonsterAbility_Blink:  Result = UI_RGBA(190, 110, 255, Alpha); break;
        case MonsterAbility_Volley: Result = UI_RGBA(255, 230,  90, Alpha); break;
        case MonsterAbility_Summon: Result = UI_RGBA(120, 255, 150, Alpha); break;
        case MonsterAbility_Mend:   Result = UI_RGBA(140, 255, 210, Alpha); break;
        case MonsterAbility_Burrow: Result = UI_RGBA(220, 170, 110, Alpha); break;
        case MonsterAbility_Smite:  Result = UI_RGBA(255,  40, 120, Alpha); break;
        default:                    Result = UI_RGBA(255, 255, 255, Alpha); break;
    }
    return Result;
}

// NOTE(zoubir): the sprite drawn again in light, a little out on every
// side, just under the outline; nothing for a monster not winding up
internal void
DrawWindupGlow(render_context *RenderContext, render_program TextureProgram,
               world_entity *Entity, u32 TextureId, v2 Position, v2 Dimensions,
               float Angle, v4 Uvs, float SortingValue, float Clock)
{
    if (Entity->Type != EntityType_Monster ||
        Entity->AbilityPhase != AbilityPhase_Windup)
    {
        return;
    }
    monster_def *Def = GetMonsterDef(Entity->MonsterKind);
    if (!Def || Entity->AbilityIndex >= Def->AbilityCount)
    {
        return;
    }
    monster_ability *Ability = &Def->Abilities[Entity->AbilityIndex];
    float Progress = Ability->Windup > 0.f ?
        Clamp01(1.f - Entity->AbilityTimer / Ability->Windup) : 1.f;
    float Beat = 0.5f + 0.5f * Sin(Clock * (8.f + 22.f * Progress));
    float Strength = (0.35f + 0.65f * Progress) * (0.7f + 0.3f * Beat);
    u32 Color = CastTellColor(Ability->Kind, (u32)(255.f * Clamp01(Strength)));
    float Width = Def->SpawnWeight == 0 ? WINDUP_GLOW_BOSS_WIDTH : WINDUP_GLOW_WIDTH;
    Width += 1.f;

    // NOTE(zoubir): the sprite's shape in flat colour; without the shader
    // the sprite's own colours, which dim the glow on a dark body
    render_program Program = RenderContext->Programs[Shader_Silhouette];
    if (Program.ID == RenderContext->TextureProgram.ID)
    {
        Program = TextureProgram;
    }
    BeginBatch(RenderContext, TextureId, SortingValue - 0.002f, Program);
    RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend = RenderBlend_Additive;
    for(u32 Side = 0; Side < 8; Side++)
    {
        float Around = 2.f * Pi32 * (float)Side / 8.f;
        RenderQuadTexture(RenderContext, Position.X + Width * Cos(Around),
                          Position.Y + Width * Sin(Around), Dimensions.X,
                          Dimensions.Y, Uvs, Color, 0.f, Angle);
    }
    EndBatch(RenderContext);
}
