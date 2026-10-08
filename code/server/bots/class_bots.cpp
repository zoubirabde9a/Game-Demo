/* Class bots (server/bots.cpp): the keys a bot of each class after the
   first three presses, one file each, and what they share (bot_dangers.cpp:
   the telegraphs a melee bot keeps out of). */

#include "bot_dangers.cpp"
#include "ranger.cpp"
#include "berserker.cpp"
#include "shadowblade.cpp"
#include "stormcaller.cpp"
#include "duelist.cpp"

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

// NOTE(zoubir): the damage class a bot in slot PlayerIndex plays. Its
// seat is how many damage players sit in the slots before it, counting
// only slots in use, as bots join in any slots (the balance probe's three
// sit in slots 5 to 7). The first damage seat is always the Fire Mage, so the balance probe's party
// of three (tools/dungeon_balance.cpp) stays the one its numbers were
// tuned against; the seats after it go round the other damage classes
// that have a kit. Developer builds: GAME_BOT_DAMAGE names a class
// ("ranger") that every damage bot plays, to measure one class
internal u32
BotDamageClass(app_state *AppState, u32 PlayerIndex)
{
#if APP_DEV
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // getenv: read, never kept
#endif
    char *Forced = getenv("GAME_BOT_DAMAGE");
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    for (u32 Role = 0; Forced && Forced[0] && Role < PlayerRole_Count; ++Role)
    {
        char *Name = GetRoleDef(Role)->Name;
        u32 At = 0;
        while (Name[At] && Forced[At] && (Name[At] | 32) == (Forced[At] | 32))
        {
            ++At;
        }
        if (!Name[At] && !Forced[At] && IsDamageRole(Role) && RoleHasKit(Role))
        {
            return Role;
        }
    }
#endif
    u32 Seat = 0;
    for (u32 Before = 0; Before < PlayerIndex && Before < MAX_PLAYERS; ++Before)
    {
        Seat += (AppState->Players[Before].Active &&
                 BotWantedRole[Before] == PlayerRole_Damage) ? 1 : 0;
    }
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
