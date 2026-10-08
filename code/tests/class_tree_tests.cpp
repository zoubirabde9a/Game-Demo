/* Class tree tests (dungeon_tests.cpp): every class's twelve talents sit
   in the role branch's shape and fill at the dungeon's top level
   (sim/dungeon/role_talents.cpp), and the stat talents change what they
   say (sim/dungeon/role_stats.cpp). */

// NOTE(zoubir): each class's row has its slot's tier, column and ranks,
// a name, and a stat only on a stat slot; the branch holds a point for
// every level from 2 to the top
internal void
TestClassTreesMatchTheShape()
{
    u32 Points = 0;
    for(u32 Index = 0; Index < ROLE_TALENTS; Index++)
    {
        Points += TalentDefs[Talent_RoleFirst + Index].MaxLevel;
    }
    Check(Points == PLAYER_MAX_LEVEL - 1);
    for(u32 Role = 0; Role < PlayerRole_Count; Role++)
    {
        for(u32 Index = 0; Index < ROLE_TALENTS; Index++)
        {
            talent_def *Shape = &TalentDefs[Talent_RoleFirst + Index];
            talent_def *Def = &RoleTalentDefs[Role][Index];
            Check(Def->Branch == TalentBranch_Role && Def->Tier == Shape->Tier &&
                  Def->Column == Shape->Column && Def->MaxLevel == Shape->MaxLevel);
            Check(Def->Name && Def->Name[0]);
            bool32 StatSlot = Index == 6 || Index == 7 || Index == 9 || Index == 10;
            Check((RoleTalentStats[Role][Index] != RoleStat_None) == StatSlot);
        }
    }
}

// NOTE(zoubir): a health rank raises the body's health at once and keeps
// what it was missing; a reset takes it back. A cooldown rank shortens
// every class spell, a damage rank raises every hit
internal void
TestStatTalents()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    world_entity *Player = Slot->Entity;

    SetPlayerRole(AppState, Slot, PlayerRole_Berserker);
    Slot->Level = PLAYER_MAX_LEVEL;
    float Base = GetRoleDef(PlayerRole_Berserker)->MaxHp;
    Check(Player->MaxHp == Base);
    Player->Hp = Base - 50.f;
    // NOTE(zoubir): two points in each of the first two tiers open tier 2,
    // where Thick Hide is
    Check(LearnTalent(AppState, 0, Talent_RoleFirst + BerserkerTalent_Brutality));
    Check(LearnTalent(AppState, 0, Talent_RoleFirst + BerserkerTalent_Brutality));
    Check(LearnTalent(AppState, 0, Talent_RoleFirst + BerserkerTalent_UnbridledWrath));
    Check(LearnTalent(AppState, 0, Talent_RoleFirst + BerserkerTalent_SweepingStrikes));
    Check(LearnTalent(AppState, 0, Talent_RoleFirst + 6));
    float Raised = Base * (1.f + RoleStatPerRank[RoleStat_Vitality]);
    Check(Player->MaxHp > Raised - 0.01f && Player->MaxHp < Raised + 0.01f);
    Check(Player->Hp > Raised - 50.01f && Player->Hp < Raised - 49.99f);
    ResetTalents(AppState, 0);
    Check(Player->MaxHp == Base && Player->Hp > Base - 50.01f && Player->Hp < Base - 49.99f);

    SetPlayerRole(AppState, Slot, PlayerRole_Ranger);
    float Cooldown = RoleSpellCooldown(Slot, 0);
    Slot->Ranks[Talent_RoleFirst + 9] = 4;
    float Faster = Cooldown * (1.f - 4.f * RoleStatPerRank[RoleStat_Haste]);
    Check(RoleSpellCooldown(Slot, 0) > Faster - 0.01f && RoleSpellCooldown(Slot, 0) < Faster + 0.01f);

    float Dealt = RoleTalentDealtScale(Slot, Player);
    Slot->Ranks[Talent_RoleFirst + 6] = 2;
    float Harder = Dealt * (1.f + 2.f * RoleStatPerRank[RoleStat_Damage]);
    Check(RoleTalentDealtScale(Slot, Player) > Harder - 0.001f &&
          RoleTalentDealtScale(Slot, Player) < Harder + 0.001f);

    float Run = RunSpeedScale(AppState, Player);
    Slot->Ranks[Talent_RoleFirst + 10] = 4;
    Check(RunSpeedScale(AppState, Player) > Run * 1.11f);
    DestroyCryptWorld(&Crypt);
}

internal void
RunClassTreeTests()
{
    TestClassTreesMatchTheShape();
    TestStatTalents();
}
