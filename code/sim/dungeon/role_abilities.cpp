/* Role abilities (docs/dungeon-plan.md, "Roles"): in a dungeon run each
   role's A, E and V keys cast that role's spells instead of the game's
   Launch, Shield and kunai, where its row in RoleSpells names one. The
   fireball (X), blink and jump are the same for everyone.

   Tank (Bulwark, role_kits/tank.cpp)
     A  Taunt: every monster near the tank attacks it, and Shield Wall
        goes up for a moment.
     E  Shield Slam: stuns and hurts what is round the tank, heals it
        for each one struck, raises Shield Wall on it and rallies the
        allies near it.
     V  Intercept: leaps to an ally and takes the threat off them.
   Healer (Mender, role_kits/healer.cpp)
     A  Sanctuary: a circle at the cursor that heals allies inside.
     E  Ward: an ally absorbs the next hits, and the allies round them
        absorb half as much.
     V  Mending Bolt: heals an ally.
   Damage (Striker, role_kits/striker.cpp)
     A  Inferno: a meteor at the cursor, then burning ground.
     E, V  the game's Shield and kunai.

   Who an ally spell lands on: the player the client says is under the
   cursor or picked on the party frames (player_input.Target, which may
   be the caster), else the most hurt player in reach, else the nearest
   other ally in reach. Only a healer with nobody in reach casts on
   themselves, so no healer spell is a self heal.

   UseRoleAbilities runs from UpdatePlayer before the game's abilities and
   takes the role's keys out of the input, so the game's spells on those
   keys never see them. While the client predicts its own player the keys
   are taken but nothing is cast: the server casts it. What lasts after a
   cast (sanctuaries, infernos, rallies, renewals) runs once a tick in
   UpdateRoleEffects. The role branch of the talent tree changes the
   numbers (role_talents.cpp).

   Between fights every living player heals REST_SHARE_PER_SECOND of
   their health a second, so a party walks into the next room whole
   whether or not it has a healer. */

// NOTE(zoubir): what a role key does with the cursor, for the client's
// aim (client/cast_targeting.cpp): nothing, an ally, or a ground circle
enum role_aim
{
    RoleAim_None,
    RoleAim_Ally,
    RoleAim_Ground,
};

// NOTE(zoubir): the keys a role casts on, and the slot each one's
// cooldown has in player_slot.RoleCooldowns
global_variable u32 RoleKeys[ROLE_KEYS] =
{
    PlayerButton_Launch,
    PlayerButton_Shield,
    PlayerButton_Kunai,
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
};

#include "role_kits/role_numbers.h"

global_variable role_spell RoleSpells[PlayerRole_Count][ROLE_KEYS] =
{
    {{"Inferno", INFERNO_COOLDOWN, "Inferno: a meteor at the cursor, then burning ground",
      RoleAim_Ground, INFERNO_RADIUS},
     {0}, {0}},
    {{"Taunt", TAUNT_COOLDOWN, "Taunt: monsters near you attack you; Shield Wall 2 s",
      RoleAim_None, 0.f},
     {"Shield Slam", SHIELD_SLAM_COOLDOWN,
      "Shield Slam: stun what is near, heal per foe hit, take 40%, shield allies",
      RoleAim_None, 0.f},
     {"Intercept", INTERCEPT_COOLDOWN, "Intercept: leap to an ally and pull their foes",
      RoleAim_Ally, INTERCEPT_RANGE}},
    {{"Sanctuary", SANCTUARY_COOLDOWN, "Sanctuary: a healing circle at the cursor",
      RoleAim_Ground, SANCTUARY_RADIUS},
     {"Ward", WARD_COOLDOWN, "Ward: shield an ally, and half on allies near them", RoleAim_Ally,
      MENDING_BOLT_RANGE},
     {"Mending Bolt", MENDING_BOLT_COOLDOWN, "Mending Bolt: heal an ally", RoleAim_Ally,
      MENDING_BOLT_RANGE}},
};

// NOTE(zoubir): Key's cooldown for Slot's role after its talents
internal float
RoleSpellCooldown(player_slot *Slot, u32 Key)
{
    u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
    float Result = Key < ROLE_KEYS ? RoleSpells[Role][Key].Cooldown : 0.f;
    if (Key == 0)
    {
        Result -= PROVOKE_COOLDOWN * (float)RoleRank(Slot, PlayerRole_Tank, TankTalent_Provoke);
        Result -= KINDLING_COOLDOWN * (float)RoleRank(Slot, PlayerRole_Damage, StrikerTalent_Kindling);
    }
    return Result;
}

// NOTE(zoubir): a ground spell's radius for Slot after its talents
internal float
RoleSpellRadius(player_slot *Slot, u32 Key)
{
    u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
    float Result = Key < ROLE_KEYS ? RoleSpells[Role][Key].Reach : 0.f;
    if (Key == 0 && RoleRank(Slot, PlayerRole_Healer, HealerTalent_HallowedGround))
    {
        Result *= HALLOWED_RADIUS;
    }
    if (Key == 0 && RoleRank(Slot, PlayerRole_Damage, StrikerTalent_Cataclysm))
    {
        Result *= CATACLYSM_RADIUS;
    }
    return Result;
}

// NOTE(zoubir): the role key Button is, or ROLE_KEYS for none
inline u32
RoleKeyForButton(u32 Button)
{
    for(u32 Key = 0; Key < ROLE_KEYS; Key++)
    {
        if (RoleKeys[Key] == Button)
        {
            return Key;
        }
    }
    return ROLE_KEYS;
}

// NOTE(zoubir): whether Slot's role casts its own spell on Key here
inline bool32
RoleOwnsKey(app_state *AppState, player_slot *Slot, u32 Key)
{
    bool32 Result = IsDungeon(AppState) && Key < ROLE_KEYS &&
        Slot->Role < PlayerRole_Count && RoleSpells[Slot->Role][Key].Name != 0;
    return Result;
}

// NOTE(zoubir): the role spell Player casts on Button, 0 when the game's
// own ability is on it
internal role_spell *
RoleSpellOnButton(app_state *AppState, world_entity *Player, u32 Button)
{
    role_spell *Result = 0;
    if (Player && Player->Type == EntityType_Player && Player->PlayerIndex < MAX_PLAYERS)
    {
        player_slot *Slot = &AppState->Players[Player->PlayerIndex];
        u32 Key = RoleKeyForButton(Button);
        if (RoleOwnsKey(AppState, Slot, Key))
        {
            Result = &RoleSpells[Slot->Role][Key];
        }
    }
    return Result;
}

// NOTE(zoubir): the controls panel's line for Button: the role spell's in
// a dungeon run where the local role owns the key, else Default
internal char *
RoleControlsLine(app_state *AppState, u32 Button, char *Default)
{
    world_entity *Player = AppState->Players[AppState->LocalPlayerIndex].Entity;
    role_spell *Spell = RoleSpellOnButton(AppState, Player, Button);
    char *Result = Spell ? Spell->Help : Default;
    return Result;
}

// NOTE(zoubir): from PlayerCooldown (sim/player_cooldowns.cpp): on a key a
// role owns, the role spell's cooldown, so the ability bar shows it and
// the snapshot carries it; 0 elsewhere
internal float *
RoleCooldownOnButton(app_state *AppState, world_entity *Player, u32 Button, float *Full)
{
    role_spell *Spell = RoleSpellOnButton(AppState, Player, Button);
    if (!Spell)
    {
        return 0;
    }
    player_slot *Slot = &AppState->Players[Player->PlayerIndex];
    u32 Key = RoleKeyForButton(Button);
    *Full = RoleSpellCooldown(Slot, Key);
    return &Slot->RoleCooldowns[Key];
}

// NOTE(zoubir): where the cursor is on the ground, from the aim
inline v2
AimPoint(world_entity *Player)
{
    v2 Result = Player->Position.XY + (PLAYER_AIM_REACH * Player->AimReach) * Player->Aim;
    return Result;
}

inline v3
ChestOf(world_entity *Unit)
{
    v3 Result = Unit->Position;
    Result.Z += 16.f;
    return Result;
}

// NOTE(zoubir): the living player the client picked (player_input.Target:
// under the cursor or on the party frames), Self included when AllowSelf;
// else 0
inline world_entity *
CursorAlly(app_state *AppState, player_slot *Slot, world_entity *Self, bool32 AllowSelf)
{
    world *World = &AppState->World;
    u32 Index = Slot->Input.Target;
    world_entity *Result = 0;
    if (Index && Index - 1 < World->EntityCount)
    {
        world_entity *Unit = &World->Entities[Index - 1];
        if ((Unit != Self || AllowSelf) && Unit->IsPresent &&
            Unit->Type == EntityType_Player && !IsDeadPlayer(Unit))
        {
            Result = Unit;
        }
    }
    return Result;
}

// NOTE(zoubir): the living player within Range of From missing the most
// health, Skip left out (Self counts), or 0 when nobody is hurt
internal world_entity *
MostHurtAlly(app_state *AppState, v2 From, float Range, world_entity *Skip = 0)
{
    world_entity *Result = 0;
    float Worst = 0.f;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (Player && Player != Skip && Length(Player->Position.XY - From) <= Range)
        {
            float Missing = Player->MaxHp - Player->Hp;
            if (Missing > Worst)
            {
                Worst = Missing;
                Result = Player;
            }
        }
    }
    return Result;
}

// NOTE(zoubir): the living player within Range of Self nearest to it,
// Self left out, or 0
internal world_entity *
NearestOtherAlly(app_state *AppState, world_entity *Self, float Range)
{
    world_entity *Result = 0;
    float Best = Range;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        float Distance = Player ? Length(Player->Position.XY - Self->Position.XY) : 0.f;
        if (Player && Player != Self && Distance <= Best)
        {
            Best = Distance;
            Result = Player;
        }
    }
    return Result;
}

// NOTE(zoubir): who a healer's ally spell lands on (the rules at the top)
internal world_entity *
PickAllyFor(app_state *AppState, player_slot *Slot, world_entity *Player, float Range)
{
    world_entity *Result = CursorAlly(AppState, Slot, Player, true);
    if (!Result || Length(Result->Position.XY - Player->Position.XY) > Range)
    {
        Result = MostHurtAlly(AppState, Player->Position.XY, Range);
    }
    if (!Result)
    {
        Result = NearestOtherAlly(AppState, Player, Range);
    }
    if (!Result)
    {
        Result = Player;
    }
    return Result;
}

// NOTE(zoubir): Amount of health to Target from the healer in slot By;
// returns what it gave. The monsters fighting the party notice
internal float
HealPlayer(app_state *AppState, u32 By, world_entity *Target, float Amount)
{
    if (!Target || IsDeadPlayer(Target))
    {
        return 0.f;
    }
    float Given = Minimum(Amount * PartySustainScale(AppState->Dungeon),
                          Target->MaxHp - Target->Hp);
    Target->Hp += Given;
    dungeon_run *Run = AppState->Dungeon;
    if (Run && Given > 0.f && By < MAX_PLAYERS)
    {
        for(u32 Index = 0; Index < THREAT_ROWS; Index++)
        {
            threat_row *Row = &Run->Threat.Rows[Index];
            if (Row->Serial && FindMonsterBySerial(&AppState->World, Row->Slot, Row->Serial))
            {
                Row->Threat[By] += HEAL_THREAT_SHARE * Given;
            }
        }
    }
    return Given;
}

#include "role_kits/tank.cpp"
#include "role_kits/healer.cpp"
#include "role_kits/striker.cpp"

// NOTE(zoubir): from UpdatePlayer, before the game's abilities
internal void
UseRoleAbilities(app_state *AppState, world *World, memory_arena *Arena,
                 player_slot *Slot, float DeltaTime)
{
    world_entity *Player = Slot->Entity;
    for(u32 Key = 0; Key < ROLE_KEYS; Key++)
    {
        Slot->RoleCooldowns[Key] = Maximum(0.f, Slot->RoleCooldowns[Key] - DeltaTime);
    }
    Slot->ShieldWallSeconds = Maximum(0.f, Slot->ShieldWallSeconds - DeltaTime);
    for(u32 Key = 0; Key < ROLE_KEYS; Key++)
    {
        if (!RoleOwnsKey(AppState, Slot, Key))
        {
            continue;
        }
        u32 Button = RoleKeys[Key];
        bool32 Pressed = WasPressed(&Slot->Input, Button);
        Slot->Input.Pressed &= ~Button;
        Slot->Input.ServerPressed &= ~Button;
        if (!Pressed || Slot->Predicted || Slot->RoleCooldowns[Key] > 0.f ||
            IsDeadPlayer(Player))
        {
            continue;
        }
        bool32 Cast = false;
        switch(Slot->Role)
        {
            case PlayerRole_Tank: Cast = CastTankKey(AppState, World, Arena, Slot, Player, Key); break;
            case PlayerRole_Healer: Cast = CastHealerKey(AppState, Slot, Player, Key); break;
            case PlayerRole_Damage: Cast = CastStrikerKey(AppState, Slot, Player, Key); break;
        }
        if (Cast)
        {
            Slot->RoleCooldowns[Key] = RoleSpellCooldown(Slot, Key);
        }
    }
}

// NOTE(zoubir): how many monsters of the fight are after each player,
// for the party frames
internal void
CountAggro(app_state *AppState, dungeon_run *Run)
{
    world *World = &AppState->World;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        AppState->Players[SlotIndex].Aggro = 0;
    }
    for(u32 Foe = 0; Foe < Run->FoeCount && Run->FightingRoom; Foe++)
    {
        world_entity *Monster = FindMonsterBySerial(World, Run->FoeSlots[Foe], Run->FoeSerials[Foe]);
        world_entity *Target = Monster ? FindMonsterTarget(AppState, World, Monster, 0) : 0;
        if (Target && Target->PlayerIndex < MAX_PLAYERS)
        {
            u8 *Aggro = &AppState->Players[Target->PlayerIndex].Aggro;
            *Aggro = (u8)Minimum(7u, (u32)*Aggro + 1);
        }
    }
}

// NOTE(zoubir): no room being fought: everyone standing heals up
internal void
RestBetweenFights(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    if (Run->FightingRoom)
    {
        return;
    }
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (Player && Player->Hp < Player->MaxHp)
        {
            Player->Hp = Minimum(Player->MaxHp,
                                 Player->Hp + REST_SHARE_PER_SECOND * Player->MaxHp * DeltaTime);
        }
    }
}

// NOTE(zoubir): once a tick, from UpdateDungeon: what the role spells
// left behind, and the rest between fights
internal void
UpdateRoleEffects(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    UpdateSanctuaries(AppState, Run, DeltaTime);
    UpdateRenewals(AppState, DeltaTime);
    UpdateInfernos(AppState, Run, DeltaTime);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        Slot->RallySeconds = Maximum(0.f, Slot->RallySeconds - DeltaTime);
    }
    CountAggro(AppState, Run);
    RestBetweenFights(AppState, Run, DeltaTime);
}
