/* Rewind overlays, drawn over the (warped) world each frame: the clock
   sigils, the ghosts of each frozen thing's way back, the afterimages of
   the playback and the flashes of the landing (DrawRewindFx); and the
   tape counter at the top of the screen while time runs backwards
   (DrawRewindHud). */

// NOTE(zoubir): how many ghosts mark a frozen thing's way back, and the
// most frozen things that get them (the nearest to the camera's middle
// first is not worth sorting for: a world rewind marks the first ones)
#define REWIND_GHOSTS 8
#define REWIND_MAX_GHOSTED 48
#define REWIND_AFTERIMAGES 5

inline u32
RewindRGBA(float R, float G, float B, float A)
{
    u32 Result = ((u32)(255.f * RewindClamp01(A)) << 24) |
        ((u32)(255.f * RewindClamp01(B)) << 16) |
        ((u32)(255.f * RewindClamp01(G)) << 8) |
        (u32)(255.f * RewindClamp01(R));
    return Result;
}

// NOTE(zoubir): the sprite a trail sample remembers, where it was, tinted
internal void
DrawRewindGhost(render_context *RenderContext, app_state *AppState,
                rewind_fx_sample *Sample, v3 CameraOffset, u32 Color,
                u32 Blend, v2 Nudge = {})
{
    assets *Assets = &AppState->Assets;
    if (!Sample->Texture.Type)
    {
        return;
    }
    loaded_texture *Texture = GetTexture(Assets, AppState->OpenGL, AppState,
                                         Sample->Texture);
    if (!Texture)
    {
        return;
    }
    zas_texture_info *Info = &GetAssetInfo(Assets, Sample->Texture)->Texture;
    float DrawZ = Maximum(Sample->Position.Z,
                          TerrainHeightAt(&AppState->World, Sample->Position.X,
                                          Sample->Position.Y));
    v2 P = Sample->Position.XY - CameraOffset.XY - Info->Origin * Sample->Dimensions + Nudge;
    P.Y -= DrawZ;
    DrawTexturedQuad(RenderContext, Texture->ID, P.X, P.Y, Sample->Dimensions.X,
                     Sample->Dimensions.Y, Sample->Uvs, Color, Blend);
}

inline v2
RewindScreenXY(v3 Position, v3 CameraOffset)
{
    v2 Result = Position.XY - CameraOffset.XY;
    return Result;
}

internal void
DrawRewindSigil(render_context *RenderContext, v2 Centre, float Size,
                float Bright, float HandTurn, float Mode, float Strength)
{
    float Turn = HandTurn - floorf(HandTurn);
    u32 Color = RewindRGBA(Bright, Turn, Mode, Strength);
    DrawShaderQuad(RenderContext, Shader_TimeSigil, Centre.X - 0.5f * Size,
                   Centre.Y - 0.5f * Size, Size, Size, Color, RenderBlend_Additive);
}

internal void
DrawRewindGlow(render_context *RenderContext, v2 Centre, float Size, u32 Color)
{
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.5f * Size,
                   Centre.Y - 0.5f * Size, Size, Size, Color, RenderBlend_Additive);
}

internal void
DrawRewindRing(render_context *RenderContext, v2 Centre, float Size, u32 Color)
{
    DrawShaderQuad(RenderContext, Shader_Ring, Centre.X - 0.5f * Size,
                   Centre.Y - 0.5f * Size, Size, Size, Color, RenderBlend_Additive);
}

// NOTE(zoubir): the hands turn backwards, slowly while the cast gathers,
// stop dead in the hold, and spin in the playback
internal float
RewindHandTurn(rewind_fx_cast *Cast, float Clock)
{
    float Since = Clock - Cast->PhaseStart;
    float Result = 0.f;
    switch(Cast->Phase)
    {
        case RewindPhase_Cast:
        {
            Result = -(0.3f * Since + 1.6f * Since * Since);
        } break;
        case RewindPhase_Hold:
        {
            Result = -0.6f;
        } break;
        case RewindPhase_Playback:
        {
            Result = -0.6f - 6.f * Since;
        } break;
        default:
        {
        } break;
    }
    return Result;
}

// NOTE(zoubir): the way back of one frozen thing during the hold: a dotted
// path through where it was, ghosts along it fading in one after another,
// and a bright ghost with a ring where it will land
internal void
DrawRewindPath(render_context *RenderContext, app_state *AppState,
               rewind_fx_track *Track, float From, float Show, v3 CameraOffset,
               float Clock)
{
    rewind_fx_sample Sample;
    u32 Dots = 30;
    for(u32 Dot = 1; Dot < Dots; Dot++)
    {
        float Along = (float)Dot / (float)Dots;
        if (Along > Show)
        {
            break;
        }
        if (SampleRewindTrack(Track, From - REWIND_SECONDS * Along, &Sample))
        {
            v2 P = RewindScreenXY(Sample.Position, CameraOffset) - V2(0.f, 10.f);
            float Twinkle = 0.6f + 0.4f * Sin(14.f * Clock - 9.f * Along);
            DrawRewindGlow(RenderContext, P, 10.f, RewindRGBA(0.4f, 0.9f, 1.f, 0.55f * Twinkle));
        }
    }
    for(u32 Ghost = 1; Ghost <= REWIND_GHOSTS; Ghost++)
    {
        float Along = (float)Ghost / (float)REWIND_GHOSTS;
        float Appear = RewindClamp01((Show - Along + 0.25f) * 4.f);
        if (Appear <= 0.f ||
            !SampleRewindTrack(Track, From - REWIND_SECONDS * Along, &Sample))
        {
            continue;
        }
        bool32 Landing = (Ghost == REWIND_GHOSTS);
        // NOTE(zoubir): light, not shadow: added, so they glow over the
        // drained colours of the hold
        u32 Color = Landing ?
            RewindRGBA(1.f, 0.95f, 0.75f, 0.9f * Appear) :
            RewindRGBA(0.45f, 0.85f, 1.f, (0.3f + 0.35f * Along) * Appear);
        DrawRewindGhost(RenderContext, AppState, &Sample, CameraOffset, Color,
                        RenderBlend_Additive);
        if (Landing)
        {
            v2 Feet = RewindScreenXY(Sample.Position, CameraOffset);
            float Pulse = 0.5f + 0.5f * Sin(10.f * Clock);
            DrawRewindRing(RenderContext, Feet, 34.f + 6.f * Pulse,
                           RewindRGBA(1.f, 0.85f, 0.5f, 0.8f * Appear));
            DrawRewindGlow(RenderContext, Feet - V2(0.f, 14.f), 70.f,
                           RewindRGBA(1.f, 0.8f, 0.45f, 0.35f * Appear));
        }
    }
}

// NOTE(zoubir): during the playback, where it just was: afterimages split
// magenta and cyan, the oldest faintest
internal void
DrawRewindAfterimages(render_context *RenderContext, app_state *AppState,
                      rewind_fx_track *Track, float Cursor, float Floor,
                      v3 CameraOffset)
{
    rewind_fx_sample Sample;
    for(u32 Image = 1; Image <= REWIND_AFTERIMAGES; Image++)
    {
        float Behind = 0.045f * REWIND_PLAYBACK_SPEED * (float)Image;
        float Time = Cursor + Behind;
        if (Time > Floor || !SampleRewindTrack(Track, Time, &Sample))
        {
            continue;
        }
        float Fade = 1.f - (float)Image / (float)(REWIND_AFTERIMAGES + 1);
        bool32 Odd = Image & 1;
        u32 Color = Odd ? RewindRGBA(1.f, 0.3f, 0.9f, 0.45f * Fade) :
            RewindRGBA(0.3f, 1.f, 1.f, 0.45f * Fade);
        DrawRewindGhost(RenderContext, AppState, &Sample, CameraOffset, Color,
                        RenderBlend_Additive, V2(Odd ? 2.f : -2.f, 0.f));
    }
}

// NOTE(zoubir): a frozen body glints, cold and still
internal void
DrawRewindFrost(render_context *RenderContext, app_state *AppState,
                rewind_fx_track *Track, v3 CameraOffset, float Clock, u32 EntityIndex)
{
    rewind_fx_sample Sample;
    if (!SampleRewindTrack(Track, Clock, &Sample))
    {
        return;
    }
    float Glint = 0.25f + 0.2f * Sin(7.f * Clock + 1.7f * (float)EntityIndex);
    DrawRewindGhost(RenderContext, AppState, &Sample, CameraOffset,
                    RewindRGBA(0.55f, 0.85f, 1.f, Glint), RenderBlend_Additive);
}

// NOTE(zoubir): the motes a cast pulls in, spiralling toward the caster
internal void
DrawRewindMotes(render_context *RenderContext, v2 Centre, float Reach,
                float Progress, float Clock)
{
    u32 Motes = 14;
    for(u32 Mote = 0; Mote < Motes; Mote++)
    {
        float Phase = (float)Mote / (float)Motes;
        float Life = Clock * (1.2f + 1.6f * Progress) + Phase;
        Life -= floorf(Life);
        float Angle = 2.f * Pi32 * Phase + 4.f * Life - 2.f * Clock;
        float Distance = Reach * (1.f - Life);
        v2 P = Centre + Distance * V2(Cos(Angle), 0.6f * Sin(Angle)) - V2(0.f, 14.f);
        float Alpha = (0.3f + 0.7f * Progress) * Sin(Pi32 * Life);
        DrawRewindGlow(RenderContext, P, 9.f + 8.f * Life,
                       RewindRGBA(0.5f, 0.92f, 1.f, Alpha));
    }
}

internal void
DrawRewindCastFx(render_context *RenderContext, app_state *AppState, rewind_fx *Fx,
                 u32 Slot, rewind_fx_cast *Cast, v3 CameraOffset)
{
    float Clock = Fx->Clock;
    float Progress = RewindPhaseProgress(Cast);
    float Hands = RewindHandTurn(Cast, Clock);
    player_slot *Caster = &AppState->Players[Slot];
    v3 CasterAt = (Caster->Active && Caster->Entity) ? Caster->Entity->Position : Cast->Centre;
    v2 Centre = RewindScreenXY(Cast->Phase == RewindPhase_Cast ? CasterAt : Cast->Centre,
                               CameraOffset);
    float BubbleSize = 2.f * Cast->Radius;

    switch(Cast->Phase)
    {
        case RewindPhase_Cast:
        {
            float Ease = Progress * Progress * (3.f - 2.f * Progress);
            float Size = Cast->Kind == RewindKind_Bubble ? BubbleSize * (0.35f + 0.65f * Ease) :
                Cast->Kind == RewindKind_World ? 150.f + 60.f * Ease : 80.f + 30.f * Ease;
            DrawRewindSigil(RenderContext, Centre, Size, Progress, Hands, 0.f,
                            0.35f + 0.65f * Ease);
            DrawRewindMotes(RenderContext, Centre, 0.6f * Size + 40.f, Progress, Clock);
            if (Cast->Kind == RewindKind_Bubble)
            {
                // NOTE(zoubir): where the bubble will close, so it can be
                // run from
                DrawRewindRing(RenderContext, Centre, BubbleSize,
                               RewindRGBA(0.4f, 0.9f, 1.f, 0.25f + 0.5f * Ease));
            }
            else if (Cast->Kind == RewindKind_World)
            {
                for(u32 Wave = 0; Wave < 3; Wave++)
                {
                    float Out = Clock * 0.9f + (float)Wave / 3.f;
                    Out -= floorf(Out);
                    DrawRewindRing(RenderContext, Centre, 120.f + 900.f * Out,
                                   RewindRGBA(0.45f, 0.9f, 1.f, 0.4f * (1.f - Out) * Ease));
                }
            }
            else
            {
                // NOTE(zoubir): a self rewind shows the caster's way back
                // as it gathers
                world_entity *Entity = Caster->Entity;
                if (Caster->Active && Entity)
                {
                    u32 Index = (u32)(Entity - AppState->World.Entities);
                    DrawRewindPath(RenderContext, AppState, GetRewindTrack(&Fx->Trails, Index),
                                   Clock, 0.5f * Ease, CameraOffset, Clock);
                }
            }
        } break;

        case RewindPhase_Hold:
        case RewindPhase_Playback:
        {
            bool32 Playback = Cast->Phase == RewindPhase_Playback;
            float Size = Cast->Kind == RewindKind_Bubble ? BubbleSize :
                Cast->Kind == RewindKind_World ? 210.f : 110.f;
            // NOTE(zoubir): over a whole bubble it is fainter, so what is
            // frozen inside still reads
            float Strength = Cast->Kind == RewindKind_Bubble ? 0.5f : 1.f;
            DrawRewindSigil(RenderContext, Centre, Size, Playback ? 1.f : 0.6f, Hands,
                            Playback ? 1.f : 0.f, (Playback ? 0.85f : 1.f) * Strength);
            float Cursor = RewindFxCursor(Cast, Clock);
            world *World = &AppState->World;
            u32 Ghosted = 0;
            for(u32 EntityIndex = 0;
                EntityIndex < World->EntityCount && Ghosted < REWIND_MAX_GHOSTED;
                EntityIndex++)
            {
                world_entity *Entity = &World->Entities[EntityIndex];
                if (!IsTrailedEntity(Entity) || !IsFrozenByRewind(Cast, EntityIndex))
                {
                    continue;
                }
                rewind_fx_track *Track = GetRewindTrack(&Fx->Trails, EntityIndex);
                if (!Track)
                {
                    continue;
                }
                Ghosted++;
                if (Playback)
                {
                    DrawRewindAfterimages(RenderContext, AppState, Track, Cursor,
                                          Cast->HoldStart, CameraOffset);
                }
                else
                {
                    DrawRewindFrost(RenderContext, AppState, Track, CameraOffset, Clock,
                                    EntityIndex);
                    DrawRewindPath(RenderContext, AppState, Track, Cast->HoldStart,
                                   0.25f + Progress, CameraOffset, Clock);
                }
            }
        } break;

        default:
        {
        } break;
    }
}

// NOTE(zoubir): after a playback lands: a ring and a flash on everything
// it brought back, and on the sigil's spot
internal void
DrawRewindLanding(render_context *RenderContext, app_state *AppState, rewind_fx *Fx,
                  rewind_fx_cast *Cast, v3 CameraOffset)
{
    float Age = RewindClamp01((Fx->Clock - Cast->EndedAt) / REWIND_FX_AFTERGLOW);
    float Fade = 1.f - Age;
    world *World = &AppState->World;
    u32 Count = Cast->Kind == RewindKind_World ? World->EntityCount : Cast->FrozenCount;
    u32 Flashed = 0;
    for(u32 Index = 0; Index < Count && Flashed < REWIND_MAX_GHOSTED; Index++)
    {
        u32 EntityIndex = Cast->Kind == RewindKind_World ? Index : Cast->Frozen[Index];
        if (EntityIndex >= World->EntityCount)
        {
            continue;
        }
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!IsTrailedEntity(Entity))
        {
            continue;
        }
        Flashed++;
        v2 Feet = RewindScreenXY(Entity->Position, CameraOffset);
        DrawRewindRing(RenderContext, Feet, 20.f + 70.f * Age,
                       RewindRGBA(1.f, 0.9f, 0.6f, 0.9f * Fade));
        DrawRewindGlow(RenderContext, Feet - V2(0.f, 14.f), 90.f * Fade + 10.f,
                       RewindRGBA(1.f, 0.95f, 0.8f, 0.6f * Fade * Fade));
    }
    if (Cast->Kind == RewindKind_Bubble)
    {
        DrawRewindRing(RenderContext, RewindScreenXY(Cast->Centre, CameraOffset),
                       2.f * Cast->Radius * (1.f + 0.3f * Age),
                       RewindRGBA(1.f, 0.85f, 0.55f, 0.7f * Fade));
    }
}

// NOTE(zoubir): client/screen_pass.inc, with the world's zoom
internal void
DrawRewindFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    rewind_fx *Fx = AppState->RewindFx;
    if (!Fx)
    {
        return;
    }
    for(u32 Slot = 0; Slot < MAX_PLAYERS; Slot++)
    {
        rewind_fx_cast *Cast = &Fx->Casts[Slot];
        if (Cast->Active)
        {
            DrawRewindCastFx(RenderContext, AppState, Fx, Slot, Cast, CameraOffset);
        }
        else if (IsInRewindAfterglow(Cast, Fx->Clock))
        {
            DrawRewindLanding(RenderContext, AppState, Fx, Cast, CameraOffset);
        }
    }
}

// NOTE(zoubir): client/screen_pass.inc, in window pixels: while a world
// rewind, or the local player's own, holds or plays back, a tape counter
// in the top middle: a blinking pause or rewind mark and the seconds of
// the rewind still to run
internal void
DrawRewindHud(render_context *RenderContext, app_state *AppState, i32 WindowWidth)
{
    rewind_fx *Fx = AppState->RewindFx;
    if (!Fx)
    {
        return;
    }
    for(u32 Slot = 0; Slot < MAX_PLAYERS; Slot++)
    {
        rewind_fx_cast *Cast = &Fx->Casts[Slot];
        bool32 Mine = Slot == AppState->LocalPlayerIndex ||
            IsLocalPlayerTimeLocked(AppState);
        if (!Cast->Active || Cast->Phase < RewindPhase_Hold ||
            (Cast->Kind != RewindKind_World && !Mine))
        {
            continue;
        }
        bool32 Playback = Cast->Phase == RewindPhase_Playback;
        float Back = Playback ?
            Minimum(REWIND_SECONDS, REWIND_PLAYBACK_SPEED * (Fx->Clock - Cast->PhaseStart)) : 0.f;
        float Left = REWIND_SECONDS - Back;
        char Text[48];
        u32 Hundredths = (u32)(Left * 100.f + 0.5f);
        snprintf(Text, sizeof(Text), "%s  -00:%02u.%02u", Playback ? "REW x4" : "PAUSE",
                 Hundredths / 100, Hundredths % 100);
        font *Font = AppState->Fonts.Title ? AppState->Fonts.Title : AppState->Fonts.Body;
        float X = 0.5f * (float)WindowWidth;
        float Y = 24.f;
        bool32 Blink = ((u32)(Fx->Clock * 4.f) & 1) != 0;
        u32 Color = Playback ? UI_RGBA(255, 120, 235, 255) : UI_RGBA(150, 225, 255, 255);
        float Width = UITextWidth(Font, Text);
        float MarkX = X - 0.5f * Width - 34.f;
        float MarkY = Y + 0.5f * UILineHeight(Font);
        if (!Playback || Blink)
        {
            if (Playback)
            {
                // NOTE(zoubir): two triangles pointing back, the tape's mark
                for(u32 Arrow = 0; Arrow < 2; Arrow++)
                {
                    float Tip = MarkX + 12.f * (float)Arrow;
                    DrawFilledQuad(RenderContext, V2(Tip, MarkY), V2(Tip + 12.f, MarkY - 9.f),
                                   V2(Tip + 12.f, MarkY + 9.f), V2(Tip, MarkY),
                                   Color, Color, Color, Color);
                }
            }
            else
            {
                DrawFilledRectangle(RenderContext, MarkX + 2.f, MarkY - 9.f, 7.f, 18.f, Color, 0.f);
                DrawFilledRectangle(RenderContext, MarkX + 14.f, MarkY - 9.f, 7.f, 18.f, Color, 0.f);
            }
        }
        UIText(RenderContext, Font, X, Y, Text, Color, UIAlign_Center);
        return;
    }
}
