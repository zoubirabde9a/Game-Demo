/* Damage meter (dungeon_hud.cpp): the last room fought, on the right
   just above the ability bar's height (clear of the buttons beside it),
   read from the run's meter (sim/dungeon/meter.cpp). A title with the
   room and how long the fight has lasted, then one row per player, the
   most damage first: their role's emblem and name, and three columns,
   damage dealt, healing done and damage taken, each the fight's total
   and, dimmer, the same per second. A bar behind each number shows it
   against the column's best. It starts again from 0 when the next fight
   starts, and shows nothing before the run's first real fight. */

#define METER_COLUMN_WIDTH 86.f
#define METER_NAME_WIDTH 112.f
#define METER_PAD 8.f
#define METER_ROW_GAP 2.f

enum meter_column
{
    MeterColumn_Damage,
    MeterColumn_Healing,
    MeterColumn_Taken,
    MeterColumn_Count,
};

global_variable char *MeterColumnNames[MeterColumn_Count] = {"Damage", "Healing", "Taken"};
global_variable u32 MeterColumnColors[MeterColumn_Count] =
{
    UI_RGBA(240, 140, 60, 255),
    UI_RGBA(110, 220, 130, 255),
    UI_RGBA(220, 70, 60, 255),
};

inline float
MeterValue(player_slot *Slot, u32 Column)
{
    float Result = Column == MeterColumn_Damage ? Slot->MeterDamage :
        (Column == MeterColumn_Healing ? Slot->MeterHealing : Slot->MeterTaken);
    return Result;
}

// NOTE(zoubir): 842, 12.4k, 1.2m: short enough for a column
internal void
FormatMeterAmount(char *Out, u32 OutSize, float Amount)
{
    if (Amount < 10000.f)
    {
        snprintf(Out, OutSize, "%.0f", Amount);
    }
    else if (Amount < 1000000.f)
    {
        snprintf(Out, OutSize, "%.1fk", Amount / 1000.f);
    }
    else
    {
        snprintf(Out, OutSize, "%.1fm", Amount / 1000000.f);
    }
}

internal void
DrawDamageMeter(render_context *RenderContext, app_state *AppState,
                u32 WindowWidth, u32 WindowHeight)
{
    // NOTE(zoubir): an empty room, the Antechamber, is a fight over at
    // once, which leaves nothing to show
    dungeon_run *Run = AppState->Dungeon;
    if (!Run->MeterFight || (!Run->FightingRoom && Run->MeterSeconds < 1.f))
    {
        return;
    }
    font *Small = AppState->Fonts.Small;
    float Line = UILineHeight(Small);

    // NOTE(zoubir): the players, most damage first
    u32 Order[MAX_PLAYERS];
    u32 Count = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (!Slot->Active || !Slot->Entity)
        {
            continue;
        }
        u32 At = Count++;
        while (At > 0 && AppState->Players[Order[At - 1]].MeterDamage < Slot->MeterDamage)
        {
            Order[At] = Order[At - 1];
            At--;
        }
        Order[At] = SlotIndex;
    }
    float Best[MeterColumn_Count] = {};
    for(u32 Row = 0; Row < Count; Row++)
    {
        for(u32 Column = 0; Column < MeterColumn_Count; Column++)
        {
            Best[Column] = Maximum(Best[Column], MeterValue(&AppState->Players[Order[Row]], Column));
        }
    }

    float RowHeight = Line + METER_ROW_GAP;
    float Width = 2.f * METER_PAD + METER_NAME_WIDTH + MeterColumn_Count * METER_COLUMN_WIDTH;
    float Height = 2.f * METER_PAD + 2.f * Line + UI_GAP_SMALL + (float)Count * RowHeight;
    float X = (float)WindowWidth - UI_GAP_LARGE - Width;
    float Y = AbilityBarPlateTop(WindowHeight) - UI_GAP - Height;
    DrawUIPanel(RenderContext, X, Y, Width, Height);

    char Text[96];
    float Seconds = Run->MeterSeconds;
    u32 Minutes = (u32)(Seconds / 60.f);
    char *Room = Run->MeterRoom ? GetRoomName(AppState->World.MapId, Run->MeterRoom) : (char *)"";
    snprintf(Text, sizeof(Text), "%s%s%u:%02u", Room, Room[0] ? "  -  " : "", Minutes,
             (u32)Seconds % 60);
    float Left = X + METER_PAD;
    float Top = Y + METER_PAD;
    UIText(RenderContext, Small, Left, Top, Text,
           Run->FightingRoom ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED);
    UIText(RenderContext, Small, X + Width - METER_PAD, Top, (char *)"total  /  per second",
           UI_COLOR_TEXT_MUTED, UIAlign_Right);
    Top += Line + UI_GAP_SMALL;
    for(u32 Column = 0; Column < MeterColumn_Count; Column++)
    {
        float Right = Left + METER_NAME_WIDTH + (float)(Column + 1) * METER_COLUMN_WIDTH;
        UIText(RenderContext, Small, Right, Top, MeterColumnNames[Column],
               MeterColumnColors[Column], UIAlign_Right);
    }
    Top += Line;

    // NOTE(zoubir): the first seconds of a fight would make the rates
    // jump, so they are over one second at least
    float PerSecond = 1.f / Maximum(1.f, Seconds);
    for(u32 Row = 0; Row < Count; Row++)
    {
        u32 SlotIndex = Order[Row];
        player_slot *Slot = &AppState->Players[SlotIndex];
        u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
        float Emblem = Line - 2.f;
        DrawRoleEmblem(RenderContext, Role, Left, Top + 1.f, Emblem, RoleUIColor(Role), 1.f);
        GetPlayerName(AppState, SlotIndex, Text, sizeof(Text));
        u32 NameColor = SlotIndex == AppState->LocalPlayerIndex ? UI_COLOR_ACCENT : UI_COLOR_TEXT;
        UIText(RenderContext, Small, Left + Emblem + 4.f, Top, Text, NameColor);
        for(u32 Column = 0; Column < MeterColumn_Count; Column++)
        {
            float Amount = MeterValue(Slot, Column);
            float CellLeft = Left + METER_NAME_WIDTH + (float)Column * METER_COLUMN_WIDTH + 4.f;
            float CellWidth = METER_COLUMN_WIDTH - 4.f;
            float Share = Best[Column] > 0.f ? Amount / Best[Column] : 0.f;
            if (Share > 0.f)
            {
                DrawRoundRect(RenderContext, CellLeft, Top + 1.f, Maximum(4.f, Share * CellWidth),
                              Line - 2.f, WithAlpha(MeterColumnColors[Column], 0.28f));
            }
            char Rate[24];
            FormatMeterAmount(Rate, sizeof(Rate), Amount * PerSecond);
            float Right = CellLeft + CellWidth;
            UIText(RenderContext, Small, Right, Top, Rate, UI_COLOR_TEXT_MUTED, UIAlign_Right);
            char Total[24];
            FormatMeterAmount(Total, sizeof(Total), Amount);
            float RateWidth = Maximum(UITextWidth(Small, Rate), UITextWidth(Small, (char *)"000"));
            UIText(RenderContext, Small, Right - RateWidth - 6.f, Top, Total,
                   Amount > 0.f ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED, UIAlign_Right);
        }
        Top += RowHeight;
    }
}
