/* The Mender's spells on X, G and T (role_kits/healer.cpp), the second
   spell of the Sanctum pair and both of the Dawn pair. Their numbers are
   role_numbers.h.

   Prayer of Healing (G, Sanctum, against Sanctuary): the
   PRAYER_OF_HEALING_ALLIES most hurt allies within PRAYER_OF_HEALING_REACH
   of the Mender heal PRAYER_OF_HEALING_HEAL at once. Sanctuary heals who
   stands in a spot over time; the prayer reaches the hurt wherever they
   are, now.

   Dawnbreak (X, Dawn, against Purify): a beam of light DAWNBREAK_LENGTH
   along the aim; every foe within DAWNBREAK_WIDTH of it takes
   DAWNBREAK_DAMAGE, and each hit heals the most hurt ally through Smite
   (OnHealerShot), so Atonement makes it the branch's big heal.

   Purify (T, Dawn, against Dawnbreak): the ally picked is rid of burning,
   poison, bleeding, slows, roots and stuns, and holds a ward of
   PURIFY_WARD. */

internal void
CastPrayerOfHealing(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    world_entity *Healed[PRAYER_OF_HEALING_ALLIES] = {};
    for(u32 Count = 0; Count < PRAYER_OF_HEALING_ALLIES; Count++)
    {
        // NOTE(zoubir): the living ally in reach missing the most health,
        // none healed already
        world_entity *Ally = 0;
        float Worst = 0.f;
        for(u32 Index = 0; Index < MAX_PLAYERS; Index++)
        {
            world_entity *Candidate = LivingPlayerInSlot(AppState, Index);
            float Missing = Candidate ? Candidate->MaxHp - Candidate->Hp : 0.f;
            bool32 Done = false;
            for(u32 Prior = 0; Prior < Count; Prior++)
            {
                Done = Done || Healed[Prior] == Candidate;
            }
            if (Candidate && !Done && Missing > Worst &&
                Length(Candidate->Position.XY - Player->Position.XY) <= PRAYER_OF_HEALING_REACH)
            {
                Worst = Missing;
                Ally = Candidate;
            }
        }
        if (!Ally)
        {
            break;
        }
        Healed[Count] = Ally;
        HealPlayer(AppState, SlotIndex, Ally, PRAYER_OF_HEALING_HEAL);
        EmitBurst(&AppState->Events, SimBurst_MendingBolt, SlotIndex, ChestOf(Ally));
    }
    EmitSound(&AppState->Events, AssetType_SfxHeal, Player->Position);
}

internal void
CastDawnbreak(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    u8 SlotIndex = (u8)Player->PlayerIndex;
    v2 Dir = NormalizeOr(GetPlayerAim(Player), V2(1.f, 0.f));
    v2 Normal = V2(-Dir.Y, Dir.X);
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            RoomAtPosition(World, Monster->Position.XY) != Room)
        {
            continue;
        }
        v2 Offset = Monster->Position.XY - Player->Position.XY;
        float Along = DotProduct(Offset, Dir);
        if (Along < 0.f || Along > DAWNBREAK_LENGTH ||
            Absolute(DotProduct(Offset, Normal)) > DAWNBREAK_WIDTH + 0.5f * Monster->Dimensions.X)
        {
            continue;
        }
        float Before = Monster->Hp;
        hit Hit = {DAWNBREAK_DAMAGE, 40.f, 0.f, 0.f, 0.f, SimBurst_Impact};
        ApplyHit(AppState, World, Monster, &Hit, Dir, Player, SlotIndex);
        EmitBurst(&AppState->Events, SimBurst_MendingBolt, SlotIndex, ChestOf(Monster));
        OnHealerShot(AppState, Player, Before - Maximum(0.f, Monster->Hp));
    }
    EmitSound(&AppState->Events, AssetType_SfxFireCast, Player->Position);
}

internal void
CastPurify(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    world_entity *Ally = PickAllyFor(AppState, Slot, Player, MENDING_BOLT_RANGE);
    status_effect Harms[] = {StatusEffect_Burning, StatusEffect_Poisoned, StatusEffect_Bleeding,
                             StatusEffect_Slowed, StatusEffect_Rooted, StatusEffect_Stunned};
    for(u32 Index = 0; Index < ArrayCount(Harms); Index++)
    {
        Ally->StatusTimers[Harms[Index]] = 0.f;
    }
    player_slot *AllySlot = &AppState->Players[Ally->PlayerIndex];
    AllySlot->WardAbsorb = Maximum(AllySlot->WardAbsorb, PURIFY_WARD);
    AllySlot->WardFull = Maximum(AllySlot->WardAbsorb, AllySlot->WardFull);
    EmitBurst(&AppState->Events, SimBurst_WardCast, SlotIndex, ChestOf(Ally));
    EmitSound(&AppState->Events, AssetType_SfxHeal, Ally->Position);
}
