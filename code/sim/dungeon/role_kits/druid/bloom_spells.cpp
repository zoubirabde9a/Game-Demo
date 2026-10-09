/* The Druid's spells on G and T (role_kits/druid.cpp), the second spell of
   each of its branches' pairs.

   Lifebloom (G, Grove, against Tranquility): on an ally it heals
   LIFEBLOOM_PER_SECOND a second for LIFEBLOOM_SECONDS, then blooms for
   LIFEBLOOM_BLOOM and LIFEBLOOM_PER_BLOOM for each Bloom spent at the
   cast. Tranquility heals everyone near for a moment; Lifebloom saves its
   big heal for one ally at a time chosen ahead.

   Starfall (T, Moon, against Entangling Roots): a star on every foe within
   STARFALL_REACH of the Druid, STARFALL_DAMAGE each, harder on a foe under
   the Druid's Moonfire with Eclipse, and a Bloom for each struck. Roots
   hold a pack; Starfall feeds the heals from it. */

// NOTE(zoubir): Lifebloom on the ally picked, taking the place of the
// Druid's last one on that ally
internal void
CastLifebloom(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    druid_run *Run = &AppState->Dungeon->Druid;
    u8 By = (u8)Player->PlayerIndex;
    world_entity *Ally = PickAllyFor(AppState, Slot, Player, LIFEBLOOM_RANGE);
    u8 AllyIndex = (u8)Ally->PlayerIndex;
    druid_lifebloom *Free = 0;
    for(u32 Index = 0; Index < DRUID_MAX_LIFEBLOOMS; Index++)
    {
        druid_lifebloom *Bloom = &Run->Lifeblooms[Index];
        if (Bloom->Seconds > 0.f && Bloom->By == By && Bloom->Ally == AllyIndex)
        {
            Free = Bloom;
            break;
        }
        if (!Free && Bloom->Seconds <= 0.f)
        {
            Free = Bloom;
        }
    }
    if (!Free)
    {
        return;
    }
    u32 Spent = SpendDruidBloom(Slot);
    Free->Seconds = LIFEBLOOM_SECONDS;
    Free->Burst = LIFEBLOOM_BLOOM + LIFEBLOOM_PER_BLOOM * (float)Spent;
    Free->Ally = AllyIndex;
    Free->By = By;
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Rejuvenation), By,
              ChestOf(Ally));
    EmitSound(&AppState->Events, AssetType_SfxHeal, Ally->Position);
}

// NOTE(zoubir): once a tick: each Lifebloom heals its ally a little, and
// blooms as it runs out; one whose ally is down is gone
internal void
UpdateDruidLifeblooms(app_state *AppState, druid_run *Run, float DeltaTime)
{
    for(u32 Index = 0; Index < DRUID_MAX_LIFEBLOOMS; Index++)
    {
        druid_lifebloom *Bloom = &Run->Lifeblooms[Index];
        if (Bloom->Seconds <= 0.f)
        {
            continue;
        }
        world_entity *Ally = LivingPlayerInSlot(AppState, Bloom->Ally);
        if (!Ally)
        {
            Bloom->Seconds = 0.f;
            continue;
        }
        HealPlayer(AppState, Bloom->By, Ally, LIFEBLOOM_PER_SECOND * DeltaTime);
        Bloom->Seconds -= DeltaTime;
        if (Bloom->Seconds <= 0.f)
        {
            Bloom->Seconds = 0.f;
            HealPlayer(AppState, Bloom->By, Ally, Bloom->Burst);
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Regrowth), Bloom->By,
                      DruidBurstSpot(ChestOf(Ally), 3));
        }
    }
}

// NOTE(zoubir): Starfall: a star on every foe near the Druid; false with
// none near, which costs nothing
internal bool32
CastStarfall(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    u32 By = Player->PlayerIndex;
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    bool32 Eclipse = RoleRank(Slot, PlayerRole_Druid, DruidTalent_Eclipse) > 0;
    bool32 Result = false;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        v2 Offset = Monster->Position.XY - Player->Position.XY;
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            RoomAtPosition(World, Monster->Position.XY) != Room || Length(Offset) > STARFALL_REACH)
        {
            continue;
        }
        float Damage = STARFALL_DAMAGE;
        if (Eclipse && IsDruidMoonfired(AppState, By, Monster))
        {
            Damage *= 1.f + ECLIPSE_SHARE;
        }
        DruidHit(AppState, By, Monster, DruidShot_Starfall, Damage, 60.f, NormalizeOr(Offset, V2(1.f, 0.f)));
        EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Starfire), (u8)By,
                  Monster->Position);
        Result = true;
    }
    if (Result)
    {
        EmitSound(&AppState->Events, AssetType_SfxGiantFireball, Player->Position);
    }
    return Result;
}
