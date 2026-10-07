/* Role requests: a dungeon role picked in the role picker
   (ui/dungeon/dungeon_hud.cpp) on its way to the server. It rides in the
   role field of the held buttons (NET_ROLE_SHIFT, net/protocol.h), held
   for a few inputs and let go like a map vote (vote_requests.cpp), so a
   lost packet loses nothing; the server takes it between fights
   (TakeRoleRequests, sim/dungeon/encounters.cpp). Offline the picker
   sets the role directly. Developer builds also read GAME_ROLE at start
   (ApplyDeveloperRole), for scripted screenshots. */

internal void
RequestDungeonRole(app_state *AppState, u32 Role)
{
    AppState->RoleRequest = Role + 1;
}

// NOTE(zoubir): online, the role field to OR into this frame's held
// buttons
internal u32
OnlineRoleBits(app_state *AppState, float DeltaTime)
{
    if (AppState->RoleHolding)
    {
        AppState->RoleHoldLeft -= DeltaTime;
        if (AppState->RoleHoldLeft <= 0.f)
        {
            AppState->RoleHolding = 0;
            AppState->RoleGapLeft = TALENT_GAP_SECONDS;
        }
    }
    else if (AppState->RoleGapLeft > 0.f)
    {
        AppState->RoleGapLeft -= DeltaTime;
    }
    else if (AppState->RoleRequest)
    {
        AppState->RoleHolding = AppState->RoleRequest;
        AppState->RoleRequest = 0;
        AppState->RoleHoldLeft = TALENT_HOLD_SECONDS;
    }
    u32 Result = (AppState->RoleHolding & NET_ROLE_MASK) << NET_ROLE_SHIFT;
    return Result;
}

// NOTE(zoubir): developer builds, offline: GAME_ROLE=tank, healer or
// damage starts a dungeon run in that role, so a scripted screenshot
// (misc\screenshot.bat) can show a role's look and spells
internal void
ApplyDeveloperRole(app_state *AppState)
{
#if APP_DEV
#pragma warning(push)
#pragma warning(disable: 4996)
    char *Value = getenv("GAME_ROLE");
#pragma warning(pop)
    if (!Value || !Value[0] || !IsDungeon(AppState))
    {
        return;
    }
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    for(u32 Role = 0; Role < PlayerRole_Count; Role++)
    {
        role_def *Def = GetRoleDef(Role);
        if (strcmp(Value, Def->Title) == 0 || strcmp(Value, Def->Name) == 0 ||
            (Value[0] | 32) == (Def->Title[0] | 32))
        {
            SetPlayerRole(AppState, Slot, Role);
        }
    }
#endif
}
