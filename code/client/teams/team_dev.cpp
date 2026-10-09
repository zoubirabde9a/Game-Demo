/* Developer switches for team duels, read once at start for scripted
   screenshots (misc/screenshot.bat): GAME_TEAMS=N plays offline in a
   team duel with N players, the others standing bots named "Bot 2" and
   up; GAME_TEAM_PANEL=1 opens the team panel (ui/teams/team_panel.cpp). */

internal void
ApplyDeveloperTeams(app_state *AppState)
{
#if APP_DEV
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // getenv: read once at startup, never kept
#endif
    char *Teams = getenv("GAME_TEAMS");
    char *Panel = getenv("GAME_TEAM_PANEL");
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    u32 Count = Teams ? (u32)atoi(Teams) : 0;
    if (!Count)
    {
        return;
    }
    world *World = &AppState->World;
    AppState->TeamDuel = true;
    for(u32 SlotIndex = 1; SlotIndex < Minimum(Count, (u32)MAX_PLAYERS); SlotIndex++)
    {
        AddPlayerToSlot(AppState, World, &AppState->WorldArena, SlotIndex,
                        PlayerSpawnPosition(World, SlotIndex));
        player_slot *Slot = &AppState->Players[SlotIndex];
        Slot->Bot = true;
        Slot->Kills = (SlotIndex * 7) % 5;
        Slot->Deaths = (SlotIndex * 3) % 4;
        Slot->Level = 1 + SlotIndex % 4;
        snprintf(Slot->Name, sizeof(Slot->Name), "Bot %u", SlotIndex + 1);
    }
    UpdateTeams(AppState, &AppState->WorldArena);
    if (Panel && Panel[0] == '1')
    {
        GetTeamUi(AppState)->PanelOpen = true;
    }
#endif
}
