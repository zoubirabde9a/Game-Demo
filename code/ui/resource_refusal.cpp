/* Resource refusal (ui_module.cpp): when the local player presses a
   spell without the class resource it costs (too little Rage for
   Whirlwind, no combo points for Eviscerate), a large "Not enough Rage"
   plate pops up over the ability bar for about a second, the pressed
   ability's slot shakes red (ability_bar.cpp, and the touch buttons,
   platform/web/touch.cpp) and a short denied buzz plays.

   It reads the two ClassFlags bits every class shares
   (sim/dungeon/role_kits/class_refusal.h), which snapshots carry, so it
   works the same online. A flip of the beat bit is a fresh refusal: the
   plate pops again and the sound plays, at most every
   RESOURCE_REFUSAL_SOUND_GAP; the refused bit held up keeps the plate up,
   so a held or mashed key refreshes one plate instead of stacking more.
   Which slot flashes is the role key the player pressed last, since the
   server only says that a press was refused. */

#define RESOURCE_REFUSAL_SECONDS 1.f
#define RESOURCE_REFUSAL_FADE 0.25f
#define RESOURCE_REFUSAL_SOUND_GAP 0.3f
// NOTE(zoubir): a refusal this long after the last role key press is
// not pinned on that key (the refused press came from elsewhere)
#define RESOURCE_REFUSAL_PRESS_AGE 1.5f
#define RESOURCE_REFUSAL_FLASH_SECONDS 0.6f
// NOTE(zoubir): how far over the ability bar's plate the plate's foot
// sits, clear of the class bars drawn on the plate's edge
#define RESOURCE_REFUSAL_LIFT 70.f

internal float AbilityBarPlateTop(u32 WindowHeight);

struct resource_refusal
{
    bool32 Seen;       // LastFlags holds a frame of this class
    u32 LastRole;
    u32 LastFlags;
    float Show;        // seconds the plate stays up
    float Pop;         // 1 on a fresh refusal, fading to 0
    float SoundGap;    // seconds before the sound may play again
    u32 PressedButton; // the role key the local player pressed last
    float PressedAge;  // seconds since
    u32 FlashButton;   // the slot that flashes red, 0 for none
    float Flash;       // 1 on a fresh refusal, fading to 0
};

internal resource_refusal *
GetResourceRefusal(app_state *AppState)
{
    if (!AppState->ResourceRefusal)
    {
        AppState->ResourceRefusal = AllocateStruct(&AppState->MemoryArena, resource_refusal);
        *AppState->ResourceRefusal = {};
    }
    return AppState->ResourceRefusal;
}

// NOTE(zoubir): what Role's class spends, as the plate names it
internal char *
ClassResourceName(u32 Role)
{
    switch(Role)
    {
        case PlayerRole_Berserker: return (char *)"Rage";
        case PlayerRole_Stormcaller: return (char *)"Charge";
        case PlayerRole_Shadowblade: return (char *)"combo points";
        case PlayerRole_Duelist: return (char *)"Tempo";
        case PlayerRole_Ranger: return (char *)"Focus";
        case PlayerRole_Druid: return (char *)"Bloom";
        case PlayerRole_FrostMage: return (char *)"Icicles";
    }
    return (char *)"power";
}

// NOTE(zoubir): how red Button's slot flashes now, 0..1, for the ability
// bar and the touch buttons
internal float
ResourceRefusalFlash(app_state *AppState, u32 Button)
{
    resource_refusal *Refusal = AppState->ResourceRefusal;
    float Result = (Refusal && Button && Refusal->FlashButton == Button) ? Refusal->Flash : 0.f;
    return Result;
}

// NOTE(zoubir): once a frame: the role key pressed last, and a refusal
// the server just sent
internal void
TrackResourceRefusal(app_state *AppState, resource_refusal *Refusal, float DeltaTime)
{
    Refusal->Show = Maximum(0.f, Refusal->Show - DeltaTime);
    Refusal->Pop = Maximum(0.f, Refusal->Pop - 4.f * DeltaTime);
    Refusal->SoundGap = Maximum(0.f, Refusal->SoundGap - DeltaTime);
    Refusal->Flash = Maximum(0.f, Refusal->Flash - DeltaTime / RESOURCE_REFUSAL_FLASH_SECONDS);
    Refusal->PressedAge += DeltaTime;
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    world_entity *Player = Slot->Entity;
    if (!IsDungeon(AppState) || !Slot->Active || !Player || IsDeadPlayer(Player))
    {
        AppState->FramePresses = 0;
        Refusal->Seen = false;
        Refusal->Show = 0.f;
        return;
    }
    u32 Pressed = AppState->FramePresses;
    AppState->FramePresses = 0;
    for(u32 Bit = 1; Bit && Bit <= Pressed; Bit <<= 1)
    {
        if ((Pressed & Bit) && RoleSpellOnButton(AppState, Player, Bit))
        {
            Refusal->PressedButton = Bit;
            Refusal->PressedAge = 0.f;
        }
    }
    u32 Flags = Slot->ClassFlags;
    bool32 Refused = (Flags & CLASS_FLAG_REFUSED) != 0;
    bool32 Fresh = Refusal->Seen && Refusal->LastRole == Slot->Role &&
        ((Flags ^ Refusal->LastFlags) & CLASS_FLAG_REFUSAL_BEAT) && Refused;
    Refusal->Seen = true;
    Refusal->LastRole = Slot->Role;
    Refusal->LastFlags = Flags;
    if (Fresh)
    {
        Refusal->Show = RESOURCE_REFUSAL_SECONDS;
        Refusal->Pop = 1.f;
        Refusal->Flash = 1.f;
        Refusal->FlashButton = Refusal->PressedAge < RESOURCE_REFUSAL_PRESS_AGE ? Refusal->PressedButton : 0;
        if (Refusal->SoundGap <= 0.f)
        {
            Refusal->SoundGap = RESOURCE_REFUSAL_SOUND_GAP;
            PlayGameSound(AppState, AssetType_SfxDenied);
        }
    }
    else if (Refused && Refusal->Show > 0.f)
    {
        // NOTE(zoubir): the server still says so (a key mashed faster
        // than a fresh refusal may come): the same plate stays up
        Refusal->Show = Maximum(Refusal->Show, RESOURCE_REFUSAL_FADE + 0.1f);
    }
}

// NOTE(zoubir): every frame, over the ability bar and the panels
internal void
DrawResourceRefusal(render_context *RenderContext, app_state *AppState, float DeltaTime,
                    u32 WindowWidth, u32 WindowHeight)
{
    resource_refusal *Refusal = GetResourceRefusal(AppState);
    TrackResourceRefusal(AppState, Refusal, DeltaTime);
    font *Font = AppState->Fonts.Title ? AppState->Fonts.Title : AppState->Fonts.Body;
    if (Refusal->Show <= 0.f || !Font)
    {
        return;
    }
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    char Text[64];
    snprintf(Text, sizeof(Text), "Not enough %s", ClassResourceName(Slot->Role));
    float Alpha = Clamp01(Refusal->Show / RESOURCE_REFUSAL_FADE);
    float Pop = Refusal->Pop;
    float Line = UILineHeight(Font);
    float TextWidth = UITextWidth(Font, Text);
    float PadX = 26.f + 10.f * Pop;
    float PadY = 10.f + 4.f * Pop;
    float Width = TextWidth + 2.f * PadX;
    float Height = Line + 2.f * PadY;
    float Shake = 7.f * Pop * Sin(55.f * Refusal->Show);
    float CentreX = 0.5f * (float)WindowWidth + Shake;
    float Bottom = Minimum(AbilityBarPlateTop(WindowHeight) - RESOURCE_REFUSAL_LIFT,
                           0.8f * (float)WindowHeight);
    float Y = Bottom - Height - 8.f * Pop;
    float X = CentreX - 0.5f * Width;

    u32 Red = UI_RGBA(255, 60, 45, 255);
    DrawShaderQuad(RenderContext, Shader_Glow, X - 70.f, Y - 40.f, Width + 140.f, Height + 80.f,
                   WithAlpha(Red, (0.3f + 0.4f * Pop) * Alpha), RenderBlend_Additive);
    DrawRoundRect(RenderContext, X, Y, Width, Height, WithAlpha(UI_RGBA(60, 8, 8, 255), 0.88f * Alpha));
    DrawRoundRect(RenderContext, X, Y, Width, Height, WithAlpha(Red, 0.55f * Pop * Alpha));
    DrawRoundOutline(RenderContext, X, Y, Width, Height,
                     WithAlpha(UIMixColor(Red, UI_RGBA(255, 210, 190, 255), Pop), Alpha));
    UIText(RenderContext, Font, CentreX, Y + PadY, Text,
           WithAlpha(UIMixColor(UI_RGBA(255, 228, 220, 255), UI_RGBA(255, 255, 255, 255), Pop), Alpha),
           UIAlign_Center);
}
