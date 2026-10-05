/* Cast bars: a bar over a body that fills as its wind-up runs, with the
   spell's name above it, blinking white in the last CAST_BAR_HOT of the
   wind-up. Monsters draw theirs with their motes and release ring
   (monster_cast_tells.cpp); players get the bar alone, for every spell
   with a cast time (sim/player_casts.cpp).

   Read only from CastSpell and CastLeft, which snapshots carry
   (client/replicas/apply.cpp) and prediction steps for the local player,
   so it looks the same online and offline.

   Entry point: DrawPlayerCastBars, from DrawPlayerAbilityFx. */

// NOTE(zoubir): share of the wind-up, at its end, where the bar blinks
#define CAST_BAR_HOT 0.25f
#define CAST_BAR_HEIGHT 5.f
#define CAST_BAR_MIN_WIDTH 40.f
// NOTE(zoubir): sprites keep their feet on the 7/8 line
#define CAST_BAR_SPRITE_TOP 0.875f

// NOTE(zoubir): the name over the head, with a dark copy one pixel down
// so it reads on grass and on sand
internal void
DrawCastName(render_context *RenderContext, font *Font, v2 At,
             char *Name, u32 Color, u32 Alpha)
{
    UIText(RenderContext, Font, At.X, At.Y + 1.f, Name,
           UI_RGBA(0, 0, 0, Alpha), UIAlign_Center);
    UIText(RenderContext, Font, At.X, At.Y, Name, Color, UIAlign_Center);
}

// NOTE(zoubir): where a body's cast bar sits: its top edge, above the head
inline float
CastBarTop(world_entity *Entity, v2 Feet)
{
    float Result = Feet.Y - Entity->Dimensions.Y * CAST_BAR_SPRITE_TOP - 18.f;
    return Result;
}

inline bool32
IsCastBarBlinking(float Progress, float Clock)
{
    bool32 Result = Progress > 1.f - CAST_BAR_HOT && ((u32)(Clock * 16.f) & 1);
    return Result;
}

internal void
DrawCastBar(render_context *RenderContext, app_state *AppState,
            world_entity *Entity, v2 Feet, float Progress, float Clock,
            char *Name, u32 Color)
{
    u32 BarColor = IsCastBarBlinking(Progress, Clock) ?
        UI_RGBA(255, 255, 255, 255) : Color;
    float Width = Maximum(CAST_BAR_MIN_WIDTH, 0.7f * Entity->Dimensions.X);
    float Top = CastBarTop(Entity, Feet);
    float Left = Feet.X - 0.5f * Width;
    DrawFilledRectangle(RenderContext, Left - 1.f, Top - 1.f, Width + 2.f,
                        CAST_BAR_HEIGHT + 2.f, UI_RGBA(0, 0, 0, 200), 0.f);
    DrawFilledRectangle(RenderContext, Left, Top, Width * Progress,
                        CAST_BAR_HEIGHT, BarColor, 0.f);

    font *Font = AppState->Fonts.Small;
    DrawCastName(RenderContext, Font, V2(Feet.X, Top - UILineHeight(Font) - 1.f),
                 Name, Color, 255);
}

internal void
DrawPlayerCastBars(render_context *RenderContext, app_state *AppState,
                   v3 CameraOffset)
{
    world *World = &AppState->World;
    float Clock = GetFxClock(AppState);
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (!Entity->IsPresent || Entity->Type != EntityType_Player ||
            !IsPlayerCasting(Entity) || Entity->CastSpell >= PlayerSpell_Count)
        {
            continue;
        }
        v2 Feet = Entity->Position.XY - CameraOffset.XY - V2(0.f, Entity->Position.Z);
        DrawCastBar(RenderContext, AppState, Entity, Feet,
                    PlayerCastProgress(Entity), Clock,
                    PlayerSpells[Entity->CastSpell].Name,
                    UI_RGBA(120, 200, 255, 255));
    }
}
