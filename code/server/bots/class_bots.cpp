/* Class bots (server/bots.cpp): the keys a bot of each class after the
   first three presses, one file each. */

#include "ranger.cpp"
#include "berserker.cpp"
#include "shadowblade.cpp"

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
    }
    return 0;
}
