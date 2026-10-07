/* Cursor targeting: which unit the mouse is on, for abilities thrown at a
   unit rather than at the ground (the kunai, sim/player_abilities/kunai.cpp).

   Picking works on what the screen shows, not on the units' feet: each
   enemy (a player or a monster, not the local player) has a body box
   built from its sprite as draw_entities.cpp places it (the texture's
   origin at the feet, lifted by its height in the air), shrunk to the
   body inside the frame. In order:
   1. the unit picked last frame stays picked while the cursor is on its
      body, so two overlapping units do not flicker;
   2. else the front-most body under the cursor (the lowest feet on
      screen, which is drawn on top);
   3. else the unit picked last frame while the cursor is within
      TARGET_KEEP_PIXELS of its body;
   4. else the nearest body within TARGET_SNAP_PIXELS of the cursor, so a
      near miss on a small or fast unit still counts;
   5. else nothing.
   The radii are window pixels, so they feel the same at any zoom.

   The pick goes into player_input.Target as the local entity index + 1
   (keyboard_input.cpp); online, client/online.cpp turns it into the
   server's Id, so the server throws at the very unit the player saw
   marked. The server checks it again (KunaiTargetFor).

   Drawn: corner brackets on the picked unit that close in when it is
   picked, pale when the kunai is ready and in range, dim while it cools
   down, red when the unit is out of range; and a short note by the
   cursor ("No target", "Out of range") when the kunai key is pressed and
   cannot throw. */

#define TARGET_SNAP_PIXELS 22.f
#define TARGET_KEEP_PIXELS 40.f
// NOTE(zoubir): share of the sprite frame the body fills, across and above
// the feet
#define TARGET_BODY_WIDTH 0.6f
#define TARGET_BODY_HEIGHT 0.85f
// NOTE(zoubir): the brackets close in from this much wider over this long
#define TARGET_MARK_GROW 10.f
#define TARGET_MARK_SECONDS 0.12f
#define TARGET_MARK_ARM 7.f
#define TARGET_NOTICE_SECONDS 0.9f

struct cursor_targeting
{
    // NOTE(zoubir): the picked unit, its local entity index + 1, 0 for none
    u32 Picked;
    // NOTE(zoubir): seconds since it was picked, for the brackets closing in
    float PickedFor;
    char *Notice;
    float NoticeLeft;
    // NOTE(zoubir): the cursor in window pixels when the note came up
    v2 NoticeAt;
};

struct target_box
{
    v2 Min;
    v2 Max;
};

inline cursor_targeting *
GetCursorTargeting(app_state *AppState)
{
    if (!AppState->CursorTargeting)
    {
        AppState->CursorTargeting = AllocateStruct(&AppState->MemoryArena, cursor_targeting);
        *AppState->CursorTargeting = {};
    }
    return AppState->CursorTargeting;
}

// NOTE(zoubir): the unit's body on screen, in world units (Y down, the
// height in the air taken off Y as the sprite is drawn)
internal target_box
TargetBodyBox(app_state *AppState, world_entity *Unit)
{
    // NOTE(zoubir): the art's feet point; the default when no art is
    // loaded (the tests' worlds)
    v2 Origin = V2(0.5f, 0.85f);
    if (Unit->Texture.Type && AppState->Assets.Infos)
    {
        Origin = GetAssetInfo(&AppState->Assets, Unit->Texture)->Texture.Origin;
    }
    v2 Feet = V2(Unit->Position.X, Unit->Position.Y - Unit->Position.Z);
    float HalfWidth = 0.5f * TARGET_BODY_WIDTH * Unit->Dimensions.X;
    float Above = TARGET_BODY_HEIGHT * Origin.Y * Unit->Dimensions.Y;
    target_box Result;
    Result.Min = V2(Feet.X - HalfWidth, Feet.Y - Above);
    Result.Max = V2(Feet.X + HalfWidth, Feet.Y + 2.f);
    return Result;
}

// NOTE(zoubir): 0 inside the box, else how far outside
inline float
DistanceToBox(target_box Box, v2 P)
{
    float DX = Maximum(0.f, Maximum(Box.Min.X - P.X, P.X - Box.Max.X));
    float DY = Maximum(0.f, Maximum(Box.Min.Y - P.Y, P.Y - Box.Max.Y));
    float Result = SquareRoot(DX * DX + DY * DY);
    return Result;
}

// NOTE(zoubir): the unit at local index + 1 Picked, if it is still one
// the cursor may pick; 0 otherwise
inline world_entity *
PickedUnit(app_state *AppState, world_entity *Local, u32 Picked)
{
    world *World = &AppState->World;
    world_entity *Result = 0;
    if (Picked > 0 && Picked <= World->EntityCount &&
        IsKunaiTarget(&World->Entities[Picked - 1], Local))
    {
        Result = &World->Entities[Picked - 1];
    }
    return Result;
}

// NOTE(zoubir): the pick for this frame (the rules at the top)
internal u32
PickCursorTarget(app_state *AppState, world_entity *Local, v2 Cursor, u32 Previous)
{
    world *World = &AppState->World;
    float Zoom = AppState->WorldZoom > 0.f ? AppState->WorldZoom : 1.f;
    world_entity *Kept = PickedUnit(AppState, Local, Previous);
    float KeptMiss = Kept ? DistanceToBox(TargetBodyBox(AppState, Kept), Cursor) : 0.f;
    if (Kept && KeptMiss == 0.f)
    {
        return Previous;
    }

    u32 Under = 0;
    float UnderFeet = 0.f;
    u32 Near = 0;
    float NearMiss = TARGET_SNAP_PIXELS / Zoom;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Unit = &World->Entities[Index];
        if (!IsKunaiTarget(Unit, Local))
        {
            continue;
        }
        target_box Box = TargetBodyBox(AppState, Unit);
        float Miss = DistanceToBox(Box, Cursor);
        if (Miss == 0.f)
        {
            if (!Under || Box.Max.Y > UnderFeet)
            {
                Under = Index + 1;
                UnderFeet = Box.Max.Y;
            }
        }
        else if (Miss <= NearMiss)
        {
            Near = Index + 1;
            NearMiss = Miss;
        }
    }
    u32 Result = Near;
    if (Under)
    {
        Result = Under;
    }
    else if (Kept && KeptMiss <= TARGET_KEEP_PIXELS / Zoom)
    {
        Result = Previous;
    }
    return Result;
}

// NOTE(zoubir): why the kunai would not go now, 0 when it would or when
// the key is not the player's or not ready (the cooldown bar says so)
internal char *
KunaiRefusal(app_state *AppState, world_entity *Local, world_entity *Target)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    char *Result = 0;
    if (AbilityLevel(Slot, PlayerButton_Kunai) &&
        Local->ActionCooldowns[PlayerAction_Kunai] <= 0.f)
    {
        if (!Target)
        {
            Result = (char *)"No target";
        }
        else if (Length(Target->Position.XY - Local->Position.XY) > PlayerStats.KunaiRange)
        {
            Result = (char *)"Out of range";
        }
    }
    return Result;
}

// NOTE(zoubir): once a frame from ReadKeyboardPlayerInput: picks the
// unit and returns player_input.Target (local entity index + 1, 0 none)
internal u32
UpdateCursorTarget(app_input *Input, app_state *AppState)
{
    cursor_targeting *Targeting = GetCursorTargeting(AppState);
    Targeting->NoticeLeft = Maximum(0.f, Targeting->NoticeLeft - Input->DeltaTime);
    world_entity *Local = GetLocalPlayer(AppState);
    if (!Local || !Local->IsPresent || IsDeadPlayer(Local))
    {
        Targeting->Picked = 0;
        return 0;
    }
    u32 Picked = PickCursorTarget(AppState, Local, CursorInWorld(Input, AppState),
                                  Targeting->Picked);
    Targeting->PickedFor = (Picked == Targeting->Picked) ?
        Targeting->PickedFor + Input->DeltaTime : 0.f;
    Targeting->Picked = Picked;

    if (ActionButtonsFromKeys(Input, true) & PlayerButton_Kunai)
    {
        char *Refusal = KunaiRefusal(AppState, Local, PickedUnit(AppState, Local, Picked));
        if (Refusal)
        {
            Targeting->Notice = Refusal;
            Targeting->NoticeLeft = TARGET_NOTICE_SECONDS;
            Targeting->NoticeAt = V2((float)Input->MouseX, (float)Input->MouseY);
        }
    }
    return Picked;
}

// NOTE(zoubir): one corner of the brackets: two arms from Tip, outlined
inline void
DrawTargetCorner(render_context *RenderContext, v2 Tip, float SX, float SY,
                 u32 Color, u32 Rim)
{
    v2 Across = V2(-SX * TARGET_MARK_ARM, 0.f);
    v2 Down = V2(0.f, -SY * TARGET_MARK_ARM);
    for(u32 Pass = 0; Pass < 2; Pass++)
    {
        float W = Pass ? 1.f : 2.f;
        u32 C = Pass ? Color : Rim;
        DrawFilledQuad(RenderContext, Tip + V2(0.f, -W * SY), Tip + Across + V2(0.f, -W * SY),
                       Tip + Across + V2(0.f, W * SY), Tip + V2(0.f, W * SY), C, C, C, C);
        DrawFilledQuad(RenderContext, Tip + V2(-W * SX, 0.f), Tip + Down + V2(-W * SX, 0.f),
                       Tip + Down + V2(W * SX, 0.f), Tip + V2(W * SX, 0.f), C, C, C, C);
    }
}

// NOTE(zoubir): the brackets, over the world (screen_pass.inc)
internal void
DrawCursorTarget(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    cursor_targeting *Targeting = GetCursorTargeting(AppState);
    world_entity *Local = GetLocalPlayer(AppState);
    world_entity *Target = Local ? PickedUnit(AppState, Local, Targeting->Picked) : 0;
    if (!Target)
    {
        return;
    }
    // NOTE(zoubir): 0xAABBGGRR
    u32 Color = 0xF0FFF0D8;
    if (Length(Target->Position.XY - Local->Position.XY) > PlayerStats.KunaiRange)
    {
        Color = 0xF04040F0;
    }
    else if (Local->ActionCooldowns[PlayerAction_Kunai] > 0.f)
    {
        Color = 0x90B0A8A0;
    }
    u32 Rim = 0xC0201810;
    float Grow = TARGET_MARK_GROW * (1.f - Clamp01(Targeting->PickedFor / TARGET_MARK_SECONDS));
    target_box Box = TargetBodyBox(AppState, Target);
    v2 Min = Box.Min - CameraOffset.XY - V2(Grow + 3.f, Grow + 3.f);
    v2 Max = Box.Max - CameraOffset.XY + V2(Grow + 3.f, Grow + 3.f);
    DrawTargetCorner(RenderContext, V2(Min.X, Min.Y), -1.f, -1.f, Color, Rim);
    DrawTargetCorner(RenderContext, V2(Max.X, Min.Y), 1.f, -1.f, Color, Rim);
    DrawTargetCorner(RenderContext, V2(Min.X, Max.Y), -1.f, 1.f, Color, Rim);
    DrawTargetCorner(RenderContext, V2(Max.X, Max.Y), 1.f, 1.f, Color, Rim);
}

// NOTE(zoubir): the note by the cursor, in window pixels (screen_pass.inc)
internal void
DrawTargetNotice(render_context *RenderContext, app_state *AppState)
{
    cursor_targeting *Targeting = GetCursorTargeting(AppState);
    if (Targeting->NoticeLeft <= 0.f || !Targeting->Notice)
    {
        return;
    }
    float Fade = Clamp01(Targeting->NoticeLeft / (0.4f * TARGET_NOTICE_SECONDS));
    float Rise = 12.f * (1.f - Targeting->NoticeLeft / TARGET_NOTICE_SECONDS);
    UIText(RenderContext, AppState->Fonts.Small, Targeting->NoticeAt.X,
           Targeting->NoticeAt.Y - 30.f - Rise, Targeting->Notice,
           WithAlpha(UI_RGBA(255, 140, 120, 255), Fade), UIAlign_Center);
}
