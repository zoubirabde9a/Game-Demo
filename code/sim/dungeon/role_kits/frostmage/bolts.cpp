/* Frost Mage bolts (role_kits/frostmage.cpp): Icicles, and the Frostbolts
   and Glacial Spikes in flight. A bolt's hit lands when the bolt gets
   there (FROSTBOLT_SPEED, GLACIAL_SPIKE_SPEED), so the damage number shows
   as the bolt the clients fly strikes. A foe is picked as the Ranger's
   shots pick one (RangerTarget, ranger/shots.cpp), in the same room. */

// NOTE(zoubir): Count more Icicles, up to five; coming full flashes the
// mage
internal void
AddIcicles(app_state *AppState, player_slot *Slot, u32 Count)
{
    frostmage_slot *Mage = &Slot->FrostMage;
    bool32 WasFull = Mage->Icicles >= FROSTMAGE_ICICLES_MOST;
    Mage->Icicles = Minimum((u32)FROSTMAGE_ICICLES_MOST, Mage->Icicles + Count);
    Mage->IcicleHold = FROSTMAGE_ICICLE_HOLD;
    if (!WasFull && Mage->Icicles >= FROSTMAGE_ICICLES_MOST && Slot->Entity)
    {
        EmitBurst(&AppState->Events, ClassBurst(SimBurst_FrostMageFirst, FrostBurst_Full),
                  (u8)Slot->Entity->PlayerIndex, ChestOf(Slot->Entity));
    }
}

// NOTE(zoubir): whether Monster is frozen: rooted or stunned, so the
// Frost Mage's hits on it shatter
inline bool32
IsFrozenFoe(world_entity *Monster)
{
    bool32 Result = Monster && (HasStatus(Monster, StatusEffect_Rooted) ||
                                HasStatus(Monster, StatusEffect_Stunned));
    return Result;
}

// NOTE(zoubir): Monster frozen for Seconds (stunned), and clients shown
// the ice on it
internal void
FreezeFoe(app_state *AppState, u32 By, world_entity *Monster, float Seconds)
{
    ApplyStatus(Monster, StatusEffect_Stunned, Seconds);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_FrostMageFirst, FrostBurst_Freeze), (u8)By,
              RangerBurstSpot(Monster->Position, (u32)(2.f * Seconds + 0.5f)));
}

// NOTE(zoubir): a hit of Shot on Monster for the Frost Mage in slot By,
// which OnFrostMageHit and FrostMageDealtScale know by Hitting
internal void
FrostMageHit(app_state *AppState, u32 By, world_entity *Monster, u32 Shot, float Damage,
             float Shove, v2 Away, status_effect Status = StatusEffect_None,
             float StatusSeconds = 0.f, bool32 Shatters = false)
{
    player_slot *Slot = &AppState->Players[By];
    world_entity *Player = Slot->Entity;
    if (!Player || !Monster->IsPresent || Monster->Hp <= 0.f)
    {
        return;
    }
    hit Hit = {Damage, Shove, 0.f, 0.f, 0.f, SimBurst_Count, Status, StatusSeconds};
    Slot->FrostMage.Hitting = Shot;
    Slot->FrostMage.Shattering = Shatters;
    ApplyHit(AppState, &AppState->World, Monster, &Hit, Away, Player, By);
    Slot->FrostMage.Hitting = FrostShot_None;
    Slot->FrostMage.Shattering = false;
}

// NOTE(zoubir): a bolt of Shot from player By at Foe, landing Delay from
// now; dropped when every bolt is in flight
internal void
LooseFrostBolt(app_state *AppState, u32 By, world_entity *Foe, u32 Shot, float Damage,
               float Delay, v2 Away, bool32 Shatters = false, u32 Icicles = 0)
{
    frostmage_run *Run = &AppState->Dungeon->FrostMage;
    for(u32 Index = 0; Index < FROSTMAGE_MAX_BOLTS; Index++)
    {
        frostmage_bolt *Bolt = &Run->Bolts[Index];
        if (Bolt->Shot == FrostShot_None)
        {
            Bolt->TargetSlot = (u32)(Foe - AppState->World.Entities);
            Bolt->TargetSerial = Foe->MonsterSerial;
            Bolt->Delay = Delay;
            Bolt->Damage = Damage;
            Bolt->Away = Away;
            Bolt->By = (u8)By;
            Bolt->Shot = (u8)Shot;
            Bolt->Shatters = (u8)(Shatters ? 1 : 0);
            Bolt->Icicles = (u8)Icicles;
            return;
        }
    }
}

// NOTE(zoubir): Frostbolt (X) at Foe, or along the aim at nothing; with
// Fingers of Frost every fourth one shatters
internal void
CastFrostbolt(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    frostmage_slot *Mage = &Slot->FrostMage;
    u8 By = (u8)Player->PlayerIndex;
    world_entity *Foe = RangerTarget(AppState, Slot, Player, FROSTBOLT_RANGE);
    v3 Spot;
    v2 Dir;
    u32 Variant = FrostBolt_Plain;
    if (Foe)
    {
        Mage->Bolts++;
        bool32 Fingers = RoleRank(Slot, PlayerRole_FrostMage, FrostMageTalent_FingersOfFrost) > 0 &&
            (Mage->Bolts % FINGERS_OF_FROST_EVERY) == 0;
        v2 Offset = Foe->Position.XY - Player->Position.XY;
        Dir = NormalizeOr(Offset, GetPlayerAim(Player));
        Spot = ChestOf(Foe);
        LooseFrostBolt(AppState, By, Foe, FrostShot_Bolt, FROSTBOLT_DAMAGE,
                       Length(Offset) / FROSTBOLT_SPEED, Dir, Fingers);
        Variant = Fingers ? FrostBolt_Fingers : FrostBolt_Plain;
    }
    else
    {
        Dir = GetPlayerAim(Player);
        Spot = ChestOf(Player);
        Spot.XY += 0.6f * FROSTBOLT_RANGE * Dir;
        Variant = FrostBolt_Miss;
    }
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_FrostMageFirst, FrostBurst_Bolt), By,
              RangerBurstSpot(Spot, Variant), ATan2(Dir.Y, Dir.X));
    EmitSound(&AppState->Events, AssetType_SfxKunai, Player->Position);
}

// NOTE(zoubir): a spike from From at Foe; Icicles rides along for the
// look and for the freeze on arrival
internal void
ThrowGlacialSpike(app_state *AppState, u32 By, v2 From, world_entity *Foe, u32 Shot,
                  float Damage, u32 Icicles)
{
    v2 Offset = Foe->Position.XY - From;
    v2 Dir = NormalizeOr(Offset, V2(1.f, 0.f));
    LooseFrostBolt(AppState, By, Foe, Shot, Damage, Length(Offset) / GLACIAL_SPIKE_SPEED, Dir,
                   false, Icicles);
    u32 Variant = Icicles + (Shot == FrostShot_SplitSpike ? 8 : 0);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_FrostMageFirst, FrostBurst_Spike), (u8)By,
              RangerBurstSpot(ChestOf(Foe), Variant), ATan2(Dir.Y, Dir.X));
}

// NOTE(zoubir): Glacial Spike, at the end of its cast: every Icicle spent
// on one spike at the foe aimed at; none there, nothing is spent
internal void
LooseGlacialSpike(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world_entity *Foe = RangerTarget(AppState, Slot, Player, GLACIAL_SPIKE_RANGE);
    if (!Foe)
    {
        return;
    }
    frostmage_slot *Mage = &Slot->FrostMage;
    u32 Icicles = Mage->Icicles;
    Mage->Icicles = 0;
    float Damage = GLACIAL_SPIKE_DAMAGE + GLACIAL_SPIKE_PER_ICICLE * (float)Icicles;
    ThrowGlacialSpike(AppState, Player->PlayerIndex, Player->Position.XY, Foe, FrostShot_Spike,
                      Damage, Icicles);
    EmitSound(&AppState->Events, AssetType_SfxKunai, Player->Position);
    if (Icicles >= FROSTMAGE_ICICLES_MOST)
    {
        EmitSound(&AppState->Events, AssetType_SfxGiantFireball, Player->Position);
    }
}

// NOTE(zoubir): a Glacial Spike reached Foe: five Icicles freeze it, and
// with Absolute Zero every foe near it; Splitting Ice throws half on to
// the nearest other foe
internal void
GlacialSpikeArrives(app_state *AppState, frostmage_bolt *Bolt, world_entity *Foe)
{
    player_slot *Slot = &AppState->Players[Bolt->By];
    world *World = &AppState->World;
    bool32 Full = Bolt->Icicles >= FROSTMAGE_ICICLES_MOST;
    if (Full && Foe->Hp > 0.f)
    {
        FreezeFoe(AppState, Bolt->By, Foe, GLACIAL_SPIKE_FREEZE);
    }
    if (Full && RoleRank(Slot, PlayerRole_FrostMage, FrostMageTalent_AbsoluteZero) && Slot->Entity)
    {
        u32 Room = RoomAtPosition(World, Foe->Position.XY);
        for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
        {
            world_entity *Monster = &World->Entities[EntityIndex];
            if (Monster != Foe && Monster->IsPresent && Monster->Type == EntityType_Monster &&
                Monster->Hp > 0.f && RoomAtPosition(World, Monster->Position.XY) == Room &&
                Length(Monster->Position.XY - Foe->Position.XY) <= ABSOLUTE_ZERO_RADIUS)
            {
                FreezeFoe(AppState, Bolt->By, Monster, GLACIAL_SPIKE_FREEZE);
            }
        }
    }
    if (Bolt->Shot == FrostShot_Spike &&
        RoleRank(Slot, PlayerRole_FrostMage, FrostMageTalent_SplittingIce))
    {
        world_entity *Skip[1] = {Foe};
        world_entity *Next = NearestFoe(World, Foe->Position.XY, SPLITTING_ICE_REACH,
                                        RoomAtPosition(World, Foe->Position.XY), Skip, 1);
        if (Next)
        {
            ThrowGlacialSpike(AppState, Bolt->By, Foe->Position.XY, Next, FrostShot_SplitSpike,
                              SPLITTING_ICE_SHARE * Bolt->Damage, 0);
        }
    }
}

// NOTE(zoubir): once a tick: bolts and spikes reach their foes
internal void
UpdateFrostBolts(app_state *AppState, frostmage_run *Run, float DeltaTime)
{
    for(u32 Index = 0; Index < FROSTMAGE_MAX_BOLTS; Index++)
    {
        frostmage_bolt *Bolt = &Run->Bolts[Index];
        if (Bolt->Shot == FrostShot_None)
        {
            continue;
        }
        Bolt->Delay -= DeltaTime;
        if (Bolt->Delay > 0.f)
        {
            continue;
        }
        world_entity *Foe = FindMonsterBySerial(&AppState->World, Bolt->TargetSlot,
                                                Bolt->TargetSerial);
        frostmage_bolt Landed = *Bolt;
        Bolt->Shot = FrostShot_None;
        if (!Foe || Foe->Hp <= 0.f)
        {
            continue;
        }
        if (Landed.Shot == FrostShot_Bolt)
        {
            float Chill = FROSTBOLT_CHILL_SECONDS +
                (RoleRank(&AppState->Players[Landed.By], PlayerRole_FrostMage,
                          FrostMageTalent_Permafrost) ? PERMAFROST_SECONDS : 0.f);
            FrostMageHit(AppState, Landed.By, Foe, FrostShot_Bolt, Landed.Damage, 30.f, Landed.Away,
                         StatusEffect_Slowed, Chill, Landed.Shatters);
            AddIcicles(AppState, &AppState->Players[Landed.By], Landed.Shatters ? 2 : 1);
        }
        else
        {
            FrostMageHit(AppState, Landed.By, Foe, Landed.Shot, Landed.Damage, 80.f, Landed.Away);
            GlacialSpikeArrives(AppState, &Landed, Foe);
        }
        EmitSound(&AppState->Events, AssetType_SfxHit, Foe->Position);
    }
}
