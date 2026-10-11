/* Class tree spell pairs (class_tree.cpp): each branch's two spells,
   of which a player takes one; the lock on the other, which lifts once
   at the top level for a damage class; and the free first spell point
   (TalentPointsToSpend, talents.cpp). */

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
// Talent takes no point. At the top level (PLAYER_MAX_LEVEL) a damage
// class's lock lifts for one pair: it casts five spells there. A tank's
// or a healer's never does, so it stays at six (RoleSpellCap)
inline bool32
ClassSpellTakenBeside(player_slot *Slot, u32 Talent)
{
    u32 Partner = ClassSpellPartner(Talent);
    u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
    bool32 Lifts = Slot->Level >= PLAYER_MAX_LEVEL && RoleKindOf(Role) != RoleKind_Tank &&
        RoleKindOf(Role) != RoleKind_Healer &&
        !ClassPairDoubled(Slot);
    bool32 Result = Partner < Talent_Count && Slot->Ranks[Partner] > 0 && !Lifts;
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
