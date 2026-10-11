/* Class tree links (class_tree.cpp): which slot of a branch hangs off
   which above it (class_slot_shape.Parents), and whether a player has
   reached a slot by holding a point in one linked above it. The rules
   refuse a point in a slot not reached (CanLearnTalent) and the panel
   draws a line along each link (ui/talent_panel/talent_nodes.cpp). */

// NOTE(zoubir): whether Talent's slot is linked to Parent's, a slot of the
// same branch in the tier above (the panel draws a line between them)
inline bool32
ClassSlotsLinked(u32 Parent, u32 Talent)
{
    u32 TreeSlot = ClassTreeSlot(Talent);
    u32 ParentSlot = ClassTreeSlot(Parent);
    bool32 Result = TreeSlot < CLASS_TREE_SLOTS && ParentSlot < CLASS_TREE_SLOTS &&
        TreeSlot / ROLE_TALENTS == ParentSlot / ROLE_TALENTS &&
        (ClassSlotShape(TreeSlot)->Parents & CLASS_PARENT(ParentSlot % ROLE_TALENTS));
    return Result;
}

// NOTE(zoubir): whether Slot holds a point in a slot linked above
// Talent's, or Talent is a branch's root; a talent of no class tree
// counts as reached
internal bool32
ClassSlotReached(player_slot *Slot, u32 Talent)
{
    u32 TreeSlot = ClassTreeSlot(Talent);
    if (TreeSlot >= CLASS_TREE_SLOTS)
    {
        return true;
    }
    u32 Parents = ClassSlotShape(TreeSlot)->Parents;
    u32 First = TreeSlot - TreeSlot % ROLE_TALENTS;
    bool32 Result = Parents == 0;
    for(u32 Index = 0; Index < ROLE_TALENTS && !Result; Index++)
    {
        Result = (Parents & CLASS_PARENT(Index)) && ClassTreeRank(Slot, First + Index) > 0;
    }
    return Result;
}
