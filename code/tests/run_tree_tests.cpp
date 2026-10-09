/* Run talent tests (dungeon_tests.cpp): the run talents a class tree
   holds do what they say (sim/dungeon/run_tree/). How the tree rolls them
   is class_tree_tests.cpp. */

// NOTE(zoubir): the fixed talents of a few classes do what they say:
// armor, a hit while low, a kill that heals and one that gives time
// back, an ally's aura, and threat
internal void
TestRunTalentsDoWhatTheySay()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    player_slot *Ally = &AppState->Players[1];
    world_entity *Player = Slot->Entity;

    SetPlayerRole(AppState, Slot, PlayerRole_Tank);
    float Taken = RoleTalentTakenScale(Slot, Player);
    SetClassRunModRank(Slot, RunMod_Stoneform, 3);
    float Less = Taken * (1.f - 3.f * RunModDefs[RunMod_Stoneform].PerRank[0]);
    Check(RoleTalentTakenScale(Slot, Player) > Less - 0.001f &&
          RoleTalentTakenScale(Slot, Player) < Less + 0.001f);
    Check(RunThreatScale(Slot) == 1.f);
    SetClassRunModRank(Slot, RunMod_Menacing, 2);
    Check(RunThreatScale(Slot) > 1.39f && RunThreatScale(Slot) < 1.41f);

    // NOTE(zoubir): Shield Brother: the ally beside the tank takes less
    Check(Ally->Active && Ally->Entity);
    Ally->Entity->Position = Player->Position + V3(40.f, 0.f, 0.f);
    SetClassRunModRank(Slot, RunMod_ShieldBrother, 3);
    UpdateRunTrees(AppState, AppState->Dungeon, 1.f / 60.f);
    float Aura = 3.f * RunModDefs[RunMod_ShieldBrother].PerRank[0];
    Check(Ally->RunAuraArmor > Aura - 0.001f && Ally->RunAuraArmor < Aura + 0.001f);
    Check(Slot->RunAuraArmor == 0.f);
    Ally->Entity->Position = Player->Position + V3(RUN_AURA_REACH + 50.f, 0.f, 0.f);
    UpdateRunTrees(AppState, AppState->Dungeon, 1.f / 60.f);
    Check(Ally->RunAuraArmor == 0.f);

    // NOTE(zoubir): Cornered Beast: more damage only under 40% health
    SetPlayerRole(AppState, Slot, PlayerRole_Berserker);
    world_entity Foe = {};
    Foe.MaxHp = 100.f;
    Foe.Hp = 60.f;
    SetClassRunModRank(Slot, RunMod_CorneredBeast, 3);
    Slot->RunHits = 0;
    Check(RunDealtScale(AppState, Slot, &Foe) == 1.f);
    Player->Hp = 0.3f * Player->MaxHp;
    float Harder = 1.f + 3.f * RunModDefs[RunMod_CorneredBeast].PerRank[0];
    Check(RunDealtScale(AppState, Slot, &Foe) > Harder - 0.001f &&
          RunDealtScale(AppState, Slot, &Foe) < Harder + 0.001f);

    // NOTE(zoubir): Gorge: a kill heals
    SetClassRunModRank(Slot, RunMod_Gorge, 3);
    float Before = Player->Hp;
    OnRunKill(AppState, Slot);
    Check(Player->Hp > Before + 1.f);

    // NOTE(zoubir): Trophy: a Ranger's kill takes time off its spells
    SetPlayerRole(AppState, Slot, PlayerRole_Ranger);
    SetClassRunModRank(Slot, RunMod_Trophy, 2);
    Slot->RoleCooldowns[0] = 5.f;
    OnRunKill(AppState, Slot);
    float Left = 5.f - 2.f * RunModDefs[RunMod_Trophy].PerRank[0];
    Check(Slot->RoleCooldowns[0] > Left - 0.001f && Slot->RoleCooldowns[0] < Left + 0.001f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the talents that follow the run: Opening Salvo early in a
// fight only, Rising Glory with the rooms cleared, Lifeline once a fight
internal void
TestRunTalentsFollowTheRun()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    world_entity *Player = Slot->Entity;
    dungeon_run *Run = AppState->Dungeon;
    // NOTE(zoubir): the Frost Mage's pools hold Opening Salvo and Rising
    // Glory, the Bulwark's Lifeline
    SetPlayerRole(AppState, Slot, PlayerRole_FrostMage);
    world_entity Foe = {};
    Foe.MaxHp = 100.f;
    Foe.Hp = 60.f;
    Slot->RunHits = 0;

    u32 At = SetClassRunModRank(Slot, RunMod_OpeningSalvo, 2);
    Check(At < Talent_Count);
    Run->FightingRoom = 2;
    Run->MeterSeconds = 2.f;
    float Early = 1.f + 2.f * RunModDefs[RunMod_OpeningSalvo].PerRank[0];
    Check(RunDealtScale(AppState, Slot, &Foe) > Early - 0.001f &&
          RunDealtScale(AppState, Slot, &Foe) < Early + 0.001f);
    Run->MeterSeconds = RUN_VANGUARD_SECONDS + 1.f;
    Check(RunDealtScale(AppState, Slot, &Foe) == 1.f);
    Slot->Ranks[At] = 0;

    At = SetClassRunModRank(Slot, RunMod_RisingGlory, 2);
    Check(At < Talent_Count);
    AppState->DungeonRoomsCleared = 4;
    float Glory = 1.f + 4.f * 2.f * RunModDefs[RunMod_RisingGlory].PerRank[0];
    Check(RunDealtScale(AppState, Slot, &Foe) > Glory - 0.001f &&
          RunDealtScale(AppState, Slot, &Foe) < Glory + 0.001f);
    AppState->DungeonRoomsCleared = 40;
    float Most = 1.f + RUN_GLORY_ROOMS * 2.f * RunModDefs[RunMod_RisingGlory].PerRank[0];
    Check(RunDealtScale(AppState, Slot, &Foe) < Most + 0.001f);
    Slot->Ranks[At] = 0;

    SetPlayerRole(AppState, Slot, PlayerRole_Tank);
    At = SetClassRunModRank(Slot, RunMod_Lifeline, 1);
    Check(At < Talent_Count);
    Player->Hp = 0.2f * Player->MaxHp;
    UpdateRunTrees(AppState, Run, 1.f / 60.f);
    Check(Player->Hp > 0.3f * Player->MaxHp && Slot->RunLifelineSpent);
    Player->Hp = 0.2f * Player->MaxHp;
    UpdateRunTrees(AppState, Run, 1.f / 60.f);
    Check(Player->Hp < 0.21f * Player->MaxHp);
    // NOTE(zoubir): the next fight it catches again
    Run->MeterFight++;
    UpdateRunTrees(AppState, Run, 1.f / 60.f);
    Check(Player->Hp > 0.3f * Player->MaxHp);
    Run->FightingRoom = 0;
    DestroyCryptWorld(&Crypt);
}

internal void
RunRunTreeTests()
{
    TestRunTalentsDoWhatTheySay();
    TestRunTalentsFollowTheRun();
}
