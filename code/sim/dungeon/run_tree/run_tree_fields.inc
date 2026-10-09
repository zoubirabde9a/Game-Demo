// player_slot fields for the class tree's second branch and its rolls
// (sim/dungeon/class_tree.cpp, sim/dungeon/run_tree/),
// included inside struct player_slot in player.h.
    // NOTE(zoubir): slots in the class tree's second branch (Talent_RunFirst on)
#define RUN_TALENTS 12
    // NOTE(zoubir): 16 bits, what the wild slots of the class tree rolled: each
    // slot's talent is a hash of this, the class and the slot
    // (ClassTreeOf), so the server sends one number and the client knows
    // the whole tree. A new run rolls it again (RerollRunTree)
    u32 TreeSeed;
    // NOTE(zoubir): hits landed this fight, for Cadence (every
    // RUN_CADENCE_EVERY th hit lands harder)
    u32 RunHits;
    // NOTE(zoubir): seconds left of the damage a kill gave (Frenzy)
    float RunFrenzySeconds;
    // NOTE(zoubir): what nearby allies' auras give this player, set once a
    // tick (UpdateRunTrees): a share less damage taken, a share more
    // dealt
    float RunAuraArmor;
    float RunAuraDamage;
    // NOTE(zoubir): Thorns owed to the monster that hit last, paid on the
    // next tick so a hit never lands inside another
    // NOTE(zoubir): the fight (dungeon_run.MeterFight) these were last
    // reset for, and whether Lifeline went off in it
    u32 RunFightSeen;
    bool32 RunLifelineSpent;
    u32 RunThornsSlot;
    u32 RunThornsSerial;
    float RunThornsOwed;
    // NOTE(zoubir): the tree TreeSeed rolled for the class, both branches,
    // kept until either changes (ClassTreeOf); RunRolledRole is the class + 1, so a
    // zeroed slot rolls on first use
    u8 RunRolled[2 * RUN_TALENTS];
    u32 RunRolledRole;
    u32 RunRolledSeed;
