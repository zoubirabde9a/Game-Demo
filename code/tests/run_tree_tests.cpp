/* Second tree tests (dungeon_tests.cpp): every class's second tree has
   its fixed talents in the fixed slots and rolls fitting ones in the
   wild slots, never two of one effect; a new run rolls the wild slots
   again and keeps the points; and the talents do what they say
   (sim/dungeon/run_tree/). */

// NOTE(zoubir): for many seeds and every class: fixed slots hold the
// class's talent, wild ones a talent the class's role kind may roll (a
// keystone in the keystone slot), no minor wild effect twice, and the same seed and
// class roll the same tree
internal void
TestRunTreesRollFittingTalents()
{
    u32 Points = 0;
    for(u32 Index = 0; Index < RUN_TALENTS; Index++)
    {
        talent_def *Shape = &TalentDefs[Talent_RunFirst + Index];
        Check(Shape->Branch == TalentBranch_Run && Shape->MaxLevel <= 3);
        Points += Shape->MaxLevel;
    }
    // NOTE(zoubir): the two trees share the points, so neither run fills both
    Check(Points < PLAYER_MAX_LEVEL - 1);
    for(u32 Role = 0; Role < PlayerRole_Count; Role++)
    {
        u32 Kind = 1u << RoleKindOf(Role);
        Check(RunTrees[Role].Name && RunTrees[Role].Name[0]);
        u32 Different = 0;
        for(u32 Seed = 0; Seed < 400; Seed++)
        {
            u8 Tree[RUN_TALENTS];
            u8 Again[RUN_TALENTS];
            RollRunTree(Seed * 7919u, Role, Tree);
            RollRunTree(Seed * 7919u, Role, Again);
            Check(memcmp(Tree, Again, sizeof(Tree)) == 0);
            for(u32 Index = 0; Index < RUN_TALENTS; Index++)
            {
                run_mod_def *Def = &RunModDefs[Tree[Index]];
                Check(Tree[Index] > RunMod_None && Tree[Index] < RunMod_Count && Def->Name[0]);
                if (RunSlotWild[Index])
                {
                    Check(Tree[Index] >= RunMod_WildFirst && Tree[Index] <= RunMod_WildLast);
                    Check((Def->Kinds & Kind) != 0);
                    Check((Def->Keystone != 0) == (Index == RUN_KEYSTONE_SLOT));
                }
                else
                {
                    Check(Tree[Index] == RunTrees[Role].Fixed[Index]);
                }
                for(u32 Other = 0; Other < RUN_TALENTS; Other++)
                {
                    if (RunSlotWild[Index] && Index != RUN_KEYSTONE_SLOT && Other != Index &&
                        Other != RUN_KEYSTONE_SLOT)
                    {
                        Check(RunModDefs[Tree[Other]].Effect[0] != Def->Effect[0]);
                    }
                }
            }
            u8 First[RUN_TALENTS];
            RollRunTree(0, Role, First);
            Different += memcmp(Tree, First, sizeof(Tree)) != 0;
        }
        // NOTE(zoubir): the seed matters
        Check(Different > 300);
    }
}

// NOTE(zoubir): the second tree takes points only in a run, opens its
// tiers on its own points, and a new run rolls its wild slots again
// with the points still in them
internal void
TestRunTreeRollsAgainEachRun()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Tank);
    Slot->Level = PLAYER_MAX_LEVEL;
    // NOTE(zoubir): class tree points do not open the second tree's tiers
    Check(LearnTalent(AppState, 0, Talent_RoleFirst + TankTalent_IronSkin));
    Check(LearnTalent(AppState, 0, Talent_RoleFirst + TankTalent_IronSkin));
    Check(!LearnTalent(AppState, 0, Talent_RunFirst + 2));
    Check(LearnTalent(AppState, 0, Talent_RunFirst + 1));
    Check(LearnTalent(AppState, 0, Talent_RunFirst + 0));
    Check(LearnTalent(AppState, 0, Talent_RunFirst + 2));
    u32 Seed = Slot->TreeSeed;
    u8 Before[RUN_TALENTS];
    memcpy(Before, RunTreeOf(Slot), sizeof(Before));

    AppState->NextMapVoted = true;
    AppState->NextMap = MapId_Depths;
    StartNextRoundMap(AppState, &Crypt.Arena);
    Check(Slot->TreeSeed != Seed && Slot->TreeSeed <= RUN_SEED_MASK);
    Check(Slot->Ranks[Talent_RunFirst + 1] == 1 && Slot->Ranks[Talent_RunFirst + 2] == 1);
    Check(RunModAtSlot(Slot, 0) == RunMod_Menacing);
    // NOTE(zoubir): another seed may roll the same talent in a slot or
    // two, but not in all six
    u32 Same = 0;
    for(u32 Index = 0; Index < RUN_TALENTS; Index++)
    {
        Same += RunSlotWild[Index] && RunModAtSlot(Slot, Index) == Before[Index];
    }
    Check(Same < 6);

    // NOTE(zoubir): a new class gives both trees' points back
    SetPlayerRole(AppState, Slot, PlayerRole_Berserker);
    Check(TalentPointsSpent(Slot, TalentBranch_Run) == 0);

    // NOTE(zoubir): and outside a run the tree takes none
    AppState->NextMapVoted = true;
    AppState->NextMap = MapId_Arena;
    StartNextRoundMap(AppState, &Crypt.Arena);
    Slot->Level = 5;
    Check(!LearnTalent(AppState, 0, Talent_RunFirst + 1));
    DestroyCryptWorld(&Crypt);
}

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
    Slot->Ranks[Talent_RunFirst + 3] = 3;
    float Less = Taken * (1.f - 3.f * RunModDefs[RunMod_Stoneform].PerRank[0]);
    Check(RoleTalentTakenScale(Slot, Player) > Less - 0.001f &&
          RoleTalentTakenScale(Slot, Player) < Less + 0.001f);
    Check(RunThreatScale(Slot) == 1.f);
    Slot->Ranks[Talent_RunFirst + 0] = 2;
    Check(RunThreatScale(Slot) > 1.39f && RunThreatScale(Slot) < 1.41f);

    // NOTE(zoubir): Shield Brother: the ally beside the tank takes less
    Check(Ally->Active && Ally->Entity);
    Ally->Entity->Position = Player->Position + V3(40.f, 0.f, 0.f);
    Slot->Ranks[Talent_RunFirst + 8] = 3;
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
    Slot->Ranks[Talent_RunFirst + 0] = 3;
    Slot->RunHits = 0;
    Check(RunDealtScale(AppState, Slot, &Foe) == 1.f);
    Player->Hp = 0.3f * Player->MaxHp;
    float Harder = 1.f + 3.f * RunModDefs[RunMod_CorneredBeast].PerRank[0];
    Check(RunDealtScale(AppState, Slot, &Foe) > Harder - 0.001f &&
          RunDealtScale(AppState, Slot, &Foe) < Harder + 0.001f);

    // NOTE(zoubir): Gorge: a kill heals
    Slot->Ranks[Talent_RunFirst + 3] = 3;
    float Before = Player->Hp;
    OnRunKill(AppState, Slot);
    Check(Player->Hp > Before + 1.f);

    // NOTE(zoubir): Trophy: a Ranger's kill takes time off its spells
    SetPlayerRole(AppState, Slot, PlayerRole_Ranger);
    Slot->Ranks[Talent_RunFirst + 8] = 2;
    Slot->RoleCooldowns[0] = 5.f;
    OnRunKill(AppState, Slot);
    float Left = 5.f - 2.f * RunModDefs[RunMod_Trophy].PerRank[0];
    Check(Slot->RoleCooldowns[0] > Left - 0.001f && Slot->RoleCooldowns[0] < Left + 0.001f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): a seed that rolls Mod in one of Role's wild slots, and
// which slot, for a test that needs a wild talent; 0 seed if none
internal u32
SeedRolling(u32 Role, u32 Mod, u32 *SlotOut)
{
    for(u32 Seed = 1; Seed <= RUN_SEED_MASK; Seed++)
    {
        u8 Tree[RUN_TALENTS];
        RollRunTree(Seed, Role, Tree);
        for(u32 Index = 0; Index < RUN_TALENTS; Index++)
        {
            if (RunSlotWild[Index] && Tree[Index] == Mod)
            {
                *SlotOut = Index;
                return Seed;
            }
        }
    }
    return 0;
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
    SetPlayerRole(AppState, Slot, PlayerRole_Ranger);
    world_entity Foe = {};
    Foe.MaxHp = 100.f;
    Foe.Hp = 60.f;
    Slot->RunHits = 0;

    u32 At = 0;
    Slot->TreeSeed = SeedRolling(PlayerRole_Ranger, RunMod_OpeningSalvo, &At);
    Check(Slot->TreeSeed != 0);
    Slot->Ranks[Talent_RunFirst + At] = 2;
    Run->FightingRoom = 2;
    Run->MeterSeconds = 2.f;
    float Early = 1.f + 2.f * RunModDefs[RunMod_OpeningSalvo].PerRank[0];
    Check(RunDealtScale(AppState, Slot, &Foe) > Early - 0.001f &&
          RunDealtScale(AppState, Slot, &Foe) < Early + 0.001f);
    Run->MeterSeconds = RUN_VANGUARD_SECONDS + 1.f;
    Check(RunDealtScale(AppState, Slot, &Foe) == 1.f);
    Slot->Ranks[Talent_RunFirst + At] = 0;

    Slot->TreeSeed = SeedRolling(PlayerRole_Ranger, RunMod_RisingGlory, &At);
    Check(Slot->TreeSeed != 0);
    Slot->Ranks[Talent_RunFirst + At] = 2;
    AppState->DungeonRoomsCleared = 4;
    float Glory = 1.f + 4.f * 2.f * RunModDefs[RunMod_RisingGlory].PerRank[0];
    Check(RunDealtScale(AppState, Slot, &Foe) > Glory - 0.001f &&
          RunDealtScale(AppState, Slot, &Foe) < Glory + 0.001f);
    AppState->DungeonRoomsCleared = 40;
    float Most = 1.f + RUN_GLORY_ROOMS * 2.f * RunModDefs[RunMod_RisingGlory].PerRank[0];
    Check(RunDealtScale(AppState, Slot, &Foe) < Most + 0.001f);
    Slot->Ranks[Talent_RunFirst + At] = 0;

    Slot->TreeSeed = SeedRolling(PlayerRole_Ranger, RunMod_Lifeline, &At);
    Check(Slot->TreeSeed != 0);
    Slot->Ranks[Talent_RunFirst + At] = 1;
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
    TestRunTreesRollFittingTalents();
    TestRunTreeRollsAgainEachRun();
    TestRunTalentsDoWhatTheySay();
    TestRunTalentsFollowTheRun();
}
