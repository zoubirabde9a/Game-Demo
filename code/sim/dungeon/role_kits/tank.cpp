/* The tank's kit (role_abilities.cpp): Taunt on A, Shield Slam on E,
   Intercept on V.

   Shield Slam is the tank's answer to a pack: every monster within
   SHIELD_SLAM_RADIUS is struck, stunned and shoved back, and takes on
   SHIELD_SLAM_THREAT for the tank; the tank heals SHIELD_SLAM_HEAL_SHARE
   of its health for each one, stands behind Shield Wall
   (SHIELD_WALL_SCALE of the damage, dungeon.cpp) and every ally within
   RALLY_RADIUS takes RALLY_SHARE less for RALLY_SECONDS. A taunt raises
   Shield Wall for TAUNT_WALL_SECONDS too.

   The slam also sunders what it strikes (foe_marks.cpp): for
   SUNDER_SECONDS, as long as its cooldown, the monster takes SUNDER_SHARE
   more from the whole party. So the tank has a rotation of its own in a
   damage race: slam the boss on cooldown, taunt the adds, leap to an
   ally in trouble.

   What keeps a tank standing late in a big party: it takes only the
   square root of the party's extra damage (PartySustainScale,
   party_scaling.cpp), and the slam's heal grows with it like a healer's. */

// NOTE(zoubir): an ally Self can leap to: alive, within Range, and in the
// same room, so a leap never crosses a gate
inline bool32
CanInterceptTo(app_state *AppState, world_entity *Self, world_entity *Ally, float Range)
{
    world *World = &AppState->World;
    bool32 Result = Ally && Ally != Self && !IsDeadPlayer(Ally) &&
        Length(Ally->Position.XY - Self->Position.XY) <= Range &&
        RoomAtPosition(World, Ally->Position.XY) == RoomAtPosition(World, Self->Position.XY);
    return Result;
}

// NOTE(zoubir): the ally Self can intercept to nearest to Point, or 0
internal world_entity *
NearestAllyTo(app_state *AppState, world_entity *Self, v2 Point, float Range)
{
    world_entity *Result = 0;
    float Best = 0.f;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (Player && CanInterceptTo(AppState, Self, Player, Range))
        {
            float Distance = Length(Player->Position.XY - Point);
            if (!Result || Distance < Best)
            {
                Best = Distance;
                Result = Player;
            }
        }
    }
    return Result;
}

internal void
CastShieldSlam(app_state *AppState, world *World, player_slot *Slot, world_entity *Player)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    bool32 Bastion = RoleRank(Slot, PlayerRole_Tank, TankTalent_Bastion) > 0;
    bool32 Rally = RoleRank(Slot, PlayerRole_Tank, TankTalent_Rally) > 0;
    bool32 Shatter = RoleRank(Slot, PlayerRole_Tank, TankTalent_ShatterArmor) > 0;
    hit Hit = {SHIELD_SLAM_DAMAGE, SHIELD_SLAM_SHOVE, 80.f, 80.f,
               SHIELD_SLAM_STUN + (Bastion ? BASTION_STUN : 0.f), SimBurst_Impact};
    u32 Struck = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        v2 Offset = Monster->Position.XY - Player->Position.XY;
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster ||
            Monster->Hp <= 0.f || Length(Offset) > SHIELD_SLAM_RADIUS)
        {
            continue;
        }
        AddThreat(&AppState->Dungeon->Threat, World, Monster, SlotIndex, SHIELD_SLAM_THREAT);
        AddSunder(AppState->Dungeon, World, Monster, SUNDER_SECONDS + (Shatter ? SHATTER_SECONDS : 0.f),
                  SUNDER_SHARE + (Shatter ? SHATTER_SHARE : 0.f));
        ApplyHit(AppState, World, Monster, &Hit, DirectionTo(Offset), Player, SlotIndex);
        Struck++;
    }
    u32 Counted = Minimum(Struck, (u32)SHIELD_SLAM_HEAL_FOES);
    HealPlayer(AppState, SlotIndex, Player,
               SHIELD_SLAM_HEAL_SHARE * (float)Counted * Player->MaxHp);
    Slot->ShieldWallSeconds = SHIELD_WALL_SECONDS + (Bastion ? BASTION_WALL_SECONDS : 0.f);
    float Reach = Rally ? RALLY_TALENT_RADIUS : RALLY_RADIUS;
    for(u32 Other = 0; Other < MAX_PLAYERS; Other++)
    {
        world_entity *Ally = LivingPlayerInSlot(AppState, Other);
        if (Ally && Ally != Player && Length(Ally->Position.XY - Player->Position.XY) <= Reach)
        {
            player_slot *AllySlot = &AppState->Players[Other];
            AllySlot->RallySeconds = RALLY_SECONDS;
            AllySlot->RallyShare = Rally ? RALLY_TALENT_SHARE : RALLY_SHARE;
        }
    }
    EmitBurst(&AppState->Events, SimBurst_ShieldSlam, SlotIndex, Player->Position,
              ATan2(Player->Aim.Y, Player->Aim.X));
}

// NOTE(zoubir): returns whether the key cast (an intercept with nobody
// to leap to does not, and keeps its cooldown)
internal bool32
CastTankKey(app_state *AppState, world *World, memory_arena *Arena,
            player_slot *Slot, world_entity *Player, u32 Key)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    switch(Key)
    {
        case 0:
        {
            float Reach = TAUNT_RADIUS * (1.f + PROVOKE_REACH *
                (float)RoleRank(Slot, PlayerRole_Tank, TankTalent_Provoke));
            TauntAround(AppState, &AppState->Dungeon->Threat, Player, Reach);
            Slot->ShieldWallSeconds = Maximum(Slot->ShieldWallSeconds, TAUNT_WALL_SECONDS);
            EmitBurst(&AppState->Events, SimBurst_Taunt, SlotIndex, Player->Position);
        } break;

        case 1:
        {
            CastShieldSlam(AppState, World, Slot, Player);
        } break;

        case 2:
        {
            world_entity *Ally = CursorAlly(AppState, Slot, Player, false);
            if (!CanInterceptTo(AppState, Player, Ally, INTERCEPT_RANGE))
            {
                Ally = NearestAllyTo(AppState, Player, AimPoint(Player), INTERCEPT_RANGE);
            }
            if (!Ally)
            {
                return false;
            }
            float Angle = ATan2(Ally->Position.Y - Player->Position.Y,
                                Ally->Position.X - Player->Position.X);
            EmitBurst(&AppState->Events, SimBurst_Lunge, SlotIndex, ChestOf(Player), Angle);
            v2 Toward = DirectionTo(Ally->Position.XY - Player->Position.XY);
            v3 Landing = Ally->Position;
            Landing.XY -= INTERCEPT_LANDING_GAP * Toward;
            MovePlayerTo(AppState, World, Arena, Player, Landing);
            // NOTE(zoubir): what was after the ally comes for the tank
            TauntAround(AppState, &AppState->Dungeon->Threat, Player, TAUNT_RADIUS * 0.5f);
            if (RoleRank(Slot, PlayerRole_Tank, TankTalent_Guardian))
            {
                player_slot *AllySlot = &AppState->Players[Ally->PlayerIndex];
                AllySlot->WardAbsorb = Maximum(AllySlot->WardAbsorb, GUARDIAN_WARD);
                AllySlot->WardFull = Maximum(AllySlot->WardAbsorb, AllySlot->WardFull);
            }
            EmitBurst(&AppState->Events, SimBurst_InterceptLand, SlotIndex, Player->Position, Angle);
        } break;
    }
    return true;
}
