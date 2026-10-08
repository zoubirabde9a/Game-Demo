/* Ranger's numbers, talents and spells, included by role_talents.cpp
   before the class tables (role_kits/ranger.cpp has what they do). A class
   that has no spell on its first key yet (RoleSpells) is left out of the
   role picker and the bots. */

// NOTE(zoubir): Focus, the Ranger's resource: hits on the foe under its
// own Hunter's Mark build it, Piercing Shot spends all of it
#define RANGER_FOCUS_MOST 100.f
// NOTE(zoubir): between fights, RANGER_FOCUS_HOLD after it last grew, it
// drains at this much a second
#define RANGER_FOCUS_HOLD 5.f
#define RANGER_FOCUS_DRAIN 8.f

// NOTE(zoubir): a mark or a trap is sent to clients again this often
// while it lasts, so its look never drops out of their short list of
// bursts (client/dungeon/role_fx.cpp) and follows a moving foe
#define RANGER_KEEP_SECONDS 1.5f

// NOTE(zoubir): arrows fly at this speed; a hit lands when its arrow
// gets there (client/dungeon/classes/ranger.cpp flies the same speed)
#define RANGER_ARROW_SPEED 1500.f

// NOTE(zoubir): Quick Shot (X): an arrow at the foe aimed at, the filler
#define QUICK_SHOT_RANGE 560.f
#define QUICK_SHOT_DAMAGE 9.f
#define QUICK_SHOT_SHOVE 50.f
#define QUICK_SHOT_COOLDOWN 1.2f
#define QUICK_SHOT_FOCUS 14.f

// NOTE(zoubir): Volley (A): VOLLEY_DRAW after the press the arrows come
// down on the circle at the cursor for VOLLEY_SECONDS, every monster
// inside struck each VOLLEY_TICK and slowed
#define VOLLEY_RADIUS 85.f
#define VOLLEY_DRAW 0.45f
#define VOLLEY_SECONDS 2.f
#define VOLLEY_TICK 0.4f
#define VOLLEY_TICK_DAMAGE 4.5f
#define VOLLEY_SLOW_SECONDS 0.8f
#define VOLLEY_FOCUS 3.f
#define VOLLEY_COOLDOWN 12.f

// NOTE(zoubir): Piercing Shot (R): after its draw (PlayerSpell_RangerA)
// a heavy arrow flies PIERCE_RANGE along the aim through every foe
// within PIERCE_WIDTH of its line, each taking PIERCE_DAMAGE plus
// PIERCE_PER_FOCUS for each point of Focus it spends (all of it)
#define PIERCE_RANGE 720.f
#define PIERCE_WIDTH 26.f
#define PIERCE_SPEED 1900.f
#define PIERCE_DAMAGE 28.f
#define PIERCE_PER_FOCUS 0.36f
#define PIERCE_SHOVE 140.f
#define PIERCE_COOLDOWN 8.f

// NOTE(zoubir): Hunter's Mark (W): an arrow that marks the foe aimed at
// for MARK_SECONDS; the Ranger deals MARK_SHARE more to it
#define MARK_RANGE 600.f
#define MARK_DAMAGE 8.f
#define MARK_SECONDS 15.f
#define MARK_SHARE 0.15f
#define MARK_COOLDOWN 6.f

// NOTE(zoubir): Disengage (C, from the tree): a leap back from the aim,
// leaving a snare where the Ranger stood that roots the first foe to
// step within TRAP_RADIUS of it
#define DISENGAGE_SPEED 680.f
#define DISENGAGE_LIFT 280.f
#define DISENGAGE_COOLDOWN 14.f
#define TRAP_RADIUS 34.f
#define TRAP_SECONDS 20.f
#define TRAP_ROOT_SECONDS 2.5f
#define TRAP_DAMAGE 10.f

// NOTE(zoubir): Rapid Fire (V, from the tree): RAPID_FIRE_ARROWS arrows
// over its cast at the foe aimed at
#define RAPID_FIRE_RANGE 600.f
#define RAPID_FIRE_ARROWS 10
#define RAPID_FIRE_DAMAGE 6.f
#define RAPID_FIRE_FOCUS 6.f
#define RAPID_FIRE_COOLDOWN 22.f

enum ranger_talent
{
    RangerTalent_Marksman,
    RangerTalent_Disengage,
    RangerTalent_Barrage,
    RangerTalent_Deadeye,
    RangerTalent_RapidFire,
    RangerTalent_LethalMark,
};

// NOTE(zoubir): per rank, or once taken
#define MARKSMAN_SHARE 0.06f
// NOTE(zoubir): Disengage's second rank: the snare roots longer and bites
#define SNARE_ROOT_SECONDS 1.5f
#define SNARE_DAMAGE 25.f
// NOTE(zoubir): Barrage: Volley this much wider, and this much longer
#define BARRAGE_RADIUS 1.3f
#define BARRAGE_SECONDS 1.2f
// NOTE(zoubir): Deadeye: a Piercing Shot on full Focus hits this much harder
#define DEADEYE_SCALE 1.6f
// NOTE(zoubir): Lethal Mark: the mark bites deeper, and when its foe dies
// it jumps to the nearest foe within this
#define LETHAL_MARK_SHARE 0.1f
#define LETHAL_MARK_JUMP 320.f

// NOTE(zoubir): the same shape as every class's branch (role_talents.cpp):
// two talents of two ranks, two of one, one, one; slot 1 unlocks the C
// spell, slot 4 the V spell
global_variable talent_def RangerTalentDefs[ROLE_TALENTS] =
{
    {"Marksman", "All your damage is higher", "+6% damage",
     TalentBranch_Role, 0, 0, 2, 0},
    {"Disengage", "C: leap back and leave a snare that roots; rank 2, a longer root that bites",
     "a new spell", TalentBranch_Role, 0, 1, 2, 0},
    {"Barrage", "Volley covers a wider circle and rains longer", "+30% radius, +1.2 s",
     TalentBranch_Role, 1, 0, 1, 0},
    {"Deadeye", "Piercing Shot on full Focus always crits", "x1.6 on full Focus",
     TalentBranch_Role, 1, 1, 1, 0},
    {"Rapid Fire", "V: a 2 s stream of arrows at a foe", "a new spell",
     TalentBranch_Role, 2, 0, 1, 0},
    {"Lethal Mark", "Hunter's Mark bites deeper and jumps to the next foe when its foe dies",
     "+10% marked, it spreads", TalentBranch_Role, 3, 0, 1, 0},
};

// NOTE(zoubir): in RoleKeys order: A, R, C, V, W, X, right click
global_variable role_spell RangerSpells[ROLE_KEYS] =
{
    {"Volley", VOLLEY_COOLDOWN, "Volley: arrows rain on the circle at the cursor for 2 s and slow",
     RoleAim_Ground, VOLLEY_RADIUS, 0},
    {"Piercing Shot", PIERCE_COOLDOWN,
     "Piercing Shot: 1 s draw, an arrow through every foe in a line; spends Focus for more",
     RoleAim_None, PIERCE_RANGE, 0},
    {"Disengage", DISENGAGE_COOLDOWN, "Disengage: leap back from the aim and leave a snare trap",
     RoleAim_None, 0.f, RangerTalent_Disengage + 1},
    {"Rapid Fire", RAPID_FIRE_COOLDOWN, "Rapid Fire: 2 s of arrows at a foe, walking slowly",
     RoleAim_None, RAPID_FIRE_RANGE, RangerTalent_RapidFire + 1},
    {"Hunter's Mark", MARK_COOLDOWN,
     "Hunter's Mark: mark a foe; you deal 15% more to it and hits on it build Focus",
     RoleAim_None, MARK_RANGE, 0},
    {"Quick Shot", QUICK_SHOT_COOLDOWN, "Quick Shot: a fast arrow at a foe; on your mark it builds Focus",
     RoleAim_None, QUICK_SHOT_RANGE, 0},
    {},
};
