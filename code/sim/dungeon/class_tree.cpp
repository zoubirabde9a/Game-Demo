/* The class tree (role_talents.cpp, docs/class-trees.md): in a dungeon
   run each class has one tree of two branches, each named for a way to
   play the class. Branch 0 is Talent_RoleFirst on, branch 1 Talent_RunFirst
   on, twelve slots each, so ranks travel as they always did. Every branch
   has the same shape (ClassSlotShapes):

   tier 0   slot 2 spell               slot 3 the other spell
   tier 1   slot 0 core (fixed)        slot 1 wild (2)
   tier 2   slot 4 fixed (3)           slot 5 wild (2)
   tier 3   slot 6 wild (2)            slot 7 fixed (3)
   tier 4   slot 8 fixed (3)           slot 9 wild (2)
   tier 5   slot 10 capstone (1)       slot 11 wild keystone (1)

   A class casts two base spells from the start (role_spell.Unlock 0) and
   one spell of each branch's pair: a point in one spell of the pair locks
   the other (ClassSpellTakenBeside), so no build casts more than four.
   The pairs sit in the first tier, so the first two points can buy both
   spells: a class has its four by level 3, and the deeper tiers are where
   a build grows.

   What sits in a slot is a content code: a talent of the class's catalog
   (<class>_defs.cpp, CLASS_CONTENT_TALENT on) or a run talent
   (run_tree/run_mods.cpp, CLASS_CONTENT_RUN on). Fixed slots are the
   class's (ClassTrees, class_tree_defs.cpp); a wild slot rolls from its
   branch's pool, and the keystone from the run keystones that fit the
   class's role kind. The roll is a hash of the player's TreeSeed, the
   class and the slot, so a different class rolls a different tree and a
   new run (RerollRunTree) rolls again; points stay in their slot and buy
   whatever it rolled. Nothing appears twice in a tree, and no two run
   talents share their first effect unless a pool runs dry.

   The kits read a talent through RoleRank (role_talents.cpp), which adds
   the ranks of every slot holding it, wherever the roll put it. */

#define CLASS_TREE_SLOTS (2 * ROLE_TALENTS)
#define CLASS_CONTENT_TALENT 1
#define CLASS_CONTENT_RUN 64
#define CLASS_POOL_MOST 8
#define CLASS_KEYSTONE_SLOT 11
#define CLASS_CAPSTONE_SLOT 10

enum class_slot_kind
{
    ClassSlot_Fixed,
    ClassSlot_Spell,
    ClassSlot_Wild,
    ClassSlot_Keystone,
};

struct class_slot_shape
{
    u8 Kind;
    u8 Tier;
    u8 Column;
    // NOTE(zoubir): the most ranks the slot takes; a talent with fewer
    // fills it sooner
    u8 Ranks;
};

// NOTE(zoubir): by slot of a branch
global_variable class_slot_shape ClassSlotShapes[ROLE_TALENTS] =
{
    {ClassSlot_Fixed, 1, 0, 2}, {ClassSlot_Wild, 1, 1, 2},
    {ClassSlot_Spell, 0, 0, 2}, {ClassSlot_Spell, 0, 1, 2},
    {ClassSlot_Fixed, 2, 0, 3}, {ClassSlot_Wild, 2, 1, 2},
    {ClassSlot_Wild, 3, 0, 2}, {ClassSlot_Fixed, 3, 1, 3},
    {ClassSlot_Fixed, 4, 0, 3}, {ClassSlot_Wild, 4, 1, 2},
    {ClassSlot_Fixed, 5, 0, 1}, {ClassSlot_Keystone, 5, 1, 1},
};

struct class_tree_def
{
    char *BranchNames[2];
    // NOTE(zoubir): by slot of the whole tree, the fixed content; 0 in a
    // wild slot
    u8 Fixed[CLASS_TREE_SLOTS];
    // NOTE(zoubir): by branch, what its wild slots roll from, 0 ending it
    u8 Pools[2][CLASS_POOL_MOST];
};

#define CT(Talent) (u8)(CLASS_CONTENT_TALENT + (Talent))
#define RM(Mod) (u8)(CLASS_CONTENT_RUN + RunMod_##Mod)
#define CLASS_BRANCH(Core, SpellA, SpellB, Third, Fourth, Fifth, Capstone) \
    Core, 0, SpellA, SpellB, Third, 0, 0, Fourth, Fifth, 0, Capstone, 0

#include "class_tree_defs.cpp"

#undef CLASS_BRANCH
#undef RM
#undef CT

// NOTE(zoubir): the slot of the whole tree Talent is, CLASS_TREE_SLOTS for
// a talent of neither branch
inline u32
ClassTreeSlot(u32 Talent)
{
    u32 Result = CLASS_TREE_SLOTS;
    if (IsRoleTalent(Talent))
    {
        Result = Talent - Talent_RoleFirst;
    }
    else if (IsRunTalent(Talent))
    {
        Result = ROLE_TALENTS + Talent - Talent_RunFirst;
    }
    return Result;
}

inline u32
ClassTreeTalent(u32 TreeSlot)
{
    u32 Result = TreeSlot < ROLE_TALENTS ? Talent_RoleFirst + TreeSlot :
        Talent_RunFirst + TreeSlot - ROLE_TALENTS;
    return Result;
}

inline class_slot_shape *
ClassSlotShape(u32 TreeSlot)
{
    class_slot_shape *Result = &ClassSlotShapes[TreeSlot % ROLE_TALENTS];
    return Result;
}

inline bool32
IsClassRunContent(u32 Content)
{
    bool32 Result = Content >= CLASS_CONTENT_RUN && Content < CLASS_CONTENT_RUN + RunMod_Count;
    return Result;
}

// NOTE(zoubir): the catalog talent Content is, or CLASS_TALENTS for a run
// talent or nothing
inline u32
ClassContentTalent(u32 Content)
{
    u32 Result = (Content >= CLASS_CONTENT_TALENT && Content < CLASS_CONTENT_RUN) ?
        Content - CLASS_CONTENT_TALENT : CLASS_TALENTS;
    return Result;
}

inline u32
ClassContentRunMod(u32 Content)
{
    u32 Result = IsClassRunContent(Content) ? Content - CLASS_CONTENT_RUN : RunMod_None;
    return Result;
}

// NOTE(zoubir): whether Content already sits in Out, or is a run talent
// whose first effect another run talent there gives
inline bool32
ClassContentTaken(u8 *Out, u32 Count, u32 Content, bool32 SameEffect)
{
    bool32 Result = false;
    u32 Mod = ClassContentRunMod(Content);
    for(u32 Index = 0; Index < Count && !Result; Index++)
    {
        Result = Out[Index] == Content;
        u32 Other = ClassContentRunMod(Out[Index]);
        if (SameEffect && Mod && Other)
        {
            Result = Result || RunModDefs[Mod].Effect[0] == RunModDefs[Other].Effect[0];
        }
    }
    return Result;
}

// NOTE(zoubir): every slot's content for Seed and Role into Out: the
// fixed ones, then each wild slot in turn from what is left of its pool
internal void
RollClassTree(u32 Seed, u32 Role, u8 *Out)
{
    class_tree_def *Def = &ClassTrees[Role < PlayerRole_Count ? Role : 0];
    for(u32 Index = 0; Index < CLASS_TREE_SLOTS; Index++)
    {
        Out[Index] = 0;
    }
    u8 Placed[CLASS_TREE_SLOTS];
    u32 PlacedCount = 0;
    for(u32 Index = 0; Index < CLASS_TREE_SLOTS; Index++)
    {
        Out[Index] = Def->Fixed[Index];
        if (Out[Index])
        {
            Placed[PlacedCount++] = Out[Index];
        }
    }
    u32 Kind = 1u << RoleKindOf(Role);
    for(u32 Index = 0; Index < CLASS_TREE_SLOTS; Index++)
    {
        u32 Shape = ClassSlotShape(Index)->Kind;
        if (Shape != ClassSlot_Wild && Shape != ClassSlot_Keystone)
        {
            continue;
        }
        u8 Pool[RunMod_Count];
        u32 PoolCount = 0;
        // NOTE(zoubir): a second pass lets a repeated effect in, so a slot
        // is never empty
        for(u32 Pass = 0; Pass < 2 && PoolCount == 0; Pass++)
        {
            if (Shape == ClassSlot_Keystone)
            {
                for(u32 Mod = RunMod_WildFirst; Mod <= RunMod_WildLast; Mod++)
                {
                    u8 Content = (u8)(CLASS_CONTENT_RUN + Mod);
                    if ((RunModDefs[Mod].Kinds & Kind) && RunModDefs[Mod].Keystone &&
                        !ClassContentTaken(Placed, PlacedCount, Content, false))
                    {
                        Pool[PoolCount++] = Content;
                    }
                }
            }
            else
            {
                u8 *Candidates = Def->Pools[Index / ROLE_TALENTS];
                for(u32 Candidate = 0; Candidate < CLASS_POOL_MOST && Candidates[Candidate]; Candidate++)
                {
                    if (!ClassContentTaken(Placed, PlacedCount, Candidates[Candidate], Pass == 0))
                    {
                        Pool[PoolCount++] = Candidates[Candidate];
                    }
                }
            }
        }
        u32 Pick = PoolCount ?
            RunHash(Seed ^ RunHash(Role * 977u + Index * 131u + 1u)) % PoolCount : 0;
        Out[Index] = PoolCount ? Pool[Pick] : (u8)(CLASS_CONTENT_RUN + RunMod_KeenEdge);
        Placed[PlacedCount++] = Out[Index];
    }
}

// NOTE(zoubir): Slot's tree as its seed and class roll it, kept on the
// slot until either changes
inline u8 *
ClassTreeOf(player_slot *Slot)
{
    u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
    if (Slot->RunRolledRole != Role + 1 || Slot->RunRolledSeed != Slot->TreeSeed)
    {
        RollClassTree(Slot->TreeSeed, Role, Slot->RunRolled);
        Slot->RunRolledRole = Role + 1;
        Slot->RunRolledSeed = Slot->TreeSeed;
    }
    return Slot->RunRolled;
}

inline u32
ClassContentAt(player_slot *Slot, u32 TreeSlot)
{
    u32 Result = TreeSlot < CLASS_TREE_SLOTS ? ClassTreeOf(Slot)[TreeSlot] : 0;
    return Result;
}

inline u32
ClassTreeRank(player_slot *Slot, u32 TreeSlot)
{
    u32 Result = Slot->Ranks[ClassTreeTalent(TreeSlot)];
    return Result;
}

// NOTE(zoubir): the ranks Slot holds in catalog talent Talent of its own
// class, from every slot that rolled or holds it
internal u32
ClassTalentRank(player_slot *Slot, u32 Talent)
{
    u32 Result = 0;
    u8 *Tree = 0;
    for(u32 Index = 0; Index < CLASS_TREE_SLOTS; Index++)
    {
        u32 Rank = ClassTreeRank(Slot, Index);
        if (Rank)
        {
            Tree = Tree ? Tree : ClassTreeOf(Slot);
            Result += ClassContentTalent(Tree[Index]) == Talent ? Rank : 0;
        }
    }
    return Result;
}

// NOTE(zoubir): what Slot's tree adds to run effect Effect: every rank of
// every run talent with it, times its amount a rank
internal float
RunEffectShare(player_slot *Slot, u32 Effect)
{
    float Result = 0.f;
    if (!Slot)
    {
        return Result;
    }
    u8 *Tree = 0;
    for(u32 Index = 0; Index < CLASS_TREE_SLOTS; Index++)
    {
        u32 Rank = ClassTreeRank(Slot, Index);
        if (!Rank)
        {
            continue;
        }
        Tree = Tree ? Tree : ClassTreeOf(Slot);
        u32 Mod = ClassContentRunMod(Tree[Index]);
        if (!Mod)
        {
            continue;
        }
        run_mod_def *Def = &RunModDefs[Mod];
        for(u32 Part = 0; Part < 2; Part++)
        {
            if (Def->Effect[Part] == Effect)
            {
                Result += Def->PerRank[Part] * (float)Rank;
            }
        }
    }
    return Result;
}

// NOTE(zoubir): the most ranks the slot of Talent takes for Slot: its
// shape's, or fewer when what sits there has fewer
internal u32
ClassTalentMaxRanks(player_slot *Slot, u32 Talent)
{
    u32 TreeSlot = ClassTreeSlot(Talent);
    u32 Result = 0;
    if (TreeSlot < CLASS_TREE_SLOTS)
    {
        Result = ClassSlotShape(TreeSlot)->Ranks;
        u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
        u32 Catalog = ClassContentTalent(ClassContentAt(Slot, TreeSlot));
        if (Catalog < CLASS_TALENTS)
        {
            Result = Minimum(Result, Maximum(1u, RoleTalentDefs[Role][Catalog].MaxLevel));
        }
    }
    return Result;
}

// NOTE(zoubir): the other spell of the pair Talent is in, Talent_Count
// when it is no spell slot
inline u32
ClassSpellPartner(u32 Talent)
{
    u32 TreeSlot = ClassTreeSlot(Talent);
    u32 Result = Talent_Count;
    if (TreeSlot < CLASS_TREE_SLOTS && ClassSlotShape(TreeSlot)->Kind == ClassSlot_Spell)
    {
        Result = ClassTreeTalent(TreeSlot ^ 1);
    }
    return Result;
}

// NOTE(zoubir): whether Slot holds both spells of a pair, which only the
// top level allows
internal bool32
ClassPairDoubled(player_slot *Slot)
{
    bool32 Result = false;
    for(u32 Index = 0; Index < CLASS_TREE_SLOTS && !Result; Index += 2)
    {
        Result = ClassSlotShape(Index)->Kind == ClassSlot_Spell &&
            ClassTreeRank(Slot, Index) > 0 && ClassTreeRank(Slot, Index + 1) > 0;
    }
    return Result;
}

// NOTE(zoubir): whether Slot took the other spell of Talent's pair, so
// Talent takes no point. At the top level (PLAYER_MAX_LEVEL) the lock
// lifts for one pair: a player there casts one spell over its class's cap
inline bool32
ClassSpellTakenBeside(player_slot *Slot, u32 Talent)
{
    u32 Partner = ClassSpellPartner(Talent);
    bool32 Result = Partner < Talent_Count && Slot->Ranks[Partner] > 0 &&
        (Slot->Level < PLAYER_MAX_LEVEL || ClassPairDoubled(Slot));
    return Result;
}

inline bool32
IsClassSpellTalent(u32 Talent)
{
    u32 TreeSlot = ClassTreeSlot(Talent);
    bool32 Result = TreeSlot < CLASS_TREE_SLOTS && ClassSlotShape(TreeSlot)->Kind == ClassSlot_Spell;
    return Result;
}

// NOTE(zoubir): whether Slot holds a point in any spell of its tree's
// pairs; the first such point is free (TalentPointsLeft)
internal bool32
ClassSpellTaken(player_slot *Slot)
{
    bool32 Result = false;
    for(u32 Index = 0; Index < CLASS_TREE_SLOTS && !Result; Index++)
    {
        Result = ClassSlotShape(Index)->Kind == ClassSlot_Spell && ClassTreeRank(Slot, Index) > 0;
    }
    return Result;
}

inline bool32
ClassSlotIsWild(u32 Talent)
{
    u32 TreeSlot = ClassTreeSlot(Talent);
    bool32 Result = TreeSlot < CLASS_TREE_SLOTS &&
        (ClassSlotShape(TreeSlot)->Kind == ClassSlot_Wild ||
         ClassSlotShape(TreeSlot)->Kind == ClassSlot_Keystone);
    return Result;
}

inline bool32
ClassSlotIsKeystone(u32 Talent)
{
    u32 TreeSlot = ClassTreeSlot(Talent);
    bool32 Result = TreeSlot < CLASS_TREE_SLOTS &&
        ClassSlotShape(TreeSlot)->Kind == ClassSlot_Keystone;
    return Result;
}

inline char *
ClassBranchName(u32 Role, u32 Branch)
{
    char *Result = ClassTrees[Role < PlayerRole_Count ? Role : 0].BranchNames[Branch ? 1 : 0];
    return Result;
}

// NOTE(zoubir): the talent as Slot's panel shows it: the slot's shape
// with what sits there. A few calls' worth are kept, as the panel holds
// one at a time
internal talent_def *
ShownClassTalentDef(player_slot *Slot, u32 Talent)
{
    local_persist talent_def Defs[4];
    local_persist u32 Next;
    talent_def *Result = &Defs[Next++ % ArrayCount(Defs)];
    *Result = TalentDefs[Talent];
    u32 TreeSlot = ClassTreeSlot(Talent);
    u32 Content = ClassContentAt(Slot, TreeSlot);
    u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
    u32 Catalog = ClassContentTalent(Content);
    if (Catalog < CLASS_TALENTS)
    {
        talent_def *Def = &RoleTalentDefs[Role][Catalog];
        Result->Name = Def->Name;
        Result->Summary = Def->Summary;
        Result->PerRank = Def->PerRank;
    }
    else
    {
        run_mod_def *Mod = &RunModDefs[ClassContentRunMod(Content)];
        Result->Name = Mod->Name;
        Result->Summary = Mod->Summary;
        Result->PerRank = "";
    }
    Result->MaxLevel = ClassTalentMaxRanks(Slot, Talent);
    return Result;
}

// NOTE(zoubir): the run talent in Talent's slot for Slot, RunMod_None
// when a catalog talent sits there (the panel's icons and numbers)
inline u32
ClassRunModAt(player_slot *Slot, u32 Talent)
{
    u32 Result = ClassContentRunMod(ClassContentAt(Slot, ClassTreeSlot(Talent)));
    return Result;
}

#include "class_tree/rank_setters.cpp"
