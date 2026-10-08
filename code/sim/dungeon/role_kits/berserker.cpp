/* The Berserker's kit (role_abilities.cpp, through class_kits.cpp): a great axe.
   Numbers, talents and the spell table are role_kits/berserker_defs.cpp; its
   state is role_kits/berserker.h.

   It fights in the melee with the tank and runs on Rage (ClassMeter,
   0..100): every hit it lands gives Rage by the damage dealt, every hit it
   takes by the health lost, and some BERSERKER_CALM_SECONDS after the
   last of either the Rage drains away.

     right click  Cleave: a wide swing through every foe in front, each
                  swing the other way round from the one before.
     X            Axe Throw: a hand axe at the foe aimed at; it lands after
                  its flight, hurts and slows, and flies back.
     A            Leap: a high jump to the cursor; landing strikes and
                  stuns everything round the spot (berserker/leap.cpp).
     R            Whirlwind: costs Rage, then spins for its cast, hitting
                  everything round the Berserker WHIRLWIND_HITS times.
     W            Execute: needs Rage; the axe goes up for its wind-up,
                  then one chop spends all the Rage, the more the harder,
                  twice as hard on a foe near death.
     C            Bloodthirst (tree): a strike that heals for part of it.
     V            Berserk (tree): seconds of more damage, less taken, and
                  Rage that holds.

   Online the clients see Rage and the flags (ClassFlags), the casts and
   the bursts, SimBurst_BerserkerFirst + berserker_burst; how they look is
   client/dungeon/classes/berserker.cpp. Every blow goes through ApplyHit
   without a burst: clients draw the blood-red sparks from the hit itself
   (HitFresh, HitBySlot). */

#include "berserker/rage.cpp"
#include "berserker/strikes.cpp"
#include "berserker/leap.cpp"

// NOTE(zoubir): whether Key only starts a wind-up when pressed (the cast
// runs through sim/player_casts.cpp, then FinishBerserkerCast fires it):
// Whirlwind and Execute
internal bool32
BerserkerKeyWindsUp(u32 Key)
{
    bool32 Result = Key == 1 || Key == 4;
    return Result;
}

// NOTE(zoubir): Key pressed, off cooldown and learned; true when it cast,
// which spends the cooldown. A spell short of Rage, or with no foe in
// reach, does not
internal bool32
CastBerserkerKey(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
           world_entity *Player, u32 Key)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    v2 Aim = GetPlayerAim(Player);
    switch(Key)
    {
        case 0:
        {
            StartLeap(AppState, Slot, Player);
        } break;

        case 1:
        {
            if (!SpendRage(Slot, WHIRLWIND_RAGE))
            {
                return false;
            }
            Slot->Berserker.WhirlHits = 0;
            StartPlayerCast(Player, PlayerSpell_BerserkerA, Aim);
            if (!Slot->Predicted)
            {
                EmitSound(&AppState->Events, AssetType_SfxAreaCast, Player->Position);
            }
        } break;

        case 2:
        {
            return CastBloodthirst(AppState, Slot, Player);
        } break;

        case 3:
        {
            Slot->Berserker.BerserkSeconds = BERSERK_SECONDS;
            Slot->ClassFlags |= BERSERKER_FLAG_BERSERK;
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_BerserkerFirst, BerserkerBurst_Berserk),
                      SlotIndex, Player->Position, ATan2(Aim.Y, Aim.X));
            EmitSound(&AppState->Events, AssetType_SfxTaunt, Player->Position);
            EmitSound(&AppState->Events, AssetType_SfxCombustion, Player->Position);
        } break;

        case 4:
        {
            world_entity *Foe = AttackTarget(AppState, Slot, Player, EXECUTE_REACH);
            if (!Foe || !HasRage(Slot, EXECUTE_MIN_RAGE))
            {
                if (Foe)
                {
                    LackRage(Slot);
                }
                return false;
            }
            Slot->RoleCastPoint = Foe->Position.XY;
            StartPlayerCast(Player, PlayerSpell_BerserkerB,
                            DirectionTo(Foe->Position.XY - Player->Position.XY));
            if (!Slot->Predicted)
            {
                EmitSound(&AppState->Events, AssetType_SfxMeteorCast, Player->Position);
            }
        } break;

        case 5:
        {
            return ThrowAxe(AppState, Slot, Player);
        } break;

        case 6:
        {
            Cleave(AppState, World, Slot, Player);
        } break;
    }
    return true;
}

// NOTE(zoubir): a wind-up of this class is over
internal void
FinishBerserkerCast(app_state *AppState, player_slot *Slot, world_entity *Player, player_spell Spell)
{
    if (Spell == PlayerSpell_BerserkerA)
    {
        while(Slot->Berserker.WhirlHits < WHIRLWIND_HITS)
        {
            WhirlHit(AppState, Slot, Player);
        }
    }
    else if (Spell == PlayerSpell_BerserkerB)
    {
        Execute(AppState, Slot, Player);
    }
}

// NOTE(zoubir): any hit of this class's player on a monster dealt Damage:
// Rage by the damage
internal void
OnBerserkerHit(app_state *AppState, player_slot *Attacker, world_entity *Target,
         world_entity *Source, float Damage)
{
    AddRage(Attacker, RAGE_PER_DAMAGE * Damage);
    Attacker->Berserker.CalmSeconds = 0.f;
}

// NOTE(zoubir): the share of a hit on Target the player deals, and of a
// hit on it the player takes, after its talents and buffs
internal float
BerserkerDealtScale(player_slot *Slot, world_entity *Target)
{
    float Result = 1.f + BRUTALITY_SHARE *
        (float)RoleRank(Slot, PlayerRole_Berserker, BerserkerTalent_Brutality);
    if (Slot->Berserker.BerserkSeconds > 0.f)
    {
        Result *= 1.f + BERSERK_DEALT_SHARE;
    }
    return Result;
}

internal float
BerserkerTakenScale(player_slot *Slot, world_entity *Player)
{
    float Result = Slot->Berserker.BerserkSeconds > 0.f ? 1.f - BERSERK_TAKEN_SHARE : 1.f;
    return Result;
}

// NOTE(zoubir): Key's cooldown and a ground spell's radius after talents,
// from the table's Base
internal float
BerserkerSpellCooldown(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

internal float
BerserkerSpellRadius(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

// NOTE(zoubir): developer builds, offline: GAME_BERSERKER=full gives a
// Berserker every talent and a full bar of Rage, kept full, so a scripted
// screenshot (misc\screenshot.bat) can show every spell and the looks Rage
// lights up
internal void
ApplyDeveloperBerserker(player_slot *Slot)
{
#if APP_DEV
#pragma warning(push)
#pragma warning(disable: 4996)
    char *Value = getenv("GAME_BERSERKER");
#pragma warning(pop)
    if (Value && Value[0] == 'f')
    {
        for(u32 Talent = 0; Talent < ROLE_TALENTS; Talent++)
        {
            Slot->Ranks[Talent_RoleFirst + Talent] = (u8)BerserkerTalentDefs[Talent].MaxLevel;
        }
        Slot->ClassMeter = BERSERKER_RAGE_MAX;
    }
#endif
}

// NOTE(zoubir): once a tick, from UpdateRoleEffects: Rage, Berserk, the
// axes in flight, the Whirlwind's spin and the Leaps, for every
// Berserker
internal void
UpdateBerserkerEffects(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    UpdateBerserkerAxes(AppState, Run, DeltaTime);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        // NOTE(zoubir): a client predicting its own Berserker leaves all
        // of it to the server, which sends the Rage and the flags back
        if (!Slot->Active || Slot->Role != PlayerRole_Berserker || !Slot->Entity ||
            Slot->Predicted)
        {
            continue;
        }
        world_entity *Player = Slot->Entity;
        ApplyDeveloperBerserker(Slot);
        UpdateRage(Slot, Player, DeltaTime);
        if (Player->CastSpell == PlayerSpell_BerserkerA)
        {
            float CastTime = PlayerSpells[PlayerSpell_BerserkerA].CastTime;
            float Done = CastTime - Player->CastLeft;
            u32 Due = Minimum((u32)(Done / (CastTime / (float)WHIRLWIND_HITS)),
                              (u32)WHIRLWIND_HITS - 1);
            while(Slot->Berserker.WhirlHits < Due)
            {
                WhirlHit(AppState, Slot, Player);
            }
        }
        UpdateLeap(AppState, Slot, Player, DeltaTime);
        u32 Flags = 0;
        Flags |= Slot->Berserker.BerserkSeconds > 0.f ? BERSERKER_FLAG_BERSERK : 0;
        Flags |= Slot->Berserker.NoRageSeconds > 0.f ? BERSERKER_FLAG_NO_RAGE : 0;
        Flags |= Slot->Berserker.Leaping ? BERSERKER_FLAG_LEAPING : 0;
        Slot->ClassFlags = (u8)Flags;
    }
}
