/* The healer's kit (role_abilities.cpp): Mending Bolt on A, Ward on R,
   Holy Fire on W, and from its tree Sanctuary on C and Radiance on V.
   All but Holy Fire are for the party: Ward and Mending
   Bolt land on an ally (PickAllyFor: the one under the cursor or picked
   on the party frames, else the most hurt in reach, else the nearest
   ally, the healer only when alone), Ward also shields the allies round
   that one, Sanctuary heals everyone standing where the cursor is, and
   Radiance heals and wards every living player within RADIANCE_RADIUS
   of the healer.
   Renewal (role_talents.cpp) leaves healing over time on whoever a bolt
   lands on, run by UpdateRenewals. Steadfast Ward makes Ward absorb more
   and come back sooner (RoleSpellCooldown) per rank. Guardian Angel, the
   capstone, catches each ally once a fight (GuardianAngelSave): the blow
   that would take them under GUARDIAN_ANGEL_HP_SHARE of their health
   cannot kill them, and they heal and are warded at once.

   A warded ally also deals WARD_EMPOWER_SHARE more while the ward holds
   (DungeonScaleDamage), so the ward is the healer's call in a damage
   race: on the tank before a big hit, on the striker the rest of the
   time to beat the boss's clock.

   Smite: with nothing to heal the healer still has a use for its
   fireball, as each one that lands heals the most hurt ally in reach for
   SMITE_SHARE of the damage it dealt. Holy Fire is the healer's attack:
   HOLY_FIRE_DAMAGE on the foe it aims at, healing through Smite like a
   fireball, so a healer with little to heal still adds to the race. */

// NOTE(zoubir): from OnRoleHit: the healer's fireball dealt
// Damage; Smite heals the most hurt ally in reach SMITE_SHARE of it
internal void
OnHealerShot(app_state *AppState, world_entity *Healer, float Damage)
{
    world_entity *Ally = MostHurtAlly(AppState, Healer->Position.XY, MENDING_BOLT_RANGE);
    if (Ally)
    {
        u8 SlotIndex = (u8)Healer->PlayerIndex;
        HealPlayer(AppState, SlotIndex, Ally, SMITE_SHARE * Damage);
        EmitBurst(&AppState->Events, SimBurst_MendingBolt, SlotIndex, ChestOf(Ally));
    }
}

internal void
CastMendingBolt(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    world_entity *Ally = PickAllyFor(AppState, Slot, Player, MENDING_BOLT_RANGE);
    float Heal = MENDING_BOLT_HEAL * (1.f + SWIFT_MENDING_SHARE *
        (float)RoleRank(Slot, PlayerRole_Healer, HealerTalent_SwiftMending));
    HealPlayer(AppState, SlotIndex, Ally, Heal);
    EmitBurst(&AppState->Events, SimBurst_MendingBolt, SlotIndex, ChestOf(Ally));
    EmitSound(&AppState->Events, AssetType_SfxHeal, Ally->Position);
    if (RoleRank(Slot, PlayerRole_Healer, HealerTalent_Renewal))
    {
        player_slot *AllySlot = &AppState->Players[Ally->PlayerIndex];
        AllySlot->RenewSeconds = RENEWAL_SECONDS;
        AllySlot->RenewPerSecond = RENEWAL_PER_SECOND;
        AllySlot->RenewBy = SlotIndex;
    }
}

// NOTE(zoubir): Ward on the ally PickAllyFor gives, and half of it on the
// allies round them, never less than the ward each already holds
internal void
CastWard(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    world_entity *Ally = PickAllyFor(AppState, Slot, Player, MENDING_BOLT_RANGE);
    float Absorb = (WARD_ABSORB + DEEP_WARD_ABSORB *
                    (float)RoleRank(Slot, PlayerRole_Healer, HealerTalent_DeepWard) +
                    STEADFAST_WARD_ABSORB *
                    (float)RoleRank(Slot, PlayerRole_Healer, HealerTalent_SteadfastWard)) *
        PartySustainScale(AppState->Dungeon);
    player_slot *AllySlot = &AppState->Players[Ally->PlayerIndex];
    AllySlot->WardAbsorb = Absorb;
    AllySlot->WardFull = Absorb;
    AllySlot->WardEmpower = WARD_EMPOWER_SHARE;
    EmitBurst(&AppState->Events, SimBurst_WardCast, SlotIndex, ChestOf(Ally));
    EmitSound(&AppState->Events, AssetType_SfxWard, Ally->Position);
    float Splash = WARD_SPLASH_SHARE * Absorb;
    for(u32 Other = 0; Other < MAX_PLAYERS; Other++)
    {
        world_entity *Near = LivingPlayerInSlot(AppState, Other);
        player_slot *NearSlot = &AppState->Players[Other];
        if (Near && Near != Ally && NearSlot->WardAbsorb < Splash &&
            Length(Near->Position.XY - Ally->Position.XY) <= WARD_SPLASH_RADIUS)
        {
            NearSlot->WardAbsorb = Splash;
            NearSlot->WardFull = Splash;
            NearSlot->WardEmpower = 0.f;
            EmitBurst(&AppState->Events, SimBurst_WardCast, SlotIndex, ChestOf(Near));
        }
    }
}

// NOTE(zoubir): Radiance: every living player round the healer, the
// healer too, heals and holds at least a small ward
internal void
CastRadiance(app_state *AppState, world_entity *Player)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    float Ward = RADIANCE_WARD * PartySustainScale(AppState->Dungeon);
    for(u32 Other = 0; Other < MAX_PLAYERS; Other++)
    {
        world_entity *Near = LivingPlayerInSlot(AppState, Other);
        if (!Near || Length(Near->Position.XY - Player->Position.XY) > RADIANCE_RADIUS)
        {
            continue;
        }
        HealPlayer(AppState, SlotIndex, Near, RADIANCE_HEAL);
        player_slot *NearSlot = &AppState->Players[Other];
        if (NearSlot->WardAbsorb < Ward)
        {
            NearSlot->WardAbsorb = Ward;
            NearSlot->WardFull = Ward;
        }
        EmitBurst(&AppState->Events, SimBurst_MendingBolt, SlotIndex, ChestOf(Near));
    }
    EmitBurst(&AppState->Events, SimBurst_SanctuaryCast, SlotIndex, Player->Position);
    EmitSound(&AppState->Events, AssetType_SfxHeal, Player->Position);
}

// NOTE(zoubir): a bolt of light at a foe in Range: Damage, and the most
// hurt ally heals for what it dealt (Smite); Holy Fire and Smite Bolt.
// Returns whether it found a foe to strike
internal bool32
CastLightAtFoe(app_state *AppState, player_slot *Slot, world_entity *Player, float Range,
               float Damage, float Shove)
{
    world_entity *Foe = AttackTarget(AppState, Slot, Player, Range);
    if (!Foe)
    {
        return false;
    }
    u8 SlotIndex = (u8)Player->PlayerIndex;
    float Before = Foe->Hp;
    hit Hit = {Damage, Shove, 0.f, 0.f, 0.f, SimBurst_Impact};
    EmitBurst(&AppState->Events, SimBurst_MendingBolt, SlotIndex, ChestOf(Foe));
    ApplyHit(AppState, &AppState->World, Foe, &Hit,
             DirectionTo(Foe->Position.XY - Player->Position.XY), Player, SlotIndex);
    OnHealerShot(AppState, Player, Before - Maximum(0.f, Foe->Hp));
    EmitSound(&AppState->Events, AssetType_SfxFireCast, Player->Position);
    return true;
}

// NOTE(zoubir): returns whether the key cast (a sanctuary with every
// circle in use, or Holy Fire with no foe in reach, does not)
internal bool32
CastHealerKey(app_state *AppState, player_slot *Slot, world_entity *Player, u32 Key)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    dungeon_run *Run = AppState->Dungeon;
    switch(Key)
    {
        case 0:
        {
            CastMendingBolt(AppState, Slot, Player);
        } break;

        case 1:
        {
            CastWard(AppState, Slot, Player);
        } break;

        case 2:
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
            // NOTE(zoubir): Sanctuary's second rank, once Hallowed Ground
            bool32 Hallowed = RoleRank(Slot, PlayerRole_Healer, HealerTalent_Sanctuary) >= 2;
            v2 Point = AimPoint(Player);
            Free->Position = V3(Point.X, Point.Y, 0.f);
            Free->Seconds = SANCTUARY_SECONDS;
            Free->By = SlotIndex;
            Free->Radius = RoleSpellRadius(Slot, Key);
            Free->HealPerSecond = SANCTUARY_HEAL_PER_SECOND * (Hallowed ? HALLOWED_HEAL : 1.f);
            EmitBurst(&AppState->Events, SimBurst_SanctuaryCast, SlotIndex, Free->Position);
            EmitSound(&AppState->Events, AssetType_SfxSanctuary, Free->Position);
        } break;

        case 3:
        {
            CastRadiance(AppState, Player);
        } break;

        case 4:
        {
            return CastLightAtFoe(AppState, Slot, Player, HOLY_FIRE_RANGE, HOLY_FIRE_DAMAGE,
                                  HOLY_FIRE_SHOVE);
        } break;

        case 6:
        {
            return CastLightAtFoe(AppState, Slot, Player, SMITE_BOLT_RANGE, SMITE_BOLT_DAMAGE,
                                  SMITE_BOLT_SHOVE);
        } break;
    }
    return true;
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
            if (Player && Length(Player->Position.XY - Zone->Position.XY) <= Zone->Radius)
            {
                HealPlayer(AppState, Zone->By, Player, Zone->HealPerSecond * DeltaTime);
            }
        }
    }
}

// NOTE(zoubir): once a tick: Renewal's healing over time
internal void
UpdateRenewals(app_state *AppState, float DeltaTime)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->RenewSeconds <= 0.f)
        {
            continue;
        }
        Slot->RenewSeconds = Maximum(0.f, Slot->RenewSeconds - DeltaTime);
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (!Player)
        {
            Slot->RenewSeconds = 0.f;
            continue;
        }
        HealPlayer(AppState, Slot->RenewBy, Player, Slot->RenewPerSecond * DeltaTime);
    }
}

// NOTE(zoubir): from DungeonScaleDamage, after the ward took its share:
// Damage is about to land on Ally in a fight. With a living healer in the
// party holding Guardian Angel, the first blow each fight that would take
// Ally under GUARDIAN_ANGEL_HP_SHARE of their health cannot kill them;
// once it lands they heal and hold a fresh ward. Returns the damage left
internal float
GuardianAngelSave(app_state *AppState, world_entity *Ally, player_slot *AllySlot, float Damage)
{
    dungeon_run *Run = AppState->Dungeon;
    if (!Run || !Run->FightingRoom || AllySlot->AngelSpent || !(Damage > 0.f) ||
        IsDeadPlayer(Ally) || Ally->Hp - Damage >= GUARDIAN_ANGEL_HP_SHARE * Ally->MaxHp)
    {
        return Damage;
    }
    u32 By = MAX_PLAYERS;
    for(u32 Index = 0; Index < MAX_PLAYERS && By == MAX_PLAYERS; Index++)
    {
        if (LivingPlayerInSlot(AppState, Index) &&
            RoleRank(&AppState->Players[Index], PlayerRole_Healer, HealerTalent_GuardianAngel))
        {
            By = Index;
        }
    }
    if (By == MAX_PLAYERS)
    {
        return Damage;
    }
    AllySlot->AngelSpent = 1;
    float Result = Minimum(Damage, Maximum(0.f, Ally->Hp - 1.f));
    // NOTE(zoubir): the heal counts from the health left after the blow,
    // which the caller takes off once this returns
    Ally->Hp -= Result;
    HealPlayer(AppState, By, Ally, GUARDIAN_ANGEL_HEAL_SHARE * Ally->MaxHp);
    Ally->Hp += Result;
    float Ward = GUARDIAN_ANGEL_WARD * PartySustainScale(Run);
    if (AllySlot->WardAbsorb < Ward)
    {
        AllySlot->WardAbsorb = Ward;
        AllySlot->WardFull = Ward;
    }
    EmitBurst(&AppState->Events, SimBurst_SanctuaryCast, (u8)By, Ally->Position);
    EmitBurst(&AppState->Events, SimBurst_WardCast, (u8)By, ChestOf(Ally));
    EmitSound(&AppState->Events, AssetType_SfxHeal, Ally->Position);
    return Result;
}

// NOTE(zoubir): once a tick: between fights every ally may be caught by
// Guardian Angel again
internal void
UpdateGuardianAngels(app_state *AppState, dungeon_run *Run)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS && !Run->FightingRoom; SlotIndex++)
    {
        AppState->Players[SlotIndex].AngelSpent = 0;
    }
}
