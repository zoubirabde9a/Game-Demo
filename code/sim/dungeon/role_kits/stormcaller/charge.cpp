/* Stormcaller Charge (role_kits/stormcaller.cpp): the pressure gauge, the
   overload it ends in, and the one hit every Stormcaller spell lands
   through. Charge is a float in stormcaller_slot and goes to clients
   rounded as ClassMeter; the flags (STORMCALLER_FLAG_*) are set here from
   the state, at once when a spell changes it and every tick. A client
   predicting its own Stormcaller never changes its Charge: the server
   sends it back. */

// NOTE(zoubir): the room a Stormcaller's lightning reaches: its own, or,
// standing in a doorway or a corridor (room 0), the room being fought, so
// a Stormcaller at a gate casts into the fight but never wakes a room
// behind one
inline u32
StormcallerRoom(app_state *AppState, world_entity *Player)
{
    world *World = &AppState->World;
    u32 Result = RoomAtPosition(World, Player->Position.XY);
    if (Result == 0 && AppState->Dungeon && AppState->Dungeon->FightingRoom)
    {
        Result = AppState->Dungeon->FightingRoom;
    }
    return Result;
}

inline bool32
IsStormcallerFoe(world *World, world_entity *Monster, u32 Room)
{
    bool32 Result = Monster->IsPresent && Monster->Type == EntityType_Monster &&
        Monster->Hp > 0.f && RoomAtPosition(World, Monster->Position.XY) == Room;
    return Result;
}

// NOTE(zoubir): whether the Stormcaller's lightning is Supercharged now
inline bool32
IsSupercharged(player_slot *Slot)
{
    bool32 Result = Slot->Stormcaller.Charge >= STORM_SUPERCHARGED || Slot->Stormcaller.EyeSeconds > 0.f;
    return Result;
}

// NOTE(zoubir): whether the Stormcaller in slot By has a Static Field down
inline bool32
StormcallerFieldDown(stormcaller_run *Run, u32 By)
{
    bool32 Result = false;
    for(u32 Index = 0; Index < STORMCALLER_MAX_FIELDS; Index++)
    {
        Result |= Run->Fields[Index].Seconds > 0.f && Run->Fields[Index].By == By;
    }
    return Result;
}

// NOTE(zoubir): ClassMeter and ClassFlags, which clients read, from the
// Stormcaller's state; a client predicting its own keeps what the server
// sent
internal void
SetStormcallerFlags(app_state *AppState, player_slot *Slot)
{
    if (Slot->Predicted)
    {
        return;
    }
    stormcaller_slot *Storm = &Slot->Stormcaller;
    u32 Index = (u32)(Slot - AppState->Players);
    bool32 Field = AppState->Dungeon && StormcallerFieldDown(&AppState->Dungeon->Stormcaller, Index);
    Slot->ClassMeter = (u8)(Minimum(STORM_CHARGE_MOST, Storm->Charge) + 0.5f);
    Slot->ClassFlags = (u8)((IsSupercharged(Slot) ? STORMCALLER_FLAG_SUPERCHARGED : 0) |
                            (Storm->EyeSeconds > 0.f ? STORMCALLER_FLAG_EYE : 0) |
                            (Storm->GroundedSeconds > 0.f ? STORMCALLER_FLAG_GROUNDED : 0) |
                            (Field ? STORMCALLER_FLAG_FIELD : 0) |
                            (Storm->EmptySeconds > 0.f ? STORMCALLER_FLAG_EMPTY : 0));
}

// NOTE(zoubir): a hit of the Stormcaller in slot By on Monster; no burst
// of its own (the spell sends the bolt's), so clients draw the spark of
// the hit itself (HitFresh, HitBySlot)
internal void
StormcallerZap(app_state *AppState, u32 By, world_entity *Monster, float Damage, float Shove,
               v2 Away, status_effect Status = StatusEffect_None, float StatusSeconds = 0.f,
               float StunSeconds = 0.f)
{
    player_slot *Slot = &AppState->Players[By];
    world_entity *Player = Slot->Entity;
    if (!Player || !Monster->IsPresent || Monster->Hp <= 0.f)
    {
        return;
    }
    hit Hit = {Damage, Shove, 0.f, 0.f, StunSeconds, SimBurst_Count, Status, StatusSeconds};
    ApplyHit(AppState, &AppState->World, Monster, &Hit, Away, Player, By);
}

// NOTE(zoubir): Charge topped out: the nova goes off round the
// Stormcaller and hurts every foe near, then Charge is gone, and without
// Live Wire it costs health and grounds every key
internal void
OverloadStormcaller(app_state *AppState, player_slot *Slot)
{
    world_entity *Player = Slot->Entity;
    stormcaller_slot *Storm = &Slot->Stormcaller;
    Storm->Charge = 0.f;
    if (!Player)
    {
        return;
    }
    world *World = &AppState->World;
    u32 By = (u32)(Slot - AppState->Players);
    bool32 LiveWire = RoleRank(Slot, PlayerRole_Stormcaller, StormcallerTalent_LiveWire) > 0;
    float Radius = OVERLOAD_RADIUS * (LiveWire ? LIVE_WIRE_SCALE : 1.f);
    float Damage = OVERLOAD_DAMAGE * (LiveWire ? LIVE_WIRE_SCALE : 1.f);
    u32 Room = StormcallerRoom(AppState, Player);
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        v2 Offset = Monster->Position.XY - Player->Position.XY;
        if (IsStormcallerFoe(World, Monster, Room) &&
            Length(Offset) <= Radius + 0.5f * Monster->Dimensions.X)
        {
            StormcallerZap(AppState, By, Monster, Damage, OVERLOAD_SHOVE, NormalizeOr(Offset, V2(1.f, 0.f)));
        }
    }
    if (!LiveWire)
    {
        Player->Hp = Maximum(Minimum(Player->Hp, 1.f), Player->Hp - OVERLOAD_HEALTH_SHARE * Player->MaxHp);
        for(u32 Key = 0; Key < ROLE_KEYS; Key++)
        {
            Slot->RoleCooldowns[Key] = Maximum(Slot->RoleCooldowns[Key], OVERLOAD_GROUNDED);
        }
        Storm->GroundedSeconds = OVERLOAD_GROUNDED;
    }
    v3 Feet = V3(Player->Position.X, Player->Position.Y, Player->GroundZ);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_StormcallerFirst, StormcallerBurst_Overload),
              (u8)By, StormcallerBurstSpot(Feet, (u32)(Radius + 0.5f)));
    EmitSound(&AppState->Events, AssetType_SfxExplosion, Player->Position);
    SetStormcallerFlags(AppState, Slot);
}

// NOTE(zoubir): Amount more Charge. Crossing STORM_SUPERCHARGED flashes the
// caster; reaching STORM_CHARGE_MOST overloads, except under Eye of the
// Storm, which holds it at the top
internal void
AddStormCharge(app_state *AppState, player_slot *Slot, float Amount)
{
    if (Slot->Predicted || Amount <= 0.f)
    {
        return;
    }
    stormcaller_slot *Storm = &Slot->Stormcaller;
    bool32 Was = Storm->Charge >= STORM_SUPERCHARGED;
    Storm->Charge += Amount;
    if (Storm->Charge >= STORM_CHARGE_MOST)
    {
        if (Storm->EyeSeconds > 0.f)
        {
            Storm->Charge = STORM_CHARGE_MOST;
        }
        else
        {
            OverloadStormcaller(AppState, Slot);
            return;
        }
    }
    if (!Was && Storm->Charge >= STORM_SUPERCHARGED && Slot->Entity)
    {
        EmitBurst(&AppState->Events, ClassBurst(SimBurst_StormcallerFirst, StormcallerBurst_Supercharged),
                  (u8)(Slot - AppState->Players), ChestOf(Slot->Entity));
    }
    SetStormcallerFlags(AppState, Slot);
}

// NOTE(zoubir): once a tick: Charge drains after STORM_IDLE_SECONDS
// without a hit dealt (never under Eye of the Storm), and the timers run
// down
internal void
UpdateStormCharge(player_slot *Slot, float DeltaTime)
{
    stormcaller_slot *Storm = &Slot->Stormcaller;
    Storm->IdleSeconds += DeltaTime;
    Storm->GroundedSeconds = Maximum(0.f, Storm->GroundedSeconds - DeltaTime);
    Storm->EmptySeconds = Maximum(0.f, Storm->EmptySeconds - DeltaTime);
    if (Storm->IdleSeconds > STORM_IDLE_SECONDS && Storm->EyeSeconds <= 0.f)
    {
        Storm->Charge = Maximum(0.f, Storm->Charge - STORM_CHARGE_DRAIN * DeltaTime);
    }
}
