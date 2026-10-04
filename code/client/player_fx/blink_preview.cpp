/* Blink preview: while the local player's blink is ready, a small faint
   ring where it would land: at the cursor, or at the edge of
   PLAYER_AIM_REACH toward it, or short of the first wall, tree or rock
   on the way (the jump stops there). Monsters and players are not
   counted: they move. Online the cooldown comes from the server
   (sim/player_cooldowns.cpp). */

#define BLINK_PREVIEW_RADIUS 10.f
#define BLINK_PREVIEW_DOTS 12
#define BLINK_PREVIEW_COLOR 0xC0FFE8B0
#define BLINK_PREVIEW_STEP 6.f

// NOTE(zoubir): whether a player standing at Position would overlap a
// wall, tree or terrain prop
internal bool32
IsBlinkSpotBlocked(app_state *AppState, world *World, v2 Position, float Z)
{
    world_entity Probe = {};
    Probe.Type = EntityType_Player;
    Probe.Position = V3(Position.X, Position.Y, Z);
    Probe.Collision = AppState->PlayerCollision;
    entity_collision_volume *Total = &Probe.Collision->TotalVolume;
    rectangle3 Box = RectCenterHalfDims(Probe.Position + Total->Offset,
                                        Total->HalfDims);
    world_entity *Nearby[256];
    u32 Count = GatherEntitiesInBox(World, Box, Nearby, ArrayCount(Nearby));
    for(u32 Index = 0; Index < Count; Index++)
    {
        world_entity *Other = Nearby[Index];
        if (Other->IsPresent && !IsWalkingUnit(Other) &&
            CanCollide(AppState, EntityType_Player, Other->Type) &&
            EntityOverlap(&Probe, Other))
        {
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): the last clear spot on the way from the player to Target
internal v2
FindBlinkLanding(app_state *AppState, world_entity *Player, v2 Target)
{
    v2 From = Player->Position.XY;
    v2 Way = Target - From;
    float Distance = Length(Way);
    v2 Result = From;
    u32 Steps = (u32)(Distance / BLINK_PREVIEW_STEP) + 1;
    for(u32 Step = 1; Step <= Steps; Step++)
    {
        v2 Spot = From + ((float)Step / (float)Steps) * Way;
        if (IsBlinkSpotBlocked(AppState, &AppState->World, Spot, Player->Position.Z))
        {
            break;
        }
        Result = Spot;
    }
    return Result;
}

internal void
DrawBlinkPreview(render_context *RenderContext, app_state *AppState,
                 v3 CameraOffset)
{
    world_entity *Player = GetLocalPlayer(AppState);
    if (!Player || !Player->IsPresent || IsDeadPlayer(Player) ||
        Player->MovementCooldowns[PlayerMove_Blink] > 0.f)
    {
        return;
    }
    float Reach = Player->AimReach > 0.f ? Player->AimReach : 1.f;
    v2 Target = Player->Position.XY +
        (Reach * PLAYER_AIM_REACH) * GetPlayerAim(Player);
    v2 Landing = FindBlinkLanding(AppState, Player, Target) - CameraOffset.XY;
    for(u32 Dot = 0; Dot < BLINK_PREVIEW_DOTS; Dot++)
    {
        float Angle = 2.f * Pi32 * (float)Dot / (float)BLINK_PREVIEW_DOTS;
        DrawFxDot(RenderContext,
                  Landing + BLINK_PREVIEW_RADIUS * V2(Cos(Angle), Sin(Angle)),
                  3.f, BLINK_PREVIEW_COLOR);
    }
}
