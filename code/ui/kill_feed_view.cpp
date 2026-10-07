/* Kill feed and death plate. The feed (client/kill_feed.cpp keeps it) is
   listed under the minimap, newest on top, "Killer > Victim", each line
   fading out in its last second; lines with the local player in them are
   gold. While the local player is dead, a glass plate in the middle of
   the screen names who killed it and counts down to the respawn with a
   bar that fills as it comes; in a dungeon it also says how to fly the
   loose camera (client/free_camera.cpp). The plate waits while the
   duel's final blow plays (ui/final_blow_view.cpp). Both read only what
   offline and online play fill the same way: the feed's events and the
   slot's RespawnTimer. */

#define KILL_FEED_TOP (UI_GAP_LARGE + MINIMAP_PIXELS + 34.f)
#define KILL_FEED_FADE_SECONDS 1.f
#define DEATH_PLATE_WIDTH 340.f

// NOTE(zoubir): the killer's name for a feed entry: a player's chosen name,
// "a Bat" for a monster, or "the world" when nothing is known
internal void
KillerName(app_state *AppState, kill_feed_entry *Entry, char *Out, u32 OutSize)
{
    if (Entry->Killer != SIM_NOBODY && Entry->Killer < MAX_PLAYERS)
    {
        GetPlayerName(AppState, Entry->Killer, Out, OutSize);
    }
    else if (Entry->KillerMonster != SIM_NOBODY && Entry->KillerMonster < MonsterKind_Count)
    {
        snprintf(Out, OutSize, "a %s", GetMonsterDef((monster_kind)Entry->KillerMonster)->Name);
    }
    else
    {
        snprintf(Out, OutSize, "the world");
    }
}

internal void
DrawKillFeed(render_context *RenderContext, app_state *AppState, u32 WindowWidth)
{
    kill_feed *Feed = AppState->KillFeed;
    font *Font = AppState->Fonts.Small;
    if (!Feed || !Font)
    {
        return;
    }
    float Right = (float)WindowWidth - UI_GAP_LARGE;
    float Y = KILL_FEED_TOP;
    for(u32 Index = 0; Index < Feed->Count; Index++)
    {
        kill_feed_entry *Entry = &Feed->Entries[Index];
        char Killer[48];
        char Victim[48];
        char Line[112];
        KillerName(AppState, Entry, Killer, sizeof(Killer));
        GetPlayerName(AppState, Entry->Victim, Victim, sizeof(Victim));
        snprintf(Line, sizeof(Line), "%s  >  %s", Killer, Victim);
        float Fade = Clamp01((KILL_FEED_SECONDS - Entry->Age) / KILL_FEED_FADE_SECONDS);
        bool32 Mine = Entry->Killer == AppState->LocalPlayerIndex ||
            Entry->Victim == AppState->LocalPlayerIndex;
        u32 Color = Mine ? UI_COLOR_ACCENT : UI_COLOR_TEXT;
        UIText(RenderContext, Font, Right, Y, Line,
               WithAlpha(Color, Fade), UIAlign_Right);
        Y += UILineHeight(Font) + 2.f;
    }
}

// NOTE(zoubir): the local player's latest death in the feed, or 0
internal kill_feed_entry *
LocalDeath(app_state *AppState)
{
    kill_feed *Feed = AppState->KillFeed;
    for(u32 Index = 0; Feed && Index < Feed->Count; Index++)
    {
        if (Feed->Entries[Index].Victim == AppState->LocalPlayerIndex)
        {
            return &Feed->Entries[Index];
        }
    }
    return 0;
}

internal void
DrawDeathPlate(render_context *RenderContext, app_state *AppState,
               u32 WindowWidth, u32 WindowHeight)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    if (!Slot->Active || !IsDeadPlayer(Slot->Entity) || FinalBlowLeft(AppState) > 0.f)
    {
        return;
    }
    font *Title = AppState->Fonts.Title ? AppState->Fonts.Title : AppState->Fonts.Body;
    font *Body = AppState->Fonts.Body;
    bool32 FreeCamera = GetMapDef((map_id)AppState->World.MapId)->Dungeon;
    char Hint[96];
    snprintf(Hint, sizeof(Hint), "%s%sSD or arrows look around, Space comes back",
             LayoutKeyName('Z'), LayoutKeyName('Q'));
    float Width = DEATH_PLATE_WIDTH;
    float Height = UILineHeight(Title) + UILineHeight(Body) + 4.f * UI_GAP;
    if (FreeCamera)
    {
        Width = Maximum(Width, UITextWidth(Body, Hint) + 2.f * UI_GAP_LARGE);
        Height += UILineHeight(Body) + UI_GAP_SMALL;
    }
    float X = 0.5f * ((float)WindowWidth - Width);
    float Y = 0.32f * (float)WindowHeight;
    DrawUIPanel(RenderContext, X, Y, Width, Height, UI_COLOR_HEALTH);

    char Text[96];
    kill_feed_entry *Death = LocalDeath(AppState);
    if (Death)
    {
        char Killer[48];
        KillerName(AppState, Death, Killer, sizeof(Killer));
        snprintf(Text, sizeof(Text), "Killed by %s", Killer);
    }
    else
    {
        snprintf(Text, sizeof(Text), "You died");
    }
    float CentreX = X + 0.5f * Width;
    UIText(RenderContext, Title, CentreX, Y + UI_GAP, Text, UI_COLOR_TEXT, UIAlign_Center);

    // NOTE(zoubir): during a round break the respawn waits for it to end
    // (sim/round_break.cpp); online the client only knows the break's
    float Seconds = AppState->RoundBreak > 0.f ? RoundBreakLeft(AppState) :
        Maximum(0.f, Slot->RespawnTimer);
    // NOTE(zoubir): on a round map the dead are out until one player is
    // left standing; the round break then counts down
    bool32 OutForRound = IsRoundMap(AppState) && AppState->RoundBreak <= 0.f;
    if (OutForRound)
    {
        Seconds = 0.f;
        snprintf(Text, sizeof(Text), "Out until one player is left");
    }
    else
    {
        snprintf(Text, sizeof(Text), "Back in %.0f", Maximum(1.f, Seconds + 0.5f));
    }
    float TextY = Y + UI_GAP + UILineHeight(Title) + UI_GAP_SMALL;
    UIText(RenderContext, Body, CentreX, TextY, Text, UI_COLOR_TEXT_MUTED, UIAlign_Center);
    if (FreeCamera)
    {
        TextY += UILineHeight(Body) + UI_GAP_SMALL;
        UIText(RenderContext, Body, CentreX, TextY, Hint, UI_COLOR_TEXT_MUTED,
               UIAlign_Center);
    }

    // NOTE(zoubir): fills toward the respawn; the full length is the
    // player's own respawn time (Second Wind shortens it)
    float Full = AppState->RoundBreak > 0.f ? ROUND_BREAK_SECONDS : RespawnSeconds(Slot);
    float Share = OutForRound ? 0.f : Full > 0.f ? Clamp01(1.f - Seconds / Full) : 1.f;
    float BarX = X + UI_GAP_LARGE;
    float BarWidth = Width - 2.f * UI_GAP_LARGE;
    float BarY = Y + Height - UI_GAP - 4.f;
    DrawRoundRect(RenderContext, BarX, BarY, BarWidth, 6.f, UI_COLOR_TRACK);
    if (Share > 0.f)
    {
        DrawRoundRect(RenderContext, BarX, BarY, Maximum(6.f, Share * BarWidth), 6.f,
                      UI_COLOR_ACCENT);
    }
}
