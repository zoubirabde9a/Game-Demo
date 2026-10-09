/* What talents change in play (talents.cpp has the table and the point
   rules): cooldowns and power by ability level, the longer stuns and
   harder shoves of a levelled area spell (AbilityLevelHit), and the duel
   perks (duel_perks.cpp): fireball damage, speed and slow, run speed,
   the damage a player deals and takes (DuelScaleDamage), its health,
   the Ward that takes one hit whole (WardTakesHit), Second Wind, Thorns,
   Momentum and the rest. The simulation calls these where it uses each
   number; a player with no talents gets the rules' plain values. */

inline player_slot *
TalentSlotOf(app_state *AppState, world_entity *Player)
{
    player_slot *Result = (Player->Type == EntityType_Player &&
                           Player->PlayerIndex < MAX_PLAYERS) ?
        &AppState->Players[Player->PlayerIndex] : 0;
    return Result;
}

// NOTE(zoubir): what Button's cooldown is multiplied by at its level
inline float
CooldownScaleForLevel(u32 Level, float PerLevel = TALENT_COOLDOWN_PER_LEVEL)
{
    float Result = Level > 1 ? 1.f - PerLevel * (float)(Level - 1) : 1.f;
    return Result;
}

// NOTE(zoubir): Talent's cooldown at Level, with the duel's own scale on
// the ability, as a share of the ability row's
inline float
TalentCooldownScale(u32 Talent, u32 Level)
{
    talent_def *Def = &TalentDefs[Talent];
    float PerLevel = Def->CooldownPerLevel > 0.f ? Def->CooldownPerLevel : TALENT_COOLDOWN_PER_LEVEL;
    float Result = CooldownScaleForLevel(Level, PerLevel);
    if (Def->DuelCooldownScale > 0.f && GameRules.RoundBreaks)
    {
        Result *= Def->DuelCooldownScale;
    }
    return Result;
}

// NOTE(zoubir): what the duel perks do to Button's cooldown
internal float
DuelPerkCooldownScale(player_slot *Slot, u32 Button)
{
    float Result = 1.f;
    switch (Button)
    {
        case PlayerButton_Cast:
        {
            Result *= 1.f - DUEL_KINDLING_COOLDOWN * (float)DuelPerkRank(Slot, DuelPerk_Kindling);
            Result *= 1.f + DUEL_PYRE_COOLDOWN * (float)DuelPerkRank(Slot, DuelPerk_Pyre);
        } break;
        case PlayerButton_Blink:
        {
            Result *= 1.f - DUEL_QUICKSTEP_COOLDOWN * (float)DuelPerkRank(Slot, DuelPerk_Quickstep);
            Result *= 1.f + DUEL_PHANTOM_COOLDOWN * (float)DuelPerkRank(Slot, DuelPerk_Phantom);
        } break;
        case PlayerButton_Kunai:
        {
            Result *= 1.f - DUEL_HUNTER_COOLDOWN * (float)DuelPerkRank(Slot, DuelPerk_Hunter);
        } break;
        case PlayerButton_Shield:
        {
            Result *= 1.f + DUEL_BASTION_COOLDOWN * (float)DuelPerkRank(Slot, DuelPerk_Bastion);
        } break;
    }
    return Result;
}

internal float
PlayerCooldownScale(app_state *AppState, world_entity *Player, u32 Button)
{
    player_slot *Slot = TalentSlotOf(AppState, Player);
    u32 Talent = TalentForButton(Button);
    float Result = 1.f;
    if (Slot && Talent < Talent_Count)
    {
        Result = TalentCooldownScale(Talent, AbilityLevel(Slot, Button)) *
            DuelPerkCooldownScale(Slot, Button);
    }
    return Result;
}

internal float
PlayerPowerScale(app_state *AppState, world_entity *Player, u32 Button)
{
    player_slot *Slot = TalentSlotOf(AppState, Player);
    u32 Talent = TalentForButton(Button);
    float Result = 1.f;
    if (Slot && Talent < Talent_Count)
    {
        u32 Level = TalentLevel(Slot, Talent);
        if (Level > 1)
        {
            Result += TalentDefs[Talent].PowerPerLevel * (float)(Level - 1);
        }
    }
    return Result;
}

// NOTE(zoubir): levels Player has bought past the first in Button's
// ability, 0 for none
internal float
ExtraAbilityLevels(app_state *AppState, world_entity *Player, u32 Button, u32 *Talent)
{
    player_slot *Slot = TalentSlotOf(AppState, Player);
    *Talent = TalentForButton(Button);
    float Result = 0.f;
    if (Slot && *Talent < Talent_Count)
    {
        u32 Level = TalentLevel(Slot, *Talent);
        Result = Level > 1 ? (float)(Level - 1) : 0.f;
    }
    return Result;
}

// NOTE(zoubir): Hit as Player's level of Button's ability lands it:
// longer stuns and statuses, harder shoves
internal hit
AbilityLevelHit(app_state *AppState, world_entity *Player, u32 Button, hit *Hit)
{
    hit Result = *Hit;
    u32 Talent;
    float Extra = ExtraAbilityLevels(AppState, Player, Button, &Talent);
    if (Extra > 0.f)
    {
        talent_def *Def = &TalentDefs[Talent];
        if (Result.StunSeconds > 0.f)
        {
            Result.StunSeconds += Def->StunPerLevel * Extra;
        }
        if (Result.StatusSeconds > 0.f)
        {
            Result.StatusSeconds += Def->StatusPerLevel * Extra;
        }
        Result.Shove *= 1.f + Def->ShovePerLevel * Extra;
    }
    return Result;
}

// NOTE(zoubir): a duel perk's rank on the player behind Player, 0 for none
inline u32
PlayerPerkRank(app_state *AppState, world_entity *Player, u32 Perk)
{
    player_slot *Slot = Player ? TalentSlotOf(AppState, Player) : 0;
    u32 Result = DuelPerkRank(Slot, Perk);
    return Result;
}

// NOTE(zoubir): the fireball's speed at Level with Swift Flames at
// SwiftRank, as a share of the plain one (the panel shows it too)
inline float
FireballScaleAt(u32 Level, u32 SwiftRank)
{
    float Extra = Level > 1 ? (float)(Level - 1) : 0.f;
    float Result = (1.f + DUEL_SWIFT_FLAMES_SCALE * (float)SwiftRank) *
        (1.f + TalentDefs[Talent_Fireball].SpeedPerLevel * Extra);
    return Result;
}

// NOTE(zoubir): Swift Flames' ranks and the fireball's own levels
inline float
FireballSpeedScale(app_state *AppState, world_entity *Player)
{
    player_slot *Slot = TalentSlotOf(AppState, Player);
    float Result = Slot ? FireballScaleAt(TalentLevel(Slot, Talent_Fireball),
                                          DuelPerkRank(Slot, DuelPerk_SwiftFlames)) : 1.f;
    return Result;
}

// NOTE(zoubir): the fireball's damage as a share of the plain one, at
// Level with Searing at SearingRank and Pyre
inline float
FireballDamageScaleAt(u32 Level, u32 SearingRank, u32 PyreRank)
{
    float Extra = Level > 1 ? (float)(Level - 1) : 0.f;
    float Result = 1.f + TalentDefs[Talent_Fireball].PowerPerLevel * Extra +
        DUEL_SEARING_DAMAGE * (float)SearingRank;
    Result *= 1.f + DUEL_PYRE_DAMAGE * (float)PyreRank;
    return Result;
}

// NOTE(zoubir): the hit a fireball of Owner's lands: its levels, Searing
// and Pyre on the damage, Scorch's slow
internal hit
FireballHitFor(app_state *AppState, world_entity *Owner, hit *Row)
{
    hit Result = *Row;
    player_slot *Slot = Owner ? TalentSlotOf(AppState, Owner) : 0;
    if (Slot)
    {
        Result.Damage *= FireballDamageScaleAt(TalentLevel(Slot, Talent_Fireball),
                                               DuelPerkRank(Slot, DuelPerk_Searing),
                                               DuelPerkRank(Slot, DuelPerk_Pyre));
        u32 Scorch = DuelPerkRank(Slot, DuelPerk_Scorch);
        if (Scorch)
        {
            Result.Status = StatusEffect_Slowed;
            Result.StatusSeconds = DUEL_SCORCH_SLOW * (float)Scorch;
        }
    }
    return Result;
}

// NOTE(zoubir): in sim/dungeon/role_stats.cpp: a class's Swiftness
internal float RoleRunScale(app_state *AppState, world_entity *Player);

inline float
RunSpeedScale(app_state *AppState, world_entity *Player)
{
    float Result = 1.f + DUEL_FLEET_FOOT_SCALE *
        (float)PlayerPerkRank(AppState, Player, DuelPerk_FleetFoot);
    if (Player->MaxHp > 0.f && Player->Hp < 0.5f * Player->MaxHp &&
        PlayerPerkRank(AppState, Player, DuelPerk_Adrenaline))
    {
        Result += DUEL_ADRENALINE_SCALE;
    }
    Result *= RoleRunScale(AppState, Player);
    return Result;
}

inline float
RespawnSeconds(player_slot *Slot)
{
    float Result = PLAYER_RESPAWN_SECONDS;
    return Result;
}

inline float
RespawnShieldSeconds(player_slot *Slot)
{
    float Result = PLAYER_SPAWN_SHIELD_SECONDS;
    return Result;
}

// NOTE(zoubir): the slot of the player behind a hit's Source: itself, or
// the owner of its sword, fireball or kunai
inline player_slot *
HitOwnerSlot(app_state *AppState, world_entity *Source)
{
    player_slot *Result = 0;
    if (Source && Source->Type == EntityType_Player && Source->PlayerIndex < MAX_PLAYERS)
    {
        Result = &AppState->Players[Source->PlayerIndex];
    }
    else if (Source && Source->HasOwner && Source->OwnerSlot < MAX_PLAYERS)
    {
        Result = &AppState->Players[Source->OwnerSlot];
    }
    return Result;
}

// NOTE(zoubir): Damage from Source on Target after the duel perks:
// Overcharge and Keen Edge on what a player deals, Thick Skin and
// Overcharge on what one takes (DealDamage). A dungeon run scales its own (DungeonScaleDamage)
internal float
DuelScaleDamage(app_state *AppState, world_entity *Target, world_entity *Source, float Damage)
{
    if (AppState->Dungeon)
    {
        return Damage;
    }
    player_slot *Attacker = HitOwnerSlot(AppState, Source);
    float Scale = 1.f;
    if (Attacker)
    {
        Scale += DUEL_OVERCHARGE_DAMAGE * (float)DuelPerkRank(Attacker, DuelPerk_Overcharge);
        if (Source->Type == EntityType_Sword || Source->Type == EntityType_Kunai)
        {
            Scale += DUEL_KEEN_EDGE_DAMAGE * (float)DuelPerkRank(Attacker, DuelPerk_KeenEdge);
        }
    }
    player_slot *Victim = TalentSlotOf(AppState, Target);
    if (Victim)
    {
        Scale *= Maximum(0.f, 1.f - DUEL_THICK_SKIN_SHARE * (float)DuelPerkRank(Victim, DuelPerk_ThickSkin));
        Scale *= 1.f + DUEL_OVERCHARGE_TAKEN * (float)DuelPerkRank(Victim, DuelPerk_Overcharge);
    }
    float Result = Damage * Scale;
    return Result;
}

// NOTE(zoubir): a stun or freeze on Target after Steadfast
inline float
DuelStunSeconds(app_state *AppState, world_entity *Target, float Seconds)
{
    float Result = Seconds * Maximum(0.f, 1.f - DUEL_STEADFAST_SHARE *
                                     (float)PlayerPerkRank(AppState, Target, DuelPerk_Steadfast));
    return Result;
}

// NOTE(zoubir): the Ward talent taking a hit of Damage on Target instead
// of it; true when it did (DamageEntity, ApplyHit). A spent ward comes
// back after its rank's seconds (UpdateProgression)
internal bool32
WardTakesHit(app_state *AppState, world_entity *Target, float Damage)
{
    player_slot *Slot = TalentSlotOf(AppState, Target);
    u32 Rank = DuelPerkRank(Slot, DuelPerk_Ward);
    bool32 Result = false;
    if (Slot && Damage > 0.f && Slot->WardReady && Rank && !Slot->Predicted)
    {
        Slot->WardReady = false;
        Slot->WardRecharge = DuelWardSeconds[Minimum(Rank, 2u)];
        v3 Chest = Target->Position;
        Chest.Z += 16.f;
        EmitBurst(&AppState->Events, SimBurst_WardBreak,
                  (u8)Target->PlayerIndex, Chest);
        EmitSound(&AppState->Events, AssetType_SfxWardBreak, Target->Position);
        Result = true;
    }
    return Result;
}

// NOTE(zoubir): Second Wind keeping Target alive through a blow that
// would kill it, once a round: 1 health left and a moment's shield. True
// when it did (DealDamage)
internal bool32
SecondWindSaves(app_state *AppState, world_entity *Target)
{
    player_slot *Slot = TalentSlotOf(AppState, Target);
    bool32 Result = false;
    if (Slot && !Slot->Predicted && !Slot->SecondWindSpent && !AppState->Dungeon &&
        DuelPerkRank(Slot, DuelPerk_SecondWind))
    {
        Slot->SecondWindSpent = true;
        Target->Hp = 1.f;
        Target->SpawnShield = Maximum(Target->SpawnShield, DUEL_SECOND_WIND_SHIELD);
        v3 Chest = Target->Position;
        Chest.Z += 16.f;
        EmitBurst(&AppState->Events, SimBurst_WardBreak, (u8)Target->PlayerIndex, Chest);
        EmitSound(&AppState->Events, AssetType_SfxShield, Target->Position);
        Result = true;
    }
    return Result;
}

// NOTE(zoubir): Bastion: the shield going up heals; only the simulation
// that owns the world, as health comes to a client in its snapshots
internal void
BastionHeals(app_state *AppState, world_entity *Player)
{
    player_slot *Slot = TalentSlotOf(AppState, Player);
    if (Slot && !Slot->Predicted && Player->Hp > 0.f && DuelPerkRank(Slot, DuelPerk_Bastion))
    {
        Player->Hp = Minimum(Player->MaxHp, Player->Hp + DUEL_BASTION_HEAL);
    }
}

// NOTE(zoubir): Phantom: a blink leaves the player untouchable a moment
inline void
PhantomAfterBlink(app_state *AppState, world_entity *Player)
{
    if (PlayerPerkRank(AppState, Player, DuelPerk_Phantom))
    {
        Player->SpawnShield = Maximum(Player->SpawnShield, DUEL_PHANTOM_SECONDS);
    }
}

// NOTE(zoubir): Mending: health back each second, by rank (UpdateProgression)
inline void
MendingHeals(player_slot *Slot, float DeltaTime)
{
    world_entity *Player = Slot->Entity;
    u32 Rank = DuelPerkRank(Slot, DuelPerk_Mending);
    if (Rank && Player && Player->IsPresent && Player->Hp > 0.f)
    {
        Player->Hp = Minimum(Player->MaxHp, Player->Hp + DUEL_MENDING_PER_SECOND * (float)Rank * DeltaTime);
    }
}

// NOTE(zoubir): forward, player_abilities/movement_abilities.cpp: Seconds
// off the blink's and the slam's cooldowns
internal void RefundMovement(world_entity *Player, float Seconds);

// NOTE(zoubir): forward, entity.cpp
internal bool32 DamageEntity(app_state *AppState, world *World, world_entity *Target,
                             float Damage, world_entity *Source);

// NOTE(zoubir): after a hit took Lost health off Target: Momentum on the
// player who landed it, and Thorns back at that player. Thorns never
// answers Thorns, or two players with it would trade forever
internal void
DuelAfterHit(app_state *AppState, world *World, world_entity *Target,
             world_entity *Source, float Lost)
{
    local_persist bool32 InThorns;
    player_slot *Attacker = HitOwnerSlot(AppState, Source);
    player_slot *Victim = TalentSlotOf(AppState, Target);
    if (!Attacker || !Victim || Attacker == Victim || Lost <= 0.f || AppState->Dungeon ||
        Attacker->Predicted || Victim->Predicted)
    {
        return;
    }
    world_entity *Hitter = Attacker->Entity;
    if (!Hitter || !Hitter->IsPresent)
    {
        return;
    }
    if (DuelPerkRank(Attacker, DuelPerk_Momentum))
    {
        RefundMovement(Hitter, DUEL_MOMENTUM_REFUND);
        Hitter->ActionCooldowns[PlayerAction_Kunai] =
            Maximum(0.f, Hitter->ActionCooldowns[PlayerAction_Kunai] - DUEL_MOMENTUM_REFUND);
    }
    if (!InThorns && DuelPerkRank(Victim, DuelPerk_Thorns) && !IsDeadPlayer(Hitter))
    {
        InThorns = true;
        DamageEntity(AppState, World, Hitter, DUEL_THORNS_SHARE * Lost, Target);
        InThorns = false;
    }
}
