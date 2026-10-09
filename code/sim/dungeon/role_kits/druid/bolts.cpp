/* Druid bolts (role_kits/druid.cpp): Bloom, and the half of the kit that
   hurts. Wrath and Starfire are on their way a moment before they hit (a
   bolt at DRUID_BOLT_SPEED, a star falling for STARFIRE_FALL), so the
   damage number shows as the clients' bolt or star strikes. Moonfire
   lands at once and leaves a burn that bites each second; it is sent to
   clients again every DRUID_KEEP_SECONDS while it lasts. Hits grow Bloom
   by what the shot says (OnDruidHit). */

// NOTE(zoubir): the foe a spell of the Druid's goes for, within Range: the
// one under the cursor, else the one nearest the cursor, else the
// nearest, in the room its shots reach. Picked as a Ranger's shot is
// (role_kits/ranger/shots.cpp), so a Druid at a gate reaches into the
// fight but never wakes the room behind
internal world_entity *
DruidTarget(app_state *AppState, player_slot *Slot, world_entity *Player, float Range)
{
    world_entity *Result = RangerTarget(AppState, Slot, Player, Range);
    return Result;
}

// NOTE(zoubir): Amount more Bloom, up to the most; coming full flashes
internal void
AddDruidBloom(app_state *AppState, player_slot *Slot, u32 Amount)
{
    druid_slot *Druid = &Slot->Druid;
    bool32 WasFull = Druid->Bloom >= DRUID_BLOOM_MOST;
    Druid->Bloom = Minimum((u32)DRUID_BLOOM_MOST, Druid->Bloom + Amount);
    Druid->BloomHold = DRUID_BLOOM_HOLD;
    Druid->BloomFade = 1.f;
    if (!WasFull && Druid->Bloom >= DRUID_BLOOM_MOST && Slot->Entity)
    {
        EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_BloomFull),
                  (u8)Slot->Entity->PlayerIndex, ChestOf(Slot->Entity));
    }
}

// NOTE(zoubir): all of Slot's Bloom, which a heal spends
internal u32
SpendDruidBloom(player_slot *Slot)
{
    u32 Result = Slot->Druid.Bloom;
    Slot->Druid.Bloom = 0;
    return Result;
}

// NOTE(zoubir): Bloom a hit of Shot grows
inline u32
DruidShotBloom(u32 Shot)
{
    u32 Result = 0;
    switch(Shot)
    {
        case DruidShot_Wrath: Result = 1; break;
        case DruidShot_Starfire: Result = STARFIRE_BLOOM; break;
        case DruidShot_Starfall: Result = 1; break;
    }
    return Result;
}

// NOTE(zoubir): a hit of Shot on Monster for the Druid in slot By, which
// OnDruidHit and DruidDealtScale know by Hitting
internal void
DruidHit(app_state *AppState, u32 By, world_entity *Monster, u32 Shot, float Damage, float Shove,
         v2 Away, status_effect Status = StatusEffect_None, float StatusSeconds = 0.f)
{
    player_slot *Slot = &AppState->Players[By];
    world_entity *Player = Slot->Entity;
    if (!Player || !Monster->IsPresent || Monster->Hp <= 0.f)
    {
        return;
    }
    hit Hit = {Damage, Shove, 0.f, 0.f, 0.f, SimBurst_Count, Status, StatusSeconds};
    Slot->Druid.Hitting = Shot;
    ApplyHit(AppState, &AppState->World, Monster, &Hit, Away, Player, By);
    Slot->Druid.Hitting = DruidShot_None;
}

// NOTE(zoubir): a bolt of Shot from player By at Foe, landing Delay from
// now; dropped when every bolt is on its way
internal void
SendDruidBolt(app_state *AppState, u32 By, world_entity *Foe, u32 Shot, float Damage, float Delay,
              v2 Away)
{
    druid_run *Run = &AppState->Dungeon->Druid;
    for(u32 Index = 0; Index < DRUID_MAX_BOLTS; Index++)
    {
        druid_bolt *Bolt = &Run->Bolts[Index];
        if (Bolt->Shot == DruidShot_None)
        {
            Bolt->TargetSlot = (u32)(Foe - AppState->World.Entities);
            Bolt->TargetSerial = Foe->MonsterSerial;
            Bolt->Delay = Delay;
            Bolt->Damage = Damage;
            Bolt->Away = Away;
            Bolt->By = (u8)By;
            Bolt->Shot = (u8)Shot;
            return;
        }
    }
}

// NOTE(zoubir): Wrath: a bolt at the foe, its burst for clients to fly;
// false with no foe in reach, which spends nothing
internal bool32
CastWrath(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world_entity *Foe = DruidTarget(AppState, Slot, Player, WRATH_RANGE);
    if (!Foe)
    {
        return false;
    }
    v2 Offset = Foe->Position.XY - Player->Position.XY;
    v2 Dir = NormalizeOr(Offset, GetPlayerAim(Player));
    SendDruidBolt(AppState, Player->PlayerIndex, Foe, DruidShot_Wrath, WRATH_DAMAGE,
                  Length(Offset) / DRUID_BOLT_SPEED, Dir);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Wrath),
              (u8)Player->PlayerIndex, ChestOf(Foe), ATan2(Dir.Y, Dir.X));
    EmitSound(&AppState->Events, AssetType_SfxFireCast, Player->Position);
    return true;
}

// NOTE(zoubir): Starfire, once its cast is over: a star falls on the foe
// aimed at; with none in reach it falls at the cursor and strikes nothing
internal void
DropStarfire(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world_entity *Foe = DruidTarget(AppState, Slot, Player, STARFIRE_RANGE);
    u8 By = (u8)Player->PlayerIndex;
    v3 Spot;
    if (Foe)
    {
        v2 Away = NormalizeOr(Foe->Position.XY - Player->Position.XY, GetPlayerAim(Player));
        SendDruidBolt(AppState, By, Foe, DruidShot_Starfire, STARFIRE_DAMAGE, STARFIRE_FALL, Away);
        Spot = ChestOf(Foe);
    }
    else
    {
        v2 Point = AimPoint(Player);
        Spot = V3(Point.X, Point.Y, Player->GroundZ);
    }
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Starfire), By,
              DruidBurstSpot(Spot, Foe ? 0 : 1));
    EmitSound(&AppState->Events, AssetType_SfxMeteorCast, Spot);
}

// NOTE(zoubir): the Moonfire of the Druid in slot By on Foe, 0 for none
internal druid_moonfire *
FindDruidMoonfire(app_state *AppState, u32 By, world_entity *Foe)
{
    druid_run *Run = &AppState->Dungeon->Druid;
    u32 FoeSlot = (u32)(Foe - AppState->World.Entities);
    for(u32 Index = 0; Index < DRUID_MAX_MOONFIRES; Index++)
    {
        druid_moonfire *Burn = &Run->Moonfires[Index];
        if (Burn->Seconds > 0.f && Burn->By == By && Burn->TargetSlot == FoeSlot &&
            Burn->TargetSerial == Foe->MonsterSerial)
        {
            return Burn;
        }
    }
    return 0;
}

// NOTE(zoubir): the burn of the Druid in slot By left on Foe, fresh
// where it already burns; nothing when every burn is in use
internal void
LeaveDruidMoonfire(app_state *AppState, u8 By, world_entity *Foe)
{
    druid_run *Run = &AppState->Dungeon->Druid;
    druid_moonfire *Burn = FindDruidMoonfire(AppState, By, Foe);
    for(u32 Index = 0; Index < DRUID_MAX_MOONFIRES && !Burn; Index++)
    {
        if (Run->Moonfires[Index].Seconds <= 0.f)
        {
            Burn = &Run->Moonfires[Index];
        }
    }
    if (Burn)
    {
        Burn->TargetSlot = (u32)(Foe - AppState->World.Entities);
        Burn->TargetSerial = Foe->MonsterSerial;
        Burn->Seconds = MOONFIRE_SECONDS;
        Burn->TickTimer = 1.f;
        Burn->Keep = DRUID_KEEP_SECONDS;
        Burn->By = By;
    }
}

// NOTE(zoubir): Moonfire: a hit at once and a burn left on the foe (a
// fresh one where it already burns); false with no foe in reach
internal bool32
CastMoonfire(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world_entity *Foe = DruidTarget(AppState, Slot, Player, MOONFIRE_RANGE);
    if (!Foe)
    {
        return false;
    }
    u8 By = (u8)Player->PlayerIndex;
    LeaveDruidMoonfire(AppState, By, Foe);
    v2 Away = NormalizeOr(Foe->Position.XY - Player->Position.XY, GetPlayerAim(Player));
    DruidHit(AppState, By, Foe, DruidShot_Moonfire, MOONFIRE_DAMAGE, 0.f, Away);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Moonfire), By,
              ChestOf(Foe));
    EmitSound(&AppState->Events, AssetType_SfxFireCast, Player->Position);
    return true;
}

// NOTE(zoubir): whether a Moonfire of the Druid in slot By burns Foe
inline bool32
IsDruidMoonfired(app_state *AppState, u32 By, world_entity *Foe)
{
    bool32 Result = Foe && AppState->Dungeon && FindDruidMoonfire(AppState, By, Foe) != 0;
    return Result;
}

// NOTE(zoubir): once a tick: bolts and stars reach their foes
internal void
UpdateDruidBolts(app_state *AppState, druid_run *Run, float DeltaTime)
{
    for(u32 Index = 0; Index < DRUID_MAX_BOLTS; Index++)
    {
        druid_bolt *Bolt = &Run->Bolts[Index];
        if (Bolt->Shot == DruidShot_None)
        {
            continue;
        }
        Bolt->Delay -= DeltaTime;
        if (Bolt->Delay > 0.f)
        {
            continue;
        }
        world_entity *Foe = FindMonsterBySerial(&AppState->World, Bolt->TargetSlot, Bolt->TargetSerial);
        u32 Shot = Bolt->Shot;
        Bolt->Shot = DruidShot_None;
        if (Foe && Foe->Hp > 0.f)
        {
            float Shove = Shot == DruidShot_Starfire ? 90.f : 30.f;
            // NOTE(zoubir): Eclipse: a bolt or a star on a foe its Druid's
            // Moonfire burns hits harder, and a star leaves the burn
            bool32 Talent = RoleRank(&AppState->Players[Bolt->By], PlayerRole_Druid,
                                     DruidTalent_Eclipse) > 0;
            bool32 Eclipse = Talent && IsDruidMoonfired(AppState, Bolt->By, Foe);
            DruidHit(AppState, Bolt->By, Foe, Shot, Bolt->Damage * (Eclipse ? 1.f + ECLIPSE_SHARE : 1.f),
                     Shove, Bolt->Away);
            if (Talent && Shot == DruidShot_Starfire && Foe->IsPresent && Foe->Hp > 0.f)
            {
                LeaveDruidMoonfire(AppState, Bolt->By, Foe);
                EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Moonfire),
                          Bolt->By, ChestOf(Foe));
            }
            EmitSound(&AppState->Events, Shot == DruidShot_Starfire ? AssetType_SfxGiantFireball :
                      AssetType_SfxHit, Foe->Position);
        }
    }
}

// NOTE(zoubir): once a tick: each Moonfire bites every second, is sent
// again to clients where its foe is now, and goes out with its foe
internal void
UpdateDruidMoonfires(app_state *AppState, druid_run *Run, float DeltaTime)
{
    for(u32 Index = 0; Index < DRUID_MAX_MOONFIRES; Index++)
    {
        druid_moonfire *Burn = &Run->Moonfires[Index];
        if (Burn->Seconds <= 0.f)
        {
            continue;
        }
        world_entity *Foe = FindMonsterBySerial(&AppState->World, Burn->TargetSlot, Burn->TargetSerial);
        if (!Foe || Foe->Hp <= 0.f || !LivingPlayerInSlot(AppState, Burn->By))
        {
            Burn->Seconds = 0.f;
            continue;
        }
        Burn->Seconds = Maximum(0.f, Burn->Seconds - DeltaTime);
        Burn->TickTimer -= DeltaTime;
        if (Burn->TickTimer <= 0.f)
        {
            Burn->TickTimer += 1.f;
            DruidHit(AppState, Burn->By, Foe, DruidShot_Moonfire, MOONFIRE_TICK_DAMAGE, 0.f,
                     V2(1.f, 0.f));
        }
        Burn->Keep -= DeltaTime;
        if (Burn->Keep <= 0.f && Burn->Seconds > 0.f)
        {
            Burn->Keep += DRUID_KEEP_SECONDS;
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_DruidFirst, DruidBurst_Moonfire), Burn->By,
                      DruidBurstSpot(ChestOf(Foe), 1));
        }
    }
}
