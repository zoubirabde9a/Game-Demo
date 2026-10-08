/* Roles and classes (dungeon.cpp): what a player is in the party. A
   player picks a class; each class belongs to one of four roles (its
   role_kind): the tank, the healer, ranged damage and melee damage. The
   tank has the most health, takes less and makes the most threat per
   point of damage; the healer deals little; the damage classes deal the
   most. One row per class in RoleTable, read only on dungeon maps. What
   each class's keys cast is role_abilities.cpp (the new classes' kits are
   role_kits/<class>.cpp); each class's talent branch is role_talents.cpp
   (role_kits/<class>_defs.cpp for the new ones).

   player_role names a class, not a role: the name stayed from the time
   there was one class per role. The first three keep their numbers, so
   old saves and tests read the same. */

enum player_role
{
    // NOTE(zoubir): first, so a slot zeroed on join is a damage player
    PlayerRole_Damage,
    PlayerRole_Tank,
    PlayerRole_Healer,
    PlayerRole_Ranger,
    PlayerRole_Berserker,
    PlayerRole_Shadowblade,
    PlayerRole_Count
};

// NOTE(zoubir): the role a class plays in the party
enum role_kind
{
    RoleKind_Tank,
    RoleKind_Healer,
    RoleKind_Ranged,
    RoleKind_Melee,
    RoleKind_Count
};

global_variable char *RoleKindNames[RoleKind_Count] = {"Tank", "Healer", "Ranged", "Melee"};

struct role_def
{
    char *Name;
    float MaxHp;
    // NOTE(zoubir): shares of a hit's damage, taken and dealt
    float DamageTaken;
    float DamageDealt;
    // NOTE(zoubir): threat made per point of damage (sim/dungeon/threat)
    float ThreatScale;
    // NOTE(zoubir): the role the class plays, as the picker groups it
    // ("Ranged"), and one line on how it plays, for the role picker
    char *Title;
    char *Keys;
    u32 Kind;
    // NOTE(zoubir): the class colour (party frames, the ring under an
    // ally, the talent branch, spell previews), red, green, blue
    u8 Color[3];
};

// NOTE(zoubir): what a role key does with the cursor, for the client's
// aim (client/cast_targeting.cpp): nothing, an ally, or a ground circle
enum role_aim
{
    RoleAim_None,
    RoleAim_Ally,
    RoleAim_Ground,
};

// NOTE(zoubir): each role's spell on each key (RoleKeys order), as the
// ability bar names it, and its cooldown before talents; a 0 name leaves
// the game's ability on the key
struct role_spell
{
    char *Name;
    float Cooldown;
    // NOTE(zoubir): one line for the controls panel (ui/controls_panel.cpp)
    char *Help;
    role_aim Aim;
    // NOTE(zoubir): a ground spell's radius before talents, and how far
    // out an ally spell reaches
    float Reach;
    // NOTE(zoubir): the slot of the class's tree that unlocks it
    // (role_talents.cpp) + 1, 0 for a main spell the class has from the
    // start
    u32 Unlock;
};

global_variable role_def RoleTable[PlayerRole_Count] =
{
    // Name          MaxHp  Taken  Dealt  Threat
    {"Fire Mage",    110.f, 1.0f,  1.35f, 1.f, "Ranged",
     "Meteor and Giant Fireball mark foes; Detonate blows the marks up",
     RoleKind_Ranged, {240, 120, 60}},
    {"Bulwark",      180.f, 0.7f,  0.7f,  4.f, "Tank",
     "Taunt pulls foes; Shield Slam sunders them, keep it on the boss",
     RoleKind_Tank, {96, 160, 245}},
    {"Mender",       100.f, 1.0f,  0.5f,  1.f, "Healer",
     "Mending Bolt heals, Ward shields and adds 12% damage; Holy Fire smites",
     RoleKind_Healer, {120, 220, 140}},
    {"Ranger",       110.f, 1.0f,  1.35f, 1.f, "Ranged",
     "A bow: arrows, a rain of arrows, a shot that pierces a line",
     RoleKind_Ranged, {90, 210, 170}},
    {"Berserker",    130.f, 0.9f,  1.35f, 1.f, "Melee",
     "A great axe: hits build Rage, Rage feeds the big swings",
     RoleKind_Melee, {225, 50, 50}},
    {"Shadowblade",  115.f, 1.0f,  1.35f, 1.f, "Melee",
     "Twin daggers: quick cuts build combo points, finishers spend them",
     RoleKind_Melee, {170, 110, 255}},
};

inline role_def *
GetRoleDef(u32 Role)
{
    role_def *Result = &RoleTable[Role < PlayerRole_Count ? Role : PlayerRole_Damage];
    return Result;
}

inline u32
RoleKindOf(u32 Role)
{
    u32 Result = GetRoleDef(Role)->Kind;
    return Result;
}

// NOTE(zoubir): a damage class, ranged or melee
inline bool32
IsDamageRole(u32 Role)
{
    u32 Kind = RoleKindOf(Role);
    bool32 Result = Kind == RoleKind_Ranged || Kind == RoleKind_Melee;
    return Result;
}

// NOTE(zoubir): the class colour as 0x00BBGGRR, the effects' format
inline u32
RoleRGB(u32 Role)
{
    u8 *C = GetRoleDef(Role)->Color;
    u32 Result = (u32)C[0] | ((u32)C[1] << 8) | ((u32)C[2] << 16);
    return Result;
}

// NOTE(zoubir): the role's health on the slot's body, full. Called when a
// player joins and when it picks a role; outside a dungeon the body keeps
// the game rules' health
internal void
ApplyRoleToPlayer(app_state *AppState, player_slot *Slot)
{
    if (!IsDungeon(AppState) || !Slot->Entity)
    {
        return;
    }
    world_entity *Player = Slot->Entity;
    Player->MaxHp = GetRoleDef(Slot->Role)->MaxHp;
    if (Player->Hp > 0.f)
    {
        Player->Hp = Player->MaxHp;
    }
}

// NOTE(zoubir): a role picked; ignored for an unknown role
internal void
SetPlayerRole(app_state *AppState, player_slot *Slot, u32 Role)
{
    if (Role < PlayerRole_Count)
    {
        // NOTE(zoubir): the old role's branch is no use to the new one
        if (Slot->Role != Role)
        {
            ResetRoleTalents(Slot);
        }
        Slot->Role = (u8)Role;
        // NOTE(zoubir): the old class's resource is no use either
        Slot->ClassMeter = 0;
        Slot->ClassFlags = 0;
        ApplyRoleToPlayer(AppState, Slot);
    }
}
