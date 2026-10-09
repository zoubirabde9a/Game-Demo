/* Spell cap (role_abilities.cpp): each class's attack, which is never
   counted, and the most class spells a class casts beside it: four for a
   damage class, six for a tank or a healer, one more at the top level
   (docs/class-trees.md). The tree keeps every build under it: a class's
   base spells and one spell of each branch's pair reach the cap
   (class_tree.cpp). */

// NOTE(zoubir): each class's attack, the key it fills the gaps with:
// a class casts it beside its spells, and it is not counted in the cap
// (RoleSpellCap). The Fire Mage's is the game's fireball, no key of its
// own (ROLE_KEYS)
global_variable u32 RoleAttackKeys[PlayerRole_Count] =
{
    ROLE_KEYS, 6, 6, 5, 6, 6, 5, 6, 5, 6,
};

// NOTE(zoubir): the most class spells Role casts, its attack not counted:
// four for a damage class, six for a tank or a healer; at the top level
// one more (ClassSpellTakenBeside lifts one pair's lock there)
inline u32
RoleSpellCap(u32 Role)
{
    u32 Kind = RoleKindOf(Role < PlayerRole_Count ? Role : PlayerRole_Damage);
    u32 Result = (Kind == RoleKind_Tank || Kind == RoleKind_Healer) ? 6 : 4;
    return Result;
}

// NOTE(zoubir): the class spells Slot casts now, its attack not counted
internal u32
RoleSpellsCounted(player_slot *Slot)
{
    u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
    u32 Result = 0;
    for(u32 Key = 0; Key < ROLE_KEYS; Key++)
    {
        Result += (Key != RoleAttackKeys[Role] && RoleSpellLearned(Slot, Key)) ? 1 : 0;
    }
    return Result;
}
