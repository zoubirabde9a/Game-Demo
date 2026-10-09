/* The Shadowblade's kit (role_abilities.cpp, through class_kits.cpp): twin daggers.
   Numbers, talents and the spell table are role_kits/shadowblade_defs.cpp; its
   state is role_kits/shadowblade.h.

   Quick cuts build combo points (player_slot.ClassMeter, 0 to
   SHADOWBLADE_MOST_POINTS), the finisher spends them:

     right click  Twin Strike: two cuts at what is in front, the second a
                  moment after the first, poisoning what they cut; a point
                  when either lands.
     A            Shadowstep: the Shadowblade vanishes and stands behind
                  that foe; its next strike within SHADOWBLADE_CRIT_SECONDS
                  is critical. A point.
     R            Fan of Knives: a short crouch (PlayerSpell_ShadowbladeA),
                  then knives cut every foe round it, a point each.
     W            Eviscerate: with points and a foe in reach, the
                  daggers go up overhead (PlayerSpell_ShadowbladeB), then
                  drop on that foe for EVISCERATE_PER_POINT a point,
                  spending them all. With no points it does not cast, and
                  the player is shown so (ShadowbladeBurst_Empty); with no
                  foe in reach it does not cast either.
     X            Deadly Throw: the ranged finisher, a dagger thrown at a
                  foe up to DEADLY_THROW_RANGE off, spending every point
                  (shadowblade/deadly_throw.cpp).
     V (tree)     Shadow Dance: for DANCE_SECONDS every Twin Strike and
                  Eviscerate lands a shadow's echo too, and Shadowstep comes
                  back in DANCE_STEP_COOLDOWN.

   Of the talents with code here, Knife Storm widens Fan of Knives and
   makes it cut harder by rank, and Kidney Shot, the capstone, stuns the
   foe a full five-point Eviscerate leaves alive (StatusEffect_Stunned,
   which every client sees on the monster).

   C does nothing for it, and X is Deadly Throw, never the fireball
   (RoleDropsFireball).
   Points fade out of a fight (shadowblade/effects.cpp). The critical
   strike and Shadow Dance are ClassFlags bits, so every client draws
   them (client/dungeon/classes/shadowblade.cpp). */

inline bool32
IsShadowbladeFoe(world *World, world_entity *Monster, u32 Room)
{
    bool32 Result = Monster->IsPresent && Monster->Type == EntityType_Monster &&
        Monster->Hp > 0.f && RoomAtPosition(World, Monster->Position.XY) == Room;
    return Result;
}

// NOTE(zoubir): whether Player stands behind Foe, as Foe faces
inline bool32
ShadowbladeBehind(world_entity *Player, world_entity *Foe)
{
    v2 ToPlayer = Player->Position.XY - Foe->Position.XY;
    bool32 Result = LengthSq(Foe->Direction) > 0.0001f && LengthSq(ToPlayer) > 1.f &&
        DotProduct(DirectionTo(ToPlayer), DirectionTo(Foe->Direction)) < OPPORTUNIST_BEHIND;
    return Result;
}

// NOTE(zoubir): the ClassFlags clients read, from the Shadowblade's state;
// at once when a spell changes it, and every tick (shadowblade/effects.cpp)
inline void
SetShadowbladeFlags(player_slot *Slot, bool32 Fading)
{
    shadowblade_slot *Blade = &Slot->Shadowblade;
    Slot->ClassFlags = (u8)((Blade->DanceSeconds > 0.f ? SHADOWBLADE_FLAG_DANCE : 0) |
                            (Blade->CritSeconds > 0.f ? SHADOWBLADE_FLAG_CRIT : 0) |
                            (Fading ? SHADOWBLADE_FLAG_FADING : 0));
}

inline void
AddComboPoints(player_slot *Slot, u32 Points)
{
    Slot->ClassMeter = (u8)Minimum((u32)SHADOWBLADE_MOST_POINTS, (u32)Slot->ClassMeter + Points);
    Slot->Shadowblade.IdleSeconds = 0.f;
}

// NOTE(zoubir): one of the Shadowblade's strikes on Foe for Damage: the
// critical strike Shadowstep left goes on it (and is spent), and under
// Shadow Dance a shadow echoes it. True when Foe died of it
internal bool32
ShadowbladeStrike(app_state *AppState, player_slot *Slot, world_entity *Player, world_entity *Foe,
                  float Damage, float Shove, v2 Away)
{
    world *World = &AppState->World;
    u8 SlotIndex = (u8)Player->PlayerIndex;
    shadowblade_slot *Blade = &Slot->Shadowblade;
    if (Blade->CritSeconds > 0.f)
    {
        Blade->CritSeconds = 0.f;
        Damage *= SHADOWBLADE_CRIT_SCALE;
        EmitBurst(&AppState->Events, SimBurst_Finisher, SlotIndex, ChestOf(Foe),
                  ATan2(Away.Y, Away.X));
    }
    hit Hit = {Damage, Shove, 0.f, 0.f, 0.f, SimBurst_Count};
    ApplyHit(AppState, World, Foe, &Hit, Away, Player, SlotIndex);
    if (Blade->DanceSeconds > 0.f && Foe->IsPresent && Foe->Hp > 0.f)
    {
        DamageEntity(AppState, World, Foe, DANCE_ECHO * Damage, Player);
    }
    Blade->IdleSeconds = 0.f;
    bool32 Result = !Foe->IsPresent || Foe->Hp <= 0.f;
    return Result;
}

// NOTE(zoubir): Foe poisoned by Slot's Shadowblade, or its poison renewed
internal void
PoisonFoe(app_state *AppState, player_slot *Slot, world_entity *Foe)
{
    if (!Foe->IsPresent || Foe->Hp <= 0.f)
    {
        return;
    }
    shadowblade_run *Run = &AppState->Dungeon->Shadowblade;
    world *World = &AppState->World;
    bool32 Venom = RoleRank(Slot, PlayerRole_Shadowblade, ShadowbladeTalent_Venom) > 0;
    u32 FoeSlot = (u32)(Foe - World->Entities);
    u32 By = (u32)(Slot - AppState->Players);
    shadowblade_poison *Row = 0;
    for(u32 Index = 0; Index < SHADOWBLADE_POISONS && !Row; Index++)
    {
        shadowblade_poison *Poison = &Run->Poisons[Index];
        if (Poison->Seconds > 0.f && Poison->Slot == FoeSlot && Poison->Serial == Foe->MonsterSerial &&
            Poison->By == By)
        {
            Row = Poison;
        }
    }
    for(u32 Index = 0; Index < SHADOWBLADE_POISONS && !Row; Index++)
    {
        if (Run->Poisons[Index].Seconds <= 0.f)
        {
            Row = &Run->Poisons[Index];
            Row->TickTimer = 0.f;
        }
    }
    if (!Row)
    {
        Row = &Run->Poisons[0];
        for(u32 Index = 1; Index < SHADOWBLADE_POISONS; Index++)
        {
            if (Run->Poisons[Index].Seconds < Row->Seconds)
            {
                Row = &Run->Poisons[Index];
            }
        }
        Row->TickTimer = 0.f;
    }
    Row->Slot = FoeSlot;
    Row->Serial = Foe->MonsterSerial;
    Row->By = By;
    float Envenom = (float)RoleRank(Slot, PlayerRole_Shadowblade, ShadowbladeTalent_Envenom);
    Row->Seconds = BLADE_POISON_SECONDS + (Venom ? VENOM_SECONDS : 0.f);
    Row->PerSecond = BLADE_POISON_PER_SECOND * (Venom ? 1.f + VENOM_SHARE : 1.f) *
        (1.f + ENVENOM_SHARE * Envenom);
    // NOTE(zoubir): the status only for its look: clients see a monster's
    // statuses, and draw the drips from it
    ApplyStatus(Foe, StatusEffect_Poisoned, Row->Seconds);
}

#include "shadowblade/twin_strike.cpp"
#include "shadowblade/deadly_throw.cpp"

// NOTE(zoubir): Shadowstep leaves the Shadowblade's threat behind: every
// monster forgets it and turns on whoever it hated next
internal void
ShadowstepDropsThreat(app_state *AppState, world_entity *Player)
{
    dungeon_run *Run = AppState->Dungeon;
    if (!Run || Player->PlayerIndex >= MAX_PLAYERS)
    {
        return;
    }
    for(u32 Index = 0; Index < THREAT_ROWS; Index++)
    {
        threat_row *Row = &Run->Threat.Rows[Index];
        Row->Threat[Player->PlayerIndex] = 0.f;
    }
}

internal bool32
CastShadowstep(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
               world_entity *Player)
{
    world_entity *Foe = AttackTarget(AppState, Slot, Player, SHADOWSTEP_RANGE);
    if (!Foe)
    {
        return false;
    }
    v2 From = Player->Position.XY;
    v2 Back = LengthSq(Foe->Direction) > 0.0001f ? -1.f * DirectionTo(Foe->Direction) :
        (LengthSq(Foe->Position.XY - From) > 1.f ? DirectionTo(Foe->Position.XY - From) : V2(1.f, 0.f));
    v3 Landing = Foe->Position;
    Landing.XY += (SHADOWSTEP_GAP + 0.5f * Foe->Dimensions.X) * Back;
    Landing.Z = Player->Position.Z;
    // NOTE(zoubir): shadow drawn in where it left
    EmitBurst(&AppState->Events, SimBurst_CastGather, (u8)Player->PlayerIndex, ChestOf(Player));
    MovePlayerTo(AppState, World, Arena, Player, Landing);
    v2 Moved = Player->Position.XY - From;
    float Angle = LengthSq(Moved) > 1.f ? ATan2(Moved.Y, Moved.X) : 0.f;
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_ShadowbladeFirst, ShadowbladeBurst_Step),
              (u8)Player->PlayerIndex, ChestOf(Player), Angle);
    EmitSound(&AppState->Events, AssetType_SfxBlink, Player->Position);
    Slot->Shadowblade.CritSeconds = SHADOWBLADE_CRIT_SECONDS;
    Slot->Shadowblade.GuardSeconds = SHADOWSTEP_GUARD_SECONDS;
    ShadowstepDropsThreat(AppState, Player);
    AddComboPoints(Slot, 1);
    return true;
}

// NOTE(zoubir): whether Key only starts a wind-up when pressed (the cast
// runs through sim/player_casts.cpp, then FinishShadowbladeCast fires it)
internal bool32
ShadowbladeKeyWindsUp(u32 Key)
{
    bool32 Result = Key == 1 || Key == 4;
    return Result;
}

// NOTE(zoubir): Key pressed, off cooldown and learned; true when it cast,
// which spends the cooldown
internal bool32
CastShadowbladeKey(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
           world_entity *Player, u32 Key)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    bool32 Result = true;
    switch(Key)
    {
        case 0:
        {
            Result = CastShadowstep(AppState, World, Arena, Slot, Player);
        } break;

        case 1:
        {
            StartPlayerCast(Player, PlayerSpell_ShadowbladeA, Player->Aim);
        } break;

        case 3:
        {
            Slot->Shadowblade.DanceSeconds = DANCE_SECONDS;
            Slot->RoleCooldowns[0] = Minimum(Slot->RoleCooldowns[0], DANCE_STEP_COOLDOWN);
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_ShadowbladeFirst, ShadowbladeBurst_Dance),
                      SlotIndex, Player->Position);
            EmitSound(&AppState->Events, AssetType_SfxBlink, Player->Position);
        } break;

        case 4:
        {
            // NOTE(zoubir): nothing to spend: no cast and no cooldown, and
            // the player sees why (the predicting client leaves it to the
            // server, which sends the burst)
            if (Slot->ClassMeter == 0)
            {
                if (!Slot->Predicted)
                {
                    EmitBurst(&AppState->Events,
                              ClassBurst(SimBurst_ShadowbladeFirst, ShadowbladeBurst_Empty),
                              SlotIndex, ChestOf(Player));
                }
                return false;
            }
            // NOTE(zoubir): the foe is picked now, as Execute's is, so
            // a press with nobody in reach costs nothing, and the strike
            // finds it after the wind-up even if it moved
            world_entity *Foe = AttackTarget(AppState, Slot, Player, EVISCERATE_REACH);
            if (!Foe)
            {
                return false;
            }
            Slot->Shadowblade.EviscerateSlot = (u32)(Foe - World->Entities);
            Slot->Shadowblade.EviscerateSerial = Foe->MonsterSerial;
            v2 ToFoe = Foe->Position.XY - Player->Position.XY;
            StartPlayerCast(Player, PlayerSpell_ShadowbladeB,
                            LengthSq(ToFoe) > 1.f ? DirectionTo(ToFoe) : Player->Aim);
        } break;

        case 5:
        {
            Result = CastDeadlyThrow(AppState, World, Slot, Player);
        } break;

        case 6:
        {
            CastTwinStrike(AppState, Slot, Player);
        } break;
    }
    if (!Slot->Predicted)
    {
        SetShadowbladeFlags(Slot, false);
    }
    return Result;
}

internal void
FinishFanOfKnives(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    // NOTE(zoubir): a critical strike lands on every knife of the fan
    bool32 Crit = Slot->Shadowblade.CritSeconds > 0.f;
    bool32 Envenom = RoleRank(Slot, PlayerRole_Shadowblade, ShadowbladeTalent_Envenom) > 0;
    float Radius = RoleSpellRadius(Slot, 1);
    float Damage = FAN_OF_KNIVES_DAMAGE * (1.f + KNIFE_STORM_DAMAGE_SHARE *
        (float)RoleRank(Slot, PlayerRole_Shadowblade, ShadowbladeTalent_KnifeStorm));
    u32 Cut = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Foe = &World->Entities[EntityIndex];
        v2 Offset = Foe->Position.XY - Player->Position.XY;
        if (!IsShadowbladeFoe(World, Foe, Room) ||
            Length(Offset) > Radius + 0.5f * Foe->Dimensions.X)
        {
            continue;
        }
        if (Crit)
        {
            Slot->Shadowblade.CritSeconds = SHADOWBLADE_CRIT_SECONDS;
        }
        ShadowbladeStrike(AppState, Slot, Player, Foe, Damage, FAN_OF_KNIVES_SHOVE,
                          LengthSq(Offset) > 1.f ? DirectionTo(Offset) : V2(1.f, 0.f));
        if (Envenom)
        {
            PoisonFoe(AppState, Slot, Foe);
        }
        Cut++;
    }
    Slot->Shadowblade.CritSeconds = Crit && !Cut ? Slot->Shadowblade.CritSeconds : 0.f;
    if (Cut)
    {
        AddComboPoints(Slot, Cut);
    }
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_ShadowbladeFirst, ShadowbladeBurst_Fan),
              (u8)Player->PlayerIndex, V3(Player->Position.X, Player->Position.Y, Player->GroundZ),
              ATan2(Player->Aim.Y, Player->Aim.X));
    EmitSound(&AppState->Events, AssetType_SfxKunai, Player->Position);
}

internal void
FinishEviscerate(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    u32 Points = Slot->ClassMeter;
    shadowblade_slot *Blade = &Slot->Shadowblade;
    world_entity *Foe = Points ? FindMonsterBySerial(World, Blade->EviscerateSlot,
                                                     Blade->EviscerateSerial) : 0;
    if (Foe && (Foe->Hp <= 0.f ||
                Length(Foe->Position.XY - Player->Position.XY) >
                EVISCERATE_HOLD_REACH + 0.5f * Foe->Dimensions.X))
    {
        Foe = 0;
    }
    if (!Foe && Points)
    {
        Foe = AttackTarget(AppState, Slot, Player, EVISCERATE_REACH + 30.f);
    }
    if (!Foe)
    {
        // NOTE(zoubir): the foe got away or died in the wind-up: the daggers
        // cut the air (a burst with no points), the points stay and the
        // key is ready again
        v2 Aim = LengthSq(Player->Aim) > 0.0001f ? DirectionTo(Player->Aim) : V2(1.f, 0.f);
        v3 Air = ChestOf(Player);
        Air.XY += (0.6f * EVISCERATE_REACH) * Aim;
        EmitBurst(&AppState->Events, ClassBurst(SimBurst_ShadowbladeFirst, ShadowbladeBurst_Eviscerate),
                  (u8)Player->PlayerIndex, ShadowbladeBurstSpot(Air, 0), ATan2(Aim.Y, Aim.X));
        EmitSound(&AppState->Events, AssetType_SfxSword, Player->Position);
        Slot->RoleCooldowns[4] = 0.f;
        return;
    }
    v2 Away = LengthSq(Foe->Position.XY - Player->Position.XY) > 1.f ?
        DirectionTo(Foe->Position.XY - Player->Position.XY) : DirectionTo(Player->Aim);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_ShadowbladeFirst, ShadowbladeBurst_Eviscerate),
              (u8)Player->PlayerIndex, ShadowbladeBurstSpot(ChestOf(Foe), Points), ATan2(Away.Y, Away.X));
    EmitSound(&AppState->Events, AssetType_SfxSword, Player->Position);
    Slot->ClassMeter = 0;
    bool32 Killed = ShadowbladeStrike(AppState, Slot, Player, Foe,
                                      EVISCERATE_DAMAGE + EVISCERATE_PER_POINT * (float)Points,
                                      EVISCERATE_SHOVE, Away);
    if (Killed && RoleRank(Slot, PlayerRole_Shadowblade, ShadowbladeTalent_Relentless))
    {
        AddComboPoints(Slot, RELENTLESS_POINTS);
        Slot->RoleCooldowns[0] = 0.f;
    }
    // NOTE(zoubir): Kidney Shot: a foe that lives through a full
    // five-point Eviscerate is stunned
    if (!Killed && Points >= SHADOWBLADE_MOST_POINTS &&
        RoleRank(Slot, PlayerRole_Shadowblade, ShadowbladeTalent_KidneyShot))
    {
        ApplyStatus(Foe, StatusEffect_Stunned, KIDNEY_SHOT_SECONDS);
        Foe->ThrownBySlot = (u32)Player->PlayerIndex + 1;
    }
}

// NOTE(zoubir): a wind-up of this class is over
internal void
FinishShadowbladeCast(app_state *AppState, player_slot *Slot, world_entity *Player, player_spell Spell)
{
    if (Spell == PlayerSpell_ShadowbladeA)
    {
        FinishFanOfKnives(AppState, Slot, Player);
    }
    else if (Spell == PlayerSpell_ShadowbladeB)
    {
        FinishEviscerate(AppState, Slot, Player);
    }
    SetShadowbladeFlags(Slot, false);
}

// NOTE(zoubir): any hit of this class's player on a monster dealt Damage;
// poison ticking keeps the Shadowblade in the fight, so its points last
internal void
OnShadowbladeHit(app_state *AppState, player_slot *Attacker, world_entity *Target,
         world_entity *Source, float Damage)
{
    Attacker->Shadowblade.IdleSeconds = 0.f;
}

// NOTE(zoubir): the share of a hit on Target the player deals, and of a
// hit on it the player takes, after its talents and buffs
internal float
ShadowbladeDealtScale(player_slot *Slot, world_entity *Target)
{
    float Result = 1.f + LETHALITY_SHARE *
        (float)RoleRank(Slot, PlayerRole_Shadowblade, ShadowbladeTalent_Lethality);
    if (RoleRank(Slot, PlayerRole_Shadowblade, ShadowbladeTalent_Opportunist) && Slot->Entity &&
        ShadowbladeBehind(Slot->Entity, Target))
    {
        Result *= 1.f + OPPORTUNIST_SHARE;
    }
    return Result;
}

internal float
ShadowbladeTakenScale(player_slot *Slot, world_entity *Player)
{
    return 1.f;
}

// NOTE(zoubir): Key's cooldown and a ground spell's radius after talents,
// from the table's Base: Shadow Dance brings Shadowstep back at once, and
// Knife Storm widens Fan of Knives
internal float
ShadowbladeSpellCooldown(player_slot *Slot, u32 Key, float Base)
{
    float Result = Base;
    if (Key == 0 && Slot->Role == PlayerRole_Shadowblade && Slot->Shadowblade.DanceSeconds > 0.f)
    {
        Result = Minimum(Base, DANCE_STEP_COOLDOWN);
    }
    return Result;
}

internal float
ShadowbladeSpellRadius(player_slot *Slot, u32 Key, float Base)
{
    float Result = Base;
    if (Key == 1)
    {
        Result *= 1.f + KNIFE_STORM_RADIUS_SHARE *
            (float)RoleRank(Slot, PlayerRole_Shadowblade, ShadowbladeTalent_KnifeStorm);
    }
    return Result;
}

#include "shadowblade/effects.cpp"
