/* Bot class tests (server/bots/class_bots.cpp): which damage and healer
   class a bot asks for. Included by server_tests.cpp, which calls RunBotClassTests. */

// NOTE(zoubir): the first damage bot of a party plays the Fire Mage
// whatever slots the party sits in, so the balance probe (three bots in
// slots 5 to 7) keeps the party its numbers were tuned against
internal void
TestFirstDamageBotIsTheFireMage()
{
    app_state *AppState = (app_state *)calloc(1, sizeof(app_state));
    for (u32 Slot = 5; Slot < 8; ++Slot)
    {
        AppState->Players[Slot].Active = true;
    }
    Check(BotWantedRole[6] == PlayerRole_Damage);
    Check(BotDamageClass(AppState, 6) == PlayerRole_Damage);

    for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
    {
        AppState->Players[Slot].Active = Slot < 5;
    }
    Check(BotDamageClass(AppState, 2) == PlayerRole_Damage);
    // NOTE(zoubir): the second damage seat goes to another damage class
    // once one has a kit
    u32 Second = BotDamageClass(AppState, 3);
    Check(IsDamageRole(Second));
    Check(Second != PlayerRole_Damage || !RoleHasKit(PlayerRole_Ranger));
    free(AppState);
}

// NOTE(zoubir): the first healer bot plays the Mender, wherever the party
// sits; the second healer seat goes to the Druid
internal void
TestSecondHealerBotIsTheDruid()
{
    app_state *AppState = (app_state *)calloc(1, sizeof(app_state));
    for (u32 Slot = 5; Slot < 8; ++Slot)
    {
        AppState->Players[Slot].Active = true;
    }
    Check(BotWantedRole[5] == PlayerRole_Healer);
    Check(BotHealerClass(AppState, 5) == PlayerRole_Healer);
    for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
    {
        AppState->Players[Slot].Active = true;
    }
    Check(BotHealerClass(AppState, 1) == PlayerRole_Healer);
    Check(BotHealerClass(AppState, 5) == PlayerRole_Druid);
    free(AppState);
}

internal void
RunBotClassTests()
{
    TestFirstDamageBotIsTheFireMage();
    TestSecondHealerBotIsTheDruid();
}
