/* Ranger ground (role_kits/ranger.cpp): what the Ranger leaves on the
   ground. A Volley's circle, where arrows rain VOLLEY_DRAW after the
   press and strike everything inside each VOLLEY_TICK, slowing it; and
   Disengage's leap back, which leaves a snare trap where the Ranger
   stood that roots the first foe to step on it. A Ranger has one trap
   down at a time: a new one takes the old one's place. Clients draw a
   circle from one burst as long as it lasts, and a trap from a burst sent
   again every RANGER_KEEP_SECONDS (as Hunter's Mark is, ranger/shots.cpp). */

// NOTE(zoubir): Volley at the cursor; false when every circle is in use
internal bool32
CastVolley(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    ranger_run *Run = &AppState->Dungeon->Ranger;
    for(u32 Index = 0; Index < RANGER_MAX_VOLLEYS; Index++)
    {
        ranger_volley *Volley = &Run->Volleys[Index];
        if (Volley->Delay <= 0.f && Volley->Seconds <= 0.f)
        {
            v2 Point = AimPoint(Player);
            bool32 Barrage = RoleRank(Slot, PlayerRole_Ranger, RangerTalent_Barrage) > 0;
            Volley->Position = V3(Point.X, Point.Y, Player->GroundZ);
            Volley->Radius = RoleSpellRadius(Slot, 0);
            Volley->Delay = VOLLEY_DRAW;
            Volley->Seconds = VOLLEY_SECONDS + (Barrage ? BARRAGE_SECONDS : 0.f);
            Volley->TickTimer = VOLLEY_TICK;
            Volley->By = (u8)Player->PlayerIndex;
            // NOTE(zoubir): Barrage rides along (RangerBurstSpot), so
            // clients draw the circle the size it is, and as long
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_RangerFirst, RangerBurst_Volley),
                      Volley->By, RangerBurstSpot(Volley->Position, Barrage ? 1 : 0));
            EmitSound(&AppState->Events, AssetType_SfxAreaCast, Player->Position);
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): one volley's arrows striking everything inside
internal void
StrikeVolley(app_state *AppState, ranger_volley *Volley)
{
    world *World = &AppState->World;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        v2 Offset = Monster->Position.XY - Volley->Position.XY;
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            Length(Offset) > Volley->Radius + 0.3f * Monster->Dimensions.X)
        {
            continue;
        }
        RangerHit(AppState, Volley->By, Monster, RangerShot_Volley, VOLLEY_TICK_DAMAGE, 0.f,
                  NormalizeOr(Offset, V2(1.f, 0.f)), StatusEffect_Slowed, VOLLEY_SLOW_SECONDS);
    }
}

internal void
UpdateRangerVolleys(app_state *AppState, ranger_run *Run, float DeltaTime)
{
    for(u32 Index = 0; Index < RANGER_MAX_VOLLEYS; Index++)
    {
        ranger_volley *Volley = &Run->Volleys[Index];
        if (Volley->Delay > 0.f)
        {
            Volley->Delay -= DeltaTime;
            if (Volley->Delay > 0.f)
            {
                continue;
            }
            Volley->Delay = 0.f;
            // NOTE(zoubir): the first arrows land half a tick into the
            // rain, as the clients' first ones come down, and the last
            // half a tick before it ends
            Volley->TickTimer = 0.5f * VOLLEY_TICK;
        }
        if (Volley->Seconds <= 0.f)
        {
            continue;
        }
        Volley->Seconds = Maximum(0.f, Volley->Seconds - DeltaTime);
        Volley->TickTimer += DeltaTime;
        if (Volley->TickTimer >= VOLLEY_TICK)
        {
            Volley->TickTimer -= VOLLEY_TICK;
            StrikeVolley(AppState, Volley);
        }
    }
}

// NOTE(zoubir): Disengage: the Ranger leaps back from its aim and a snare
// goes down where it stood, taking the place of its last one
internal void
CastDisengage(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    ranger_run *Run = &AppState->Dungeon->Ranger;
    u8 By = (u8)Player->PlayerIndex;
    ranger_trap *Free = 0;
    for(u32 Index = 0; Index < RANGER_MAX_TRAPS; Index++)
    {
        ranger_trap *Trap = &Run->Traps[Index];
        if (Trap->Seconds > 0.f && Trap->By == By)
        {
            Trap->Seconds = 0.f;
        }
        if (!Free && Trap->Seconds <= 0.f)
        {
            Free = Trap;
        }
    }
    v3 Feet = V3(Player->Position.X, Player->Position.Y, Player->GroundZ);
    if (Free)
    {
        Free->Position = Feet;
        Free->Seconds = TRAP_SECONDS;
        Free->Keep = RANGER_KEEP_SECONDS;
        Free->By = By;
        EmitBurst(&AppState->Events, ClassBurst(SimBurst_RangerFirst, RangerBurst_Trap), By, Feet);
    }
    v2 Back = -1.f * GetPlayerAim(Player);
    Player->Velocity.XY = DISENGAGE_SPEED * Back;
    Player->Velocity.Z = Maximum(Player->Velocity.Z, DISENGAGE_LIFT);
    // NOTE(zoubir): as a long jump, the air drag cut till it lands, so the
    // leap carries (sim/player_update/movement.cpp)
    Player->LongJump = true;
    // NOTE(zoubir): the leap is a dodge for a moment, and its streak
    Player->DashFlash = Maximum(Player->DashFlash, 0.2f);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_RangerFirst, RangerBurst_Leap), By, Feet,
              ATan2(Back.Y, Back.X));
    EmitSound(&AppState->Events, AssetType_SfxDash, Player->Position);
}

// NOTE(zoubir): once a tick: a snare springs on the first foe within
// reach of it, rooting it (and with Disengage's second rank, biting)
internal void
UpdateRangerTraps(app_state *AppState, ranger_run *Run, float DeltaTime)
{
    world *World = &AppState->World;
    for(u32 Index = 0; Index < RANGER_MAX_TRAPS; Index++)
    {
        ranger_trap *Trap = &Run->Traps[Index];
        if (Trap->Seconds <= 0.f)
        {
            continue;
        }
        Trap->Seconds = Maximum(0.f, Trap->Seconds - DeltaTime);
        player_slot *Slot = &AppState->Players[Trap->By];
        if (!Slot->Entity || Slot->Role != PlayerRole_Ranger)
        {
            Trap->Seconds = 0.f;
            continue;
        }
        Trap->Keep -= DeltaTime;
        if (Trap->Keep <= 0.f && Trap->Seconds > 0.f)
        {
            Trap->Keep += RANGER_KEEP_SECONDS;
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_RangerFirst, RangerBurst_Trap), Trap->By,
                      RangerBurstSpot(Trap->Position, 1));
        }
        for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
        {
            world_entity *Monster = &World->Entities[EntityIndex];
            v2 Offset = Monster->Position.XY - Trap->Position.XY;
            if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
                Length(Offset) > TRAP_RADIUS + 0.4f * Monster->Dimensions.X)
            {
                continue;
            }
            bool32 Snare = RoleRank(Slot, PlayerRole_Ranger, RangerTalent_Disengage) >= 2;
            float Root = TRAP_ROOT_SECONDS + (Snare ? SNARE_ROOT_SECONDS : 0.f);
            Monster->Velocity.XY = V2(0.f, 0.f);
            RangerHit(AppState, Trap->By, Monster, RangerShot_Trap,
                      TRAP_DAMAGE + (Snare ? SNARE_DAMAGE : 0.f), 0.f,
                      NormalizeOr(Offset, V2(1.f, 0.f)), StatusEffect_Rooted, Root);
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_RangerFirst, RangerBurst_TrapSnap),
                      Trap->By, Trap->Position);
            EmitSound(&AppState->Events, AssetType_SfxSword, Trap->Position);
            Trap->Seconds = 0.f;
            break;
        }
    }
}

// NOTE(zoubir): whether the Ranger in slot By has a snare down
inline bool32
RangerTrapDown(ranger_run *Run, u32 By)
{
    bool32 Result = false;
    for(u32 Index = 0; Index < RANGER_MAX_TRAPS; Index++)
    {
        Result |= Run->Traps[Index].Seconds > 0.f && Run->Traps[Index].By == By;
    }
    return Result;
}
