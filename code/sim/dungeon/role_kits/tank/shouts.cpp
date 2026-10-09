/* Tank shouts (role_kits/tank.cpp): the Bulwark's spells on G and T, the
   second spell of each of its branches' pairs.

   Rallying Cry (G, Bastion, against Last Stand): the tank and every ally
   within RALLYING_CRY_REACH heal RALLYING_CRY_HEAL_SHARE of their health
   and hold a ward of RALLYING_CRY_WARD. Last Stand saves the tank; the
   cry carries the party through a hard stretch.

   Demoralizing Roar (T, Vanguard, against Intercept): every foe within
   DEMORALIZING_ROAR_REACH deals DEMORALIZING_ROAR_SHARE less for
   DEMORALIZING_ROAR_SECONDS (the weakness Disarm leaves,
   role_kits/foe_marks.cpp) and turns on the tank as a Taunt does.
   Intercept goes to an ally in trouble; the roar takes the bite out of
   the whole pack. Their numbers are role_numbers.h. */

internal void
CastRallyingCry(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    for(u32 Index = 0; Index < MAX_PLAYERS; Index++)
    {
        world_entity *Ally = LivingPlayerInSlot(AppState, Index);
        if (!Ally || Length(Ally->Position.XY - Player->Position.XY) > RALLYING_CRY_REACH)
        {
            continue;
        }
        player_slot *AllySlot = &AppState->Players[Index];
        HealPlayer(AppState, SlotIndex, Ally, RALLYING_CRY_HEAL_SHARE * Ally->MaxHp);
        AllySlot->WardAbsorb = Maximum(AllySlot->WardAbsorb, RALLYING_CRY_WARD);
        AllySlot->WardFull = Maximum(AllySlot->WardAbsorb, AllySlot->WardFull);
        EmitBurst(&AppState->Events, SimBurst_WardCast, SlotIndex, ChestOf(Ally));
    }
    EmitBurst(&AppState->Events, SimBurst_Taunt, SlotIndex, Player->Position);
    EmitSound(&AppState->Events, AssetType_SfxTaunt, Player->Position);
}

internal void
CastDemoralizingRoar(app_state *AppState, world *World, player_slot *Slot, world_entity *Player)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            RoomAtPosition(World, Monster->Position.XY) != Room ||
            Length(Monster->Position.XY - Player->Position.XY) > DEMORALIZING_ROAR_REACH)
        {
            continue;
        }
        WeakenFoe(AppState->Dungeon, World, Monster, DEMORALIZING_ROAR_SECONDS, DEMORALIZING_ROAR_SHARE);
    }
    TauntAround(AppState, &AppState->Dungeon->Threat, Player, DEMORALIZING_ROAR_REACH);
    EmitBurst(&AppState->Events, SimBurst_Taunt, SlotIndex, Player->Position);
    EmitSound(&AppState->Events, AssetType_SfxTaunt, Player->Position);
}
