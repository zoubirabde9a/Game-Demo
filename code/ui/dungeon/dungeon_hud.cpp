/* Dungeon HUD (sim/dungeon/, docs/dungeon-plan.md): what a dungeon run
   adds to the screen, at the top in the middle, under the cast mode
   checkbox. Nothing here draws outside a run.

   - The objective: the room being fought and the enemies left in it, or
     the next room to head for, or the crypt cleared; wipes so far.
   - The boss bar while a boss fights: its name and its health.
   - Party frames, bottom left: each player's role, name and health, the
     local one first; a downed player's bar is grey.
   - The role picker, in the Antechamber between fights: three buttons,
     the picked one outlined, and what its keys do. Offline the pick
     takes at once; online it goes to the server as a request
     (client/dungeon/role_requests.cpp) and shows once a snapshot says so.

   The fight is read from dungeon_run's Shown fields, which the run sets
   offline and the snapshot sets online. */

#define DUNGEON_HUD_TOP 44.f
#define DUNGEON_BOSS_BAR_WIDTH 420.f
#define DUNGEON_BOSS_BAR_HEIGHT 10.f
#define DUNGEON_ROLE_BUTTON_WIDTH 120.f
#define DUNGEON_ROLE_BUTTON_HEIGHT 34.f
#define DUNGEON_BOSS_COLOR UI_RGBA(176, 52, 60, 255)
#define DUNGEON_PARTY_WIDTH 190.f
#define DUNGEON_PARTY_BAR_HEIGHT 6.f
#define DUNGEON_PARTY_MARGIN 14.f

// NOTE(zoubir): each role's colour on the party frames, by player_role
global_variable u32 DungeonRoleColors[PlayerRole_Count] =
{
    UI_RGBA(230, 110, 70, 255),
    UI_RGBA(90, 150, 240, 255),
    UI_RGBA(110, 210, 120, 255),
};

// NOTE(zoubir): the next room the party has to clear, 0 when all are
internal u32
NextDungeonRoom(dungeon_run *Run)
{
    for(u32 Room = 1; Room <= Run->RoomCount; Room++)
    {
        if (Run->RoomStates[Room] != RoomState_Cleared)
        {
            return Room;
        }
    }
    return 0;
}

// NOTE(zoubir): one centred line at Y; returns the line's height
internal float
DungeonHudLine(render_context *RenderContext, font *Font, float CenterX, float Y,
               char *Text, u32 Color)
{
    UIText(RenderContext, Font, CenterX, Y, Text, Color, UIAlign_Center);
    float Result = UILineHeight(Font);
    return Result;
}

internal float
DrawDungeonObjective(render_context *RenderContext, app_state *AppState,
                     float CenterX, float Y)
{
    dungeon_run *Run = AppState->Dungeon;
    world *World = &AppState->World;
    font *Body = AppState->Fonts.Body;
    char Text[128];
    if (Run->FightingRoom)
    {
        snprintf(Text, sizeof(Text), "%s  -  %u %s left",
                 GetRoomName(World->MapId, Run->FightingRoom), Run->ShownFoesLeft,
                 Run->ShownFoesLeft == 1 ? "enemy" : "enemies");
    }
    else
    {
        u32 Next = NextDungeonRoom(Run);
        if (!Next)
        {
            snprintf(Text, sizeof(Text), "The crypt is cleared");
        }
        else if (Next == 1)
        {
            snprintf(Text, sizeof(Text), "Gather in the %s", GetRoomName(World->MapId, 1));
        }
        else
        {
            snprintf(Text, sizeof(Text), "Next: the %s", GetRoomName(World->MapId, Next));
        }
    }
    float Result = DungeonHudLine(RenderContext, Body, CenterX, Y, Text, UI_COLOR_TEXT);
    if (Run->Wipes)
    {
        snprintf(Text, sizeof(Text), "%u %s", Run->Wipes, Run->Wipes == 1 ? "wipe" : "wipes");
        Result += DungeonHudLine(RenderContext, AppState->Fonts.Small, CenterX, Y + Result,
                                 Text, UI_COLOR_TEXT_MUTED);
    }
    return Result;
}

internal float
DrawDungeonBossBar(render_context *RenderContext, app_state *AppState,
                   float CenterX, float Y)
{
    dungeon_run *Run = AppState->Dungeon;
    if (Run->ShownBossKind >= MonsterKind_Count)
    {
        return 0.f;
    }
    font *Body = AppState->Fonts.Body;
    float Width = DUNGEON_BOSS_BAR_WIDTH;
    float X = CenterX - 0.5f * Width;
    float NameHeight = DungeonHudLine(RenderContext, Body, CenterX, Y,
                                      GetMonsterDef((monster_kind)Run->ShownBossKind)->Name,
                                      UI_COLOR_ACCENT);
    float BarY = Y + NameHeight + 2.f;
    float Share = Clamp01(Run->ShownBossShare);
    DrawRoundRect(RenderContext, X, BarY, Width, DUNGEON_BOSS_BAR_HEIGHT, UI_COLOR_TRACK);
    if (Share > 0.f)
    {
        DrawRoundRect(RenderContext, X, BarY, Maximum(DUNGEON_BOSS_BAR_HEIGHT, Share * Width),
                      DUNGEON_BOSS_BAR_HEIGHT, DUNGEON_BOSS_COLOR);
    }
    DrawRoundOutline(RenderContext, X, BarY, Width, DUNGEON_BOSS_BAR_HEIGHT, UI_COLOR_BORDER);
    float Result = NameHeight + 2.f + DUNGEON_BOSS_BAR_HEIGHT + UI_GAP_SMALL;
    return Result;
}

// NOTE(zoubir): three role buttons and the picked role's keys
internal void
DoDungeonRolePicker(render_context *RenderContext, app_state *AppState,
                    app_input *Input, float CenterX, float Y)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    dungeon_run *Run = AppState->Dungeon;
    if (!Slot->Active || !Slot->Entity || Run->FightingRoom ||
        RoomAtPosition(&AppState->World, Slot->Entity->Position.XY) != 1)
    {
        return;
    }
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    // NOTE(zoubir): left to right as players think of them
    u32 Order[PlayerRole_Count] = {PlayerRole_Tank, PlayerRole_Healer, PlayerRole_Damage};
    float Width = PlayerRole_Count * DUNGEON_ROLE_BUTTON_WIDTH +
        (PlayerRole_Count - 1) * UI_GAP_SMALL;
    float PlateWidth = Width + 2.f * UI_GAP;
    float PlateHeight = UILineHeight(Body) + DUNGEON_ROLE_BUTTON_HEIGHT +
        UILineHeight(Small) + 2.f * UI_GAP + 2.f * UI_GAP_SMALL;
    DrawUIPanel(RenderContext, CenterX - 0.5f * PlateWidth, Y, PlateWidth, PlateHeight);
    float LineY = Y + UI_GAP;
    LineY += DungeonHudLine(RenderContext, Body, CenterX, LineY, "Pick your role",
                            UI_COLOR_TEXT) + UI_GAP_SMALL;
    float X = CenterX - 0.5f * Width;
    for(u32 Index = 0; Index < PlayerRole_Count; Index++)
    {
        u32 Role = Order[Index];
        bool32 Picked = Slot->Role == Role;
        if (OptionsButton(RenderContext, Input, X, LineY, DUNGEON_ROLE_BUTTON_WIDTH,
                          DUNGEON_ROLE_BUTTON_HEIGHT, Picked))
        {
            if (IsOnline(AppState->Online))
            {
                RequestDungeonRole(AppState, Role);
            }
            else
            {
                SetPlayerRole(AppState, Slot, Role);
            }
        }
        UIText(RenderContext, Body, X + 0.5f * DUNGEON_ROLE_BUTTON_WIDTH,
               LineY + 0.5f * (DUNGEON_ROLE_BUTTON_HEIGHT - UILineHeight(Body)),
               GetRoleDef(Role)->Title, Picked ? UI_COLOR_ACCENT : UI_COLOR_TEXT,
               UIAlign_Center);
        X += DUNGEON_ROLE_BUTTON_WIDTH + UI_GAP_SMALL;
    }
    LineY += DUNGEON_ROLE_BUTTON_HEIGHT + UI_GAP_SMALL;
    DungeonHudLine(RenderContext, Small, CenterX, LineY, GetRoleDef(Slot->Role)->Keys,
                   UI_COLOR_TEXT_MUTED);
}

// NOTE(zoubir): one player's frame with its top at Y; returns its height
internal float
DrawPartyFrame(render_context *RenderContext, app_state *AppState, u32 SlotIndex,
               float X, float Y)
{
    player_slot *Slot = &AppState->Players[SlotIndex];
    world_entity *Player = Slot->Entity;
    font *Small = AppState->Fonts.Small;
    role_def *Role = GetRoleDef(Slot->Role);
    u32 RoleColor = DungeonRoleColors[Slot->Role < PlayerRole_Count ? Slot->Role : 0];
    bool32 Down = IsDeadPlayer(Player);
    char Name[32];
    GetPlayerName(AppState, SlotIndex, Name, sizeof(Name));
    char Line[64];
    snprintf(Line, sizeof(Line), "%s  %s", Role->Title, Name);
    UIText(RenderContext, Small, X, Y, Line,
           SlotIndex == AppState->LocalPlayerIndex ? UI_COLOR_ACCENT : UI_COLOR_TEXT);
    float BarY = Y + UILineHeight(Small) + 2.f;
    float Share = (Player->MaxHp > 0.f && !Down) ? Clamp01(Player->Hp / Player->MaxHp) : 0.f;
    DrawRoundRect(RenderContext, X, BarY, DUNGEON_PARTY_WIDTH, DUNGEON_PARTY_BAR_HEIGHT,
                  Down ? UI_COLOR_DIM : UI_COLOR_TRACK);
    if (Share > 0.f)
    {
        DrawRoundRect(RenderContext, X, BarY,
                      Maximum(DUNGEON_PARTY_BAR_HEIGHT, Share * DUNGEON_PARTY_WIDTH),
                      DUNGEON_PARTY_BAR_HEIGHT, RoleColor);
    }
    float Result = UILineHeight(Small) + 2.f + DUNGEON_PARTY_BAR_HEIGHT + UI_GAP_SMALL;
    return Result;
}

// NOTE(zoubir): the local player at the bottom, the others stacked above
internal void
DrawDungeonParty(render_context *RenderContext, app_state *AppState, u32 WindowHeight)
{
    float FrameHeight = UILineHeight(AppState->Fonts.Small) + 2.f +
        DUNGEON_PARTY_BAR_HEIGHT + UI_GAP_SMALL;
    float X = DUNGEON_PARTY_MARGIN;
    float Y = (float)WindowHeight - DUNGEON_PARTY_MARGIN - FrameHeight;
    for(u32 Pass = 0; Pass < 2; Pass++)
    {
        for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
        {
            player_slot *Slot = &AppState->Players[SlotIndex];
            bool32 Local = SlotIndex == AppState->LocalPlayerIndex;
            if (!Slot->Active || !Slot->Entity || Local != (Pass == 0))
            {
                continue;
            }
            DrawPartyFrame(RenderContext, AppState, SlotIndex, X, Y);
            Y -= FrameHeight;
        }
    }
}

// NOTE(zoubir): from the screen pass, every frame
internal void
DoDungeonHud(render_context *RenderContext, app_state *AppState, app_input *Input,
             u32 WindowWidth, u32 WindowHeight)
{
    if (!IsDungeon(AppState))
    {
        return;
    }
    float CenterX = 0.5f * (float)WindowWidth;
    float Y = DUNGEON_HUD_TOP;
    Y += DrawDungeonObjective(RenderContext, AppState, CenterX, Y) + UI_GAP_SMALL;
    Y += DrawDungeonBossBar(RenderContext, AppState, CenterX, Y);
    DoDungeonRolePicker(RenderContext, AppState, Input, CenterX, Y);
    DrawDungeonParty(RenderContext, AppState, WindowHeight);
}
