/* Monster telegraphs: what is drawn over the world each frame on and round
   monsters: the rings and lanes of attacks in windup that the ground does
   not paint (client/danger_zones.cpp paints the rest), burrow ripples,
   enrage bursts, shell arcs, elite auras and status pips.

   Entry point: DrawMonsterTelegraphs, once a frame from screen_pass.inc. */

#define TELEGRAPH_DOTS 28
#define TELEGRAPH_COLOR_DANGER 0xFF3040FF
#define TELEGRAPH_COLOR_HOT 0xFF60D0FF
#define TELEGRAPH_COLOR_SPIRIT 0xFFF0E070
#define TELEGRAPH_COLOR_MEND 0xFF70F070

// NOTE(zoubir): a ring of small squares; the renderer has no circles
internal void
DrawDottedCircle(render_context *RenderContext, v2 Center, float Radius,
                 u32 Color, float DotSize)
{
    for(u32 Dot = 0; Dot < TELEGRAPH_DOTS; Dot++)
    {
        float Angle = 2.f * Pi32 * (float)Dot / (float)TELEGRAPH_DOTS;
        // NOTE(zoubir): round, like the area the hit checks; the ground is
        // seen from straight above
        v2 P = Center + V2(Radius * Cos(Angle), Radius * Sin(Angle));
        DrawFilledRectangle(RenderContext, P.X - 0.5f * DotSize,
                            P.Y - 0.5f * DotSize, DotSize, DotSize, Color, 0.f);
    }
}

internal void
DrawDottedLine(render_context *RenderContext, v2 From, v2 To, u32 Color,
               float DotSize, float Spacing)
{
    float Distance = Length(To - From);
    u32 Count = (u32)(Distance / Spacing) + 1;
    for(u32 Dot = 0; Dot <= Count; Dot++)
    {
        v2 P = Lerp2(From, (float)Dot / (float)Count, To);
        DrawFilledRectangle(RenderContext, P.X - 0.5f * DotSize,
                            P.Y - 0.5f * DotSize, DotSize, DotSize, Color, 0.f);
    }
}

#define STATUS_PIP_SIZE 4.f

// NOTE(zoubir): a row of small squares above anyone with a status, one
// per effect; each blinks during its last second
internal void
DrawStatusPips(render_context *RenderContext, world *World, v3 CameraOffset)
{
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent || Entity->Hp <= 0.f ||
            (Entity->Type != EntityType_Player &&
             Entity->Type != EntityType_Monster))
        {
            continue;
        }
        v2 Head = Entity->Position.XY - CameraOffset.XY;
        Head.Y -= Entity->Position.Z + Entity->Dimensions.Y + 8.f;
        float X = Head.X - 0.5f * STATUS_PIP_SIZE;
        for(u32 Effect = 1; Effect < StatusEffect_Count; Effect++)
        {
            float Left = Entity->StatusTimers[Effect];
            if (Left <= 0.f || !StatusTable[Effect].Color)
            {
                continue;
            }
            bool32 Hidden = Left < 1.f && ((u32)(Left * 8.f) % 2) == 0;
            if (!Hidden)
            {
                DrawFilledRectangle(RenderContext, X, Head.Y, STATUS_PIP_SIZE,
                                    STATUS_PIP_SIZE, StatusTable[Effect].Color, 0.f);
            }
            X += STATUS_PIP_SIZE + 2.f;
        }
    }
}

// NOTE(zoubir): the danger zone of every monster in windup. The ring
// closes in from twice its size as the windup runs out, so the moment it
// matches the real radius is the moment it hits
// NOTE(zoubir): a slowly turning ring of dots at an elite's feet, in its
// affix color
internal void
DrawEliteAuras(render_context *RenderContext, world *World, v3 CameraOffset)
{
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent || Entity->Type != EntityType_Monster ||
            !Entity->EliteAffix)
        {
            continue;
        }
        monster_affix_def *Affix = GetAffix(Entity->EliteAffix);
        v2 Feet = Entity->Position.XY - CameraOffset.XY;
        float Radius = 0.45f * Entity->Dimensions.X;
        // NOTE(zoubir): steps round with the animation clock
        float Turn = 0.35f * (float)Entity->AnimationState.SlotIndex;
        u32 Dots = 12;
        for(u32 Dot = 0; Dot < Dots; Dot++)
        {
            float Angle = Turn + 2.f * Pi32 * (float)Dot / (float)Dots;
            v2 P = Feet + V2(Radius * Cos(Angle), 0.45f * Radius * Sin(Angle));
            float Size = (Dot % 3 == 0) ? 3.f : 2.f;
            DrawFilledRectangle(RenderContext, P.X - 0.5f * Size, P.Y - 0.5f * Size,
                                Size, Size, Affix->AuraColor, 0.f);
        }
    }
}

#define BURROW_RIPPLE_COLOR 0xFF60A0D0

// NOTE(zoubir): a burrowing monster shows as a line of churned ground
// running from where it dug in toward where it will come up; once the
// spot locks, a closing ring marks the eruption
internal void
DrawBurrowTelegraphs(render_context *RenderContext, world *World,
                     v3 CameraOffset)
{
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent || Entity->Type != EntityType_Monster ||
            !Entity->Burrowed || Entity->AbilityPhase != AbilityPhase_Active)
        {
            continue;
        }
        monster_def *Def = GetMonsterDef(Entity->MonsterKind);
        monster_ability *Ability = &Def->Abilities[Entity->AbilityIndex];
        float Progress = Ability->Active > 0.f ?
            ArtClamp01(1.f - Entity->AbilityTimer / Ability->Active) : 1.f;
        v2 From = Entity->AbilityPoints[1] - CameraOffset.XY;
        v2 To = Entity->AbilityPoints[0] - CameraOffset.XY;
        // NOTE(zoubir): the head of the ripple travels along the tunnel
        v2 Head = Lerp2(From, Progress, To);
        DrawDottedLine(RenderContext, From, Head, BURROW_RIPPLE_COLOR, 2.f, 9.f);
        DrawDottedCircle(RenderContext, Head, 10.f, BURROW_RIPPLE_COLOR, 3.f);
        bool32 Locked = Entity->AbilityTimer <= BURROW_LOCK_SHARE * Ability->Active;
        if (Locked)
        {
            float LockProgress = 1.f - Entity->AbilityTimer /
                (BURROW_LOCK_SHARE * Ability->Active);
            bool32 Flash = LockProgress > 0.6f &&
                ((u32)(LockProgress * 20.f) % 2) == 0;
            u32 Color = Flash ? TELEGRAPH_COLOR_HOT : TELEGRAPH_COLOR_DANGER;
            DrawDottedCircle(RenderContext, To, Ability->Radius, Color, 2.f);
            DrawDottedCircle(RenderContext, To, Ability->Radius * (2.f - LockProgress),
                             Color, 3.f);
        }
    }
}

#define ENRAGE_BURST_COLOR 0xFF2050FF

// NOTE(zoubir): a ring blowing outward the moment a monster enrages
internal void
DrawEnrageBursts(render_context *RenderContext, world *World, v3 CameraOffset)
{
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent || Entity->Type != EntityType_Monster ||
            Entity->PhaseFlash <= 0.f)
        {
            continue;
        }
        float Progress = 1.f - Entity->PhaseFlash / ENRAGE_FLASH_SECONDS;
        v2 Feet = Entity->Position.XY - CameraOffset.XY;
        float Radius = Entity->Dimensions.X * (0.4f + 1.2f * Progress);
        DrawDottedCircle(RenderContext, Feet, Radius, ENRAGE_BURST_COLOR, 3.f);
        DrawDottedCircle(RenderContext, Feet, 0.7f * Radius, ENRAGE_BURST_COLOR, 2.f);
    }
}

#define SHELL_ARC_COLOR 0xFFB0E0F0
#define SHELL_BLOCK_COLOR 0xFFFFFFFF

// NOTE(zoubir): an arc of dots on the ground in front of armored monsters,
// showing the side their shell covers; it flashes white on a block
internal void
DrawShellArcs(render_context *RenderContext, world *World, v3 CameraOffset)
{
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent || Entity->Type != EntityType_Monster)
        {
            continue;
        }
        monster_def *Def = GetMonsterDef(Entity->MonsterKind);
        if (Def->FrontArmor <= 0.f || LengthSq(Entity->Direction) < 0.0001f)
        {
            continue;
        }
        v2 Feet = Entity->Position.XY - CameraOffset.XY;
        float Radius = 0.6f * Entity->Dimensions.X;
        float Facing = ATan2(Entity->Direction.Y, Entity->Direction.X);
        float HalfArc = 0.5f * Def->FrontArcDegrees * (Pi32 / 180.f);
        bool32 Blocked = Entity->BlockFlash > 0.f;
        u32 Color = Blocked ? SHELL_BLOCK_COLOR : SHELL_ARC_COLOR;
        float Size = Blocked ? 3.f : 2.f;
        u32 Dots = 9;
        for(u32 Dot = 0; Dot < Dots; Dot++)
        {
            float Angle = Facing - HalfArc + 2.f * HalfArc * (float)Dot / (float)(Dots - 1);
            v2 P = Feet + V2(Radius * Cos(Angle), 0.6f * Radius * Sin(Angle));
            DrawFilledRectangle(RenderContext, P.X - 0.5f * Size, P.Y - 0.5f * Size,
                                Size, Size, Color, 0.f);
        }
    }
}

internal void
DrawMonsterTelegraphs(render_context *RenderContext, world *World,
                      v3 CameraOffset)
{
    DrawEliteAuras(RenderContext, World, CameraOffset);
    DrawShellArcs(RenderContext, World, CameraOffset);
    DrawEnrageBursts(RenderContext, World, CameraOffset);
    DrawBurrowTelegraphs(RenderContext, World, CameraOffset);
    DrawStatusPips(RenderContext, World, CameraOffset);
    bool32 PaintsDangerZones = RenderContext->Programs[Shader_DangerZone].ID !=
        RenderContext->TextureProgram.ID;
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent || Entity->Type != EntityType_Monster ||
            Entity->AbilityPhase != AbilityPhase_Windup)
        {
            continue;
        }
        monster_def *Def = GetMonsterDef(Entity->MonsterKind);
        monster_ability *Ability = &Def->Abilities[Entity->AbilityIndex];
        // NOTE(zoubir): these are painted on the ground under the units
        // (client/danger_zones.cpp) when its shader loaded
        bool32 Painted = PaintsDangerZones &&
            (Ability->Kind == MonsterAbility_Slam || Ability->Kind == MonsterAbility_Charge ||
             Ability->Kind == MonsterAbility_Mortar || Ability->Kind == MonsterAbility_Blink ||
             Ability->Kind == MonsterAbility_Volley || Ability->Kind == MonsterAbility_Summon);
        if (Painted)
        {
            continue;
        }
        float Progress = Ability->Windup > 0.f ?
            1.f - Entity->AbilityTimer / Ability->Windup : 1.f;
        Progress = ArtClamp01(Progress);
        float Closing = 2.f - Progress;
        // NOTE(zoubir): blinks faster as the hit gets close
        bool32 Flash = Progress > 0.7f &&
            ((u32)(Progress * 20.f) % 2) == 0;
        u32 Color = Flash ? TELEGRAPH_COLOR_HOT : TELEGRAPH_COLOR_DANGER;
        v2 Self = Entity->Position.XY - CameraOffset.XY;

        switch(Ability->Kind)
        {
            case MonsterAbility_Slam:
            {
                DrawDottedCircle(RenderContext, Self, Ability->Radius, Color, 2.f);
                DrawDottedCircle(RenderContext, Self, Ability->Radius * Closing,
                                 Color, 3.f);
            } break;

            case MonsterAbility_Charge:
            {
                float Reach = Ability->Speed * Ability->Active;
                v2 End = Self + Reach * Entity->AbilityAim;
                v2 Side = Ability->Radius * 0.5f *
                    V2(-Entity->AbilityAim.Y, Entity->AbilityAim.X);
                DrawDottedLine(RenderContext, Self + Side, End + Side, Color, 2.f, 8.f);
                DrawDottedLine(RenderContext, Self - Side, End - Side, Color, 2.f, 8.f);
                // NOTE(zoubir): the filling arrow shows how long is left
                DrawDottedLine(RenderContext, Self, Self + Progress * (End - Self),
                               Color, 4.f, 6.f);
            } break;

            case MonsterAbility_Mortar:
            {
                for(u32 PointIndex = 0;
                    PointIndex < Entity->AbilityPointCount;
                    PointIndex++)
                {
                    v2 Spot = Entity->AbilityPoints[PointIndex] - CameraOffset.XY;
                    DrawDottedCircle(RenderContext, Spot, Ability->Radius, Color, 2.f);
                    DrawDottedCircle(RenderContext, Spot, Ability->Radius * Progress,
                                     Color, 3.f);
                }
            } break;

            case MonsterAbility_Blink:
            {
                v2 Spot = Entity->AbilityPoints[0] - CameraOffset.XY;
                u32 SpiritColor = Flash ? TELEGRAPH_COLOR_HOT : TELEGRAPH_COLOR_SPIRIT;
                DrawDottedCircle(RenderContext, Spot, Ability->Radius, SpiritColor, 2.f);
                DrawDottedCircle(RenderContext, Spot, Ability->Radius * Closing,
                                 SpiritColor, 3.f);
            } break;

            case MonsterAbility_Summon:
            {
                // NOTE(zoubir): graves that open when the windup ends; a
                // player standing on one keeps it shut
                u32 GraveColor = Flash ? TELEGRAPH_COLOR_HOT : TELEGRAPH_COLOR_MEND;
                for(u32 PointIndex = 0;
                    PointIndex < Entity->AbilityPointCount;
                    PointIndex++)
                {
                    v2 Spot = Entity->AbilityPoints[PointIndex] - CameraOffset.XY;
                    DrawDottedCircle(RenderContext, Spot, 16.f, GraveColor, 2.f);
                    DrawDottedCircle(RenderContext, Spot, 16.f * Progress,
                                     GraveColor, 3.f);
                }
            } break;

            case MonsterAbility_Burrow:
            {
                // NOTE(zoubir): sand sinking in around it as it digs
                DrawDottedCircle(RenderContext, Self, 0.5f * Entity->Dimensions.X *
                                 (1.f + Progress), BURROW_RIPPLE_COLOR, 2.f);
            } break;

            case MonsterAbility_Mend:
            {
                // NOTE(zoubir): a beam to the ally being healed, which follows
                // it if it moves
                world_entity *Ally = FindMonsterBySerial(World, Entity->AbilityTargetSlot,
                                                         Entity->AbilityTargetSerial);
                if (Ally)
                {
                    v2 AllySpot = Ally->Position.XY - CameraOffset.XY;
                    DrawDottedLine(RenderContext, Self,
                                   Self + Progress * (AllySpot - Self),
                                   TELEGRAPH_COLOR_MEND, 3.f, 6.f);
                    DrawDottedCircle(RenderContext, AllySpot,
                                     0.5f * Ally->Dimensions.X * (2.f - Progress),
                                     TELEGRAPH_COLOR_MEND, 2.f);
                }
            } break;

            case MonsterAbility_Volley:
            {
                // NOTE(zoubir): one lane per shot, as long as it can fly
                v2 Directions[MAX_VOLLEY_SHOTS];
                u32 Count = GetVolleyDirections(Ability, Entity->AbilityAim,
                                                Directions, MAX_VOLLEY_SHOTS);
                float Reach = Ability->Speed * Ability->Active;
                for(u32 ShotIndex = 0; ShotIndex < Count; ShotIndex++)
                {
                    v2 End = Self + Reach * Directions[ShotIndex];
                    DrawDottedLine(RenderContext, Self, End, Color, 2.f, 10.f);
                    DrawDottedLine(RenderContext, Self,
                                   Self + Progress * (End - Self), Color, 3.f, 7.f);
                }
            } break;

            default:
            {
            } break;
        }
    }
}
