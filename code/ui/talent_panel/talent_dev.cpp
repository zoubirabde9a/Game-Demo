/* Developer switch for the talent panel, read once when the panel is
   first made, for scripted screenshots (misc/screenshot.bat):
   GAME_TALENT_PANEL=N plays offline with the local player at level N and
   the panel open on its tree, the duel tree rolled for the match. */

internal void
ApplyDeveloperTalentPanel(app_state *AppState, talent_panel *Panel)
{
#if APP_DEV
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // getenv: read once, never kept
#endif
    char *Level = getenv("GAME_TALENT_PANEL");
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    player_slot *Local = &AppState->Players[AppState->LocalPlayerIndex % MAX_PLAYERS];
    if (Level && atoi(Level) > 1 && !IsOnline(AppState->Online) && Local->Active)
    {
        AwardXp(AppState, Local, XpToReach((u32)atoi(Level)));
        Panel->Open = true;
    }
#endif
}
