/* Frost Mage ground (role_kits/frostmage.cpp): what the Frost Mage leaves
   in the world, and its two spells round itself. A Blizzard's circle at
   the cursor, ice falling on everything inside each BLIZZARD_TICK; a
   Frozen Orb rolling along the aim, striking what is near it each
   FROZEN_ORB_TICK; Frost Nova freezing every foe round the mage in place;
   Ice Barrier, which is the Fire Mage's Fireguard shield on the slot
   (FireguardAbsorb, taken in DungeonScaleDamage) under another name.
   Clients draw a circle or an orb from one burst for as long as it lasts,
   as they draw a Ranger's Volley. */

// NOTE(zoubir): Blizzard at the cursor; false when every circle is in use
internal bool32
CastBlizzard(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    frostmage_run *Run = &AppState->Dungeon->FrostMage;
    for(u32 Index = 0; Index < FROSTMAGE_MAX_BLIZZARDS; Index++)
    {
        frostmage_blizzard *Blizzard = &Run->Blizzards[Index];
        if (Blizzard->Seconds <= 0.f)
        {
            v2 Point = AimPoint(Player);
            bool32 Permafrost = RoleRank(Slot, PlayerRole_FrostMage, FrostMageTalent_Permafrost) > 0;
            Blizzard->Position = V3(Point.X, Point.Y, Player->GroundZ);
            Blizzard->Radius = RoleSpellRadius(Slot, 0);
            Blizzard->Seconds = BLIZZARD_SECONDS;
            // NOTE(zoubir): the first ice lands half a tick in, as the
            // clients' first flakes come down
            Blizzard->TickTimer = 0.5f * BLIZZARD_TICK;
            Blizzard->Chill = BLIZZARD_CHILL_SECONDS + (Permafrost ? PERMAFROST_SECONDS : 0.f);
            Blizzard->By = (u8)Player->PlayerIndex;
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_FrostMageFirst, FrostBurst_Blizzard),
                      Blizzard->By, Blizzard->Position);
            EmitSound(&AppState->Events, AssetType_SfxAreaCast, Player->Position);
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): every live foe within Radius of Point (and a bit of its
// body), struck by Shot for Damage and given Status
internal u32
StrikeFrostCircle(app_state *AppState, u32 By, v2 Point, float Radius, u32 Shot, float Damage,
                  status_effect Status, float StatusSeconds)
{
    world *World = &AppState->World;
    u32 Struck = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        v2 Offset = Monster->Position.XY - Point;
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            Length(Offset) > Radius + 0.3f * Monster->Dimensions.X)
        {
            continue;
        }
        FrostMageHit(AppState, By, Monster, Shot, Damage, 0.f, NormalizeOr(Offset, V2(1.f, 0.f)),
                     Status, StatusSeconds);
        Struck++;
    }
    return Struck;
}

internal void
UpdateBlizzards(app_state *AppState, frostmage_run *Run, float DeltaTime)
{
    for(u32 Index = 0; Index < FROSTMAGE_MAX_BLIZZARDS; Index++)
    {
        frostmage_blizzard *Blizzard = &Run->Blizzards[Index];
        if (Blizzard->Seconds <= 0.f)
        {
            continue;
        }
        Blizzard->Seconds = Maximum(0.f, Blizzard->Seconds - DeltaTime);
        Blizzard->TickTimer += DeltaTime;
        if (Blizzard->TickTimer >= BLIZZARD_TICK)
        {
            Blizzard->TickTimer -= BLIZZARD_TICK;
            StrikeFrostCircle(AppState, Blizzard->By, Blizzard->Position.XY, Blizzard->Radius,
                              FrostShot_Blizzard, BLIZZARD_TICK_DAMAGE, StatusEffect_Slowed,
                              Blizzard->Chill);
        }
    }
}

// NOTE(zoubir): Frost Nova: every foe round the mage takes a little and is
// frozen in place (rooted: it can still strike what stands beside it);
// Deep Freeze holds it longer
internal void
CastFrostNova(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    u32 By = Player->PlayerIndex;
    u32 Ranks = RoleRank(Slot, PlayerRole_FrostMage, FrostMageTalent_DeepFreeze);
    float Hold = FROST_NOVA_ROOT + DEEP_FREEZE_SECONDS * (float)Ranks;
    u32 Room = RangerShotRoom(AppState, Player);
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        v2 Offset = Monster->Position.XY - Player->Position.XY;
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            RoomAtPosition(World, Monster->Position.XY) != Room ||
            Length(Offset) > FROST_NOVA_RADIUS + 0.3f * Monster->Dimensions.X)
        {
            continue;
        }
        FrostMageHit(AppState, By, Monster, FrostShot_Nova, FROST_NOVA_DAMAGE, 0.f,
                     NormalizeOr(Offset, V2(1.f, 0.f)), StatusEffect_Rooted, Hold);
    }
    // NOTE(zoubir): Deep Freeze's ranks ride along, so clients draw the
    // ice as long as it holds
    v3 Feet = V3(Player->Position.X, Player->Position.Y, Player->GroundZ);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_FrostMageFirst, FrostBurst_Nova), (u8)By,
              RangerBurstSpot(Feet, Ranks));
    EmitSound(&AppState->Events, AssetType_SfxAreaCast, Player->Position);
}

// NOTE(zoubir): Ice Barrier: the slot's fire shield (FireguardTakes,
// role_kits/striker.cpp) as ice; a fresh one replaces what is left
internal void
CastIceBarrier(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    u32 Ranks = RoleRank(Slot, PlayerRole_FrostMage, FrostMageTalent_IceBarrier);
    Slot->FireguardAbsorb = Ranks >= 2 ? ICE_BARRIER_ABSORB_2 : ICE_BARRIER_ABSORB;
    Slot->FireguardSeconds = ICE_BARRIER_SECONDS;
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_FrostMageFirst, FrostBurst_Barrier),
              (u8)Player->PlayerIndex, ChestOf(Player));
    EmitSound(&AppState->Events, AssetType_SfxWard, Player->Position);
}

// NOTE(zoubir): Frozen Orb along the aim; false when every orb is in use
internal bool32
CastFrozenOrb(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    frostmage_run *Run = &AppState->Dungeon->FrostMage;
    for(u32 Index = 0; Index < FROSTMAGE_MAX_ORBS; Index++)
    {
        frostmage_orb *Orb = &Run->Orbs[Index];
        if (Orb->Seconds <= 0.f)
        {
            v2 Dir = GetPlayerAim(Player);
            Orb->Position = ChestOf(Player);
            Orb->Position.XY += 20.f * Dir;
            Orb->Direction = Dir;
            Orb->Seconds = FROZEN_ORB_SECONDS;
            Orb->TickTimer = 0.f;
            Orb->By = (u8)Player->PlayerIndex;
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_FrostMageFirst, FrostBurst_Orb), Orb->By,
                      Orb->Position, ATan2(Dir.Y, Dir.X));
            EmitSound(&AppState->Events, AssetType_SfxAreaCast, Player->Position);
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): an orb rolls on through foes (it is never stopped), and
// each tick that strikes something grows its mage an Icicle
internal void
UpdateFrozenOrbs(app_state *AppState, frostmage_run *Run, float DeltaTime)
{
    for(u32 Index = 0; Index < FROSTMAGE_MAX_ORBS; Index++)
    {
        frostmage_orb *Orb = &Run->Orbs[Index];
        if (Orb->Seconds <= 0.f)
        {
            continue;
        }
        Orb->Seconds = Maximum(0.f, Orb->Seconds - DeltaTime);
        Orb->Position.XY += FROZEN_ORB_SPEED * DeltaTime * Orb->Direction;
        Orb->TickTimer += DeltaTime;
        if (Orb->TickTimer >= FROZEN_ORB_TICK)
        {
            Orb->TickTimer -= FROZEN_ORB_TICK;
            float Chill = FROSTBOLT_CHILL_SECONDS;
            u32 Struck = StrikeFrostCircle(AppState, Orb->By, Orb->Position.XY, FROZEN_ORB_RADIUS,
                                           FrostShot_Orb, FROZEN_ORB_DAMAGE, StatusEffect_Slowed, Chill);
            player_slot *Slot = &AppState->Players[Orb->By];
            if (Struck && Slot->Role == PlayerRole_FrostMage)
            {
                AddIcicles(AppState, Slot, 1);
            }
        }
    }
}

// NOTE(zoubir): whether a Frozen Orb of player By still rolls
inline bool32
FrozenOrbRolls(frostmage_run *Run, u32 By)
{
    for(u32 Index = 0; Index < FROSTMAGE_MAX_ORBS; Index++)
    {
        if (Run->Orbs[Index].Seconds > 0.f && Run->Orbs[Index].By == By)
        {
            return true;
        }
    }
    return false;
}
