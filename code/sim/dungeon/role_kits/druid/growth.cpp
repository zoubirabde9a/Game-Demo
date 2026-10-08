/* Druid growth (role_kits/druid.cpp): the half of the kit that heals,
   and the roots. Every heal goes through HealPlayer, so the meter and the
   monsters' threat count it as they count the Mender's.

   Rejuvenation heals an ally over time and at once for each Bloom it
   spends; one Druid keeps one on each ally, a new cast taking the old
   one's place. With Wild Growth it also lands on the most hurt allies
   near its target. Each is sent to clients again every
   DRUID_KEEP_SECONDS, so its leaves follow the ally everywhere.
   Regrowth heals at once, more for the Bloom it spends. Tranquility heals
   every ally near the Druid each TRANQUILITY_TICK of its channel.
   Entangling Roots holds a circle of ground: every foe in it when it
   grows is rooted, it bites each second, and a foe walking in later is
   caught too unless it has just broken free (Rooted's immunity,
   sim/status_effects.cpp). */

// NOTE(zoubir): a Rejuvenation of the Druid in slot By on Ally, fresh
internal void
PlantRejuvenation(app_state *AppState, player_slot *Slot, world_entity *Ally)
{
    druid_run *Run = &AppState->Dungeon->Druid;
    u8 By = (u8)(Slot - AppState->Players);
    u8 AllyIndex = (u8)Ally->PlayerIndex;
    druid_rejuvenation *Free = 0;
    for(u32 Index = 0; Index < DRUID_MAX_REJUVENATIONS; Index++)
    {
        druid_rejuvenation *Rejuv = &Run->Rejuvenations[Index];
        if (Rejuv->Seconds > 0.f && Rejuv->By == By && Rejuv->Ally == AllyIndex)
        {
            Free = Rejuv;
            break;
        }
        if (!Free && Rejuv->Seconds <= 0.f)
        {
            Free = Rejuv;
        }
    }
    if (!Free)
    {
        return;
    }
    bool32 Verdancy = RoleRank(Slot, PlayerRole_Druid, DruidTalent_Verdancy) > 0;
    Free->Seconds = REJUVENATION_SECONDS + (Verdancy ? VERDANCY_SECONDS : 0.f);
    Free->PerSecond = REJUVENATION_PER_SECOND * (Verdancy ? 1.f + VERDANCY_SHARE : 1.f);
    Free->Keep = DRUID_KEEP_SECONDS;
    Free->Ally = AllyIndex;
    Free->By = By;
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Rejuvenation), By,
              ChestOf(Ally));
}

// NOTE(zoubir): the living player within Range of From missing the most
// health, none of the Skip ones, or 0 when nobody is hurt
internal world_entity *
MostHurtAllyBut(app_state *AppState, v2 From, float Range, world_entity **Skip, u32 SkipCount)
{
    world_entity *Result = 0;
    float Worst = 0.f;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        bool32 Skipped = false;
        for(u32 Index = 0; Index < SkipCount; Index++)
        {
            Skipped |= Skip[Index] == Player;
        }
        if (Player && !Skipped && Length(Player->Position.XY - From) <= Range &&
            Player->MaxHp - Player->Hp > Worst)
        {
            Worst = Player->MaxHp - Player->Hp;
            Result = Player;
        }
    }
    return Result;
}

// NOTE(zoubir): Rejuvenation on the ally PickAllyFor gives: the Bloom
// spent heals at once, the rest over time; Wild Growth spreads it
internal void
CastRejuvenation(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    u8 By = (u8)Player->PlayerIndex;
    world_entity *Ally = PickAllyFor(AppState, Slot, Player, REJUVENATION_RANGE);
    u32 Bloom = SpendDruidBloom(Slot);
    if (Bloom)
    {
        HealPlayer(AppState, By, Ally, REJUVENATION_PER_BLOOM * (float)Bloom);
    }
    PlantRejuvenation(AppState, Slot, Ally);
    if (RoleRank(Slot, PlayerRole_Druid, DruidTalent_WildGrowth))
    {
        world_entity *Planted[WILD_GROWTH_ALLIES + 1] = {Ally};
        for(u32 Spread = 0; Spread < WILD_GROWTH_ALLIES; Spread++)
        {
            world_entity *Next = MostHurtAllyBut(AppState, Ally->Position.XY, WILD_GROWTH_REACH,
                                                 Planted, Spread + 1);
            if (!Next)
            {
                break;
            }
            PlantRejuvenation(AppState, Slot, Next);
            Planted[Spread + 1] = Next;
        }
    }
    EmitSound(&AppState->Events, AssetType_SfxHeal, Ally->Position);
}

// NOTE(zoubir): Regrowth: the ally heals at once, more for the Bloom spent
// and with Overgrowth's ranks
internal void
CastRegrowth(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    u8 By = (u8)Player->PlayerIndex;
    world_entity *Ally = PickAllyFor(AppState, Slot, Player, REGROWTH_RANGE);
    u32 Bloom = SpendDruidBloom(Slot);
    float Overgrowth = OVERGROWTH_SHARE * (float)RoleRank(Slot, PlayerRole_Druid, DruidTalent_Overgrowth);
    float Heal = (REGROWTH_HEAL + REGROWTH_PER_BLOOM * (float)Bloom) * (1.f + Overgrowth);
    HealPlayer(AppState, By, Ally, Heal);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Regrowth), By,
              DruidBurstSpot(ChestOf(Ally), Bloom));
    EmitSound(&AppState->Events, AssetType_SfxHeal, Ally->Position);
}

// NOTE(zoubir): from OnDruidHit: Symbiosis heals the most hurt ally near
// the Druid for a share of what Wrath or Starfire dealt
internal void
DruidSymbiosis(app_state *AppState, player_slot *Slot, float Damage)
{
    world_entity *Player = Slot->Entity;
    world_entity *Ally = Player ? MostHurtAlly(AppState, Player->Position.XY, SYMBIOSIS_REACH) : 0;
    if (Ally && Damage > 0.f)
    {
        u8 By = (u8)Player->PlayerIndex;
        HealPlayer(AppState, By, Ally, SYMBIOSIS_SHARE * Damage);
        EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Regrowth), By,
                  DruidBurstSpot(ChestOf(Ally), DRUID_REGROWTH_SYMBIOSIS));
    }
}

// NOTE(zoubir): one of Tranquility's heals: every ally near the Druid
internal void
PulseTranquility(app_state *AppState, world_entity *Player)
{
    u8 By = (u8)Player->PlayerIndex;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Ally = LivingPlayerInSlot(AppState, SlotIndex);
        if (Ally && Length(Ally->Position.XY - Player->Position.XY) <= TRANQUILITY_RADIUS)
        {
            HealPlayer(AppState, By, Ally, TRANQUILITY_HEAL);
        }
    }
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Tranquility), By,
              V3(Player->Position.X, Player->Position.Y, Player->GroundZ));
}

// NOTE(zoubir): whether Monster is a live foe inside Roots
inline bool32
DruidRootsReach(druid_roots *Roots, world_entity *Monster)
{
    v2 Offset = Monster->Position.XY - Roots->Position.XY;
    bool32 Result = Monster->IsPresent && Monster->Type == EntityType_Monster && Monster->Hp > 0.f &&
        Length(Offset) <= Roots->Radius + 0.3f * Monster->Dimensions.X;
    return Result;
}

// NOTE(zoubir): the roots bite every foe inside, holding each for the
// rest of their time; Damage 0 only holds
internal void
GripDruidRoots(app_state *AppState, druid_roots *Roots, float Damage)
{
    world *World = &AppState->World;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (!DruidRootsReach(Roots, Monster))
        {
            continue;
        }
        v2 Away = NormalizeOr(Monster->Position.XY - Roots->Position.XY, V2(1.f, 0.f));
        if (!HasStatus(Monster, StatusEffect_Rooted))
        {
            Monster->Velocity.XY = V2(0.f, 0.f);
        }
        DruidHit(AppState, Roots->By, Monster, DruidShot_Roots, Damage, 0.f, Away,
                 StatusEffect_Rooted, Maximum(0.5f, Roots->Hold));
    }
}

// NOTE(zoubir): Entangling Roots at the cursor; false when every patch is
// in use
internal bool32
CastEntanglingRoots(app_state *AppState, player_slot *Slot, world_entity *Player, u32 Key)
{
    druid_run *Run = &AppState->Dungeon->Druid;
    for(u32 Index = 0; Index < DRUID_MAX_ROOTS; Index++)
    {
        druid_roots *Roots = &Run->Roots[Index];
        if (Roots->Seconds > 0.f)
        {
            continue;
        }
        bool32 Long = RoleRank(Slot, PlayerRole_Druid, DruidTalent_EntanglingRoots) >= 2;
        v2 Point = AimPoint(Player);
        Roots->Position = V3(Point.X, Point.Y, Player->GroundZ);
        Roots->Radius = RoleSpellRadius(Slot, Key);
        Roots->Seconds = Long ? ROOTS_SECONDS_2 : ROOTS_SECONDS;
        Roots->Hold = Roots->Seconds;
        Roots->TickTimer = 1.f;
        Roots->By = (u8)Player->PlayerIndex;
        GripDruidRoots(AppState, Roots, ROOTS_TICK_DAMAGE);
        // NOTE(zoubir): the second rank rides along, so clients draw the
        // vines as long as they hold
        EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Roots), Roots->By,
                  DruidBurstSpot(Roots->Position, Long ? 1 : 0));
        EmitSound(&AppState->Events, AssetType_SfxAreaCast, Player->Position);
        return true;
    }
    return false;
}

// NOTE(zoubir): once a tick: Rejuvenations heal, each sent to clients
// again where its ally is now, and end with their ally
internal void
UpdateDruidRejuvenations(app_state *AppState, druid_run *Run, float DeltaTime)
{
    for(u32 Index = 0; Index < DRUID_MAX_REJUVENATIONS; Index++)
    {
        druid_rejuvenation *Rejuv = &Run->Rejuvenations[Index];
        if (Rejuv->Seconds <= 0.f)
        {
            continue;
        }
        world_entity *Ally = LivingPlayerInSlot(AppState, Rejuv->Ally);
        if (!Ally)
        {
            Rejuv->Seconds = 0.f;
            continue;
        }
        float Step = Minimum(DeltaTime, Rejuv->Seconds);
        Rejuv->Seconds -= Step;
        HealPlayer(AppState, Rejuv->By, Ally, Rejuv->PerSecond * Step);
        Rejuv->Keep -= DeltaTime;
        if (Rejuv->Keep <= 0.f && Rejuv->Seconds > 0.f)
        {
            Rejuv->Keep += DRUID_KEEP_SECONDS;
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Rejuvenation),
                      Rejuv->By, DruidBurstSpot(ChestOf(Ally), 1));
        }
    }
}

// NOTE(zoubir): once a tick: roots bite each second and wither with time
internal void
UpdateDruidRoots(app_state *AppState, druid_run *Run, float DeltaTime)
{
    for(u32 Index = 0; Index < DRUID_MAX_ROOTS; Index++)
    {
        druid_roots *Roots = &Run->Roots[Index];
        if (Roots->Seconds <= 0.f)
        {
            continue;
        }
        Roots->Seconds = Maximum(0.f, Roots->Seconds - DeltaTime);
        Roots->Hold = Roots->Seconds;
        Roots->TickTimer -= DeltaTime;
        if (Roots->TickTimer <= 0.f && Roots->Seconds > 0.f)
        {
            Roots->TickTimer += 1.f;
            GripDruidRoots(AppState, Roots, ROOTS_TICK_DAMAGE);
        }
    }
}
