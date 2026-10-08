/* Role requests: a dungeon role picked in the role picker
   (ui/dungeon/dungeon_hud.cpp) on its way to the server. It rides in the
   role byte of the input (net_input.Role, net/protocol.h), held
   for a few inputs and let go like a map vote (vote_requests.cpp), so a
   lost packet loses nothing; the server takes it between fights
   (TakeRoleRequests, sim/dungeon/encounters.cpp). Offline the picker
   sets the role directly. Developer builds also read GAME_ROLE at start
   (ApplyDeveloperRole), GAME_ROOM to start in a later room
   (ApplyDeveloperRoom) and GAME_MARKS to mark the fight's monsters
   (ApplyDeveloperMarks) and GAME_BOSS_HEALTH to start a boss part
   spent (ApplyDeveloperBossHealth), for scripted screenshots. */

internal void
RequestDungeonRole(app_state *AppState, u32 Role)
{
    AppState->RoleRequest = Role + 1;
}

// NOTE(zoubir): online, the role byte of this frame's input
internal u32
OnlineRoleRequest(app_state *AppState, float DeltaTime)
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
    u32 Result = AppState->RoleHolding;
    return Result;
}

// NOTE(zoubir): whether A and B are the same words, case aside
internal bool32
SameWordsAnyCase(char *A, char *B)
{
    for(; *A && *B; A++, B++)
    {
        if ((*A | 32) != (*B | 32))
        {
            return false;
        }
    }
    bool32 Result = *A == 0 && *B == 0;
    return Result;
}

// NOTE(zoubir): developer builds, offline: GAME_ROLE names a class
// ("ranger", "berserker", "fire mage") or a role ("tank", "healer",
// "ranged", "melee", and "damage" for the fire mage), and starts a dungeon
// run as that class (a role's first one), so a scripted screenshot
// (misc\screenshot.bat) can show a class's look and spells
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
    u32 Picked = PlayerRole_Count;
    for(u32 Role = 0; Role < PlayerRole_Count && Picked == PlayerRole_Count; Role++)
    {
        if (SameWordsAnyCase(Value, GetRoleDef(Role)->Name))
        {
            Picked = Role;
        }
    }
    for(u32 Role = 0; Role < PlayerRole_Count && Picked == PlayerRole_Count; Role++)
    {
        if (SameWordsAnyCase(Value, GetRoleDef(Role)->Title))
        {
            Picked = Role;
        }
    }
    if (Picked == PlayerRole_Count && SameWordsAnyCase(Value, "damage"))
    {
        Picked = PlayerRole_Damage;
    }
    if (Picked < PlayerRole_Count)
    {
        SetPlayerRole(AppState, Slot, Picked);
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

// NOTE(zoubir): GAME_ROLE and GAME_ROOM, once per local player added,
// as soon as the dungeon run exists: when a screenshot skips the connect
// screen (misc\screenshot.bat) the player is added before the first
// tick builds the run, so this runs again after each offline tick
// (online/world_tick.cpp) until it can
global_variable bool32 DeveloperStartDone;

internal void
ApplyDeveloperStart(app_state *AppState, memory_arena *Arena)
{
    if (DeveloperStartDone || !IsDungeon(AppState) || !AppState->Dungeon)
    {
        return;
    }
    DeveloperStartDone = true;
    ApplyDeveloperRole(AppState);
    ApplyDeveloperRoom(AppState, Arena);
}

// NOTE(zoubir): developer builds, offline, once a fight with a boss
// starts: GAME_BOSS_HEALTH=0.59 puts the boss at that share of its
// health, so a screenshot can show a phase (a Hollow Champion at 60%),
// and GAME_BOSS_CLOCK=5 leaves that many seconds on its enrage timer,
// to show the enrage and its Doom waves
internal void
ApplyDeveloperBossHealth(app_state *AppState)
{
#if APP_DEV
#pragma warning(push)
#pragma warning(disable: 4996)
    char *Value = getenv("GAME_BOSS_HEALTH");
    char *ClockValue = getenv("GAME_BOSS_CLOCK");
#pragma warning(pop)
    local_persist u32 SetSerial = 0;
    dungeon_run *Run = AppState->Dungeon;
    world_entity *Boss = Run ? FightBoss(&AppState->World, Run) : 0;
    bool32 Health = Value && Value[0];
    bool32 Clock = ClockValue && ClockValue[0];
    if ((!Health && !Clock) || !Boss || Boss->MonsterSerial == SetSerial ||
        Run->Clock.BossSerial != Boss->MonsterSerial)
    {
        return;
    }
    SetSerial = Boss->MonsterSerial;
    if (Health)
    {
        float Share = Clamp01((float)atof(Value));
        Boss->Hp = Maximum(1.f, Share * Boss->MaxHp);
    }
    if (Clock)
    {
        Run->Clock.Limit = (Run->Seconds - Run->Clock.StartSeconds) + (float)atof(ClockValue);
    }
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

