/* Class tree tests (dungeon_tests.cpp): every class's tree has two named
   branches of the shape class_tree.cpp lays out, two base spells and a
   pair of spells in each branch, so no build casts more than four; a
   spell of a pair locks the other; one branch and the other's spell fit
   in the run's points but both branches do not; and the stat talents
   change what they say (sim/dungeon/role_stats.cpp). */

// NOTE(zoubir): the catalog talent that unlocks a spell of Role, or
// CLASS_TALENTS when Talent unlocks none
inline bool32
TalentUnlocksSpell(u32 Role, u32 Catalog)
{
    bool32 Result = false;
    for(u32 Key = 0; Key < ROLE_KEYS; Key++)
    {
        role_spell *Spell = &RoleSpells[Role][Key];
        Result = Result || (Spell->Name && Spell->Unlock == Catalog + 1);
    }
    return Result;
}

// NOTE(zoubir): the most ranks a fixed slot of Role takes
inline u32
FixedSlotRanks(u32 Role, u32 Index)
{
    u32 Content = ClassTrees[Role].Fixed[Index];
    u32 Catalog = ClassContentTalent(Content);
    u32 Result = ClassSlotShape(Index)->Ranks;
    if (Catalog < CLASS_TALENTS)
    {
        Result = Minimum(Result, Maximum(1u, RoleTalentDefs[Role][Catalog].MaxLevel));
    }
    return Result;
}

internal void
TestClassTreesMatchTheShape()
{
    for(u32 Index = 0; Index < ROLE_TALENTS; Index++)
    {
        class_slot_shape *Shape = ClassSlotShape(Index);
        Check(TalentDefs[Talent_RoleFirst + Index].Tier == Shape->Tier &&
              TalentDefs[Talent_RunFirst + Index].Tier == Shape->Tier);
        Check(TalentDefs[Talent_RoleFirst + Index].Column == Shape->Column &&
              TalentDefs[Talent_RunFirst + Index].Column == Shape->Column);
        // NOTE(zoubir): the second branch's ranks travel in 2 bits
        Check(Shape->Ranks <= 3);
    }
    for(u32 Role = 0; Role < PlayerRole_Count; Role++)
    {
        class_tree_def *Def = &ClassTrees[Role];
        // NOTE(zoubir): two base spells, the Fire Mage's fireball one of them
        u32 Base = RoleDropsFireball(Role) ? 0 : 1;
        for(u32 Key = 0; Key < ROLE_KEYS; Key++)
        {
            role_spell *Spell = &RoleSpells[Role][Key];
            Base += Spell->Name && !Spell->Unlock;
            if (Spell->Name && Spell->Unlock)
            {
                // NOTE(zoubir): every other spell sits in a pair, never in a
                // fixed or wild slot, so nothing gives a fifth
                u32 Found = 0;
                for(u32 Index = 0; Index < CLASS_TREE_SLOTS; Index++)
                {
                    if (ClassContentTalent(Def->Fixed[Index]) == Spell->Unlock - 1)
                    {
                        Check(ClassSlotShape(Index)->Kind == ClassSlot_Spell);
                        Found++;
                    }
                }
                Check(Found == 1);
            }
        }
        Check(Base == 2);
        u32 BranchPoints[2] = {};
        for(u32 Branch = 0; Branch < 2; Branch++)
        {
            Check(Def->BranchNames[Branch] && Def->BranchNames[Branch][0]);
            u32 Pool = 0;
            for(; Pool < CLASS_POOL_MOST && Def->Pools[Branch][Pool]; Pool++)
            {
                u32 Catalog = ClassContentTalent(Def->Pools[Branch][Pool]);
                Check(Catalog == CLASS_TALENTS || !TalentUnlocksSpell(Role, Catalog));
                Check(Catalog == CLASS_TALENTS || RoleTalentDefs[Role][Catalog].Name[0]);
            }
            Check(Pool >= 5);
        }
        for(u32 Index = 0; Index < CLASS_TREE_SLOTS; Index++)
        {
            u32 Kind = ClassSlotShape(Index)->Kind;
            bool32 Fixed = Kind == ClassSlot_Fixed || Kind == ClassSlot_Spell;
            Check((Def->Fixed[Index] != 0) == Fixed);
            u32 Catalog = ClassContentTalent(Def->Fixed[Index]);
            Check(!Fixed || Catalog < CLASS_TALENTS || IsClassRunContent(Def->Fixed[Index]));
            Check(!Fixed || Catalog == CLASS_TALENTS || RoleTalentDefs[Role][Catalog].Name[0]);
            // NOTE(zoubir): a spell slot holds the most points of its pair
            if (Kind != ClassSlot_Spell || !(Index & 1))
            {
                BranchPoints[Index / ROLE_TALENTS] += Fixed ? FixedSlotRanks(Role, Index) :
                    ClassSlotShape(Index)->Ranks;
            }
        }
        // NOTE(zoubir): one branch whole and the other's first two tiers
        // fit in the run's points; both branches whole do not
        for(u32 Branch = 0; Branch < 2; Branch++)
        {
            Check(BranchPoints[Branch] + 3 <= PLAYER_MAX_LEVEL - 1);
        }
        Check(BranchPoints[0] + BranchPoints[1] > PLAYER_MAX_LEVEL - 1);
    }
}

// NOTE(zoubir): for many seeds and every class: fixed slots hold the
// class's content, a wild slot something of its branch's pool, the
// keystone a keystone the class's role kind may roll, nothing twice, and
// the same seed and class roll the same tree
internal void
TestClassTreesRollFromTheirPools()
{
    for(u32 Role = 0; Role < PlayerRole_Count; Role++)
    {
        class_tree_def *Def = &ClassTrees[Role];
        u32 Kind = 1u << RoleKindOf(Role);
        u32 Different = 0;
        u8 First[CLASS_TREE_SLOTS];
        RollClassTree(0, Role, First);
        for(u32 Seed = 0; Seed < 300; Seed++)
        {
            u8 Tree[CLASS_TREE_SLOTS];
            u8 Again[CLASS_TREE_SLOTS];
            RollClassTree(Seed * 7919u, Role, Tree);
            RollClassTree(Seed * 7919u, Role, Again);
            Check(memcmp(Tree, Again, sizeof(Tree)) == 0);
            for(u32 Index = 0; Index < CLASS_TREE_SLOTS; Index++)
            {
                u32 Shape = ClassSlotShape(Index)->Kind;
                if (Shape == ClassSlot_Keystone)
                {
                    u32 Mod = ClassContentRunMod(Tree[Index]);
                    Check(Mod && RunModDefs[Mod].Keystone && (RunModDefs[Mod].Kinds & Kind));
                }
                else if (Shape == ClassSlot_Wild)
                {
                    bool32 InPool = false;
                    u8 *Pool = Def->Pools[Index / ROLE_TALENTS];
                    for(u32 Entry = 0; Entry < CLASS_POOL_MOST && Pool[Entry]; Entry++)
                    {
                        InPool = InPool || Pool[Entry] == Tree[Index];
                    }
                    Check(InPool);
                }
                else
                {
                    Check(Tree[Index] == Def->Fixed[Index]);
                }
                for(u32 Other = Index + 1; Other < CLASS_TREE_SLOTS; Other++)
                {
                    Check(Tree[Other] != Tree[Index]);
                }
            }
            Different += memcmp(Tree, First, sizeof(Tree)) != 0;
        }
        // NOTE(zoubir): the seed matters
        Check(Different > 250);
    }
}

// NOTE(zoubir): a branch's tiers open on its own points; a spell of a
// pair takes a point and locks the other; a wild slot rolls again with a
// new run and keeps its points; another class gives every point back
internal void
TestClassTreePointRules()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_FrostMage);
    Slot->Level = PLAYER_MAX_LEVEL;
    u32 NovaTalent = ClassTalentId(Slot, FrostMageTalent_FrostNova);
    u32 BlizzardTalent = ClassTalentId(Slot, FrostMageTalent_Blizzard);
    Check(NovaTalent == Talent_RoleFirst + 2 && BlizzardTalent == Talent_RoleFirst + 3);
    // NOTE(zoubir): the base spells cast from the start, a pair's not yet
    Check(RoleSpellLearned(Slot, 5) && RoleSpellLearned(Slot, 1));
    Check(!RoleSpellLearned(Slot, 4) && !RoleSpellLearned(Slot, 0));
    Check(!LearnTalent(AppState, 0, NovaTalent));
    Check(LearnTalent(AppState, 0, Talent_RoleFirst + 0));
    Check(LearnTalent(AppState, 0, Talent_RoleFirst + 1));
    // NOTE(zoubir): the other branch's tiers stay shut
    Check(!LearnTalent(AppState, 0, Talent_RunFirst + 2));
    Check(LearnTalent(AppState, 0, NovaTalent));
    Check(RoleSpellLearned(Slot, 4) && !RoleSpellLearned(Slot, 0));
    Check(CanLearnTalent(Slot, BlizzardTalent) == TalentRefusal_OtherSpell);
    Check(!LearnTalent(AppState, 0, BlizzardTalent));
    // NOTE(zoubir): a spell's second rank strengthens it, if it has one
    Check(!LearnTalent(AppState, 0, NovaTalent));

    // NOTE(zoubir): a new run rolls the wild slots again; points stay
    u32 Seed = Slot->TreeSeed;
    u8 Before[CLASS_TREE_SLOTS];
    memcpy(Before, ClassTreeOf(Slot), sizeof(Before));
    AppState->NextMapVoted = true;
    AppState->NextMap = MapId_Depths;
    StartNextRoundMap(AppState, &Crypt.Arena);
    Check(Slot->TreeSeed != Seed && Slot->TreeSeed <= RUN_SEED_MASK);
    Check(Slot->Ranks[Talent_RoleFirst + 1] == 1 && Slot->Ranks[NovaTalent] == 1);
    u32 Same = 0;
    for(u32 Index = 0; Index < CLASS_TREE_SLOTS; Index++)
    {
        Same += ClassSlotIsWild(ClassTreeTalent(Index)) && ClassContentAt(Slot, Index) == Before[Index];
    }
    Check(Same < 8);

    // NOTE(zoubir): a new class gives every point back
    SetPlayerRole(AppState, Slot, PlayerRole_Berserker);
    Check(TalentPointsSpent(Slot, TalentBranch_Role) == 0 && TalentPointsSpent(Slot, TalentBranch_Run) == 0);
    DestroyCryptWorld(&Crypt);
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
    SetClassTalentRank(Slot, BerserkerTalent_ThickHide, 1);
    RefreshRoleHealth(AppState, Slot);
    float Raised = Base * (1.f + RoleStatPerRank[RoleStat_Vitality]);
    Check(Player->MaxHp > Raised - 0.01f && Player->MaxHp < Raised + 0.01f);
    Check(Player->Hp > Raised - 50.01f && Player->Hp < Raised - 49.99f);
    ResetTalents(AppState, 0);
    Check(Player->MaxHp == Base && Player->Hp > Base - 50.01f && Player->Hp < Base - 49.99f);

    SetPlayerRole(AppState, Slot, PlayerRole_Ranger);
    float Cooldown = RoleSpellCooldown(Slot, 1);
    SetClassTalentRank(Slot, RangerTalent_SteadyHands, 2);
    float Faster = Cooldown * (1.f - 2.f * RoleStatPerRank[RoleStat_Haste]);
    Check(RoleSpellCooldown(Slot, 1) > Faster - 0.01f && RoleSpellCooldown(Slot, 1) < Faster + 0.01f);
    DestroyCryptWorld(&Crypt);
}

internal void
RunClassTreeTests()
{
    TestClassTreesMatchTheShape();
    TestClassTreesRollFromTheirPools();
    TestClassTreePointRules();
    TestStatTalents();
}
