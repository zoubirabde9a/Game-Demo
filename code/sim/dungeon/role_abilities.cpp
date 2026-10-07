/* Role abilities (docs/dungeon-plan.md, "Roles"): in a dungeon run the
   tank's and the healer's A, E and V keys cast their role's spells
   instead of Launch, Shield and the kunai. The damage role keeps the
   game's own kit. Left click, blink and jump are the same for everyone.

   Tank (Bulwark)
     A  Taunt: every monster within TAUNT_RADIUS attacks the tank.
     E  Shield Wall: SHIELD_WALL_SCALE of the damage for a few seconds.
     V  Intercept: leaps to the ally under the cursor (or the one nearest
        the aim), landing just short of them, and takes the threat off
        them. Only within the tank's own room: never through a gate.
   Healer (Mender)
     A  Sanctuary: a circle at the cursor that heals allies inside.
     E  Ward: the ally under the cursor (or the most hurt one) absorbs
        the next WARD_ABSORB of damage.
     V  Mending Bolt: heals the ally under the cursor (or the most hurt).

   UseRoleAbilities runs from UpdatePlayer before the game's abilities and
   takes the role's keys out of the input, so the game's spells on those
   keys never see them. While the client predicts its own player the keys
   are taken but nothing is cast: the server casts it. Healing makes
   threat on every monster with threat already, half the health given. */

#define TAUNT_RADIUS 260.f
#define TAUNT_COOLDOWN 8.f
#define SHIELD_WALL_SECONDS 4.f
#define SHIELD_WALL_COOLDOWN 15.f
#define INTERCEPT_RANGE 420.f
#define INTERCEPT_COOLDOWN 10.f
// NOTE(zoubir): how far short of the ally an intercept lands
#define INTERCEPT_LANDING_GAP 36.f
#define SANCTUARY_RADIUS 110.f
#define SANCTUARY_SECONDS 5.f
#define SANCTUARY_HEAL_PER_SECOND 8.f
#define SANCTUARY_COOLDOWN 14.f
#define WARD_ABSORB 30.f
#define WARD_COOLDOWN 10.f
#define MENDING_BOLT_HEAL 30.f
#define MENDING_BOLT_RANGE 500.f
#define MENDING_BOLT_COOLDOWN 2.5f
#define HEAL_THREAT_SHARE 0.5f

// NOTE(zoubir): the keys a role casts on, and the slot each one's
// cooldown has in player_slot.RoleCooldowns
global_variable u32 RoleKeys[ROLE_KEYS] =
{
    PlayerButton_Launch,
    PlayerButton_Shield,
    PlayerButton_Kunai,
};

inline bool32
RoleOwnsKeys(app_state *AppState, player_slot *Slot)
{
    bool32 Result = IsDungeon(AppState) &&
        (Slot->Role == PlayerRole_Tank || Slot->Role == PlayerRole_Healer);
    return Result;
}

// NOTE(zoubir): each role's spell on each key (RoleKeys order), as the
// ability bar names it, and its full cooldown
struct role_spell
{
    char *Name;
    float Cooldown;
    // NOTE(zoubir): one line for the controls panel (ui/controls_panel.cpp)
    char *Help;
};

global_variable role_spell RoleSpells[PlayerRole_Count][ROLE_KEYS] =
{
    {{0, 0.f, 0}, {0, 0.f, 0}, {0, 0.f, 0}},
    {{"Taunt", TAUNT_COOLDOWN, "Taunt: monsters near you attack you"},
     {"Shield Wall", SHIELD_WALL_COOLDOWN, "Shield Wall: take 40% damage for 4 s"},
     {"Intercept", INTERCEPT_COOLDOWN, "Intercept: leap to the ally under the cursor"}},
    {{"Sanctuary", SANCTUARY_COOLDOWN, "Sanctuary: a healing circle at the cursor"},
     {"Ward", WARD_COOLDOWN, "Ward: the next 30 damage on an ally is absorbed"},
     {"Mending Bolt", MENDING_BOLT_COOLDOWN, "Mending Bolt: heal an ally for 30"}},
};

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
        if (Key < ROLE_KEYS && RoleOwnsKeys(AppState, Slot))
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
    *Full = Spell->Cooldown;
    player_slot *Slot = &AppState->Players[Player->PlayerIndex];
    return &Slot->RoleCooldowns[RoleKeyForButton(Button)];
}

// NOTE(zoubir): where the cursor is on the ground, from the aim
inline v2
AimPoint(world_entity *Player)
{
    v2 Result = Player->Position.XY + (PLAYER_AIM_REACH * Player->AimReach) * Player->Aim;
    return Result;
}

// NOTE(zoubir): the living player under the cursor (player_input.Target)
// other than Self, or 0
inline world_entity *
CursorAlly(app_state *AppState, player_slot *Slot, world_entity *Self)
{
    world *World = &AppState->World;
    u32 Index = Slot->Input.Target;
    world_entity *Result = 0;
    if (Index && Index - 1 < World->EntityCount)
    {
        world_entity *Unit = &World->Entities[Index - 1];
        if (Unit != Self && Unit->IsPresent && Unit->Type == EntityType_Player &&
            !IsDeadPlayer(Unit))
        {
            Result = Unit;
        }
    }
    return Result;
}

// NOTE(zoubir): the living player within Range of From missing the most
// health (Self counts), or 0 when nobody is hurt
internal world_entity *
MostHurtAlly(app_state *AppState, v2 From, float Range)
{
    world_entity *Result = 0;
    float Worst = 0.f;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (Player && Length(Player->Position.XY - From) <= Range)
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

// NOTE(zoubir): an ally Self can leap to: alive, within Range, and in the
// same room, so a leap never crosses a gate
inline bool32
CanInterceptTo(app_state *AppState, world_entity *Self, world_entity *Ally, float Range)
{
    world *World = &AppState->World;
    bool32 Result = Ally && Ally != Self && !IsDeadPlayer(Ally) &&
        Length(Ally->Position.XY - Self->Position.XY) <= Range &&
        RoomAtPosition(World, Ally->Position.XY) == RoomAtPosition(World, Self->Position.XY);
    return Result;
}

// NOTE(zoubir): the ally Self can intercept to nearest to Point, or 0
internal world_entity *
NearestAllyTo(app_state *AppState, world_entity *Self, v2 Point, float Range)
{
    world_entity *Result = 0;
    float Best = 0.f;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (Player && CanInterceptTo(AppState, Self, Player, Range))
        {
            float Distance = Length(Player->Position.XY - Point);
            if (!Result || Distance < Best)
            {
                Best = Distance;
                Result = Player;
            }
        }
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
    float Given = Minimum(Amount, Target->MaxHp - Target->Hp);
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

inline v3
ChestOf(world_entity *Unit)
{
    v3 Result = Unit->Position;
    Result.Z += 16.f;
    return Result;
}

internal bool32
CastTankKey(app_state *AppState, world *World, memory_arena *Arena,
            player_slot *Slot, world_entity *Player, u32 Key)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    switch(Key)
    {
        case 0:
        {
            TauntAround(AppState, &AppState->Dungeon->Threat, Player, TAUNT_RADIUS);
            EmitBurst(&AppState->Events, SimBurst_ShockwaveRing, SlotIndex, Player->Position);
            Slot->RoleCooldowns[Key] = TAUNT_COOLDOWN;
        } break;

        case 1:
        {
            Slot->ShieldWallSeconds = SHIELD_WALL_SECONDS;
            EmitBurst(&AppState->Events, SimBurst_WardBreak, SlotIndex, ChestOf(Player));
            Slot->RoleCooldowns[Key] = SHIELD_WALL_COOLDOWN;
        } break;

        case 2:
        {
            world_entity *Ally = CursorAlly(AppState, Slot, Player);
            if (!CanInterceptTo(AppState, Player, Ally, INTERCEPT_RANGE))
            {
                Ally = NearestAllyTo(AppState, Player, AimPoint(Player), INTERCEPT_RANGE);
            }
            if (!Ally)
            {
                return false;
            }
            float Angle = ATan2(Ally->Position.Y - Player->Position.Y,
                                Ally->Position.X - Player->Position.X);
            EmitBurst(&AppState->Events, SimBurst_Lunge, SlotIndex, ChestOf(Player), Angle);
            v2 Toward = DirectionTo(Ally->Position.XY - Player->Position.XY);
            v3 Landing = Ally->Position;
            Landing.XY -= INTERCEPT_LANDING_GAP * Toward;
            MovePlayerTo(AppState, World, Arena, Player, Landing);
            // NOTE(zoubir): what was after the ally comes for the tank
            TauntAround(AppState, &AppState->Dungeon->Threat, Player, TAUNT_RADIUS * 0.5f);
            Slot->RoleCooldowns[Key] = INTERCEPT_COOLDOWN;
        } break;
    }
    return true;
}

internal bool32
CastHealerKey(app_state *AppState, player_slot *Slot, world_entity *Player, u32 Key)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    dungeon_run *Run = AppState->Dungeon;
    switch(Key)
    {
        case 0:
        {
            sanctuary *Free = 0;
            for(u32 Index = 0; Index < MAX_SANCTUARIES; Index++)
            {
                if (Run->Sanctuaries[Index].Seconds <= 0.f)
                {
                    Free = &Run->Sanctuaries[Index];
                    break;
                }
            }
            if (!Free)
            {
                return false;
            }
            v2 Point = AimPoint(Player);
            Free->Position = V3(Point.X, Point.Y, 0.f);
            Free->Seconds = SANCTUARY_SECONDS;
            Free->By = SlotIndex;
            EmitBurst(&AppState->Events, SimBurst_LaunchMark, SlotIndex, Free->Position);
            Slot->RoleCooldowns[Key] = SANCTUARY_COOLDOWN;
        } break;

        case 1:
        case 2:
        {
            world_entity *Ally = CursorAlly(AppState, Slot, Player);
            if (!Ally || Length(Ally->Position.XY - Player->Position.XY) > MENDING_BOLT_RANGE)
            {
                Ally = MostHurtAlly(AppState, Player->Position.XY, MENDING_BOLT_RANGE);
            }
            if (!Ally)
            {
                Ally = Player;
            }
            if (Key == 1)
            {
                player_slot *AllySlot = &AppState->Players[Ally->PlayerIndex];
                AllySlot->WardAbsorb = WARD_ABSORB;
                Slot->RoleCooldowns[Key] = WARD_COOLDOWN;
            }
            else
            {
                HealPlayer(AppState, SlotIndex, Ally, MENDING_BOLT_HEAL);
                Slot->RoleCooldowns[Key] = MENDING_BOLT_COOLDOWN;
            }
            EmitBurst(&AppState->Events, SimBurst_Spawn, (u8)Ally->PlayerIndex, Ally->Position);
        } break;
    }
    return true;
}

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
    if (!RoleOwnsKeys(AppState, Slot))
    {
        return;
    }
    for(u32 Key = 0; Key < ROLE_KEYS; Key++)
    {
        u32 Button = RoleKeys[Key];
        bool32 Pressed = WasPressed(&Slot->Input, Button);
        Slot->Input.Pressed &= ~Button;
        Slot->Input.ServerPressed &= ~Button;
        if (!Pressed || Slot->Predicted || Slot->RoleCooldowns[Key] > 0.f ||
            IsDeadPlayer(Player))
        {
            continue;
        }
        if (Slot->Role == PlayerRole_Tank)
        {
            CastTankKey(AppState, World, Arena, Slot, Player, Key);
        }
        else
        {
            CastHealerKey(AppState, Slot, Player, Key);
        }
    }
}

// NOTE(zoubir): once a tick: each sanctuary heals the living players in it
internal void
UpdateSanctuaries(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    for(u32 Index = 0; Index < MAX_SANCTUARIES; Index++)
    {
        sanctuary *Zone = &Run->Sanctuaries[Index];
        if (Zone->Seconds <= 0.f)
        {
            continue;
        }
        Zone->Seconds -= DeltaTime;
        for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
        {
            world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
            if (Player && Length(Player->Position.XY - Zone->Position.XY) <= SANCTUARY_RADIUS)
            {
                HealPlayer(AppState, Zone->By, Player, SANCTUARY_HEAL_PER_SECOND * DeltaTime);
            }
        }
    }
}
