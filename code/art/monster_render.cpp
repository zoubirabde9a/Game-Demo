/* Monster rendering that only the client needs: turning each kind's draw
   function into a sprite-sheet texture at startup, and drawing ability
   telegraphs (the danger zones on the ground) over the world. */

#include "monster_fx.cpp"

inline u32
MonsterSheetWidth(monster_def *Def)
{
    return Def->FrameSize * MONSTER_SHEET_COLUMNS;
}

inline u32
MonsterSheetHeight(monster_def *Def)
{
    return Def->FrameSize * MonsterRow_Count;
}

// NOTE(zoubir): Pixels holds MonsterSheetWidth * MonsterSheetHeight u32s
internal void
BuildMonsterSheet(monster_kind Kind, u32 *Pixels)
{
    monster_def *Def = GetMonsterDef(Kind);
    u32 Width = MonsterSheetWidth(Def);
    u32 Height = MonsterSheetHeight(Def);
    ZeroSize(Pixels, Width * Height * sizeof(u32));

    animation_type RowTypes[MonsterRow_Count] =
        {
            AnimationType_Stand,
            AnimationType_Move,
            AnimationType_Cast,
            AnimationType_Attack,
            AnimationType_Stop,
        };
    for(u32 Row = 0; Row < MonsterRow_Count; Row++)
    {
        u32 FrameCount = Def->FrameCounts[Row];
        Assert(FrameCount <= MONSTER_SHEET_COLUMNS);
        for(u32 Frame = 0; Frame < FrameCount; Frame++)
        {
            sprite_canvas Canvas = CanvasFrame(Pixels, Width, Def->FrameSize,
                                               Frame, Row);
            monster_pose Pose = {};
            Pose.Anim = RowTypes[Row];
            Pose.Frame = Frame;
            Pose.FrameCount = FrameCount;
            Pose.t = (float)Frame / (float)FrameCount;
            // NOTE(zoubir): one-shot rows (windup, attack) should reach
            // their last pose on the last frame instead of looping back
            if (Row == MonsterRow_Windup || Row == MonsterRow_Attack)
            {
                Pose.t = FrameCount > 1 ?
                    (float)Frame / (float)(FrameCount - 1) : 1.f;
            }
            Pose.Wave = Sin(2.f * Pi32 * Pose.t);
            Pose.Wave2 = Cos(2.f * Pi32 * Pose.t);
            MonsterDrawFunctions[Kind](&Canvas, Pose);
        }
    }
}

// NOTE(zoubir): call once after InitializeAssets
internal void
AddMonsterTextures(assets *Assets, open_gl *OpenGL, memory_arena *TempArena)
{
    ReserveGeneratedAssets(Assets, AssetType_Monster, MonsterKind_Count);
    for(u32 KindIndex = 0; KindIndex < MonsterKind_Count; KindIndex++)
    {
        monster_def *Def = GetMonsterDef((monster_kind)KindIndex);
        u32 Width = MonsterSheetWidth(Def);
        u32 Height = MonsterSheetHeight(Def);
        temporary_memory Temp = BeginTemporaryMemory(TempArena);
        u32 *Pixels = AllocateArray(TempArena, Width * Height, u32);
        BuildMonsterSheet((monster_kind)KindIndex, Pixels);
        // NOTE(zoubir): feet sit on the 7/8 line like the player sprite
        AddGeneratedTexture(Assets, OpenGL, {AssetType_Monster, KindIndex},
                            Pixels, Width, Height,
                            MONSTER_SHEET_COLUMNS, MonsterRow_Count,
                            V2(0.5f, 0.875f));
        EndTemporaryMemory(Temp);
    }

    ReserveGeneratedAssets(Assets, AssetType_MonsterShot, ShotStyle_Count);
    for(u32 Style = 0; Style < ShotStyle_Count; Style++)
    {
        u32 Width = SHOT_FRAME_SIZE * SHOT_FRAMES;
        temporary_memory Temp = BeginTemporaryMemory(TempArena);
        u32 *Pixels = AllocateArray(TempArena, Width * SHOT_FRAME_SIZE, u32);
        BuildShotSheet((monster_shot_style)Style, Pixels);
        AddGeneratedTexture(Assets, OpenGL, {AssetType_MonsterShot, Style},
                            Pixels, Width, SHOT_FRAME_SIZE,
                            SHOT_FRAMES, 1, V2(0.5f, 0.5f));
        EndTemporaryMemory(Temp);
    }
}

#define TELEGRAPH_DOTS 28
#define TELEGRAPH_COLOR_DANGER 0xFF3040FF
#define TELEGRAPH_COLOR_HOT 0xFF60D0FF
#define TELEGRAPH_COLOR_SPIRIT 0xFFF0E070

// NOTE(zoubir): a ring of small squares; the renderer has no circles
internal void
DrawDottedCircle(render_context *RenderContext, v2 Center, float Radius,
                 u32 Color, float DotSize)
{
    for(u32 Dot = 0; Dot < TELEGRAPH_DOTS; Dot++)
    {
        float Angle = 2.f * Pi32 * (float)Dot / (float)TELEGRAPH_DOTS;
        // NOTE(zoubir): squashed vertically to sit on the ground plane
        v2 P = Center + V2(Radius * Cos(Angle), 0.6f * Radius * Sin(Angle));
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

// NOTE(zoubir): the danger zone of every monster in windup. The ring
// closes in from twice its size as the windup runs out, so the moment it
// matches the real radius is the moment it hits
internal void
DrawMonsterTelegraphs(render_context *RenderContext, world *World,
                      v3 CameraOffset)
{
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
