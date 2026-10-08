/* Role abilities (docs/dungeon-plan.md, "Roles"): in a dungeon run each
   class casts its own spells on A, R, C and V (RoleSpells), the tank and
   the healer an attack on W too, and everyone shares the game's fireball
   (X), shield (E) and blink (F), and jumps. Every other game ability
   does nothing in a run (RunAllowedButtons).
   A, R and W are the class's main spells; C and V come from its tree
   (role_talents.cpp), and until a point unlocks one its key does nothing.
   A class may also own X (the fireball's key) and the right click (the
   sword's): the melee classes swing their own weapon there. The classes
   after the first three are role_kits/<class>.cpp, through
   role_kits/class_kits.cpp, each in its own files.

   Tank (Bulwark, role_kits/tank.cpp)
     A  Taunt: every monster near the tank attacks it, and Shield Wall
        goes up for a moment.
     R  Shield Slam: stuns, sunders and hurts what is round the tank,
        heals it for each one struck, raises Shield Wall on it and
        rallies the allies near it.
     C  Intercept: leaps to an ally and takes the threat off them.
     V  Last Stand: heals the tank and raises Shield Wall.
     W  Shield Throw: the shield hits a foe and bounces to two more,
        sundering each.
     X  Shield Charge: rushes a foe, stuns it and cancels its wind-up.
   Healer (Mender, role_kits/healer.cpp)
     A  Mending Bolt: heals an ally.
     R  Ward: an ally absorbs the next hits, and the allies round them
        absorb half as much.
     C  Sanctuary: a circle at the cursor that heals allies inside.
     V  Radiance: heals and wards every ally round the healer.
     W  Holy Fire: a bolt of light at a foe that heals the most hurt
        ally for what it deals.
   Damage (Striker, role_kits/striker.cpp)
     A  Meteor: a 1 s cast, then a meteor at the cursor and burning
        ground.
     R  Giant Fireball: a 1.5 s cast, then a slow fireball that blows up
        on the first monster it reaches.
     C  Detonate: blows up the Searing marks fireballs and meteors leave.
     V  Combustion: a few seconds of far more damage.
   The tank and the healer also have a weak basic attack on the right
   click, so neither is ever idle in a fight: Shield Bash and Smite Bolt.
   Every class's fireball (X) means something to it (OnRoleHit): the
   striker's leave Searing stacks, the tank's keep a Sunder going, the
   healer's heal the most hurt ally.

   Who an ally spell lands on: the player the client says is under the
   cursor or picked on the party frames (player_input.Target, which may
   be the caster), else the most hurt player in reach, else the nearest
   other ally in reach. Only a healer with nobody in reach casts on
   themselves, so no healer spell is a self heal (role_kits/allies.cpp).

   UseRoleAbilities runs from UpdatePlayer before the game's abilities and
   takes the class's keys out of the input, so the game's spells on those
   keys never see them. While the client predicts its own player the keys
   are taken but nothing is cast: the server casts it. A spell with a cast
   time starts the game's cast (sim/player_casts.cpp), so it shows a cast
   bar everywhere, and goes off in FinishRoleCast. What lasts after a cast
   (sanctuaries, infernos, giant fireballs, rallies, renewals) runs once a
   tick in UpdateRoleEffects. The class's tree changes the numbers
   (role_talents.cpp).

   Between fights every living player heals REST_SHARE_PER_SECOND of
   their health a second, so a party walks into the next room whole
   whether or not it has a healer. */

// NOTE(zoubir): the keys a role casts on, and the slot each one's
// cooldown has in player_slot.RoleCooldowns
global_variable u32 RoleKeys[ROLE_KEYS] =
{
    PlayerButton_Launch,
    PlayerButton_Push,
    PlayerButton_Slam,
    PlayerButton_Kunai,
    PlayerButton_Shockwave,
    PlayerButton_Cast,
    PlayerButton_Attack,
};

// NOTE(zoubir): the game's abilities every class keeps in a run
#define DUNGEON_SHARED_BUTTONS (PlayerButton_Jump | PlayerButton_Cast | \
                                PlayerButton_Shield | PlayerButton_Blink)

global_variable role_spell StrikerSpells[ROLE_KEYS] =
{
    {"Meteor", INFERNO_COOLDOWN, "Meteor: 1 s cast, a meteor at the cursor that marks and burns",
      RoleAim_Ground, INFERNO_RADIUS, 0},
     {"Giant Fireball", GIANT_FIREBALL_COOLDOWN,
      "Giant Fireball: 1.5 s cast, a slow fireball that blows up a pack",
      RoleAim_Line, GIANT_FIREBALL_RANGE, 0},
     {"Detonate", DETONATE_COOLDOWN,
      "Detonate: blow up the Searing marks on a foe; in fire, every marked foe there",
      RoleAim_Foe, DETONATE_RANGE, StrikerTalent_Detonate + 1},
     {"Combustion", COMBUSTION_COOLDOWN, "Combustion: 6 s of 40% more damage",
      RoleAim_None, 0.f, StrikerTalent_Combustion + 1},
     {}};

global_variable role_spell TankSpells[ROLE_KEYS] =
{
    {"Taunt", TAUNT_COOLDOWN, "Taunt: monsters near you attack you; Shield Wall 2 s",
      RoleAim_None, 0.f, 0},
     {"Shield Slam", SHIELD_SLAM_COOLDOWN,
      "Shield Slam: stun and sunder what is near (+15% damage taken), heal per foe, shield allies",
      RoleAim_None, 0.f, 0},
     {"Intercept", INTERCEPT_COOLDOWN, "Intercept: leap to an ally and pull their foes",
      RoleAim_Ally, INTERCEPT_RANGE, TankTalent_Intercept + 1},
     {"Last Stand", LAST_STAND_COOLDOWN, "Last Stand: heal 30%, Shield Wall for 6 s",
      RoleAim_None, 0.f, TankTalent_LastStand + 1},
     {"Shield Throw", SHIELD_THROW_COOLDOWN,
      "Shield Throw: hit a foe and bounce to two more, sundering each",
      RoleAim_Foe, SHIELD_THROW_RANGE, 0},
     {"Shield Charge", SHIELD_CHARGE_COOLDOWN,
      "Shield Charge: rush a foe and stun it 2 s; an attack it is winding up is cancelled",
      RoleAim_Foe, SHIELD_CHARGE_RANGE, 0},
     {"Shield Bash", SHIELD_BASH_COOLDOWN, "Shield Bash: strike what is in front with your shield",
      RoleAim_None, 0.f, 0}};

global_variable role_spell HealerSpells[ROLE_KEYS] =
{
    {"Mending Bolt", MENDING_BOLT_COOLDOWN, "Mending Bolt: heal an ally", RoleAim_Ally,
      MENDING_BOLT_RANGE, 0},
     {"Ward", WARD_COOLDOWN, "Ward: shield an ally (+12% damage while it holds), half on allies near",
      RoleAim_Ally, MENDING_BOLT_RANGE, 0},
     {"Sanctuary", SANCTUARY_COOLDOWN, "Sanctuary: a healing circle at the cursor",
      RoleAim_Ground, SANCTUARY_RADIUS, HealerTalent_Sanctuary + 1},
     {"Radiance", RADIANCE_COOLDOWN, "Radiance: heal and ward every ally around you",
      RoleAim_None, 0.f, HealerTalent_Radiance + 1},
     {"Holy Fire", HOLY_FIRE_COOLDOWN,
      "Holy Fire: strike a foe with light; the most hurt ally heals for it",
      RoleAim_Foe, HOLY_FIRE_RANGE, 0},
     {},
     {"Smite Bolt", SMITE_BOLT_COOLDOWN,
      "Smite Bolt: a quick bolt of light at a foe; the most hurt ally heals a little",
      RoleAim_None, SMITE_BOLT_RANGE, 0}};

// NOTE(zoubir): by player_role; the later classes' rows are their
// role_kits/<class>_defs.cpp
global_variable role_spell *RoleSpells[PlayerRole_Count] =
{
    StrikerSpells, TankSpells, HealerSpells, RangerSpells, BerserkerSpells, ShadowbladeSpells,
};

// NOTE(zoubir): whether a class has a kit yet, a spell on its first
// key: the role picker and the bots leave out one that has not
inline bool32
RoleHasKit(u32 Role)
{
    bool32 Result = Role < PlayerRole_Count && RoleSpells[Role][0].Name != 0;
    return Result;
}

// NOTE(zoubir): the later classes' hooks (role_kits/class_kits.cpp)
internal bool32 ClassKeyWindsUp(player_slot *Slot, u32 Key);
internal float ClassSpellCooldown(player_slot *Slot, u32 Key, float Base);
internal float ClassSpellRadius(player_slot *Slot, u32 Key, float Base);
internal void FinishClassCast(app_state *AppState, player_slot *Slot, world_entity *Player,
                              player_spell Spell);

// NOTE(zoubir): Key's cooldown for Slot's role after its talents
internal float
RoleSpellCooldown(player_slot *Slot, u32 Key)
{
    u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
    float Result = Key < ROLE_KEYS ? RoleSpells[Role][Key].Cooldown : 0.f;
    if (Key == 0)
    {
        Result -= PROVOKE_COOLDOWN * (float)RoleRank(Slot, PlayerRole_Tank, TankTalent_Provoke);
    }
    Result = ClassSpellCooldown(Slot, Key, Result);
    Result *= RoleStatCooldownScale(Slot);
    return Result;
}

// NOTE(zoubir): a ground spell's radius for Slot after its talents
internal float
RoleSpellRadius(player_slot *Slot, u32 Key)
{
    u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
    float Result = Key < ROLE_KEYS ? RoleSpells[Role][Key].Reach : 0.f;
    if (Key == 2 && RoleRank(Slot, PlayerRole_Healer, HealerTalent_Sanctuary) >= 2)
    {
        Result *= HALLOWED_RADIUS;
    }
    Result = ClassSpellRadius(Slot, Key, Result);
    return Result;
}

// NOTE(zoubir): whether Slot may cast Key's spell: a main spell always, a
// tree spell once a point unlocked it, a key with no spell for the class
// (the striker's W) never
inline bool32
RoleSpellLearned(player_slot *Slot, u32 Key)
{
    u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
    u32 Unlock = Key < ROLE_KEYS ? RoleSpells[Role][Key].Unlock : 0;
    bool32 Result = Key < ROLE_KEYS && RoleSpells[Role][Key].Name &&
        (!Unlock || RoleRank(Slot, Role, Unlock - 1) > 0);
    return Result;
}

// NOTE(zoubir): the classes the shared fireball is no part of: their own
// spells fill five damage keys without it, as the striker's four and the
// fireball do, so X does nothing for them
inline bool32
RoleDropsFireball(u32 Role)
{
    bool32 Result = Role == PlayerRole_Berserker || Role == PlayerRole_Shadowblade;
    return Result;
}

// NOTE(zoubir): the buttons that do anything for Slot: in a dungeon run
// the shared ones and the class spells it has, else Allowed, the game's
// (UpdatePlayer, the ability bar, the client's aim)
internal u32
RunAllowedButtons(app_state *AppState, player_slot *Slot, u32 Allowed)
{
    u32 Result = Allowed;
    if (IsDungeon(AppState))
    {
        Result = DUNGEON_SHARED_BUTTONS;
        if (RoleDropsFireball(Slot->Role))
        {
            Result &= ~(u32)PlayerButton_Cast;
        }
        for(u32 Key = 0; Key < ROLE_KEYS; Key++)
        {
            if (RoleSpellLearned(Slot, Key))
            {
                Result |= RoleKeys[Key];
            }
        }
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

// NOTE(zoubir): whether Slot's spell on Key only starts a wind-up
// (sim/player_casts.cpp) when pressed: the striker's Meteor and Giant
// Fireball, and what the later classes say
inline bool32
RoleKeyWindsUp(player_slot *Slot, u32 Key)
{
    bool32 Result = (Slot->Role == PlayerRole_Damage && Key <= 1) || ClassKeyWindsUp(Slot, Key);
    return Result;
}

// NOTE(zoubir): whether Talent does nothing for Slot in this run, so it
// takes no point (LearnTalent, the talent panel): in a dungeon run only
// the class's own tree counts
internal bool32
RoleReplacesTalent(app_state *AppState, player_slot *Slot, u32 Talent)
{
    bool32 Result = IsDungeon(AppState) && Talent < Talent_Count && !IsRoleTalent(Talent);
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

#include "role_kits/allies.cpp"
#include "role_kits/foe_pick.cpp"
#include "role_kits/foe_marks.cpp"
#include "role_kits/tank.cpp"
#include "role_kits/healer.cpp"
#include "role_kits/striker.cpp"
#include "role_kits/class_kits.cpp"

// NOTE(zoubir): from DungeonScaleDamage: Attacker's hit on a monster dealt
// Damage; a fireball means something to each class
internal void
OnRoleHit(app_state *AppState, player_slot *Attacker, world_entity *Target,
          world_entity *Source, float Damage)
{
    OnClassHit(AppState, Attacker, Target, Source, Damage);
    float Lifesteal = RoleStatShare(Attacker, RoleStat_Lifesteal);
    if (Lifesteal > 0.f && Attacker->Entity && Damage > 0.f)
    {
        HealPlayer(AppState, Attacker->Entity->PlayerIndex, Attacker->Entity, Lifesteal * Damage);
    }
    if (!Source || Source->Type != EntityType_FireBall)
    {
        return;
    }
    switch(Attacker->Role)
    {
        case PlayerRole_Damage: OnStrikerShot(AppState, Target); break;
        case PlayerRole_Tank: OnTankShot(AppState, Attacker, Target); break;
        case PlayerRole_Healer:
        {
            if (Attacker->Entity)
            {
                OnHealerShot(AppState, Attacker->Entity, Damage);
            }
        } break;
    }
}

// NOTE(zoubir): a class key pressed with this much of its cooldown left
// still casts, the rest added to the next cooldown, so the rate holds.
// The same 0.25 s the game's sword and fireball keep a press for
// (PLAYER_ACTION_LINGER): without it a click a moment early was lost, and
// a Shadowblade's half-second Twin Strike dropped most of a player's clicks
#define ROLE_EARLY_PRESS_SECONDS 0.25f

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
        // NOTE(zoubir): one cast at a time: nothing while a spell winds up.
        // A client predicting its own player starts only the wind-ups
        // (the pose, the slowdown, the bar); the rest waits for the server
        if (!Pressed || (Slot->Predicted && !RoleKeyWindsUp(Slot, Key)) ||
            Slot->RoleCooldowns[Key] > ROLE_EARLY_PRESS_SECONDS ||
            IsDeadPlayer(Player) || !RoleSpellLearned(Slot, Key) ||
            IsPlayerCasting(Player))
        {
            continue;
        }
        float Early = Slot->RoleCooldowns[Key];
        bool32 Cast = false;
        switch(Slot->Role)
        {
            case PlayerRole_Tank: Cast = CastTankKey(AppState, World, Arena, Slot, Player, Key); break;
            case PlayerRole_Healer: Cast = CastHealerKey(AppState, Slot, Player, Key); break;
            case PlayerRole_Damage: Cast = CastStrikerKey(AppState, Slot, Player, Key); break;
            default: Cast = CastClassKey(AppState, World, Arena, Slot, Player, Key); break;
        }
        if (Cast)
        {
            // NOTE(zoubir): a cast may give part of its cooldown back
            // (Overload, role_kits/striker.cpp)
            Slot->RoleCooldowns[Key] = Maximum(0.f, Early + RoleSpellCooldown(Slot, Key) - Slot->CastRefund);
            Slot->CastRefund = 0.f;
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
    UpdateGiantFireballs(AppState, Run, DeltaTime);
    UpdateClassEffects(AppState, Run, DeltaTime);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        Slot->RallySeconds = Maximum(0.f, Slot->RallySeconds - DeltaTime);
        Slot->CombustSeconds = Maximum(0.f, Slot->CombustSeconds - DeltaTime);
    }
    CountAggro(AppState, Run);
    RestBetweenFights(AppState, Run, DeltaTime);
}
