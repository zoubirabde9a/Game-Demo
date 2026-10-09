/* Class tree ranks set by hand (class_tree.cpp): for the tests and the
   developer switches that give a player a talent or a whole tree without
   the point rules, wherever the tree holds the talent this run. */

// NOTE(zoubir): every slot full, for the developer switches that give a
// local player its whole tree in a screenshot. Each pair's first spell
internal void
GrantWholeClassTree(player_slot *Slot)
{
    for(u32 Index = 0; Index < CLASS_TREE_SLOTS; Index++)
    {
        u32 Talent = ClassTreeTalent(Index);
        bool32 Second = ClassSlotShape(Index)->Kind == ClassSlot_Spell && (Index & 1);
        Slot->Ranks[Talent] = Second ? 0 : (u8)ClassTalentMaxRanks(Slot, Talent);
    }
}

// NOTE(zoubir): the talent id of the slot of Slot's tree that holds
// Content, Talent_Count when none does this run
internal u32
ClassContentTalentId(player_slot *Slot, u32 Content)
{
    u32 Result = Talent_Count;
    for(u32 Index = 0; Index < CLASS_TREE_SLOTS && Result == Talent_Count; Index++)
    {
        if (ClassContentAt(Slot, Index) == Content)
        {
            Result = ClassTreeTalent(Index);
        }
    }
    return Result;
}

inline u32
ClassTalentId(player_slot *Slot, u32 Catalog)
{
    u32 Result = ClassContentTalentId(Slot, CLASS_CONTENT_TALENT + Catalog);
    return Result;
}

// NOTE(zoubir): Rank points straight into the slot holding Content, past
// the point rules, for the tests and the developer switches. Content only
// a wild slot can hold makes the tree roll seeds until one holds it;
// returns the talent id it went into, Talent_Count if none
internal u32
SetClassContentRank(player_slot *Slot, u32 Content, u32 Rank)
{
    u32 Talent = ClassContentTalentId(Slot, Content);
    for(u32 Seed = 1; Seed <= RUN_SEED_MASK && Talent == Talent_Count; Seed++)
    {
        Slot->TreeSeed = Seed;
        Talent = ClassContentTalentId(Slot, Content);
    }
    if (Talent < Talent_Count)
    {
        Slot->Ranks[Talent] = (u8)Rank;
    }
    return Talent;
}

inline u32
SetClassTalentRank(player_slot *Slot, u32 Catalog, u32 Rank)
{
    u32 Result = SetClassContentRank(Slot, CLASS_CONTENT_TALENT + Catalog, Rank);
    return Result;
}

inline u32
SetClassRunModRank(player_slot *Slot, u32 Mod, u32 Rank)
{
    u32 Result = SetClassContentRank(Slot, CLASS_CONTENT_RUN + Mod, Rank);
    return Result;
}
