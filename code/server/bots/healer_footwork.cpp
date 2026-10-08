/* Healer footwork (server/bots.cpp): how a bot in the healer column moves,
   the Mender's (server/bots.cpp, BotRoleButtons) and the Druid's
   (server/bots/druid.cpp) alike. */

// NOTE(zoubir): how a healer-column bot moves (the Mender's and the
// Druid's, server/bots/druid.cpp): out of the melee, it backs off what
// comes close; an ally down in the fight, it walks to the body and stands
// over it until it is up (sim/dungeon/revive.cpp), its spells still
// casting on the way
internal void
BotHealerFootwork(app_state *AppState, world_entity *Self, world_entity *Target, float Distance,
                  v2 Direction, u32 *Held)
{
    if (Target && Distance < 180.f)
    {
        *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down |
                        NetButton_Sword);
        *Held |= NetButtonsToward(-Direction);
    }
    world_entity *Downed = 0;
    float DownedGap = 0.f;
    for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; ++SlotIndex)
    {
        player_slot *Other = &AppState->Players[SlotIndex];
        world_entity *Body = Other->Entity;
        float Gap = Body ? Length(Body->Position.XY - Self->Position.XY) : 0.f;
        if (Other->Active && Body && Body != Self && IsDeadPlayer(Body) &&
            AppState->Dungeon->FightingRoom &&
            RoomAtPosition(&AppState->World, Body->Position.XY) == AppState->Dungeon->FightingRoom &&
            (!Downed || Gap < DownedGap))
        {
            Downed = Body;
            DownedGap = Gap;
        }
    }
    if (Downed)
    {
        *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down |
                        NetButton_Sword);
        if (DownedGap > 0.5f * REVIVE_RADIUS)
        {
            *Held |= NetButtonsToward(DirectionTo(Downed->Position.XY - Self->Position.XY));
        }
    }
}
