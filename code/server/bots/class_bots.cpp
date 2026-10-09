/* Class bots (server/bots.cpp): the keys a bot of each class after the
   first three presses, one file each, and what they share (bot_dangers.cpp:
   the telegraphs a melee bot keeps out of). */

#include "bot_dangers.cpp"
#include "bot_rift_dangers.cpp"
#include "bot_starless_dangers.cpp"
#include "ranger.cpp"
#include "berserker.cpp"
#include "shadowblade.cpp"
#include "stormcaller.cpp"
#include "duelist.cpp"
#include "frostmage.cpp"
#include "druid.cpp"

// NOTE(zoubir): 0 for a class with no file here
internal u32
BotClassButtons(bot_brain *Bot, app_state *AppState, world_entity *Self, world_entity *Target,
                float Distance, v2 Direction, u32 *Held, u16 *Pick)
{
    switch(AppState->Players[Self->PlayerIndex].Role)
    {
        case PlayerRole_Ranger: return BotRangerButtons(Bot, AppState, Self, Target, Distance, Direction, Held, Pick);
        case PlayerRole_Berserker: return BotBerserkerButtons(Bot, AppState, Self, Target, Distance, Direction, Held, Pick);
        case PlayerRole_Shadowblade: return BotShadowbladeButtons(Bot, AppState, Self, Target, Distance, Direction, Held, Pick);
        case PlayerRole_Stormcaller: return BotStormcallerButtons(Bot, AppState, Self, Target, Distance, Direction, Held, Pick);
        case PlayerRole_Duelist: return BotDuelistButtons(Bot, AppState, Self, Target, Distance, Direction, Held, Pick);
        case PlayerRole_FrostMage: return BotFrostMageButtons(Bot, AppState, Self, Target, Distance, Direction, Held, Pick);
        case PlayerRole_Druid: return BotDruidButtons(Bot, AppState, Self, Target, Distance, Direction, Held, Pick);
    }
    return 0;
}

// NOTE(zoubir): the role a bot asks for by its slot (server/bots.cpp):
// a tank, a healer and damage first, then more damage, a second healer
// at six and a second tank at eight
global_variable u32 BotWantedRole[MAX_PLAYERS] =
{
    PlayerRole_Tank, PlayerRole_Healer, PlayerRole_Damage, PlayerRole_Damage,
    PlayerRole_Damage, PlayerRole_Healer, PlayerRole_Damage, PlayerRole_Tank,
};

// NOTE(zoubir): developer builds: the class an environment variable
// names (GAME_BOT_DAMAGE="ranger"), when it is one of Kind's classes with
// a kit; PlayerRole_Count for none
internal u32
BotForcedClass(char *Variable, u32 Kind)
{
    u32 Result = PlayerRole_Count;
#if APP_DEV
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // getenv: read, never kept
#endif
    char *Forced = getenv(Variable);
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    for (u32 Role = 0; Forced && Forced[0] && Role < PlayerRole_Count && Result == PlayerRole_Count; ++Role)
    {
        char *Name = GetRoleDef(Role)->Name;
        u32 At = 0;
        while (Name[At] && Forced[At] && (Name[At] | 32) == (Forced[At] | 32))
        {
            ++At;
        }
        if (!Name[At] && !Forced[At] && RoleKindOf(Role) == Kind && RoleHasKit(Role))
        {
            Result = Role;
        }
    }
#endif
    return Result;
}

// NOTE(zoubir): how many slots in use before PlayerIndex ask for Wanted,
// the bot's seat among them: bots join in any slots (the balance probe's
// three sit in slots 5 to 7)
internal u32
BotSeat(app_state *AppState, u32 PlayerIndex, u32 Wanted)
{
    u32 Result = 0;
    for (u32 Before = 0; Before < PlayerIndex && Before < MAX_PLAYERS; ++Before)
    {
        Result += (AppState->Players[Before].Active && BotWantedRole[Before] == Wanted) ? 1 : 0;
    }
    return Result;
}

// NOTE(zoubir): the damage class a bot in slot PlayerIndex plays, by its
// seat among the damage bots. The first damage seat is always the Fire
// Mage, so the balance probe's party of three (tools/dungeon_balance.cpp)
// stays the one its numbers were tuned against; the seats after it go
// round the other damage classes that have a kit. Developer builds:
// GAME_BOT_DAMAGE names a class ("ranger") that every damage bot plays,
// to measure one class
internal u32
BotDamageClass(app_state *AppState, u32 PlayerIndex)
{
    u32 Forced = BotForcedClass("GAME_BOT_DAMAGE", RoleKind_Ranged);
    if (Forced == PlayerRole_Count)
    {
        Forced = BotForcedClass("GAME_BOT_DAMAGE", RoleKind_Melee);
    }
    if (Forced < PlayerRole_Count)
    {
        return Forced;
    }
    u32 Seat = BotSeat(AppState, PlayerIndex, PlayerRole_Damage);
    if (Seat == 0)
    {
        return PlayerRole_Damage;
    }
    u32 Classes[PlayerRole_Count];
    u32 Count = 0;
    for (u32 Role = 0; Role < PlayerRole_Count; ++Role)
    {
        if (Role != PlayerRole_Damage && IsDamageRole(Role) && RoleHasKit(Role))
        {
            Classes[Count++] = Role;
        }
    }
    u32 Result = Count ? Classes[(Seat - 1) % Count] : PlayerRole_Damage;
    return Result;
}

// NOTE(zoubir): the healer class a bot in slot PlayerIndex plays, by its
// seat among the healer bots: the first is always the Mender, so the
// balance probe's party keeps its healer; the seats after it go round the
// other healer classes that have a kit (the Druid). Developer builds:
// GAME_BOT_HEALER names one ("druid") that every healer bot plays
internal u32
BotHealerClass(app_state *AppState, u32 PlayerIndex)
{
    u32 Forced = BotForcedClass("GAME_BOT_HEALER", RoleKind_Healer);
    if (Forced < PlayerRole_Count)
    {
        return Forced;
    }
    u32 Seat = BotSeat(AppState, PlayerIndex, PlayerRole_Healer);
    u32 Classes[PlayerRole_Count];
    u32 Count = 0;
    for (u32 Role = 0; Role < PlayerRole_Count; ++Role)
    {
        if (Role != PlayerRole_Healer && RoleKindOf(Role) == RoleKind_Healer && RoleHasKit(Role))
        {
            Classes[Count++] = Role;
        }
    }
    u32 Result = (Seat == 0 || !Count) ? (u32)PlayerRole_Healer : Classes[(Seat - 1) % Count];
    return Result;
}
