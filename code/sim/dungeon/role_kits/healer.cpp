/* The healer's kit (role_abilities.cpp): Sanctuary on A, Ward on E,
   Mending Bolt on V. Every one of them is for the party: Ward and Mending
   Bolt land on an ally (PickAllyFor: the one under the cursor or picked
   on the party frames, else the most hurt in reach, else the nearest
   ally, the healer only when alone), Ward also shields the allies round
   that one, and Sanctuary heals everyone standing where the cursor is.
   Renewal (role_talents.cpp) leaves healing over time on whoever a bolt
   lands on, run by UpdateRenewals.

   A warded ally also deals WARD_EMPOWER_SHARE more while the ward holds
   (DungeonScaleDamage), so the ward is the healer's call in a damage
   race: on the tank before a big hit, on the striker the rest of the
   time to beat the boss's clock. */

internal void
CastMendingBolt(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    world_entity *Ally = PickAllyFor(AppState, Slot, Player, MENDING_BOLT_RANGE);
    float Heal = MENDING_BOLT_HEAL * (1.f + SWIFT_MENDING_SHARE *
        (float)RoleRank(Slot, PlayerRole_Healer, HealerTalent_SwiftMending));
    HealPlayer(AppState, SlotIndex, Ally, Heal);
    EmitBurst(&AppState->Events, SimBurst_MendingBolt, SlotIndex, ChestOf(Ally));
    if (RoleRank(Slot, PlayerRole_Healer, HealerTalent_Renewal))
    {
        player_slot *AllySlot = &AppState->Players[Ally->PlayerIndex];
        AllySlot->RenewSeconds = RENEWAL_SECONDS;
        AllySlot->RenewPerSecond = RENEWAL_PER_SECOND;
        AllySlot->RenewBy = SlotIndex;
    }
    if (RoleRank(Slot, PlayerRole_Healer, HealerTalent_Beacon))
    {
        world_entity *Next = MostHurtAlly(AppState, Ally->Position.XY, BEACON_RANGE, Ally);
        if (Next)
        {
            HealPlayer(AppState, SlotIndex, Next, BEACON_SHARE * Heal);
            EmitBurst(&AppState->Events, SimBurst_MendingBolt, SlotIndex, ChestOf(Next));
        }
    }
}

// NOTE(zoubir): returns whether the key cast (a sanctuary with every
// circle in use does not)
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
            bool32 Hallowed = RoleRank(Slot, PlayerRole_Healer, HealerTalent_HallowedGround) > 0;
            v2 Point = AimPoint(Player);
            Free->Position = V3(Point.X, Point.Y, 0.f);
            Free->Seconds = SANCTUARY_SECONDS;
            Free->By = SlotIndex;
            Free->Radius = RoleSpellRadius(Slot, Key);
            Free->HealPerSecond = SANCTUARY_HEAL_PER_SECOND * (Hallowed ? HALLOWED_HEAL : 1.f);
            EmitBurst(&AppState->Events, SimBurst_SanctuaryCast, SlotIndex, Free->Position);
        } break;

        case 1:
        {
            world_entity *Ally = PickAllyFor(AppState, Slot, Player, MENDING_BOLT_RANGE);
            float Absorb = (WARD_ABSORB + DEEP_WARD_ABSORB *
                            (float)RoleRank(Slot, PlayerRole_Healer, HealerTalent_DeepWard)) *
                PartySustainScale(AppState->Dungeon);
            player_slot *AllySlot = &AppState->Players[Ally->PlayerIndex];
            AllySlot->WardAbsorb = Absorb;
            AllySlot->WardFull = Absorb;
            EmitBurst(&AppState->Events, SimBurst_WardCast, SlotIndex, ChestOf(Ally));
            // NOTE(zoubir): the allies round the warded one get part of
            // it, never less than the ward they already hold
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
                    EmitBurst(&AppState->Events, SimBurst_WardCast, SlotIndex, ChestOf(Near));
                }
            }
        } break;

        case 2:
        {
            CastMendingBolt(AppState, Slot, Player);
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
