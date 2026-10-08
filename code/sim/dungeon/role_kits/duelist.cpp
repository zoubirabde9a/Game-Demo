/* The Duelist's kit (role_abilities.cpp, through class_kits.cpp): a rapier.
   Numbers, talents and the spell table are role_kits/duelist_defs.cpp; its
   state is role_kits/duelist.h.

   It is the class for one big foe: Tempo (player_slot.ClassMeter, 0 to
   DUELIST_MOST_TEMPO) is a buff it keeps, built by weaving its keys and
   lost to the hits it takes (duelist/tempo.cpp), and one foe at a time
   is all most of its blows reach.

     right click  Thrust: a quick stab at the first foe in a narrow line
                  in front.
     A            Lunge: a dash to just in front of the foe under the
                  cursor, and a strike; with Footwork the Duelist runs
                  faster for a moment after it.
     R            Riposte: on guard for a moment; the first hit in it is
                  parried and the attacker countered (duelist/riposte.cpp).
     W            Heartseeker: picks the foe in front when pressed (no foe
                  in reach, no cast), draws the point back
                  (PlayerSpell_DuelistB), then strikes it, harder by
                  Tempo, which it does not spend, and harder again on a
                  hurt foe. At full Tempo, Crescendo sweeps the foes beside
                  it and Masterstroke strikes it a second time.
     V (tree)     Perfect Form: for FORM_SECONDS Tempo holds, every key
                  builds it, and every Thrust strikes twice.

   C and X do nothing for it (RoleDropsFireball), so it has five damage
   keys with its whole tree.

   Online the clients see Tempo and the flags (ClassFlags), Heartseeker's
   cast and the bursts, SimBurst_DuelistFirst + duelist_burst; how they
   look is client/dungeon/classes/duelist.cpp. Every blow goes through
   ApplyHit without a burst of the hit's own: clients draw the sparks from
   the hit itself (HitFresh, HitBySlot). */

// NOTE(zoubir): the ClassFlags clients read, from the Duelist's state; at
// once when a spell changes it, and every tick (duelist/effects.cpp)
inline void
SetDuelistFlags(player_slot *Slot, bool32 Fading)
{
    duelist_slot *Duel = &Slot->Duelist;
    bool32 Guard = Duel->GuardSeconds > 0.f;
    Slot->ClassFlags = (u8)((Guard ? DUELIST_FLAG_GUARD : 0) |
                            (Guard && Duel->Parried ? DUELIST_FLAG_PARRIED : 0) |
                            (Slot->ClassMeter >= DUELIST_MOST_TEMPO ? DUELIST_FLAG_TEMPO : 0) |
                            (Duel->FormSeconds > 0.f ? DUELIST_FLAG_FORM : 0) |
                            (Fading ? DUELIST_FLAG_FADING : 0));
}

#include "duelist/tempo.cpp"
#include "duelist/strikes.cpp"
#include "duelist/riposte.cpp"

// NOTE(zoubir): whether Key only starts a wind-up when pressed (the cast
// runs through sim/player_casts.cpp, then FinishDuelistCast fires it):
// Heartseeker
internal bool32
DuelistKeyWindsUp(u32 Key)
{
    bool32 Result = Key == 4;
    return Result;
}

// NOTE(zoubir): Key pressed, off cooldown and learned; true when it cast,
// which spends the cooldown. Lunge and Heartseeker with no foe in reach
// do not
internal bool32
CastDuelistKey(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
               world_entity *Player, u32 Key)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    bool32 Result = true;
    switch(Key)
    {
        case 0:
        {
            Result = CastLunge(AppState, World, Arena, Slot, Player);
        } break;

        case 1:
        {
            StartGuard(AppState, Slot, Player);
        } break;

        case 3:
        {
            if (DuelistUseKey(Slot, 3))
            {
                AddTempo(Slot, 1);
            }
            Slot->Duelist.FormSeconds = FORM_SECONDS;
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_DuelistFirst, DuelistBurst_Form),
                      SlotIndex, Player->Position);
            EmitSound(&AppState->Events, AssetType_SfxCombustion, Player->Position);
        } break;

        case 4:
        {
            // NOTE(zoubir): the foe is picked now, as Eviscerate's is, so a
            // press with nobody in reach costs nothing, and the strike
            // finds it after the wind-up even if it moved. A client
            // predicting its own Duelist only starts the wind-up
            world_entity *Foe = AttackTarget(AppState, Slot, Player, HEARTSEEKER_REACH);
            if (!Foe)
            {
                return false;
            }
            if (!Slot->Predicted)
            {
                Slot->Duelist.HeartSlot = (u32)(Foe - World->Entities);
                Slot->Duelist.HeartSerial = Foe->MonsterSerial;
                Slot->Duelist.HeartFresh = DuelistUseKey(Slot, 4);
                Slot->Duelist.MasterDelay = 0.f;
                EmitSound(&AppState->Events, AssetType_SfxMeteorCast, Player->Position);
            }
            v2 ToFoe = Foe->Position.XY - Player->Position.XY;
            StartPlayerCast(Player, PlayerSpell_DuelistB,
                            LengthSq(ToFoe) > 1.f ? DirectionTo(ToFoe) : Player->Aim);
        } break;

        case 6:
        {
            CastThrust(AppState, Slot, Player);
        } break;
    }
    if (!Slot->Predicted)
    {
        SetDuelistFlags(Slot, false);
    }
    return Result;
}

// NOTE(zoubir): a wind-up of this class is over
internal void
FinishDuelistCast(app_state *AppState, player_slot *Slot, world_entity *Player, player_spell Spell)
{
    if (Spell == PlayerSpell_DuelistB)
    {
        FinishHeartseeker(AppState, Slot, Player);
    }
    SetDuelistFlags(Slot, false);
}

// NOTE(zoubir): any hit of this class's player on a monster dealt Damage;
// it keeps the Duelist in the fight, so Tempo holds
internal void
OnDuelistHit(app_state *AppState, player_slot *Attacker, world_entity *Target,
             world_entity *Source, float Damage)
{
    Attacker->Duelist.IdleSeconds = 0.f;
}

// NOTE(zoubir): the share of a hit on Target the player deals, and of a
// hit on it the player takes, after its talents and buffs: Finesse and
// Tempo on what it deals; in Riposte's guard it takes nothing, the damage
// that is not a blow included (a burn, poison), which DuelistParriesHit
// does not see
internal float
DuelistDealtScale(player_slot *Slot, world_entity *Target)
{
    float Result = (1.f + FINESSE_SHARE * (float)RoleRank(Slot, PlayerRole_Duelist, DuelistTalent_Finesse)) *
        (1.f + TEMPO_SHARE * (float)DuelistTempo(Slot));
    return Result;
}

internal float
DuelistTakenScale(player_slot *Slot, world_entity *Player)
{
    float Result = Slot->Duelist.GuardSeconds > 0.f ? 0.f : 1.f;
    return Result;
}

// NOTE(zoubir): Key's cooldown and a ground spell's radius after talents,
// from the table's Base: Footwork brings Lunge back sooner
internal float
DuelistSpellCooldown(player_slot *Slot, u32 Key, float Base)
{
    float Result = Base;
    if (Key == 0)
    {
        Result -= FOOTWORK_COOLDOWN * (float)RoleRank(Slot, PlayerRole_Duelist, DuelistTalent_Footwork);
    }
    return Result;
}

internal float
DuelistSpellRadius(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

#include "duelist/effects.cpp"
