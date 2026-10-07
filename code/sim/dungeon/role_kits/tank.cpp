/* The tank's kit (role_abilities.cpp): Taunt on A, Shield Slam on R,
   Shield Throw on W, and from its tree Intercept on C and Last Stand on
   V.

   Shield Throw is the tank's attack: the shield hits the foe it aims at
   for SHIELD_THROW_DAMAGE, then bounces to the nearest foe the throw has
   not hit yet, up to SHIELD_THROW_BOUNCES times, each hit
   SHIELD_THROW_BOUNCE_SHARE of the one before. Every foe it hits takes
   SHIELD_THROW_THREAT and is sundered like the tank's fireball does it,
   so the throw also picks up a pack from range.

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
   ally in trouble. Between slams its fireball keeps the sunder
   going: a shot on a sundered monster adds SUNDERING_SHOT_SECONDS (never
   past a fresh slam's), and on a clean one starts a short sunder of
   SUNDERING_SHOT_FRESH, so a tank that has to stay away still holds the
   party's bonus up.

   What keeps a tank standing late in a big party: it takes only the
   square root of the party's extra damage (PartySustainScale,
   party_scaling.cpp), the slam's heal grows with it like a healer's, and
   Last Stand heals LAST_STAND_HEAL_SHARE of its health behind
   LAST_STAND_SECONDS of Shield Wall for the hit it cannot take. */

// NOTE(zoubir): from OnRoleHit: the tank's fireball landed
internal void
OnTankShot(app_state *AppState, player_slot *Slot, world_entity *Target)
{
    dungeon_run *Run = AppState->Dungeon;
    world *World = &AppState->World;
    bool32 Shatter = RoleRank(Slot, PlayerRole_Tank, TankTalent_ShatterArmor) > 0;
    float Most = SUNDER_SECONDS + (Shatter ? SHATTER_SECONDS : 0.f);
    float Share = SUNDER_SHARE + (Shatter ? SHATTER_SHARE : 0.f);
    foe_mark *Mark = FindFoeMark(Run, World, Target);
    if (Mark && Mark->SunderSeconds > 0.f)
    {
        Mark->SunderSeconds = Minimum(Most, Mark->SunderSeconds + SUNDERING_SHOT_SECONDS);
        Mark->SunderShare = Maximum(Mark->SunderShare, Share);
    }
    else
    {
        AddSunder(Run, World, Target, SUNDERING_SHOT_FRESH, Share);
    }
}

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
    for(u32 Other = 0; Other < MAX_PLAYERS; Other++)
    {
        world_entity *Ally = LivingPlayerInSlot(AppState, Other);
        if (Ally && Ally != Player && Length(Ally->Position.XY - Player->Position.XY) <= RALLY_RADIUS)
        {
            player_slot *AllySlot = &AppState->Players[Other];
            AllySlot->RallySeconds = RALLY_SECONDS;
            AllySlot->RallyShare = RALLY_SHARE;
        }
    }
    EmitBurst(&AppState->Events, SimBurst_ShieldSlam, SlotIndex, Player->Position,
              ATan2(Player->Aim.Y, Player->Aim.X));
    EmitSound(&AppState->Events, AssetType_SfxShieldSlam, Player->Position);
}

// NOTE(zoubir): returns whether the shield found a foe to hit
internal bool32
CastShieldThrow(app_state *AppState, world *World, player_slot *Slot, world_entity *Player)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    world_entity *Foe = AttackTarget(AppState, Slot, Player, SHIELD_THROW_RANGE);
    if (!Foe)
    {
        return false;
    }
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    world_entity *Struck[1 + SHIELD_THROW_BOUNCES];
    u32 StruckCount = 0;
    v2 From = Player->Position.XY;
    float Damage = SHIELD_THROW_DAMAGE;
    EmitBurst(&AppState->Events, SimBurst_Lunge, SlotIndex, ChestOf(Player),
              ATan2(Foe->Position.Y - From.Y, Foe->Position.X - From.X));
    while(Foe && StruckCount < ArrayCount(Struck))
    {
        Struck[StruckCount++] = Foe;
        hit Hit = {Damage, SHIELD_THROW_SHOVE, 0.f, 0.f, 0.f, SimBurst_Impact};
        AddThreat(&AppState->Dungeon->Threat, World, Foe, SlotIndex, SHIELD_THROW_THREAT);
        OnTankShot(AppState, Slot, Foe);
        v2 Toward = DirectionTo(Foe->Position.XY - From);
        From = Foe->Position.XY;
        ApplyHit(AppState, World, Foe, &Hit, Toward, Player, SlotIndex);
        Damage *= SHIELD_THROW_BOUNCE_SHARE;
        Foe = NearestFoe(World, From, SHIELD_THROW_BOUNCE_RADIUS, Room, Struck, StruckCount);
    }
    EmitSound(&AppState->Events, AssetType_SfxShieldSlam, Player->Position);
    return true;
}

// NOTE(zoubir): returns whether the key cast (an intercept with nobody
// to leap to, or a shield throw with no foe in reach, does not, and
// keeps its cooldown)
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
            EmitSound(&AppState->Events, AssetType_SfxTaunt, Player->Position);
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
            // NOTE(zoubir): Intercept's second rank wards the ally
            if (RoleRank(Slot, PlayerRole_Tank, TankTalent_Intercept) >= 2)
            {
                player_slot *AllySlot = &AppState->Players[Ally->PlayerIndex];
                AllySlot->WardAbsorb = Maximum(AllySlot->WardAbsorb, GUARDIAN_WARD);
                AllySlot->WardFull = Maximum(AllySlot->WardAbsorb, AllySlot->WardFull);
            }
            EmitBurst(&AppState->Events, SimBurst_InterceptLand, SlotIndex, Player->Position, Angle);
            EmitSound(&AppState->Events, AssetType_SfxDash, Player->Position);
        } break;

        case 3:
        {
            HealPlayer(AppState, SlotIndex, Player, LAST_STAND_HEAL_SHARE * Player->MaxHp);
            Slot->ShieldWallSeconds = Maximum(Slot->ShieldWallSeconds, LAST_STAND_SECONDS);
            EmitSound(&AppState->Events, AssetType_SfxShield, Player->Position);
            EmitBurst(&AppState->Events, SimBurst_ShieldSlam, SlotIndex, Player->Position,
                      ATan2(Player->Aim.Y, Player->Aim.X));
        } break;

        case 4:
        {
            return CastShieldThrow(AppState, World, Slot, Player);
        } break;
    }
    return true;
}
