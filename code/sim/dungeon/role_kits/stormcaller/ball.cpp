/* Ball Lightning (role_kits/stormcaller.cpp, the right click): the
   Conduction branch's spell against Static Field. A slow ball rolls along
   the aim for BALL_LIGHTNING_SECONDS; every BALL_LIGHTNING_TICK it zaps
   each foe within its radius for BALL_LIGHTNING_DAMAGE and gives the
   Stormcaller BALL_LIGHTNING_CHARGE for each, so it feeds Thunderclap as
   it goes. Arc Field, the branch's fixed talent, widens it and makes it
   bite harder, as it does a Static Field.

   Clients get the ball as a keep burst (StormcallerKeep_Ball) at the cast
   and again every STORMCALLER_KEEP_SECONDS, carrying where it is, its way
   (the burst's angle), its radius and the seconds it has left; each
   client rolls it on from there. Each zap is a short bolt from the ball
   to the foe (StormcallerBolt_Ball). */

// NOTE(zoubir): the ball as clients get it, sent again every
// STORMCALLER_KEEP_SECONDS
inline void
EmitBallKeep(app_state *AppState, stormcaller_ball *Ball)
{
    Ball->Keep += STORMCALLER_KEEP_SECONDS;
    EmitStormcallerKeep(AppState, Ball->By, Ball->Position, StormcallerKeep_Ball, Ball->Seconds, Ball->Radius,
                        ATan2(Ball->Direction.Y, Ball->Direction.X));
}

// NOTE(zoubir): Ball Lightning along the aim; false when every ball is in
// use
internal bool32
CastBallLightning(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    if (Slot->Predicted)
    {
        return true;
    }
    stormcaller_run *Run = &AppState->Dungeon->Stormcaller;
    float Arc = (float)RoleRank(Slot, PlayerRole_Stormcaller, StormcallerTalent_ArcField);
    for(u32 Index = 0; Index < STORMCALLER_MAX_BALLS; Index++)
    {
        stormcaller_ball *Ball = &Run->Balls[Index];
        if (Ball->Seconds <= 0.f)
        {
            v2 Dir = NormalizeOr(GetPlayerAim(Player), V2(1.f, 0.f));
            Ball->Position = ChestOf(Player);
            Ball->Position.XY += 20.f * Dir;
            Ball->Direction = Dir;
            Ball->Seconds = BALL_LIGHTNING_SECONDS;
            Ball->TickTimer = 0.f;
            Ball->Radius = BALL_LIGHTNING_RADIUS * (1.f + ARC_FIELD_RADIUS_SHARE * Arc);
            Ball->TickDamage = BALL_LIGHTNING_DAMAGE * (1.f + ARC_FIELD_DAMAGE_SHARE * Arc);
            Ball->By = (u8)Player->PlayerIndex;
            Ball->Keep = 0.f;
            EmitBallKeep(AppState, Ball);
            EmitSound(&AppState->Events, AssetType_SfxAreaCast, Player->Position);
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): once a tick: each ball rolls on through foes and zaps what
// is near it on its ticks; it goes with its time, or its Stormcaller
internal void
UpdateBallLightning(app_state *AppState, stormcaller_run *Run, float DeltaTime)
{
    world *World = &AppState->World;
    for(u32 Index = 0; Index < STORMCALLER_MAX_BALLS; Index++)
    {
        stormcaller_ball *Ball = &Run->Balls[Index];
        if (Ball->Seconds <= 0.f)
        {
            continue;
        }
        player_slot *Slot = &AppState->Players[Ball->By];
        if (!Slot->Entity || Slot->Role != PlayerRole_Stormcaller)
        {
            Ball->Seconds = 0.f;
            continue;
        }
        Ball->Seconds = Maximum(0.f, Ball->Seconds - DeltaTime);
        Ball->Position.XY += BALL_LIGHTNING_SPEED * DeltaTime * Ball->Direction;
        Ball->Keep -= DeltaTime;
        if (Ball->Keep <= 0.f && Ball->Seconds > 0.f)
        {
            EmitBallKeep(AppState, Ball);
        }
        Ball->TickTimer += DeltaTime;
        if (Ball->TickTimer < BALL_LIGHTNING_TICK)
        {
            continue;
        }
        Ball->TickTimer -= BALL_LIGHTNING_TICK;
        u32 Struck = 0;
        for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
        {
            world_entity *Monster = &World->Entities[EntityIndex];
            v2 Offset = Monster->Position.XY - Ball->Position.XY;
            if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
                Length(Offset) > Ball->Radius + 0.3f * Monster->Dimensions.X)
            {
                continue;
            }
            StormcallerZap(AppState, Ball->By, Monster, Ball->TickDamage, 0.f,
                           NormalizeOr(Offset, Ball->Direction));
            EmitStormcallerBolt(AppState, Ball->By, Ball->Position, ChestOf(Monster), StormcallerBolt_Ball);
            Struck++;
        }
        if (Struck)
        {
            AddStormCharge(AppState, Slot, BALL_LIGHTNING_CHARGE * (float)Struck);
        }
    }
}
