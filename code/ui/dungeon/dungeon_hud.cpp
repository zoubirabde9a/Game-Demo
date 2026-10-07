/* Dungeon HUD (sim/dungeon/, docs/dungeon-plan.md): what a dungeon run
   adds to the screen, at the top in the middle, under the cast mode
   checkbox. Nothing here draws outside a run.

   - The objective: the room being fought and the enemies left in it, or
     the next room to head for, or the crypt cleared (offline with the
     run's time and the seconds to the next run); wipes so far.
   - The boss bar while a boss fights: its name, its health, and its
     enrage timer under it (sim/dungeon/boss_clock.cpp), red in the last
     seconds and once the boss has enraged; and while the boss has timed
     adds, the seconds until the soonest returns to it, red for one that
     erupts on the party.
   - Party frames, bottom left (party_frames.cpp): each player's role,
     health, shields and the monsters after them; clicking one picks the
     ally a tank's or healer's spells go to.
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
#define DUNGEON_ALARM_COLOR UI_RGBA(255, 120, 80, 255)
#define DUNGEON_ADD_ALARM_SECONDS 10

#include "party_frames.cpp"

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
        u32 Next = NextRoomToClear(Run->RoomStates, Run->RoomCount);
        // NOTE(zoubir): a cleared level goes on to the next one
        // (NextRunMap, sim/dungeon/levels.cpp)
        char *Cleared = GetMapDef((map_id)World->MapId)->Name;
        u32 NextMap = NextRunMap(World->MapId);
        char *Onward = GetMapDef((map_id)NextMap)->Name;
        dungeon_level *NextLevel = GetDungeonLevel(NextMap);
        char *Verb = (NextLevel && NextLevel->Number > 1) ? "Down to" : "Back up to";
        if (!Next && IsOnline(AppState->Online))
        {
            snprintf(Text, sizeof(Text), "%s cleared!  %s the %s soon",
                     Cleared, Verb, Onward);
        }
        else if (!Next)
        {
            u32 Minutes = (u32)(Run->Seconds / 60.f);
            u32 Seconds = (u32)Run->Seconds % 60;
            snprintf(Text, sizeof(Text), "%s cleared in %u:%02u!  %s the %s in %.0f",
                     Cleared, Minutes, Seconds, Verb, Onward,
                     Maximum(1.f, DUNGEON_VICTORY_SECONDS - Run->VictorySeconds + 0.5f));
        }
        else if (Next == 1)
        {
            dungeon_level *Level = GetDungeonLevel(World->MapId);
            snprintf(Text, sizeof(Text), "Level %u, the %s: gather in the %s",
                     Level ? Level->Number : 1, Cleared, GetRoomName(World->MapId, 1));
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

// NOTE(zoubir): the boss's enrage timer, one small line; returns its
// height, 0 with no timer
internal float
DrawDungeonBossClock(render_context *RenderContext, app_state *AppState,
                     float CenterX, float Y)
{
    boss_clock *Clock = &AppState->Dungeon->Clock;
    if (Clock->ShownStage == BossClock_None)
    {
        return 0.f;
    }
    char Text[64];
    u32 Color = UI_COLOR_TEXT_MUTED;
    if (Clock->ShownStage == BossClock_Enraged)
    {
        snprintf(Text, sizeof(Text), "ENRAGED  -  every hit grows harder");
        Color = DUNGEON_BOSS_COLOR;
    }
    else
    {
        u32 Left = Clock->ShownSecondsLeft;
        snprintf(Text, sizeof(Text), "Enrage in %u:%02u", Left / 60, Left % 60);
        if (Clock->ShownStage == BossClock_Warned)
        {
            Color = DUNGEON_BOSS_COLOR;
        }
    }
    float Result = DungeonHudLine(RenderContext, AppState->Fonts.Small, CenterX, Y, Text, Color) +
        UI_GAP_SMALL;
    if (Clock->ShownAddSeconds && Clock->ShownAddBursts)
    {
        // NOTE(zoubir): the line that decides the run: in the body font,
        // bright, and beating in its last DUNGEON_ADD_ALARM_SECONDS
        snprintf(Text, sizeof(Text), "Kill the champion: it erupts in %u s", Clock->ShownAddSeconds);
        float Alpha = 1.f;
        if (Clock->ShownAddSeconds <= DUNGEON_ADD_ALARM_SECONDS)
        {
            Alpha = 0.55f + 0.45f * Absolute(Sin(6.f * GetFxClock(AppState)));
        }
        Result += DungeonHudLine(RenderContext, AppState->Fonts.Body, CenterX, Y + Result, Text,
                                 WithAlpha(DUNGEON_ALARM_COLOR, Alpha)) + UI_GAP_SMALL;
    }
    else if (Clock->ShownAddSeconds)
    {
        snprintf(Text, sizeof(Text), "Adds return to the boss in %u s", Clock->ShownAddSeconds);
        Result += DungeonHudLine(RenderContext, AppState->Fonts.Small, CenterX, Y + Result, Text,
                                 UI_COLOR_TEXT_MUTED) + UI_GAP_SMALL;
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
    Result += DrawDungeonBossClock(RenderContext, AppState, CenterX, Y + Result);
    return Result;
}

// NOTE(zoubir): three role buttons and the picked role's keys
internal void
DoDungeonRolePicker(render_context *RenderContext, app_state *AppState,
                    app_input *Input, float CenterX, float Y)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    dungeon_run *Run = AppState->Dungeon;
    // NOTE(zoubir): not over the talent panel, which is drawn first
    if (!Slot->Active || !Slot->Entity || Run->FightingRoom || GetTalentPanel(AppState)->Open ||
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
    float PlateWidth = Maximum(Width, UITextWidth(Small, GetRoleDef(Slot->Role)->Keys)) +
        2.f * UI_GAP;
    float PlateHeight = UILineHeight(Body) + DUNGEON_ROLE_BUTTON_HEIGHT +
        2.f * UILineHeight(Small) + 2.f * UI_GAP + 2.f * UI_GAP_SMALL;
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
    LineY += DungeonHudLine(RenderContext, Small, CenterX, LineY, GetRoleDef(Slot->Role)->Keys,
                            UI_COLOR_TEXT_MUTED);
    DungeonHudLine(RenderContext, Small, CenterX, LineY,
                   "Each role has its own talents: press N", UI_COLOR_TEXT_MUTED);
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
    DrawDungeonParty(RenderContext, AppState, Input, WindowHeight);
}
