/* The Stormcaller's kit (role_abilities.cpp, through class_kits.cpp):
   lightning that leaps foe to foe, and Charge that builds until it
   overloads. Numbers, talents and the spell table are
   role_kits/stormcaller_defs.cpp; its state is role_kits/stormcaller.h.

   X  Spark: an instant bolt at the foe aimed at that jumps to the nearest
      other foe; the filler, building Charge (stormcaller/bolts.cpp).
   A  Chain Lightning: a short cast (PlayerSpell_StormcallerA), then a bolt
      at the foe it was pressed on that leaps foe to foe, never the same
      one twice, weaker each jump; Charge for every foe it struck.
      Conductor makes it leap farther.
   R  Static Field: a circle at the cursor that shocks and slows what is
      inside; a Spark or Chain Lightning landing on a foe inside arcs to
      every other foe inside (stormcaller/storm.cpp).
   W  Thunderclap: needs THUNDERCLAP_MIN_CHARGE. The foe is locked at the
      press, as Eviscerate's is; after the cast (PlayerSpell_StormcallerB)
      a bolt from the sky spends all the Charge on it. From
      STORM_SUPERCHARGED it also stuns and splashes, and with Capacitor or
      Stormbringer gives Charge back or calls more bolts.
   C  Lightning Dash (tree): a dash along the aim that shocks and slows
      what it passes.
   V  Eye of the Storm (tree): seconds of bolts from a cloud on random
      foes of the room, Charge holding at the top without overloading.

   Charge (stormcaller/charge.cpp) is a pressure gauge, not a purse: it
   fills as the Stormcaller casts, Spark and Chain Lightning hit harder
   and jump farther from STORM_SUPERCHARGED on, and at STORM_CHARGE_MOST it
   overloads: a nova round the Stormcaller that still hurts the foes near,
   then Charge is gone, health is lost and every key waits. Live Wire
   takes the cost away and makes the nova bigger. The skill is to ride
   the top band and vent with Thunderclap before it caps.

   The right click does nothing for it (the game's default), so with the
   tree it has five damage keys and a dash, as the striker does.

   Lightning lands at once, so the hits are dealt as the spell goes off;
   clients draw each bolt from a burst (StormcallerBurst_Bolt) that
   carries where it came from in its height. Charge goes to clients as
   ClassMeter and what the looks and the HUD need as ClassFlags
   (STORMCALLER_FLAG_*). */

// NOTE(zoubir): the class's bursts, SimBurst_StormcallerFirst + n; the rows
// are client/dungeon/classes/stormcaller_bursts.inc
enum stormcaller_burst
{
    StormcallerBurst_Bolt,         // a bolt ending at Position, come along Angle (StormcallerBoltSpot)
    StormcallerBurst_Field,        // a Static Field at Position, for its life; variant its radius
    StormcallerBurst_Thunderclap,  // the sky bolt on Position; variant 1 when Supercharged
    StormcallerBurst_SkyBolt,      // a small bolt from the sky (Eye of the Storm, Stormbringer)
    StormcallerBurst_Overload,     // the nova round Position; variant its radius
    StormcallerBurst_Dash,         // Lightning Dash from Position along Angle; variant its length
    StormcallerBurst_Supercharged, // Charge reaching STORM_SUPERCHARGED on the Stormcaller at Position
    StormcallerBurst_Empty,        // Thunderclap pressed short of Charge, on the Stormcaller at Position
};

// NOTE(zoubir): a bolt burst's look, the low two bits of its variant
enum stormcaller_bolt_kind
{
    StormcallerBolt_Spark,
    StormcallerBolt_Chain,
    StormcallerBolt_Arc,
    StormcallerBolt_Fizzle,
};

// NOTE(zoubir): a burst carries a small number too (a bolt's kind and
// length, a circle's radius, a dash's length), as whole steps of
// STORMCALLER_BURST_STEP added to its height, as the Ranger's do (online a
// burst's angle shrinks to a byte but its position goes whole; no burst
// is drawn that high), and StormcallerBurstPlace takes it off
#define STORMCALLER_BURST_STEP 4096.f
// NOTE(zoubir): a bolt's length rides in whole steps of this
#define STORMCALLER_BOLT_UNIT 8.f

inline v3
StormcallerBurstSpot(v3 Position, u32 Variant)
{
    v3 Result = Position;
    Result.Z += STORMCALLER_BURST_STEP * (float)Variant;
    return Result;
}

inline u32
StormcallerBurstVariant(v3 Position)
{
    float Steps = floorf((Position.Z + 0.5f * STORMCALLER_BURST_STEP) / STORMCALLER_BURST_STEP);
    u32 Result = Steps > 0.f ? (u32)Steps : 0;
    return Result;
}

// NOTE(zoubir): where the burst really is
inline v3
StormcallerBurstPlace(v3 Position)
{
    v3 Result = Position;
    Result.Z -= STORMCALLER_BURST_STEP * (float)StormcallerBurstVariant(Position);
    return Result;
}

// NOTE(zoubir): a bolt's variant: its kind, and its length in
// STORMCALLER_BOLT_UNIT steps
inline u32
StormcallerBoltVariant(u32 Kind, float Length)
{
    u32 Result = (Kind & 3) | ((u32)(Length / STORMCALLER_BOLT_UNIT + 0.5f) << 2);
    return Result;
}

inline u32
StormcallerBoltKind(u32 Variant)
{
    u32 Result = Variant & 3;
    return Result;
}

inline float
StormcallerBoltLength(u32 Variant)
{
    float Result = STORMCALLER_BOLT_UNIT * (float)(Variant >> 2);
    return Result;
}

#include "stormcaller/charge.cpp"
#include "stormcaller/bolts.cpp"
#include "stormcaller/storm.cpp"
#include "stormcaller/ball.cpp"

// NOTE(zoubir): whether Key only starts a wind-up when pressed (the cast
// runs through sim/player_casts.cpp, then FinishStormcallerCast fires it):
// Chain Lightning and Thunderclap
internal bool32
StormcallerKeyWindsUp(u32 Key)
{
    bool32 Result = Key == 0 || Key == 4;
    return Result;
}

// NOTE(zoubir): Key pressed, off cooldown and learned; true when it cast,
// which spends the cooldown. A wind-up with no foe in reach, or a
// Thunderclap short of Charge, does not
internal bool32
CastStormcallerKey(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
                   world_entity *Player, u32 Key)
{
    bool32 Result = true;
    switch(Key)
    {
        case 0:
        {
            Result = StartStormcallerWindUp(AppState, Slot, Player, PlayerSpell_StormcallerA, CHAIN_RANGE);
        } break;

        case 1:
        {
            Result = CastStaticField(AppState, Slot, Player);
        } break;

        case 2:
        {
            CastLightningDash(AppState, World, Arena, Slot, Player);
        } break;

        case 3:
        {
            CastEyeOfTheStorm(AppState, Slot, Player);
        } break;

        case 4:
        {
            // NOTE(zoubir): short of Charge: no cast and no cooldown, and
            // the player sees why. A client predicting its own Stormcaller
            // reads the Charge the server sent (ClassMeter)
            float Charge = Slot->Predicted ? (float)Slot->ClassMeter : Slot->Stormcaller.Charge;
            if (Charge < THUNDERCLAP_MIN_CHARGE)
            {
                RefuseThunderclap(AppState, Slot, Player);
                return false;
            }
            Result = StartStormcallerWindUp(AppState, Slot, Player, PlayerSpell_StormcallerB,
                                            THUNDERCLAP_RANGE);
        } break;

        case 5:
        {
            CastSpark(AppState, Slot, Player);
        } break;

        case 6:
        {
            Result = CastBallLightning(AppState, Slot, Player);
        } break;

        default:
        {
            return false;
        } break;
    }
    if (!Slot->Predicted)
    {
        SetStormcallerFlags(AppState, Slot);
    }
    return Result;
}

// NOTE(zoubir): a wind-up of this class is over
internal void
FinishStormcallerCast(app_state *AppState, player_slot *Slot, world_entity *Player, player_spell Spell)
{
    if (Spell == PlayerSpell_StormcallerA)
    {
        FinishChainLightning(AppState, Slot, Player);
    }
    else if (Spell == PlayerSpell_StormcallerB)
    {
        FinishThunderclap(AppState, Slot, Player);
    }
    SetStormcallerFlags(AppState, Slot);
}

// NOTE(zoubir): any hit of this class's player on a monster dealt Damage;
// a hit keeps the Charge from draining
internal void
OnStormcallerHit(app_state *AppState, player_slot *Attacker, world_entity *Target,
                 world_entity *Source, float Damage)
{
    Attacker->Stormcaller.IdleSeconds = 0.f;
}

// NOTE(zoubir): the share of a hit on Target the player deals, and of a
// hit on it the player takes, after its talents and buffs
internal float
StormcallerDealtScale(player_slot *Slot, world_entity *Target)
{
    float Result = 1.f + VOLTAGE_SHARE *
        (float)RoleRank(Slot, PlayerRole_Stormcaller, StormcallerTalent_Voltage);
    return Result;
}

internal float
StormcallerTakenScale(player_slot *Slot, world_entity *Player)
{
    return 1.f;
}

// NOTE(zoubir): Key's cooldown and a ground spell's radius after talents,
// from the table's Base: Lightning Dash's second rank brings it back
// sooner, and Arc Field widens Static Field
internal float
StormcallerSpellCooldown(player_slot *Slot, u32 Key, float Base)
{
    float Result = Base;
    if (Key == 2 && RoleRank(Slot, PlayerRole_Stormcaller, StormcallerTalent_LightningDash) >= 2)
    {
        Result = LIGHTNING_DASH_RANK2_COOLDOWN;
    }
    return Result;
}

internal float
StormcallerSpellRadius(player_slot *Slot, u32 Key, float Base)
{
    float Result = Base;
    if (Key == 1)
    {
        Result *= 1.f + ARC_FIELD_RADIUS_SHARE *
            (float)RoleRank(Slot, PlayerRole_Stormcaller, StormcallerTalent_ArcField);
    }
    return Result;
}

// NOTE(zoubir): developer builds: GAME_STORMCALLER gives the local
// Stormcaller what a scripted screenshot (misc\screenshot.bat) needs to
// show the class: "full" every talent, a number the Charge held there,
// and after a colon the keys it casts by itself whenever they are ready
// (A, R, V, W, X), from the seconds after an @ on, and a ! keeps its
// health full ("full85@11!:XRW").
// Lightning Dash is left out: it moves the player, which needs the arena
// this tick has not got
global_variable float StormcallerDeveloperClock;

internal void
ApplyDeveloperStormcaller(app_state *AppState, player_slot *Slot, float DeltaTime)
{
#if APP_DEV
#pragma warning(push)
#pragma warning(disable: 4996)
    char *Value = getenv("GAME_STORMCALLER");
#pragma warning(pop)
    world_entity *Player = Slot->Entity;
    if (!Value || !Value[0] || !Player || Slot != &AppState->Players[AppState->LocalPlayerIndex] ||
        Slot->Predicted || IsDeadPlayer(Player))
    {
        return;
    }
    if (Value[0] == 'f')
    {
        GrantWholeClassTree(Slot);
    }
    char *At = Value;
    while(*At && *At != ':' && (*At < '0' || *At > '9'))
    {
        At++;
    }
    if (*At >= '0' && *At <= '9' && Player->CastSpell != PlayerSpell_StormcallerB)
    {
        Slot->Stormcaller.Charge = Minimum(STORM_CHARGE_MOST - 1.f, (float)atoi(At));
        Slot->Stormcaller.IdleSeconds = 0.f;
    }
    // NOTE(zoubir): a ! keeps the Stormcaller standing while the shot waits
    if (strchr(Value, '!'))
    {
        Player->Hp = Player->MaxHp;
    }
    StormcallerDeveloperClock += DeltaTime;
    char *From = Value;
    while(*From && *From != '@')
    {
        From++;
    }
    char *Keys = Value;
    while(*Keys && *Keys != ':')
    {
        Keys++;
    }
    if (*From == '@' && StormcallerDeveloperClock < (float)atoi(From + 1))
    {
        Keys = From + 1 + strlen(From + 1);
    }
    // NOTE(zoubir): the spells go at the foe nearest it, the cursor's
    // ground spell on that foe's spot, whatever the mouse is doing
    world_entity *Near = NearestFoe(&AppState->World, Player->Position.XY, SPARK_RANGE,
                                    StormcallerRoom(AppState, Player), 0, 0);
    if (Near && *Keys)
    {
        v2 Way = Near->Position.XY - Player->Position.XY;
        Player->Aim = NormalizeOr(Way, Player->Aim);
        Player->AimReach = Minimum(1.f, Length(Way) / PLAYER_AIM_REACH);
    }
    char *Letters = "ARCVWX";
    for(; *Keys && !IsPlayerCasting(Player); Keys++)
    {
        for(u32 Key = 0; Key < 6; Key++)
        {
            if ((*Keys | 32) == (Letters[Key] | 32) && Key != 2 && Slot->RoleCooldowns[Key] <= 0.f &&
                RoleSpellLearned(Slot, Key) &&
                CastStormcallerKey(AppState, &AppState->World, 0, Slot, Player, Key))
            {
                Slot->RoleCooldowns[Key] = RoleSpellCooldown(Slot, Key);
            }
        }
    }
#endif
}

// NOTE(zoubir): once a tick, from UpdateClassEffects: what the class's
// spells left behind, for every player of the class
internal void
UpdateStormcallerEffects(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Role == PlayerRole_Stormcaller && Slot->Predicted)
        {
            // NOTE(zoubir): a client predicting its own Stormcaller leaves
            // all of it to the server, which sends the Charge and flags back
            continue;
        }
        if (Slot->Active && Slot->Role == PlayerRole_Stormcaller)
        {
            ApplyDeveloperStormcaller(AppState, Slot, DeltaTime);
            UpdateStormcallerSlot(AppState, Run, Slot, DeltaTime);
        }
        else
        {
            // NOTE(zoubir): a player who left the class leaves its Charge
            // and its storm behind
            Slot->Stormcaller.Charge = 0.f;
            Slot->Stormcaller.EyeSeconds = 0.f;
        }
    }
    UpdateStaticFields(AppState, &Run->Stormcaller, DeltaTime);
    UpdateBallLightning(AppState, &Run->Stormcaller, DeltaTime);
}
