/* Duelist strikes (role_kits/duelist.cpp): the rapier's blows. Thrust stabs
   the first foe in a narrow line in front (and again under Perfect Form),
   Lunge dashes to a foe and strikes it, and Heartseeker strikes the foe it
   was pressed on when its wind-up ends, harder by Tempo and on a hurt
   foe; at full Tempo Crescendo sweeps the foes beside it and Masterstroke
   strikes it again (from duelist/effects.cpp). Each blow reaches only foes
   in the Duelist's room, so none wakes the room behind a gate. */

// NOTE(zoubir): a living monster in Room
inline bool32
IsDuelistFoe(world *World, world_entity *Monster, u32 Room)
{
    bool32 Result = Monster->IsPresent && Monster->Type == EntityType_Monster &&
        Monster->Hp > 0.f && RoomAtPosition(World, Monster->Position.XY) == Room;
    return Result;
}

// NOTE(zoubir): whether Foe is within Reach (plus half its width) of the
// Duelist and HalfArc of Direction, and how far it is
inline bool32
InDuelistArc(world *World, world_entity *Player, world_entity *Foe, u32 Room, v2 Direction,
             float Reach, float HalfArc, float *Distance)
{
    v2 Offset = Foe->Position.XY - Player->Position.XY;
    *Distance = Length(Offset);
    bool32 Result = IsDuelistFoe(World, Foe, Room) &&
        *Distance <= Reach + 0.5f * Foe->Dimensions.X &&
        (*Distance <= 0.5f * Foe->Dimensions.X ||
         DotProduct(Offset, Direction) >= Cos(HalfArc) * *Distance);
    return Result;
}

// NOTE(zoubir): the nearest foe in the arc, 0 for none
internal world_entity *
DuelistFrontFoe(app_state *AppState, world_entity *Player, v2 Direction, float Reach, float HalfArc)
{
    world *World = &AppState->World;
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    world_entity *Result = 0;
    float Best = 0.f;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Foe = &World->Entities[EntityIndex];
        float Distance;
        if (InDuelistArc(World, Player, Foe, Room, Direction, Reach, HalfArc, &Distance) &&
            (!Result || Distance < Best))
        {
            Result = Foe;
            Best = Distance;
        }
    }
    return Result;
}

inline v2
DuelistAway(world_entity *Player, world_entity *Foe)
{
    v2 Result = NormalizeOr(Foe->Position.XY - Player->Position.XY,
                            NormalizeOr(GetPlayerAim(Player), V2(1.f, 0.f)));
    return Result;
}

// NOTE(zoubir): one of the Duelist's blows on Foe; true when Foe died of it
internal bool32
DuelistStrike(app_state *AppState, player_slot *Slot, world_entity *Player, world_entity *Foe,
              float Damage, float Shove, float Stun = 0.f)
{
    hit Hit = {Damage, Shove, 0.f, 0.f, Stun, SimBurst_Count};
    ApplyHit(AppState, &AppState->World, Foe, &Hit, DuelistAway(Player, Foe), Player,
             Player->PlayerIndex);
    Slot->Duelist.IdleSeconds = 0.f;
    bool32 Result = !Foe->IsPresent || Foe->Hp <= 0.f;
    return Result;
}

// NOTE(zoubir): a Thrust along Direction; true when it struck a foe
internal bool32
ThrustAlong(app_state *AppState, player_slot *Slot, world_entity *Player, v2 Direction)
{
    float Damage = THRUST_DAMAGE * (1.f + FLURRY_SHARE *
        (float)RoleRank(Slot, PlayerRole_Duelist, DuelistTalent_Flurry));
    world_entity *Foe = DuelistFrontFoe(AppState, Player, Direction, THRUST_REACH, THRUST_HALF_ARC);
    if (Foe)
    {
        DuelistStrike(AppState, Slot, Player, Foe, Damage, THRUST_SHOVE);
    }
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DuelistFirst, DuelistBurst_Thrust),
              (u8)Player->PlayerIndex, DuelistBurstSpot(ChestOf(Player), Foe ? 1 : 0),
              ATan2(Direction.Y, Direction.X));
    EmitSound(&AppState->Events, AssetType_SfxSword, Player->Position);
    return Foe != 0;
}

// NOTE(zoubir): Thrust (right click): at the foe under the cursor or
// nearest it if one is in reach, else where the player aims. It lands
// Tempo on a new key; under Perfect Form a second one follows
internal void
CastThrust(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    bool32 Fresh = DuelistUseKey(Slot, 6);
    v2 Direction = NormalizeOr(GetPlayerAim(Player), V2(1.f, 0.f));
    world_entity *Target = AttackTarget(AppState, Slot, Player, THRUST_REACH + 40.f);
    if (Target && LengthSq(Target->Position.XY - Player->Position.XY) > 1.f)
    {
        Direction = DirectionTo(Target->Position.XY - Player->Position.XY);
    }
    if (ThrustAlong(AppState, Slot, Player, Direction) && Fresh)
    {
        AddTempo(Slot, 1);
    }
    if (Slot->Duelist.FormSeconds > 0.f)
    {
        Slot->Duelist.EchoDelay = FORM_ECHO_DELAY;
        Slot->Duelist.EchoDirection = Direction;
    }
}

// NOTE(zoubir): Lunge (A): to just in front of the foe and a strike;
// false with no foe in range, which costs nothing
internal bool32
CastLunge(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
          world_entity *Player)
{
    world_entity *Foe = AttackTarget(AppState, Slot, Player, LUNGE_RANGE);
    if (!Foe)
    {
        return false;
    }
    bool32 Fresh = DuelistUseKey(Slot, 0);
    v2 From = Player->Position.XY;
    v2 Toward = DuelistAway(Player, Foe);
    float Gap = LUNGE_GAP + 0.5f * Foe->Dimensions.X;
    if (Length(Foe->Position.XY - From) > Gap)
    {
        v3 Landing = Foe->Position;
        Landing.XY -= Gap * Toward;
        Landing.Z = Player->Position.Z;
        MovePlayerTo(AppState, World, Arena, Player, Landing);
    }
    float Came = Length(Player->Position.XY - From);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DuelistFirst, DuelistBurst_Lunge),
              (u8)Player->PlayerIndex,
              DuelistBurstSpot(ChestOf(Player), (u32)(Came / DUELIST_LUNGE_STEP + 0.5f)),
              ATan2(Toward.Y, Toward.X));
    EmitSound(&AppState->Events, AssetType_SfxDash, Player->Position);
    DuelistStrike(AppState, Slot, Player, Foe, LUNGE_DAMAGE, LUNGE_SHOVE);
    if (Fresh)
    {
        AddTempo(Slot, 1);
    }
    // NOTE(zoubir): Footwork: quick on the feet after it (a status, which
    // snapshots carry, so a predicting client runs as fast)
    if (RoleRank(Slot, PlayerRole_Duelist, DuelistTalent_Footwork))
    {
        ApplyStatus(Player, StatusEffect_Hasted, FOOTWORK_HASTE_SECONDS);
    }
    return true;
}

// NOTE(zoubir): Crescendo: every other foe in a wide arc toward Foe takes
// Damage; returns how many
internal u32
CrescendoSweep(app_state *AppState, player_slot *Slot, world_entity *Player, world_entity *Foe,
               float Damage)
{
    world *World = &AppState->World;
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    v2 Direction = DuelistAway(Player, Foe);
    u32 Result = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Other = &World->Entities[EntityIndex];
        float Distance;
        if (Other != Foe && InDuelistArc(World, Player, Other, Room, Direction,
                                         HEARTSEEKER_REACH + 30.f, CRESCENDO_HALF_ARC, &Distance))
        {
            DuelistStrike(AppState, Slot, Player, Other, Damage, HEARTSEEKER_SHOVE);
            Result++;
        }
    }
    return Result;
}

// NOTE(zoubir): Heartseeker's strike, its wind-up over: on the foe it was
// pressed on if still in reach, else the one in front; with none the
// rapier pierces the air and the key is ready again
internal void
FinishHeartseeker(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    duelist_slot *Duel = &Slot->Duelist;
    world_entity *Foe = FindMonsterBySerial(World, Duel->HeartSlot, Duel->HeartSerial);
    if (Foe && (Foe->Hp <= 0.f ||
                Length(Foe->Position.XY - Player->Position.XY) >
                HEARTSEEKER_HOLD_REACH + 0.5f * Foe->Dimensions.X))
    {
        Foe = 0;
    }
    if (!Foe)
    {
        Foe = AttackTarget(AppState, Slot, Player, HEARTSEEKER_REACH + 30.f);
    }
    if (!Foe)
    {
        v2 Aim = NormalizeOr(GetPlayerAim(Player), V2(1.f, 0.f));
        v3 Air = ChestOf(Player);
        Air.XY += (0.7f * HEARTSEEKER_REACH) * Aim;
        EmitBurst(&AppState->Events, ClassBurst(SimBurst_DuelistFirst, DuelistBurst_Heartseeker),
                  (u8)Player->PlayerIndex, DuelistBurstSpot(Air, 0), ATan2(Aim.Y, Aim.X));
        EmitSound(&AppState->Events, AssetType_SfxSword, Player->Position);
        Slot->RoleCooldowns[4] = 0.f;
        return;
    }
    u32 Tempo = DuelistTempo(Slot);
    bool32 Full = Tempo >= DUELIST_MOST_TEMPO;
    float Base = HEARTSEEKER_DAMAGE + HEARTSEEKER_PER_TEMPO * (float)Tempo;
    float Low = RoleRank(Slot, PlayerRole_Duelist, DuelistTalent_Precision) ?
        PRECISION_LOW_HEALTH : HEARTSEEKER_LOW_HEALTH;
    float Damage = Base;
    if (Foe->MaxHp > 0.f && Foe->Hp < Low * Foe->MaxHp)
    {
        Damage *= HEARTSEEKER_LOW_SCALE;
    }
    u32 Variant = Tempo + 1;
    v2 Away = DuelistAway(Player, Foe);
    if (Full && RoleRank(Slot, PlayerRole_Duelist, DuelistTalent_Crescendo))
    {
        CrescendoSweep(AppState, Slot, Player, Foe, CRESCENDO_SPLASH * Base);
        Slot->RoleCooldowns[0] = 0.f;
        Variant |= DUELIST_BURST_SWEEP;
    }
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DuelistFirst, DuelistBurst_Heartseeker),
              (u8)Player->PlayerIndex, DuelistBurstSpot(ChestOf(Foe), Variant), ATan2(Away.Y, Away.X));
    EmitSound(&AppState->Events, AssetType_SfxSword, Player->Position);
    bool32 Killed = DuelistStrike(AppState, Slot, Player, Foe, Damage, HEARTSEEKER_SHOVE);
    if (Duel->HeartFresh)
    {
        AddTempo(Slot, 1);
    }
    // NOTE(zoubir): Masterstroke: a kill readies Heartseeker; a foe left
    // standing at full Tempo is struck again a moment later
    bool32 Master = RoleRank(Slot, PlayerRole_Duelist, DuelistTalent_Masterstroke) > 0;
    if (Master && Killed)
    {
        Slot->RoleCooldowns[4] = 0.f;
    }
    else if (Master && Full)
    {
        Duel->MasterDelay = MASTERSTROKE_DELAY;
        Duel->MasterDamage = MASTERSTROKE_SHARE * Damage;
    }
}

// NOTE(zoubir): Masterstroke's second strike, from UpdateDuelistEffects,
// on the foe Heartseeker struck if it is still there
internal void
MasterstrokeAgain(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    duelist_slot *Duel = &Slot->Duelist;
    world_entity *Foe = FindMonsterBySerial(&AppState->World, Duel->HeartSlot, Duel->HeartSerial);
    if (!Foe || Foe->Hp <= 0.f ||
        Length(Foe->Position.XY - Player->Position.XY) >
        HEARTSEEKER_HOLD_REACH + 0.5f * Foe->Dimensions.X)
    {
        return;
    }
    v2 Away = DuelistAway(Player, Foe);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DuelistFirst, DuelistBurst_Heartseeker),
              (u8)Player->PlayerIndex,
              DuelistBurstSpot(ChestOf(Foe), (DuelistTempo(Slot) + 1) | DUELIST_BURST_SECOND),
              ATan2(Away.Y, Away.X));
    EmitSound(&AppState->Events, AssetType_SfxSword, Player->Position);
    if (DuelistStrike(AppState, Slot, Player, Foe, Duel->MasterDamage, HEARTSEEKER_SHOVE))
    {
        Slot->RoleCooldowns[4] = 0.f;
    }
}
