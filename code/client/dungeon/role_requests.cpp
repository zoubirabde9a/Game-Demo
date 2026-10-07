/* Role requests: a dungeon role picked in the role picker
   (ui/dungeon/dungeon_hud.cpp) on its way to the server. It rides in the
   role field of the held buttons (NET_ROLE_SHIFT, net/protocol.h), held
   for a few inputs and let go like a map vote (vote_requests.cpp), so a
   lost packet loses nothing; the server takes it between fights
   (TakeRoleRequests, sim/dungeon/encounters.cpp). Offline the picker
   sets the role directly. Developer builds also read GAME_ROLE at start
   (ApplyDeveloperRole), GAME_ROOM to start in a later room
   (ApplyDeveloperRoom) and GAME_MARKS to mark the fight's monsters
   (ApplyDeveloperMarks), for scripted screenshots. */

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

// NOTE(zoubir): developer builds, offline: GAME_ROOM=N starts the run in
// room N, the rooms before it cleared, so a screenshot can show a fight
internal void
ApplyDeveloperRoom(app_state *AppState, memory_arena *Arena)
{
#if APP_DEV
#pragma warning(push)
#pragma warning(disable: 4996)
    char *Value = getenv("GAME_ROOM");
#pragma warning(pop)
    dungeon_run *Run = AppState->Dungeon;
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    u32 Room = Value ? (u32)atoi(Value) : 0;
    if (!Run || !Slot->Entity || Room < 2 || Room > Run->RoomCount)
    {
        return;
    }
    for(u32 Before = 1; Before < Room; Before++)
    {
        Run->RoomStates[Before] = RoomState_Cleared;
    }
    MovePlayerTo(AppState, &AppState->World, Arena, Slot->Entity, Run->RoomEntry[Room]);
#endif
}

// NOTE(zoubir): developer builds, offline, after each tick: with
// GAME_MARKS set, the monsters of a fight that starts get Searing stacks
// (one to three in turn) and every other one a Sunder, so a screenshot
// shows the foe marks (client/dungeon/foe_mark_fx.cpp)
internal void
ApplyDeveloperMarks(app_state *AppState)
{
#if APP_DEV
#pragma warning(push)
#pragma warning(disable: 4996)
    char *Value = getenv("GAME_MARKS");
#pragma warning(pop)
    local_persist u32 MarkedRoom = 0;
    dungeon_run *Run = AppState->Dungeon;
    if (!Value || !Value[0] || !Run || !Run->FightingRoom || Run->FightingRoom == MarkedRoom)
    {
        return;
    }
    MarkedRoom = Run->FightingRoom;
    world *World = &AppState->World;
    for(u32 Foe = 0; Foe < Run->FoeCount; Foe++)
    {
        world_entity *Monster = FindMonsterBySerial(World, Run->FoeSlots[Foe], Run->FoeSerials[Foe]);
        if (!Monster)
        {
            continue;
        }
        for(u32 Stack = 0; Stack <= Foe % SEARING_MOST; Stack++)
        {
            AddSearing(Run, World, Monster);
        }
        if (Foe % 2 == 0)
        {
            AddSunder(Run, World, Monster, 60.f, SUNDER_SHARE);
        }
        // NOTE(zoubir): long enough to see in a shot taken seconds later
        foe_mark *Mark = FindFoeMark(Run, World, Monster);
        Mark->Seconds = 60.f;
    }
#endif
}

