/* The Frost Mage's kit (role_abilities.cpp, through class_kits.cpp): ice.
   Numbers, talents and the spell table are role_kits/frostmage_defs.cpp;
   its state is role_kits/frostmage.h.

   X  Frostbolt: a bolt at the foe aimed at, the filler; it chills the foe
      (slowed) and grows an Icicle (frostmage/bolts.cpp). With Fingers of
      Frost every fourth one shatters and grows two.
   A  Blizzard: ice falls on the circle at the cursor for three seconds,
      striking and chilling (frostmage/ground.cpp).
   R  Glacial Spike: a 1.25 s cast (PlayerSpell_FrostMageA), then a spike
      at the foe aimed at that spends every Icicle for more damage; on
      five it freezes the foe (stunned), and with Absolute Zero every foe
      near it. Splitting Ice throws half of it on to the next foe.
   W  Frost Nova: every foe round the mage is frozen in place (rooted).
   C  Ice Barrier (tree): a shield of ice that takes the next hits.
   V  Frozen Orb (tree): an orb rolls along the aim, striking and chilling
      what is near it, each strike growing an Icicle.

   Shatter: every hit of the Frost Mage on a frozen foe, rooted or
   stunned, deals FROST_SHATTER_SHARE more. So the rotation is Frostbolt
   to five Icicles, freeze a pack with Frost Nova and Blizzard it, and
   spend the Icicles on a Glacial Spike, which freezes its foe for the
   bolts after it.

   The right click does nothing for it; it owns X, so it has no shared
   fireball. Icicles go to clients as ClassMeter, and what the HUD and the
   looks need as ClassFlags (FROSTMAGE_FLAG_*). Every burst carries a
   small number in its height as the Ranger's do (RangerBurstSpot,
   role_kits/ranger.cpp), and a foe is picked as the Ranger picks one
   (RangerTarget). */

// NOTE(zoubir): the class's bursts, SimBurst_FrostMageFirst + n; the rows
// are client/dungeon/classes/frostmage_bursts.inc
enum frostmage_burst
{
    FrostBurst_Bolt,     // a Frostbolt arriving at Position along Angle (FrostBolt_*)
    FrostBurst_Blizzard, // a Blizzard's circle at Position, for its life
    FrostBurst_Spike,    // a Glacial Spike arriving at Position along Angle; its Icicles, +8 split
    FrostBurst_Nova,     // a Frost Nova round Position; Deep Freeze's ranks
    FrostBurst_Barrier,  // Ice Barrier raised on the mage at Position
    FrostBurst_Orb,      // a Frozen Orb leaving Position along Angle
    FrostBurst_Freeze,   // a foe at Position frozen; seconds in halves
    FrostBurst_Full,     // five Icicles on the mage at Position
};

// NOTE(zoubir): what a hit is, for OnFrostMageHit and the bolts in flight
enum frostmage_shot
{
    FrostShot_None,
    FrostShot_Bolt,
    FrostShot_Spike,
    FrostShot_SplitSpike,
    FrostShot_Blizzard,
    FrostShot_Nova,
    FrostShot_Orb,
    FrostShot_Cone,
};

// NOTE(zoubir): a Frostbolt burst's look
enum frostmage_bolt_variant
{
    FrostBolt_Plain,
    FrostBolt_Miss,
    FrostBolt_Fingers,
};

#include "frostmage/bolts.cpp"
#include "frostmage/ground.cpp"

// NOTE(zoubir): whether Key only starts a wind-up when pressed (the cast
// runs through sim/player_casts.cpp, then FinishFrostMageCast fires it)
internal bool32
FrostMageKeyWindsUp(u32 Key)
{
    return Key == 1;
}

// NOTE(zoubir): Key pressed, off cooldown and learned; true when it cast,
// which spends the cooldown
internal bool32
CastFrostMageKey(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
                 world_entity *Player, u32 Key)
{
    switch(Key)
    {
        case 0: return CastBlizzard(AppState, Slot, Player);
        case 1:
        {
            StartPlayerCast(Player, PlayerSpell_FrostMageA, GetPlayerAim(Player));
            if (!Slot->Predicted)
            {
                EmitSound(&AppState->Events, AssetType_SfxMeteorCast, Player->Position);
            }
        } break;
        case 2: CastIceBarrier(AppState, Slot, Player); break;
        case 3: return CastFrozenOrb(AppState, Slot, Player);
        case 4: CastFrostNova(AppState, Slot, Player); break;
        case 5: CastFrostbolt(AppState, Slot, Player); break;
        case 6: CastConeOfCold(AppState, Slot, Player); break;
        default: return false;
    }
    return true;
}

// NOTE(zoubir): a wind-up of this class is over
internal void
FinishFrostMageCast(app_state *AppState, player_slot *Slot, world_entity *Player, player_spell Spell)
{
    if (Spell == PlayerSpell_FrostMageA)
    {
        LooseGlacialSpike(AppState, Slot, Player);
    }
}

// NOTE(zoubir): any hit of this class's player on a monster dealt Damage;
// Icicles grow where the bolt lands (UpdateFrostBolts), so nothing here
internal void
OnFrostMageHit(app_state *AppState, player_slot *Attacker, world_entity *Target,
               world_entity *Source, float Damage)
{
}

// NOTE(zoubir): the share of a hit on Target the player deals: Frostbite,
// and Shatter on a frozen foe or a Fingers of Frost bolt
internal float
FrostMageDealtScale(player_slot *Slot, world_entity *Target)
{
    float Result = 1.f + FROSTBITE_SHARE * (float)RoleRank(Slot, PlayerRole_FrostMage,
                                                           FrostMageTalent_Frostbite);
    bool32 Frozen = Target && Target->Type == EntityType_Monster && IsFrozenFoe(Target);
    if (Frozen || Slot->FrostMage.Shattering)
    {
        Result *= 1.f + FROST_SHATTER_SHARE;
    }
    return Result;
}

internal float
FrostMageTakenScale(player_slot *Slot, world_entity *Player)
{
    return 1.f;
}

internal float
FrostMageSpellCooldown(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

internal float
FrostMageSpellRadius(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

// NOTE(zoubir): once a tick for each Frost Mage: Icicles melting between
// fights, and what clients see of it
internal void
UpdateFrostMageSlot(app_state *AppState, dungeon_run *Run, player_slot *Slot, float DeltaTime)
{
    frostmage_slot *Mage = &Slot->FrostMage;
    Mage->IcicleHold = Maximum(0.f, Mage->IcicleHold - DeltaTime);
    if (!Run->FightingRoom && Mage->IcicleHold <= 0.f && Mage->Icicles)
    {
        Mage->MeltTimer += DeltaTime;
        if (Mage->MeltTimer >= 1.f)
        {
            Mage->MeltTimer -= 1.f;
            Mage->Icicles--;
        }
    }
    else
    {
        Mage->MeltTimer = 0.f;
    }
    u32 Index = (u32)(Slot - AppState->Players);
    bool32 Fingers = RoleRank(Slot, PlayerRole_FrostMage, FrostMageTalent_FingersOfFrost) > 0 &&
        ((Mage->Bolts + 1) % FINGERS_OF_FROST_EVERY) == 0;
    Slot->ClassMeter = (u8)Mage->Icicles;
    Slot->ClassFlags = (u8)((Slot->FireguardAbsorb > 0.f ? FROSTMAGE_FLAG_BARRIER : 0) |
                            (Fingers ? FROSTMAGE_FLAG_FINGERS : 0) |
                            (FrozenOrbRolls(&Run->FrostMage, Index) ? FROSTMAGE_FLAG_ORB : 0) |
                            (Mage->Icicles >= FROSTMAGE_ICICLES_MOST ? FROSTMAGE_FLAG_FULL : 0));
}

// NOTE(zoubir): once a tick, from UpdateRoleEffects: what the class's
// spells left behind, for every player of the class
internal void
UpdateFrostMageEffects(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Role == PlayerRole_FrostMage)
        {
            UpdateFrostMageSlot(AppState, Run, Slot, DeltaTime);
        }
        else
        {
            // NOTE(zoubir): a player who left the class leaves its Icicles
            Slot->FrostMage.Icicles = 0;
            Slot->FrostMage.Bolts = 0;
        }
    }
    UpdateFrostBolts(AppState, &Run->FrostMage, DeltaTime);
    UpdateBlizzards(AppState, &Run->FrostMage, DeltaTime);
    UpdateFrozenOrbs(AppState, &Run->FrostMage, DeltaTime);
}
