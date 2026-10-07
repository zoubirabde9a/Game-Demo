/* Roles (dungeon.cpp): what a player is in the party. The tank has the
   most health, takes less and makes the most threat per point of damage;
   the healer deals little; the damage role deals the most. One row each
   in RoleTable, read only on dungeon maps. What each role's keys cast is
   role_abilities.cpp; each role's talent branch is role_talents.cpp. */

enum player_role
{
    // NOTE(zoubir): first, so a slot zeroed on join is a damage player
    PlayerRole_Damage,
    PlayerRole_Tank,
    PlayerRole_Healer,
    PlayerRole_Count
};

struct role_def
{
    char *Name;
    float MaxHp;
    // NOTE(zoubir): shares of a hit's damage, taken and dealt
    float DamageTaken;
    float DamageDealt;
    // NOTE(zoubir): threat made per point of damage (sim/dungeon/threat)
    float ThreatScale;
    // NOTE(zoubir): what the role is called on screen, and what its keys
    // do (role_abilities.cpp), for the role picker
    char *Title;
    char *Keys;
};

global_variable role_def RoleTable[PlayerRole_Count] =
{
    // Name      MaxHp  Taken  Dealt  Threat
    {"Striker",  110.f, 1.0f,  1.35f, 1.f, "Damage",
     "A Inferno   E Shield   V Kunai   every hit 35% harder"},
    {"Bulwark",  180.f, 0.7f,  0.7f,  4.f, "Tank",
     "A Taunt   E Shield Slam, heals you per foe   V Intercept an ally"},
    {"Mender",   100.f, 1.0f,  0.5f,  1.f, "Healer",
     "A Sanctuary   E Ward the party   V Mending Bolt   on the ally you point at or click on the left"},
};

inline role_def *
GetRoleDef(u32 Role)
{
    role_def *Result = &RoleTable[Role < PlayerRole_Count ? Role : PlayerRole_Damage];
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
        ApplyRoleToPlayer(AppState, Slot);
    }
}
