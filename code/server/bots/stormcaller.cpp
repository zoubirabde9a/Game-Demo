/* Stormcaller bots (server/bots.cpp, BotRoleButtons): which of the class's keys
   a bot presses this tick, as NetButton bits; *Held may change its
   movement and *Pick becomes the unit a spell goes at. A stub: no bot
   plays a Stormcaller until its first key has a spell. */

internal u32
BotStormcallerButtons(bot_brain *Bot, app_state *AppState, world_entity *Self, world_entity *Target,
                      float Distance, v2 Direction, u32 *Held, u16 *Pick)
{
    return 0;
}
