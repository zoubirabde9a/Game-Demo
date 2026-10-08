/* Stormcaller bots (server/bots.cpp, BotRoleButtons): which of the class's keys
   a bot presses this tick, as NetButton bits; *Held may change its
   movement and *Pick becomes the unit a spell goes at.

   For now it keeps its distance like every ranged bot (BotThink holds it
   BOT_STRIKER_RANGE off its target), fires Spark on cooldown at what it
   fights and vents the Charge with Thunderclap before it caps, so a party
   with a Stormcaller bot still has its damage; the rest of the kit comes
   with the bot's own turn. */

internal u32
BotStormcallerButtons(bot_brain *Bot, app_state *AppState, world_entity *Self, world_entity *Target,
                      float Distance, v2 Direction, u32 *Held, u16 *Pick)
{
    player_slot *Slot = &AppState->Players[Self->PlayerIndex];
    // NOTE(zoubir): X and W are the Stormcaller's: the game's fireball and
    // shockwave presses from BotThink would cast them at random
    *Held &= ~(u32)(NetButton_Fireball | NetButton_Shockwave);
    if (!Target || Target->Type != EntityType_Monster || IsPlayerCasting(Self))
    {
        return 0;
    }
    u32 Result = 0;
    *Pick = (u16)(Target->ID + 1);
    // NOTE(zoubir): vents with Thunderclap before the Charge caps
    if (Slot->RoleCooldowns[4] <= 0.f && Distance < THUNDERCLAP_RANGE &&
        Slot->Stormcaller.Charge >= 75.f)
    {
        Result |= NetButton_Shockwave;
    }
    else if (Slot->RoleCooldowns[5] <= 0.f && Distance < SPARK_RANGE)
    {
        Result |= NetButton_Fireball;
    }
    return Result;
}
