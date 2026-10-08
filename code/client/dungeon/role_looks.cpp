/* Role looks (sim/dungeon/roles.cpp): in a dungeon run each player looks
   like their role, so a glance tells the tank from the healer:

   - Bulwark (tank): a steel kite shield with a blue field carried on the
     side the player aims at, swaying with the walk, punching forward
     with a flare on each Shield Bash, and a cold rim of light round the
     body; the sprite is tinted toward steel.
   - Mender (healer): a gold halo bobbing over the head, two motes of
     light circling the body, and a soft green glow at the feet; the
     sprite is tinted toward pale gold.
   - Striker (damage): flames licking up from both hands, embers rising
     off the shoulders; the sprite is tinted warm.

   The later classes' looks are their own files (classes/<class>.cpp).

   Allies also stand on a ring in their class's colour instead of the red
   marker the duel gives other players (RoleMarkerColor, draw_entities.cpp).
   All of it is drawn from the slot's role and the body's position, which
   snapshots carry, so it shows the same online. */

#define ROLE_LOOK_SHIELD_RGB 0x00D8D0C8
#define ROLE_LOOK_FIELD_RGB 0x00B06030
#define ROLE_LOOK_HALO_RGB 0x0060E0FF
#define ROLE_LOOK_MOTE_RGB 0x00A0FFC0
#define ROLE_LOOK_FLAME_RGB 0x002080FF
#define ROLE_LOOK_CORE_RGB 0x0080E0FF

// NOTE(zoubir): the role of the player drawn as Entity, or
// PlayerRole_Count outside a run and for anything else
inline u32
EntityRole(app_state *AppState, world_entity *Entity)
{
    u32 Result = PlayerRole_Count;
    if (IsDungeon(AppState) && Entity->Type == EntityType_Player &&
        Entity->PlayerIndex < MAX_PLAYERS)
    {
        Result = AppState->Players[Entity->PlayerIndex].Role;
    }
    return Result;
}

// NOTE(zoubir): from DrawEntity: the colour a player's sprite is drawn
// with in a run (a light tint toward the role), white elsewhere
internal u32
RoleBodyTint(app_state *AppState, world_entity *Entity)
{
    u32 Result = RGBA8_WHITE;
    switch(EntityRole(AppState, Entity))
    {
        case PlayerRole_Tank:   { Result = 0xFFFFF0E4; } break;
        case PlayerRole_Healer: { Result = 0xFFE8FFFF; } break;
        case PlayerRole_Damage: { Result = 0xFFE4ECFF; } break;
        case PlayerRole_Count: break;
        // NOTE(zoubir): the later classes: a light tint toward the class colour
        default:
        {
            u32 RGB = RoleRGB(EntityRole(AppState, Entity));
            u32 R = 255 - (255 - (RGB & 0xFF)) / 8;
            u32 G = 255 - (255 - ((RGB >> 8) & 0xFF)) / 8;
            u32 B = 255 - (255 - ((RGB >> 16) & 0xFF)) / 8;
            Result = 0xFF000000 | R | (G << 8) | (B << 16);
        } break;
    }
    return Result;
}

// NOTE(zoubir): from DrawEntity: the ring under another player, in its
// role's colour in a run; Duel elsewhere
internal u32
RoleMarkerColor(app_state *AppState, world_entity *Entity, u32 Duel)
{
    u32 Result = Duel;
    u32 Role = EntityRole(AppState, Entity);
    if (Role < PlayerRole_Count)
    {
        u8 *C = GetRoleDef(Role)->Color;
        Result = UI_RGBA(C[0], C[1], C[2], 220);
    }
    return Result;
}

// NOTE(zoubir): where a body's middle is on screen, Up of its height
inline v2
RoleLookPoint(world_entity *Player, float Up, v3 CameraOffset)
{
    v3 P = Player->Position;
    P.Z += Up * Player->Dimensions.Y;
    v2 Result = BurstToScreen(P, CameraOffset);
    return Result;
}

// NOTE(zoubir): a kite shield at Centre, Size tall, leaning by Lean
internal void
DrawKiteShield(render_context *RenderContext, v2 Centre, float Size, float Lean, float Alpha)
{
    v2 Up = V2(Sin(Lean), -Cos(Lean));
    v2 Side = V2(-Up.Y, Up.X);
    float W = 0.36f * Size;
    v2 Top = Centre + (0.5f * Size) * Up;
    v2 Shoulder = Centre + (0.22f * Size) * Up;
    v2 Tip = Centre - (0.5f * Size) * Up;
    u32 Dark = FxColor(0.85f * Alpha, 0x00201810);
    u32 Rim = FxColor(Alpha, ROLE_LOOK_SHIELD_RGB);
    u32 Field = FxColor(Alpha, ROLE_LOOK_FIELD_RGB);
    float O = 1.6f;
    DrawFilledQuad(RenderContext, Top - (W + O) * Side, Top + (W + O) * Side,
                   Shoulder + (W + O) * Side, Shoulder - (W + O) * Side, Dark, Dark, Dark, Dark,
                   RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Shoulder - (W + O) * Side, Shoulder + (W + O) * Side,
                   Tip - O * Up, Tip - O * Up, Dark, Dark, Dark, Dark, RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Top - W * Side, Top + W * Side, Shoulder + W * Side,
                   Shoulder - W * Side, Rim, Rim, Rim, Rim, RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Shoulder - W * Side, Shoulder + W * Side, Tip, Tip,
                   Rim, Rim, Rim, Rim, RenderBlend_Alpha);
    float F = 0.72f * W;
    v2 FieldTop = Top - (0.12f * Size) * Up;
    DrawFilledQuad(RenderContext, FieldTop - F * Side, FieldTop + F * Side,
                   Shoulder + F * Side, Shoulder - F * Side, Field, Field, Field, Field,
                   RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Shoulder - F * Side, Shoulder + F * Side,
                   Tip + (0.14f * Size) * Up, Tip + (0.14f * Size) * Up,
                   Field, Field, Field, Field, RenderBlend_Alpha);
    // NOTE(zoubir): a gold boss in the middle, catching the light
    DrawFxDot(RenderContext, Centre + (0.08f * Size) * Up, 0.18f * Size,
              FxColor(Alpha, 0x0050C8F0));
    DrawFxDot(RenderContext, Centre + (0.11f * Size) * Up - 0.03f * Size * Side,
              0.07f * Size, FxColor(Alpha, 0x00C0F0FF));
}

// NOTE(zoubir): how long a Shield Bash's punch lasts on screen, and how
// far forward the shield goes, a share of the body's width
#define TANK_BASH_SECONDS 0.24f
#define TANK_BASH_REACH 0.45f

// NOTE(zoubir): per player slot, the bash count last seen in its
// ClassFlags (TANK_FLAG_BASH_COUNT) and when it changed
struct tank_bash_seen
{
    u8 Count;
    float At;
};

global_variable tank_bash_seen TankBashSeen[MAX_PLAYERS];

// NOTE(zoubir): 1 as a bash lands, easing back to 0, from the count the
// slot's ClassFlags carries, so it plays the same online
internal float
TankBashPunch(player_slot *Slot, u32 SlotIndex, float Clock)
{
    tank_bash_seen *Seen = &TankBashSeen[SlotIndex];
    u8 Count = (u8)(Slot->ClassFlags & TANK_FLAG_BASH_COUNT);
    if (Count != Seen->Count)
    {
        Seen->Count = Count;
        Seen->At = Clock;
    }
    float T = (Clock - Seen->At) / TANK_BASH_SECONDS;
    float Result = 0.f;
    if (T >= 0.f && T < 1.f)
    {
        // NOTE(zoubir): out fast, back slow
        Result = T < 0.25f ? T / 0.25f : 1.f - Square((T - 0.25f) / 0.75f);
        Result = Maximum(0.f, Result);
    }
    return Result;
}

internal void
DrawTankLook(render_context *RenderContext, player_slot *Slot, world_entity *Player,
             float Clock, v3 CameraOffset)
{
    float Punch = TankBashPunch(Slot, Player->PlayerIndex, Clock);
    v2 Aim = GetPlayerAim(Player);
    float Facing = Aim.X < -0.1f ? -1.f : 1.f;
    float Speed = Length(Player->Velocity.XY);
    float Sway = 0.12f * Sin(9.f * Clock) * Minimum(1.f, Speed / 150.f);
    // NOTE(zoubir): aiming away from the camera the shield goes behind,
    // so it is drawn smaller and fainter
    float Behind = Aim.Y < -0.4f ? 0.55f : 1.f;
    v2 Body = RoleLookPoint(Player, 0.4f, CameraOffset);
    v2 Centre = Body + V2(Facing * (0.34f + TANK_BASH_REACH * Punch) * Player->Dimensions.X, 2.f);
    float Size = 0.48f * Player->Dimensions.Y * (0.8f + 0.2f * Behind) * (1.f + 0.12f * Punch);
    // NOTE(zoubir): the shield squares up to the blow, flat to its target
    DrawKiteShield(RenderContext, Centre, Size, (Facing * 0.18f + Sway) * (1.f - Punch), Behind);
    if (Punch > 0.f)
    {
        float Flare = Size * (0.9f + 0.5f * Punch);
        DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.5f * Flare, Centre.Y - 0.5f * Flare,
                       Flare, Flare, FxColor(0.55f * Punch, ROLE_LOOK_SHIELD_RGB),
                       RenderBlend_Additive);
    }
}

internal void
DrawHealerLook(render_context *RenderContext, world_entity *Player, float Clock,
               v3 CameraOffset)
{
    float Bob = 2.f * Sin(2.5f * Clock + (float)Player->PlayerIndex);
    v2 Head = RoleLookPoint(Player, 0.9f, CameraOffset) + V2(0.f, Bob);
    float Wide = 0.22f * Player->Dimensions.X;
    // NOTE(zoubir): the halo, a flat ring of light seen from the side
    for(u32 Dot = 0; Dot < 18; Dot++)
    {
        float Angle = 2.f * Pi32 * (float)Dot / 18.f + 0.6f * Clock;
        v2 P = Head + V2(Wide * Cos(Angle), 0.28f * Wide * Sin(Angle));
        float Front = Sin(Angle) > 0.f ? 1.f : 0.6f;
        DrawFxDot(RenderContext, P, 3.f, FxColor(0.9f * Front, ROLE_LOOK_HALO_RGB));
    }
    DrawShaderQuad(RenderContext, Shader_Glow, Head.X - Wide * 1.6f, Head.Y - Wide * 0.8f,
                   Wide * 3.2f, Wide * 1.6f, FxColor(0.35f, ROLE_LOOK_HALO_RGB),
                   RenderBlend_Additive);
    // NOTE(zoubir): two motes circling the body, the near one bigger
    v2 Body = RoleLookPoint(Player, 0.45f, CameraOffset);
    for(u32 Mote = 0; Mote < 2; Mote++)
    {
        float Angle = 2.2f * Clock + Pi32 * (float)Mote;
        float Near = Sin(Angle);
        v2 P = Body + V2(0.55f * Player->Dimensions.X * Cos(Angle), 6.f * Near - 4.f);
        float Size = 3.5f + 1.5f * Near;
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - 3.f * Size, P.Y - 3.f * Size,
                       6.f * Size, 6.f * Size, FxColor(0.5f, ROLE_LOOK_MOTE_RGB),
                       RenderBlend_Additive);
        DrawFxDot(RenderContext, P, Size, FxColor(0.95f, 0x00E0FFF0));
    }
    v2 Feet = RoleLookPoint(Player, 0.f, CameraOffset);
    float Pool = 1.4f * Player->Dimensions.X;
    DrawShaderQuad(RenderContext, Shader_Glow, Feet.X - 0.5f * Pool, Feet.Y - 0.2f * Pool,
                   Pool, 0.4f * Pool, FxColor(0.3f + 0.08f * Sin(3.f * Clock), ROLE_LOOK_MOTE_RGB),
                   RenderBlend_Additive);
}

// NOTE(zoubir): a flame licking up from Base: dots climbing and shrinking
internal void
DrawHandFlame(render_context *RenderContext, v2 Base, float Size, float Clock, u32 Seed)
{
    DrawShaderQuad(RenderContext, Shader_Glow, Base.X - 2.5f * Size, Base.Y - 3.f * Size,
                   5.f * Size, 5.f * Size, FxColor(0.7f, ROLE_LOOK_FLAME_RGB),
                   RenderBlend_Additive);
    for(u32 Lick = 0; Lick < 7; Lick++)
    {
        float Phase = DungeonFxFraction(1.8f * Clock + 0.143f * (float)Lick +
                                        0.37f * (float)Seed);
        float Wobble = 0.35f * Size * Sin(11.f * Clock + 2.f * (float)Lick + (float)Seed);
        v2 P = Base + V2(Wobble * Phase, -1.6f * Size * Phase);
        float Dot = Size * (1.f - 0.7f * Phase);
        DrawFxDot(RenderContext, P, Dot, FxColor(1.f - Phase,
                                                 Phase < 0.4f ? ROLE_LOOK_CORE_RGB : ROLE_LOOK_FLAME_RGB));
    }
}

internal void
DrawStrikerLook(render_context *RenderContext, world_entity *Player, float Clock,
                v3 CameraOffset)
{
    v2 Body = RoleLookPoint(Player, 0.36f, CameraOffset);
    float Out = 0.4f * Player->Dimensions.X;
    float Size = 0.14f * Player->Dimensions.Y;
    DrawHandFlame(RenderContext, Body + V2(-Out, 0.f), Size, Clock, 2 * Player->PlayerIndex);
    DrawHandFlame(RenderContext, Body + V2(Out, 0.f), Size, Clock, 2 * Player->PlayerIndex + 1);
    // NOTE(zoubir): embers drifting up off the shoulders
    v2 Shoulders = RoleLookPoint(Player, 0.7f, CameraOffset);
    for(u32 Ember = 0; Ember < 4; Ember++)
    {
        float Rise = DungeonFxFraction(0.7f * Clock + 0.25f * (float)Ember);
        float X = (BurstJitter(Ember + 8 * Player->PlayerIndex, 61) - 0.5f) * 1.2f * Out * 2.f;
        v2 P = Shoulders + V2(X + 4.f * Sin(4.f * Clock + (float)Ember), -26.f * Rise);
        DrawFxDot(RenderContext, P, 2.5f, FxColor(0.9f * (1.f - Rise), ROLE_LOOK_FLAME_RGB));
    }
}

internal void DrawClassLook(render_context *RenderContext, app_state *AppState, player_slot *Slot,
                           world_entity *Player, float Clock, v3 CameraOffset);

// NOTE(zoubir): every living party member's role look, over the world
internal void
DrawRoleLooks(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    float Clock = GetFxClock(AppState);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = Slot->Entity;
        if (!Slot->Active || !Player || !Player->IsPresent || IsDeadPlayer(Player))
        {
            continue;
        }
        switch(Slot->Role)
        {
            case PlayerRole_Tank:   { DrawTankLook(RenderContext, Slot, Player, Clock, CameraOffset); } break;
            case PlayerRole_Healer: { DrawHealerLook(RenderContext, Player, Clock, CameraOffset); } break;
            case PlayerRole_Damage: { DrawStrikerLook(RenderContext, Player, Clock, CameraOffset); } break;
            default: { DrawClassLook(RenderContext, AppState, Slot, Player, Clock, CameraOffset); } break;
        }
    }
}
