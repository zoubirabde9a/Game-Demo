/* Stormcaller bolts (role_kits/stormcaller.cpp): Spark, Chain Lightning
   and Thunderclap. A bolt lands at once on the foe it goes at, then
   jumps to the nearest foe it has not struck within STORM_JUMP_RADIUS of
   the last, weaker each jump; a foe struck standing in the Stormcaller's
   own Static Field arcs to every other foe in it (stormcaller/storm.cpp).
   Each bolt is one burst for clients, drawn from where it came from to
   where it struck. */

// NOTE(zoubir): the foe a Stormcaller spell goes at, within Range: the one
// under the cursor, else the one nearest the cursor, else the one nearest
// the Stormcaller, in StormcallerRoom
internal world_entity *
StormcallerTarget(app_state *AppState, player_slot *Slot, world_entity *Player, float Range)
{
    world *World = &AppState->World;
    u32 Room = StormcallerRoom(AppState, Player);
    u32 Index = Slot->Input.Target;
    if (Index && Index - 1 < World->EntityCount)
    {
        world_entity *Unit = &World->Entities[Index - 1];
        if (IsStormcallerFoe(World, Unit, Room) &&
            Length(Unit->Position.XY - Player->Position.XY) <= Range)
        {
            return Unit;
        }
    }
    world_entity *Result = NearestFoe(World, AimPoint(Player), ATTACK_PICK_RADIUS, Room, 0, 0);
    if (!Result || Length(Result->Position.XY - Player->Position.XY) > Range)
    {
        Result = NearestFoe(World, Player->Position.XY, Range, Room, 0, 0);
    }
    return Result;
}

// NOTE(zoubir): a bolt's burst from From to the foe struck at To
internal void
EmitStormcallerBolt(app_state *AppState, u32 By, v3 From, v3 To, u32 Kind)
{
    v2 Way = To.XY - From.XY;
    float Angle = LengthSq(Way) > 1.f ? ATan2(Way.Y, Way.X) : 0.f;
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_StormcallerFirst, StormcallerBurst_Bolt), (u8)By,
              StormcallerBurstSpot(To, StormcallerBoltVariant(Kind, Length(Way))), Angle);
}

// NOTE(zoubir): the foes one spell has struck or arced to, so neither a
// jump nor an arc lands twice on one foe
struct stormcaller_struck
{
    world_entity *Jumped[16];
    u32 JumpCount;
    world_entity *Arced[32];
    u32 ArcCount;
};

internal bool32 StormcallerFieldHolds(app_state *AppState, u32 By, world_entity *Monster,
                                      stormcaller_field **Field);

// NOTE(zoubir): Damage on Foe struck by a bolt from slot By; when Foe stands
// in By's own Static Field the strike arcs to every other foe in it that
// this spell has not arced to yet
internal void
StormcallerBoltHit(app_state *AppState, u32 By, world_entity *Foe, float Damage, float Shove,
                   v2 Away, stormcaller_struck *Struck)
{
    StormcallerZap(AppState, By, Foe, Damage, Shove, Away);
    stormcaller_field *Field = 0;
    if (!StormcallerFieldHolds(AppState, By, Foe, &Field))
    {
        return;
    }
    world *World = &AppState->World;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Other = &World->Entities[EntityIndex];
        bool32 Done = Other == Foe;
        for(u32 Index = 0; Index < Struck->ArcCount; Index++)
        {
            Done |= Struck->Arced[Index] == Other;
        }
        if (Done || Struck->ArcCount >= ArrayCount(Struck->Arced) ||
            Other->Type != EntityType_Monster || Other->Hp <= 0.f ||
            !StormcallerFieldHolds(AppState, By, Other, &Field))
        {
            continue;
        }
        Struck->Arced[Struck->ArcCount++] = Other;
        v2 Offset = Other->Position.XY - Foe->Position.XY;
        StormcallerZap(AppState, By, Other, STATIC_ARC_SHARE * Damage, 0.f, NormalizeOr(Offset, Away));
        EmitStormcallerBolt(AppState, By, ChestOf(Foe), ChestOf(Other), StormcallerBolt_Arc);
    }
}

// NOTE(zoubir): a bolt from Player on First for Damage, jumping Jumps more
// times, each Share of the one before, never on a foe twice; returns how
// many foes it struck
internal u32
StormcallerChain(app_state *AppState, player_slot *Slot, world_entity *Player, world_entity *First,
                 float Damage, u32 Jumps, float Share, float Shove, u32 Kind)
{
    world *World = &AppState->World;
    u32 By = (u32)(Slot - AppState->Players);
    u32 Room = StormcallerRoom(AppState, Player);
    stormcaller_struck Struck = {};
    v3 From = ChestOf(Player);
    world_entity *Foe = First;
    for(u32 Strike = 0; Foe && Strike <= Jumps && Struck.JumpCount < ArrayCount(Struck.Jumped); Strike++)
    {
        Struck.Jumped[Struck.JumpCount++] = Foe;
        v3 To = ChestOf(Foe);
        v2 Away = NormalizeOr(To.XY - From.XY, GetPlayerAim(Player));
        EmitStormcallerBolt(AppState, By, From, To, Kind);
        StormcallerBoltHit(AppState, By, Foe, Damage, Shove, Away, &Struck);
        From = To;
        Damage *= Share;
        Foe = NearestFoe(World, To.XY, STORM_JUMP_RADIUS, Room, Struck.Jumped, Struck.JumpCount);
    }
    return Struck.JumpCount;
}

// NOTE(zoubir): Spark: a bolt at the foe aimed at that jumps; aimed at
// nothing it fizzles out along the aim
internal void
CastSpark(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    if (Slot->Predicted)
    {
        return;
    }
    world_entity *Foe = StormcallerTarget(AppState, Slot, Player, SPARK_RANGE);
    if (!Foe)
    {
        v3 From = ChestOf(Player);
        v3 To = From;
        To.XY += 0.5f * SPARK_RANGE * GetPlayerAim(Player);
        EmitStormcallerBolt(AppState, (u32)Player->PlayerIndex, From, To, StormcallerBolt_Fizzle);
        EmitSound(&AppState->Events, AssetType_SfxKunai, Player->Position);
        return;
    }
    bool32 Super = IsSupercharged(Slot);
    float Damage = SPARK_DAMAGE * (Super ? 1.f + SUPERCHARGED_SHARE : 1.f);
    u32 Jumps = SPARK_JUMPS + (Super ? SUPERCHARGED_JUMPS : 0);
    StormcallerChain(AppState, Slot, Player, Foe, Damage, Jumps, SPARK_JUMP_SHARE, SPARK_SHOVE,
                     StormcallerBolt_Spark);
    EmitSound(&AppState->Events, AssetType_SfxKunai, Player->Position);
    AddStormCharge(AppState, Slot, SPARK_CHARGE);
}

// NOTE(zoubir): a wind-up (Chain Lightning, Thunderclap) on the foe in
// Range, which is locked now and struck when it ends, as Eviscerate's
// is; no foe, no cast
internal bool32
StartStormcallerWindUp(app_state *AppState, player_slot *Slot, world_entity *Player, player_spell Spell,
                       float Range)
{
    world_entity *Foe = StormcallerTarget(AppState, Slot, Player, Range);
    if (!Foe)
    {
        return false;
    }
    Slot->Stormcaller.LockSlot = (u32)(Foe - AppState->World.Entities);
    Slot->Stormcaller.LockSerial = Foe->MonsterSerial;
    v2 ToFoe = Foe->Position.XY - Player->Position.XY;
    StartPlayerCast(Player, Spell, LengthSq(ToFoe) > 1.f ? DirectionTo(ToFoe) : GetPlayerAim(Player));
    if (!Slot->Predicted)
    {
        EmitSound(&AppState->Events, AssetType_SfxMeteorCast, Player->Position);
    }
    return true;
}

// NOTE(zoubir): the foe a wind-up was locked on, while it lives and is in
// STORM_HOLD_RANGE; 0 when it got away or died
internal world_entity *
StormcallerLockedFoe(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world_entity *Result = FindMonsterBySerial(&AppState->World, Slot->Stormcaller.LockSlot,
                                               Slot->Stormcaller.LockSerial);
    if (Result && (Result->Hp <= 0.f ||
                   Length(Result->Position.XY - Player->Position.XY) > STORM_HOLD_RANGE))
    {
        Result = 0;
    }
    return Result;
}

internal void
FinishChainLightning(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world_entity *Foe = StormcallerLockedFoe(AppState, Slot, Player);
    if (!Foe)
    {
        Foe = StormcallerTarget(AppState, Slot, Player, CHAIN_RANGE);
    }
    if (!Foe)
    {
        // NOTE(zoubir): nothing left to strike: the key is ready again
        Slot->RoleCooldowns[0] = 0.f;
        return;
    }
    bool32 Super = IsSupercharged(Slot);
    bool32 Conductor = RoleRank(Slot, PlayerRole_Stormcaller, StormcallerTalent_Conductor) > 0;
    u32 Jumps = CHAIN_JUMPS + (Conductor ? CONDUCTOR_JUMPS : 0) + (Super ? SUPERCHARGED_JUMPS : 0);
    float Share = Conductor ? CONDUCTOR_JUMP_SHARE : CHAIN_JUMP_SHARE;
    float Damage = CHAIN_DAMAGE * (Super ? 1.f + SUPERCHARGED_SHARE : 1.f);
    u32 Struck = StormcallerChain(AppState, Slot, Player, Foe, Damage, Jumps, Share, CHAIN_SHOVE,
                                  StormcallerBolt_Chain);
    EmitSound(&AppState->Events, AssetType_SfxKunai, Player->Position);
    EmitSound(&AppState->Events, AssetType_SfxBlink, Foe->Position);
    AddStormCharge(AppState, Slot, CHAIN_CHARGE * (float)Struck);
}

// NOTE(zoubir): Thunderclap pressed short of Charge: the HUD shakes and the
// caster fizzles, once in a while however long the key is held
internal void
RefuseThunderclap(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    if (Slot->Predicted || Slot->Stormcaller.EmptySeconds > 0.f)
    {
        return;
    }
    Slot->Stormcaller.EmptySeconds = THUNDERCLAP_EMPTY_SECONDS;
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_StormcallerFirst, StormcallerBurst_Empty),
              (u8)Player->PlayerIndex, ChestOf(Player));
    SetStormcallerFlags(AppState, Slot);
}

// NOTE(zoubir): a small bolt from the sky on Foe for Damage (Stormbringer,
// Eye of the Storm)
internal void
StormcallerSkyBolt(app_state *AppState, u32 By, world_entity *Foe, float Damage)
{
    StormcallerZap(AppState, By, Foe, Damage, 0.f, V2(1.f, 0.f));
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_StormcallerFirst, StormcallerBurst_SkyBolt), (u8)By,
              V3(Foe->Position.X, Foe->Position.Y, Foe->GroundZ));
}

// NOTE(zoubir): the sky answers: all the Charge spent on the locked foe;
// from STORM_SUPERCHARGED it stuns and splashes, and Capacitor and
// Stormbringer pay off
internal void
FinishThunderclap(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    stormcaller_slot *Storm = &Slot->Stormcaller;
    world_entity *Foe = StormcallerLockedFoe(AppState, Slot, Player);
    if (!Foe || Storm->Charge <= 0.f)
    {
        // NOTE(zoubir): the foe got away or died in the wind-up: the Charge
        // stays and the key is ready again
        Slot->RoleCooldowns[4] = 0.f;
        return;
    }
    u32 By = (u32)(Slot - AppState->Players);
    u32 Room = StormcallerRoom(AppState, Player);
    float Spent = Storm->Charge;
    bool32 Big = Spent >= STORM_SUPERCHARGED;
    float Damage = THUNDERCLAP_DAMAGE + THUNDERCLAP_PER_CHARGE * Spent;
    Storm->Charge = 0.f;
    v2 Away = NormalizeOr(Foe->Position.XY - Player->Position.XY, GetPlayerAim(Player));
    StormcallerZap(AppState, By, Foe, Damage, THUNDERCLAP_SHOVE, Away, StatusEffect_None, 0.f,
                   Big ? THUNDERCLAP_STUN : 0.f);
    if (Big && Foe->IsPresent && Foe->Hp > 0.f)
    {
        Foe->ThrownBySlot = By + 1;
    }
    v3 Ground = V3(Foe->Position.X, Foe->Position.Y, Foe->GroundZ);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_StormcallerFirst, StormcallerBurst_Thunderclap),
              (u8)By, StormcallerBurstSpot(Ground, Big ? 1 : 0));
    EmitSound(&AppState->Events, AssetType_SfxExplosion, Foe->Position);
    if (Big)
    {
        for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
        {
            world_entity *Other = &World->Entities[EntityIndex];
            v2 Offset = Other->Position.XY - Ground.XY;
            if (Other != Foe && IsStormcallerFoe(World, Other, Room) &&
                Length(Offset) <= THUNDERCLAP_SPLASH_RADIUS + 0.5f * Other->Dimensions.X)
            {
                StormcallerZap(AppState, By, Other, THUNDERCLAP_SPLASH_SHARE * Damage, THUNDERCLAP_SHOVE,
                               NormalizeOr(Offset, Away));
            }
        }
        if (RoleRank(Slot, PlayerRole_Stormcaller, StormcallerTalent_Stormbringer))
        {
            world_entity *Skip[STORMBRINGER_BOLTS + 1] = {Foe};
            u32 SkipCount = 1;
            for(u32 Bolt = 0; Bolt < STORMBRINGER_BOLTS; Bolt++)
            {
                world_entity *Next = NearestFoe(World, Ground.XY, STORMBRINGER_RADIUS, Room, Skip, SkipCount);
                if (!Next)
                {
                    break;
                }
                Skip[SkipCount++] = Next;
                StormcallerSkyBolt(AppState, By, Next, STORMBRINGER_SHARE * Damage);
            }
        }
        if (RoleRank(Slot, PlayerRole_Stormcaller, StormcallerTalent_Capacitor))
        {
            Slot->RoleCooldowns[0] = 0.f;
            AddStormCharge(AppState, Slot, CAPACITOR_CHARGE);
        }
    }
}
