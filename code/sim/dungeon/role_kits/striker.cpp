/* The damage role's kit (role_abilities.cpp): Inferno on A; E and V stay
   the game's Shield and kunai.

   Inferno calls a meteor down at the cursor. It lands INFERNO_DELAY
   later, so a monster can be seen to walk out of the marked circle, and
   strikes everything inside for INFERNO_DAMAGE; the ground there then
   burns for INFERNO_BURN_SECONDS, hurting any monster standing in it each
   INFERNO_BURN_TICK. The damage counts as the caster's, so their role and talents
   scale it and it makes threat for them. */

// NOTE(zoubir): returns whether the key cast (with every inferno in use
// it does not)
internal bool32
CastStrikerKey(app_state *AppState, player_slot *Slot, world_entity *Player, u32 Key)
{
    if (Key != 0)
    {
        return false;
    }
    dungeon_run *Run = AppState->Dungeon;
    inferno *Free = 0;
    for(u32 Index = 0; Index < MAX_INFERNOS; Index++)
    {
        inferno *Zone = &Run->Infernos[Index];
        if (Zone->Delay <= 0.f && Zone->Seconds <= 0.f)
        {
            Free = Zone;
            break;
        }
    }
    if (!Free)
    {
        return false;
    }
    u8 SlotIndex = (u8)Player->PlayerIndex;
    v2 Point = AimPoint(Player);
    Free->Position = V3(Point.X, Point.Y, Player->GroundZ);
    Free->Delay = INFERNO_DELAY;
    Free->Seconds = INFERNO_BURN_SECONDS + WILDFIRE_SECONDS *
        (float)RoleRank(Slot, PlayerRole_Damage, StrikerTalent_Wildfire);
    Free->Radius = RoleSpellRadius(Slot, Key);
    Free->By = SlotIndex;
    EmitBurst(&AppState->Events, SimBurst_InfernoCast, SlotIndex, ChestOf(Player),
              ATan2(Point.Y - Player->Position.Y, Point.X - Player->Position.X));
    return true;
}

// NOTE(zoubir): Damage to every living monster within Radius of Centre,
// by the player in slot By; with Blast, a full hit with its shove
internal void
BurnAround(app_state *AppState, world *World, v3 Centre, float Radius, u32 By,
           float Damage, bool32 Blast)
{
    world_entity *Caster = AppState->Players[By].Entity;
    hit Hit = {Damage, 160.f, 140.f, 140.f, 0.f, SimBurst_Count, StatusEffect_Burning, 1.f};
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        v2 Offset = Monster->Position.XY - Centre.XY;
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster ||
            Monster->Hp <= 0.f || Length(Offset) > Radius)
        {
            continue;
        }
        if (Blast)
        {
            ApplyHit(AppState, World, Monster, &Hit, DirectionTo(Offset), Caster, By);
        }
        else
        {
            DamageEntity(AppState, World, Monster, Damage, Caster);
        }
    }
}

// NOTE(zoubir): once a tick: meteors falling, then the ground burning
internal void
UpdateInfernos(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    world *World = &AppState->World;
    for(u32 Index = 0; Index < MAX_INFERNOS; Index++)
    {
        inferno *Zone = &Run->Infernos[Index];
        if (Zone->Delay > 0.f)
        {
            Zone->Delay -= DeltaTime;
            if (Zone->Delay <= 0.f)
            {
                Zone->Delay = 0.f;
                Zone->TickTimer = 0.f;
                BurnAround(AppState, World, Zone->Position, Zone->Radius, Zone->By,
                           INFERNO_DAMAGE, true);
                EmitBurst(&AppState->Events, SimBurst_InfernoBlast, (u8)Zone->By, Zone->Position);
            }
        }
        else if (Zone->Seconds > 0.f)
        {
            Zone->Seconds = Maximum(0.f, Zone->Seconds - DeltaTime);
            Zone->TickTimer += DeltaTime;
            if (Zone->TickTimer >= INFERNO_BURN_TICK)
            {
                Zone->TickTimer -= INFERNO_BURN_TICK;
                BurnAround(AppState, World, Zone->Position, Zone->Radius, Zone->By,
                           INFERNO_BURN_PER_SECOND * INFERNO_BURN_TICK, false);
            }
        }
    }
}

// NOTE(zoubir): from DungeonScaleDamage: a hit by Attacker that is about
// to kill Target; Bloodlust takes seconds off Inferno
internal void
OnRoleKill(player_slot *Attacker)
{
    if (RoleRank(Attacker, PlayerRole_Damage, StrikerTalent_Bloodlust))
    {
        Attacker->RoleCooldowns[0] = Maximum(0.f, Attacker->RoleCooldowns[0] - BLOODLUST_SECONDS);
    }
}
