/* Stormcaller storm (role_kits/stormcaller.cpp): what is not a bolt.
   Static Field, a circle on the ground that shocks and slows what is
   inside each STATIC_FIELD_TICK and makes lightning landing in it arc
   across it (stormcaller/bolts.cpp asks StormcallerFieldHolds); Lightning
   Dash; Eye of the Storm and its bolts; and each Stormcaller's tick.
   Clients draw a field from one burst for its life, and the cloud of the
   Eye from its ClassFlags bit. */

// NOTE(zoubir): Static Field at the cursor; false when every circle is in use
internal bool32
CastStaticField(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    if (Slot->Predicted)
    {
        return true;
    }
    stormcaller_run *Run = &AppState->Dungeon->Stormcaller;
    for(u32 Index = 0; Index < STORMCALLER_MAX_FIELDS; Index++)
    {
        stormcaller_field *Field = &Run->Fields[Index];
        if (Field->Seconds <= 0.f)
        {
            v2 Point = AimPoint(Player);
            float Arc = (float)RoleRank(Slot, PlayerRole_Stormcaller, StormcallerTalent_ArcField);
            Field->Position = V3(Point.X, Point.Y, Player->GroundZ);
            Field->Radius = RoleSpellRadius(Slot, 1);
            Field->Seconds = STATIC_FIELD_SECONDS;
            // NOTE(zoubir): the first shock half a tick in, the last half a
            // tick before it ends, STATIC_FIELD_SECONDS / STATIC_FIELD_TICK in all
            Field->TickTimer = 0.5f * STATIC_FIELD_TICK;
            Field->TickDamage = STATIC_FIELD_TICK_DAMAGE * (1.f + ARC_FIELD_DAMAGE_SHARE * Arc);
            Field->By = (u8)Player->PlayerIndex;
            // NOTE(zoubir): the radius rides along, so clients draw the
            // dome the size Arc Field made it
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_StormcallerFirst, StormcallerBurst_Field),
                      Field->By, StormcallerBurstSpot(Field->Position, (u32)(Field->Radius + 0.5f)));
            EmitSound(&AppState->Events, AssetType_SfxAreaCast, Player->Position);
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): whether Field reaches Monster
inline bool32
StormcallerFieldReaches(stormcaller_field *Field, world_entity *Monster)
{
    v2 Offset = Monster->Position.XY - Field->Position.XY;
    bool32 Result = Field->Seconds > 0.f && Monster->IsPresent && Monster->Type == EntityType_Monster &&
        Monster->Hp > 0.f && Length(Offset) <= Field->Radius + 0.3f * Monster->Dimensions.X;
    return Result;
}

// NOTE(zoubir): whether Monster stands in a Static Field of the Stormcaller
// in slot By; *Field the one it was found in, which is asked first
internal bool32
StormcallerFieldHolds(app_state *AppState, u32 By, world_entity *Monster, stormcaller_field **Field)
{
    if (*Field && StormcallerFieldReaches(*Field, Monster))
    {
        return true;
    }
    stormcaller_run *Run = &AppState->Dungeon->Stormcaller;
    for(u32 Index = 0; Index < STORMCALLER_MAX_FIELDS; Index++)
    {
        stormcaller_field *Each = &Run->Fields[Index];
        if (Each->By == By && StormcallerFieldReaches(Each, Monster))
        {
            *Field = Each;
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): once a tick: each field shocks what is inside on its
// ticks, and goes when
// its time does or its Stormcaller left the class
internal void
UpdateStaticFields(app_state *AppState, stormcaller_run *Run, float DeltaTime)
{
    world *World = &AppState->World;
    for(u32 Index = 0; Index < STORMCALLER_MAX_FIELDS; Index++)
    {
        stormcaller_field *Field = &Run->Fields[Index];
        if (Field->Seconds <= 0.f)
        {
            continue;
        }
        player_slot *Slot = &AppState->Players[Field->By];
        if (!Slot->Entity || Slot->Role != PlayerRole_Stormcaller)
        {
            Field->Seconds = 0.f;
            continue;
        }
        Field->Seconds = Maximum(0.f, Field->Seconds - DeltaTime);
        Field->TickTimer += DeltaTime;
        if (Field->TickTimer < STATIC_FIELD_TICK)
        {
            continue;
        }
        Field->TickTimer -= STATIC_FIELD_TICK;
        for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
        {
            world_entity *Monster = &World->Entities[EntityIndex];
            if (StormcallerFieldReaches(Field, Monster))
            {
                v2 Offset = Monster->Position.XY - Field->Position.XY;
                StormcallerZap(AppState, Field->By, Monster, Field->TickDamage, 0.f,
                               NormalizeOr(Offset, V2(1.f, 0.f)), StatusEffect_Slowed,
                               STATIC_FIELD_SLOW_SECONDS);
            }
        }
    }
}

// NOTE(zoubir): where a Lightning Dash from Player along Dir stops: its
// full length, or the last clear step before a wall, a pit, lava or a
// closed gate
internal v2
LightningDashEnd(app_state *AppState, world_entity *Player, v2 Dir)
{
    v2 Result = ClearDashEnd(AppState, Player, Dir, LIGHTNING_DASH_LENGTH, LIGHTNING_DASH_STEP);
    return Result;
}

// NOTE(zoubir): Lightning Dash: the Stormcaller goes along the aim as a
// bolt, and every foe it passes is shocked and slowed
internal void
CastLightningDash(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
                  world_entity *Player)
{
    if (Slot->Predicted)
    {
        return;
    }
    v2 Dir = GetPlayerAim(Player);
    v2 From = Player->Position.XY;
    v2 To = LightningDashEnd(AppState, Player, Dir);
    float Length2 = Length(To - From);
    u32 By = (u32)Player->PlayerIndex;
    u32 Room = StormcallerRoom(AppState, Player);
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (!IsStormcallerFoe(World, Monster, Room))
        {
            continue;
        }
        v2 Offset = Monster->Position.XY - From;
        float Along = DotProduct(Offset, Dir);
        float Across = Absolute(DotProduct(Offset, V2(-Dir.Y, Dir.X)));
        if (Along > -LIGHTNING_DASH_WIDTH && Along < Length2 + LIGHTNING_DASH_WIDTH &&
            Across < LIGHTNING_DASH_WIDTH + 0.5f * Monster->Dimensions.X)
        {
            v2 Side = DotProduct(Offset, V2(-Dir.Y, Dir.X)) >= 0.f ? V2(-Dir.Y, Dir.X) : V2(Dir.Y, -Dir.X);
            StormcallerZap(AppState, By, Monster, LIGHTNING_DASH_DAMAGE, 0.f, Side, StatusEffect_Slowed,
                           LIGHTNING_DASH_SLOW_SECONDS);
        }
    }
    v3 Feet = V3(From.X, From.Y, Player->GroundZ);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_StormcallerFirst, StormcallerBurst_Dash), (u8)By,
              StormcallerBurstSpot(Feet, (u32)(Length2 + 0.5f)), ATan2(Dir.Y, Dir.X));
    EmitSound(&AppState->Events, AssetType_SfxDash, Player->Position);
    if (Length2 > 1.f)
    {
        v3 Landing = V3(To.X, To.Y, Player->Position.Z);
        MovePlayerTo(AppState, World, Arena, Player, Landing);
    }
    // NOTE(zoubir): as lightning it slips what was coming for a moment
    Player->DashFlash = Maximum(Player->DashFlash, 0.2f);
    AddStormCharge(AppState, Slot, LIGHTNING_DASH_CHARGE);
}

// NOTE(zoubir): Eye of the Storm: the cloud gathers over the Stormcaller
internal void
CastEyeOfTheStorm(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    if (Slot->Predicted)
    {
        return;
    }
    Slot->Stormcaller.EyeSeconds = EYE_SECONDS;
    Slot->Stormcaller.EyeTimer = EYE_BOLT_SECONDS;
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_StormcallerFirst, StormcallerBurst_Supercharged),
              (u8)Player->PlayerIndex, ChestOf(Player));
    EmitSound(&AppState->Events, AssetType_SfxCombustion, Player->Position);
}

// NOTE(zoubir): an Eye of the Storm bolt on a random living foe of the
// Stormcaller's room, none when there is none
internal void
StrikeFromTheEye(app_state *AppState, dungeon_run *Run, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    u32 Room = StormcallerRoom(AppState, Player);
    world_entity *Foes[64];
    u32 Count = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount && Count < ArrayCount(Foes); EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (IsStormcallerFoe(World, Monster, Room))
        {
            Foes[Count++] = Monster;
        }
    }
    if (Count)
    {
        world_entity *Foe = Foes[RandomChoice(&Run->Series, Count)];
        StormcallerSkyBolt(AppState, (u32)(Slot - AppState->Players), Foe, EYE_BOLT_DAMAGE);
    }
}

// NOTE(zoubir): once a tick for each Stormcaller: Charge, the Eye's bolts,
// and what clients see of it
internal void
UpdateStormcallerSlot(app_state *AppState, dungeon_run *Run, player_slot *Slot, float DeltaTime)
{
    stormcaller_slot *Storm = &Slot->Stormcaller;
    world_entity *Player = Slot->Entity;
    bool32 Alive = Player && Player->IsPresent && !IsDeadPlayer(Player);
    if (Storm->EyeSeconds > 0.f)
    {
        Storm->EyeSeconds = Alive ? Maximum(0.f, Storm->EyeSeconds - DeltaTime) : 0.f;
        Storm->EyeTimer -= DeltaTime;
        if (Storm->EyeTimer <= 0.f && Storm->EyeSeconds > 0.f)
        {
            Storm->EyeTimer += EYE_BOLT_SECONDS;
            StrikeFromTheEye(AppState, Run, Slot, Player);
        }
    }
    // NOTE(zoubir): grounded, no key comes back before the smoke clears,
    // the one whose cast overloaded included (its cooldown was set after)
    for(u32 Key = 0; Key < ROLE_KEYS && Storm->GroundedSeconds > 0.f; Key++)
    {
        Slot->RoleCooldowns[Key] = Maximum(Slot->RoleCooldowns[Key], Storm->GroundedSeconds);
    }
    UpdateStormCharge(Slot, DeltaTime);
    SetStormcallerFlags(AppState, Slot);
}
