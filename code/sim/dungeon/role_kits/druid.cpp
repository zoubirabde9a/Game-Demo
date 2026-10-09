/* The Druid's kit (role_abilities.cpp, through class_kits.cpp): half
   healer, half caster, in the healer's column of the role picker.
   Numbers, talents and the spell table are role_kits/druid_defs.cpp; its
   state is role_kits/druid.h.

   Right click  Wrath: a quick bolt of nature at a foe; grows a Bloom.
   X  Moonfire: moonlight burns a foe, a little at once and more each
      second for MOONFIRE_SECONDS.
   A  Rejuvenation: an ally heals over time, and at once for each Bloom
      spent (druid/growth.cpp).
   R  Starfire: a 1.5 s cast (PlayerSpell_DruidA), then a star falls on a
      foe; grows two Bloom. With Eclipse it hits a foe under the Druid's
      Moonfire harder.
   W  Regrowth: an ally heals at once, more for each Bloom spent.
   C  Entangling Roots (tree): roots every foe in a circle at the cursor
      and bites them each second.
   V  Tranquility (tree): a 3 s channel (PlayerSpell_DruidB) that heals
      every ally near the Druid each TRANQUILITY_TICK.

   Bloom is what ties the halves together: the damage spells grow it, up
   to DRUID_BLOOM_MOST, and the next Rejuvenation or Regrowth spends all
   of it for more healing. So a Druid with a quiet party spends its time
   on Wrath and Starfire and banks Bloom, and when the party gets hurt the
   first heal lands big. Symbiosis turns some of the damage into healing
   as it lands. Like the Mender, a living Druid revives a downed ally by
   standing over the body (sim/dungeon/revive.cpp).

   Bloom goes to clients as ClassMeter, and what the HUD and the looks
   need as ClassFlags (DRUID_FLAG_*). */

// NOTE(zoubir): the class's bursts, SimBurst_DruidFirst + n; the rows are
// client/dungeon/classes/druid_bursts.inc
enum druid_burst
{
    DruidBurst_Wrath,        // a Wrath bolt arriving at Position along Angle
    DruidBurst_Moonfire,     // moonlight on the foe at Position; variant 1 a burn sent again
    DruidBurst_Starfire,     // a star falling on Position; variant 1 at nothing
    DruidBurst_Rejuvenation, // leaves round the ally at Position; variant 1 sent again
    DruidBurst_Regrowth,     // a bloom on the ally at Position; variant the Bloom spent
    DruidBurst_Roots,        // roots growing at Position; variant 1 the longer hold
    DruidBurst_Tranquility,  // one of Tranquility's heals, round the Druid's feet at Position
    DruidBurst_BloomFull,    // Bloom coming full on the Druid at Position
};

// NOTE(zoubir): what a hit is, for OnDruidHit, DruidDealtScale and the
// bolts on their way
enum druid_shot
{
    DruidShot_None,
    DruidShot_Wrath,
    DruidShot_Starfire,
    DruidShot_Moonfire,
    DruidShot_Roots,
    DruidShot_Starfall,
};

// NOTE(zoubir): a Regrowth burst's variant that is Symbiosis's touch, not
// a Regrowth
#define DRUID_REGROWTH_SYMBIOSIS 9

// NOTE(zoubir): a burst carries a small number too (a variant), as whole
// steps of DRUID_BURST_STEP added to its height, the way the Ranger's
// do (role_kits/ranger.cpp): online a burst's position goes whole
#define DRUID_BURST_STEP 4096.f

inline v3
DruidBurstSpot(v3 Position, u32 Variant)
{
    v3 Result = Position;
    Result.Z += DRUID_BURST_STEP * (float)Variant;
    return Result;
}

inline u32
DruidBurstVariant(v3 Position)
{
    float Steps = floorf((Position.Z + 0.5f * DRUID_BURST_STEP) / DRUID_BURST_STEP);
    u32 Result = Steps > 0.f ? (u32)Steps : 0;
    return Result;
}

// NOTE(zoubir): where the burst really is
inline v3
DruidBurstPlace(v3 Position)
{
    v3 Result = Position;
    Result.Z -= DRUID_BURST_STEP * (float)DruidBurstVariant(Position);
    return Result;
}

#include "druid/bolts.cpp"
#include "druid/growth.cpp"
#include "druid/bloom_spells.cpp"

// NOTE(zoubir): whether Key only starts a wind-up when pressed (the cast
// runs through sim/player_casts.cpp, then FinishDruidCast fires it)
internal bool32
DruidKeyWindsUp(u32 Key)
{
    return Key == 1 || Key == 3;
}

// NOTE(zoubir): Key pressed, off cooldown and learned; true when it cast,
// which spends the cooldown
internal bool32
CastDruidKey(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
             world_entity *Player, u32 Key)
{
    switch(Key)
    {
        case 0:
        {
            CastRejuvenation(AppState, Slot, Player);
        } break;

        case 1:
        {
            StartPlayerCast(Player, PlayerSpell_DruidA, GetPlayerAim(Player));
            if (!Slot->Predicted)
            {
                EmitSound(&AppState->Events, AssetType_SfxMeteorCast, Player->Position);
            }
        } break;

        case 2:
        {
            return CastEntanglingRoots(AppState, Slot, Player, Key);
        } break;

        case 3:
        {
            // NOTE(zoubir): the first heal goes a tick into the channel;
            // the last as it ends (FinishDruidCast)
            StartPlayerCast(Player, PlayerSpell_DruidB, GetPlayerAim(Player));
            Slot->Druid.TranquilityTimer = 0.f;
        } break;

        case 4:
        {
            CastRegrowth(AppState, Slot, Player);
        } break;

        case 5:
        {
            return CastMoonfire(AppState, Slot, Player);
        } break;

        case 6:
        {
            return CastWrath(AppState, Slot, Player);
        } break;

        case 7:
        {
            CastLifebloom(AppState, Slot, Player);
        } break;

        case 8:
        {
            return CastStarfall(AppState, Slot, Player);
        } break;

        default:
        {
            return false;
        } break;
    }
    return true;
}

// NOTE(zoubir): a wind-up of this class is over
internal void
FinishDruidCast(app_state *AppState, player_slot *Slot, world_entity *Player, player_spell Spell)
{
    if (Spell == PlayerSpell_DruidA)
    {
        DropStarfire(AppState, Slot, Player);
    }
    else if (Spell == PlayerSpell_DruidB)
    {
        PulseTranquility(AppState, Player);
    }
}

// NOTE(zoubir): any hit of this class's player on a monster dealt Damage:
// Wrath and Starfire grow Bloom, one more on a foe under the Druid's
// Moonfire with Lunar Bloom (the Moon branch's capstone), and with
// Symbiosis heal for it
internal void
OnDruidHit(app_state *AppState, player_slot *Attacker, world_entity *Target,
           world_entity *Source, float Damage)
{
    u32 Shot = Attacker->Druid.Hitting;
    u32 Bloom = DruidShotBloom(Shot);
    if (Bloom && RoleRank(Attacker, PlayerRole_Druid, DruidTalent_LunarBloom) &&
        IsDruidMoonfired(AppState, (u32)(Attacker - AppState->Players), Target))
    {
        Bloom++;
    }
    if (Bloom)
    {
        AddDruidBloom(AppState, Attacker, Bloom);
        if (RoleRank(Attacker, PlayerRole_Druid, DruidTalent_Symbiosis))
        {
            DruidSymbiosis(AppState, Attacker, Damage);
        }
    }
}

// NOTE(zoubir): the share of a hit on Target the player deals, and of a
// hit on it the player takes, after its talents (Eclipse is the star's
// own, UpdateDruidBolts)
internal float
DruidDealtScale(player_slot *Slot, world_entity *Target)
{
    float Result = 1.f + NATURES_WRATH_SHARE *
        (float)RoleRank(Slot, PlayerRole_Druid, DruidTalent_NaturesWrath);
    return Result;
}

internal float
DruidTakenScale(player_slot *Slot, world_entity *Player)
{
    return 1.f;
}

// NOTE(zoubir): Key's cooldown and a ground spell's radius after talents,
// from the table's Base
internal float
DruidSpellCooldown(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

internal float
DruidSpellRadius(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

// NOTE(zoubir): whether any of List's Count rows of the Druid in slot By
// is running, for its flags
#define DRUID_ANY_OF(List, Count, Field, By, Result) \
    for(u32 Row = 0; Row < (Count); Row++) { (Result) |= (List)[Row].Field > 0.f && (List)[Row].By == (By); }

// NOTE(zoubir): once a tick for each Druid: Tranquility's heals, Bloom
// fading between fights, and what clients see of it
internal void
UpdateDruidSlot(app_state *AppState, dungeon_run *Run, player_slot *Slot, float DeltaTime)
{
    druid_slot *Druid = &Slot->Druid;
    world_entity *Player = Slot->Entity;
    bool32 Channel = Player && !IsDeadPlayer(Player) && Player->CastSpell == PlayerSpell_DruidB;
    if (Channel)
    {
        Druid->TranquilityTimer += DeltaTime;
        // NOTE(zoubir): the last heal goes as the cast ends (FinishDruidCast)
        if (Druid->TranquilityTimer >= TRANQUILITY_TICK && Player->CastLeft > DeltaTime)
        {
            Druid->TranquilityTimer -= TRANQUILITY_TICK;
            PulseTranquility(AppState, Player);
        }
    }
    Druid->BloomHold = Maximum(0.f, Druid->BloomHold - DeltaTime);
    if (!Run->FightingRoom && Druid->BloomHold <= 0.f && Druid->Bloom)
    {
        Druid->BloomFade -= DeltaTime;
        if (Druid->BloomFade <= 0.f)
        {
            Druid->BloomFade += 1.f;
            Druid->Bloom--;
        }
    }
    u32 Index = (u32)(Slot - AppState->Players);
    bool32 Rejuv = false;
    bool32 Moon = false;
    bool32 Roots = false;
    druid_run *Kept = &Run->Druid;
    for(u32 Row = 0; Row < DRUID_MAX_REJUVENATIONS; Row++)
    {
        Rejuv |= Kept->Rejuvenations[Row].Seconds > 0.f && Kept->Rejuvenations[Row].By == Index;
    }
    for(u32 Row = 0; Row < DRUID_MAX_MOONFIRES; Row++)
    {
        Moon |= Kept->Moonfires[Row].Seconds > 0.f && Kept->Moonfires[Row].By == Index;
    }
    for(u32 Row = 0; Row < DRUID_MAX_ROOTS; Row++)
    {
        Roots |= Kept->Roots[Row].Seconds > 0.f && Kept->Roots[Row].By == Index;
    }
    Slot->ClassMeter = (u8)Druid->Bloom;
    Slot->ClassFlags = (u8)((Druid->Bloom >= DRUID_BLOOM_MOST ? DRUID_FLAG_BLOOM_FULL : 0) |
                            (Channel ? DRUID_FLAG_TRANQUILITY : 0) |
                            (Rejuv ? DRUID_FLAG_REJUVENATION : 0) |
                            (Moon ? DRUID_FLAG_MOONFIRE : 0) |
                            (Roots ? DRUID_FLAG_ROOTS : 0));
}

// NOTE(zoubir): once a tick, from UpdateRoleEffects: what the class's
// spells left behind, for every player of the class
internal void
UpdateDruidEffects(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Role == PlayerRole_Druid)
        {
            UpdateDruidSlot(AppState, Run, Slot, DeltaTime);
        }
        else
        {
            // NOTE(zoubir): a player who left the class leaves its Bloom
            Slot->Druid.Bloom = 0;
        }
    }
    UpdateDruidBolts(AppState, &Run->Druid, DeltaTime);
    UpdateDruidMoonfires(AppState, &Run->Druid, DeltaTime);
    UpdateDruidRejuvenations(AppState, &Run->Druid, DeltaTime);
    UpdateDruidRoots(AppState, &Run->Druid, DeltaTime);
    UpdateDruidLifeblooms(AppState, &Run->Druid, DeltaTime);
}
