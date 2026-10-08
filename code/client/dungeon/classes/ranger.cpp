/* Ranger effects (sim/dungeon/role_kits/ranger.cpp), included by
   classes/class_fx.cpp: how a Ranger looks in a run, its bursts
   (SimBurst_RangerFirst on, their rows in ranger_bursts.inc) and what its
   spells leave in the world.

   The look: a longbow in the hand on the aim side and a quiver of
   teal-fletched arrows over the back shoulder, with a little teal wind
   at the feet while it runs. The bow follows the cast: Piercing Shot
   draws the string back for its whole wind-up, a heavy arrow nocked and
   a glint gathering at its head (brighter with more Focus, gold when
   Deadeye will crit), and a line along the aim shows where it will fly;
   Rapid Fire draws and looses in a quick rhythm; any shot snaps the
   string forward and leaves it shivering. All of it reads the cast, the
   aim and the bursts, which every client has, so it looks the same
   online. The shapes are ranger/bow.cpp, the flying bursts
   ranger/shot_fx.cpp, the lasting ones ranger/ground_fx.cpp. */

#include "ranger/bow.cpp"
#include "ranger/shot_fx.cpp"
#include "ranger/ground_fx.cpp"

// NOTE(zoubir): a living Ranger's look, over its sprite, every frame
internal void
DrawRangerLook(render_context *RenderContext, app_state *AppState, player_slot *Slot,
            world_entity *Player, float Clock, v3 CameraOffset)
{
    u32 SlotIndex = Player->PlayerIndex;
#if APP_DEV
    // NOTE(zoubir): developer builds, offline: GAME_RANGER_TALENTS="221111"
    // gives the local Ranger those ranks, slot by slot, so a scripted
    // screenshot (misc\screenshot.bat) can show the tree's spells
#pragma warning(push)
#pragma warning(disable: 4996)
    char *Ranks = getenv("GAME_RANGER_TALENTS");
#pragma warning(pop)
    if (Ranks && SlotIndex == AppState->LocalPlayerIndex && !IsOnline(AppState->Online))
    {
        for(u32 Talent = 0; Talent < ROLE_TALENTS && Ranks[Talent] >= '0' && Ranks[Talent] <= '9'; Talent++)
        {
            Slot->Ranks[Talent_RoleFirst + Talent] = (u8)(Ranks[Talent] - '0');
        }
    }
    // NOTE(zoubir): and GAME_RANGER_FOCUS=100 holds its Focus there until
    // a Piercing Shot is drawn, to show the bar and the draw full
#pragma warning(push)
#pragma warning(disable: 4996)
    char *HeldFocus = getenv("GAME_RANGER_FOCUS");
#pragma warning(pop)
    if (HeldFocus && HeldFocus[0] && SlotIndex == AppState->LocalPlayerIndex && !IsOnline(AppState->Online) &&
        Player->CastSpell != PlayerSpell_RangerA && RangerBurstAge(AppState, SlotIndex, RangerBurst_Pierce) > 1.f)
    {
        Slot->Ranger.Focus = Minimum(RANGER_FOCUS_MOST, (float)atoi(HeldFocus));
    }
#endif
    v2 Aim = RangerScreenAim(Player);
    float Facing = Aim.X < -0.1f ? -1.f : 1.f;
    float Speed = Length(Player->Velocity.XY);
    float Moving = Minimum(1.f, Speed / 150.f);
    float Height = Player->Dimensions.Y;

    // NOTE(zoubir): wind curling off the heels while it runs
    v2 Feet = RoleLookPoint(Player, 0.05f, CameraOffset);
    for(u32 Wisp = 0; Wisp < 3 && Moving > 0.2f; Wisp++)
    {
        float Phase = DungeonFxFraction(1.4f * Clock + 0.33f * (float)Wisp);
        v2 Back = NormalizeOr(-1.f * Player->Velocity.XY, V2(-Facing, 0.f));
        v2 Side = V2(-Back.Y, Back.X);
        v2 P = Feet + (8.f + 30.f * Phase) * Back + (6.f * Sin(7.f * Phase + (float)Wisp)) * Side -
            V2(0.f, 4.f + 10.f * Phase);
        DrawFxStroke(RenderContext, P, P + 10.f * Back + 4.f * Side, 2.5f, 0.5f,
                     FxColor(0.5f * Moving * (1.f - Phase), RANGER_FX_TEAL_RGB),
                     FxColor(0.f, RANGER_FX_PALE_RGB));
    }

    // NOTE(zoubir): the quiver over the shoulder away from the aim
    v2 Shoulder = RoleLookPoint(Player, 0.62f, CameraOffset) +
        V2(-Facing * 0.24f * Player->Dimensions.X, 1.5f * Sin(9.f * Clock) * Moving);
    DrawRangerQuiver(RenderContext, Shoulder, 0.42f * Height, -Facing * 0.55f);

    // NOTE(zoubir): what the bow is doing
    float Draw = 0.f;
    bool32 Nock = false;
    float Heavy = 0.f;
    float Quiver = 0.f;
    float Glint = 0.f;
    u32 GlintRGB = RANGER_FX_TEAL_RGB;
    float Focus = (float)Slot->ClassMeter / RANGER_FOCUS_MOST;
    float SinceShot = Minimum(Minimum(RangerBurstAge(AppState, SlotIndex, RangerBurst_Arrow),
                                      RangerBurstAge(AppState, SlotIndex, RangerBurst_Pierce)),
                              RangerBurstAge(AppState, SlotIndex, RangerBurst_Mark));
    if (Player->CastSpell == PlayerSpell_RangerA)
    {
        float Progress = PlayerCastProgress(Player);
        Draw = Progress * Progress * (3.f - 2.f * Progress);
        Nock = true;
        Heavy = 1.f;
        Glint = Progress * (0.5f + 0.5f * Focus);
        if (Slot->ClassFlags & RANGER_FLAG_DEADEYE)
        {
            GlintRGB = RANGER_FX_GOLD_RGB;
        }
    }
    else if (Player->CastSpell == PlayerSpell_RangerB)
    {
        float Beat = PlayerSpells[PlayerSpell_RangerB].CastTime / (float)RAPID_FIRE_ARROWS;
        float Into = PlayerSpells[PlayerSpell_RangerB].CastTime - Player->CastLeft;
        float Phase = DungeonFxFraction(Into / Beat);
        Nock = Phase < 0.75f;
        Draw = Nock ? 0.8f * Phase / 0.75f : 0.f;
        Quiver = Nock ? 0.f : 1.f;
    }
    else if (SinceShot < 0.35f)
    {
        Quiver = 1.f - SinceShot / 0.35f;
    }
    else
    {
        // NOTE(zoubir): ready in a fight, an arrow resting on the string
        Nock = AppState->Dungeon && AppState->Dungeon->FightingRoom;
        Draw = Nock ? 0.08f : 0.f;
    }
    float Recoil = SinceShot < 0.15f ? 4.f * (1.f - SinceShot / 0.15f) : 0.f;
    v2 Grip = RangerBowGrip(Player, CameraOffset) - Recoil * Aim +
        V2(0.f, 1.2f * Sin(2.f * Clock + (float)SlotIndex));
    float Size = 0.95f * Height;
    DrawRangerBow(RenderContext, Grip, Aim, Size, Draw, Nock, Heavy, Quiver, Clock);
    if (Glint > 0.f)
    {
        // NOTE(zoubir): the glint at the heavy arrow's head, wind motes
        // closing in on it
        v2 Head = Grip - (0.16f + 0.14f * Draw) * 0.5f * Size * Aim - Draw * 0.55f * Size * Aim +
            (1.15f * Size) * Aim;
        float G = 10.f + 16.f * Glint;
        DrawShaderQuad(RenderContext, Shader_Glow, Head.X - G, Head.Y - G, 2.f * G, 2.f * G,
                       FxColor(0.9f * Glint, GlintRGB), RenderBlend_Additive);
        float Spin = 3.f * Clock;
        float Arm = 5.f + 9.f * Glint;
        for(u32 Ray = 0; Ray < 4; Ray++)
        {
            float A = Spin + 0.5f * Pi32 * (float)Ray;
            DrawFxStroke(RenderContext, Head, Head + Arm * V2(Cos(A), Sin(A)), 2.2f, 0.f,
                         FxColor(Glint, 0x00FFFFFF), FxColor(0.f, GlintRGB));
        }
        for(u32 Mote = 0; Mote < 6; Mote++)
        {
            float Phase = DungeonFxFraction(2.f * Clock + 0.167f * (float)Mote);
            float A = 2.f * Pi32 * BurstJitter(Mote, 241) + Clock;
            v2 P = Head + (36.f * (1.f - Phase)) * V2(Cos(A), Sin(A));
            DrawFxDot(RenderContext, P, 2.5f, FxColor(Glint * Phase, GlintRGB));
        }
    }
}

// NOTE(zoubir): burst Index of the class's (Burst->Kind -
// SimBurst_RangerFirst), T from 0 to 1 over its row's Seconds
internal void
DrawRangerBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
             u32 Index, float T, v3 CameraOffset)
{
    switch(Index)
    {
        case RangerBurst_Arrow: DrawRangerArrowBurst(RenderContext, AppState, Burst, T, CameraOffset); break;
        case RangerBurst_Volley: DrawRangerVolley(RenderContext, AppState, Burst, T, CameraOffset); break;
        case RangerBurst_Pierce: DrawRangerPierce(RenderContext, AppState, Burst, T, CameraOffset); break;
        case RangerBurst_Mark: DrawRangerMark(RenderContext, AppState, Burst, T, CameraOffset); break;
        case RangerBurst_Leap: DrawRangerLeap(RenderContext, Burst, T, CameraOffset); break;
        case RangerBurst_Trap: DrawRangerTrap(RenderContext, AppState, Burst, T, CameraOffset); break;
        case RangerBurst_TrapSnap: DrawRangerTrapSnap(RenderContext, Burst, T, CameraOffset); break;
        case RangerBurst_FocusFull: DrawRangerFocusFull(RenderContext, AppState, Burst, T, CameraOffset); break;
    }
}

// NOTE(zoubir): every frame in a run, over the world: what lasts (zones,
// marks, projectiles). While a Ranger draws a Piercing Shot, the line it
// will fly along, filling in as the draw goes on
internal void
DrawRangerFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = Slot->Entity;
        if (!Slot->Active || Slot->Role != PlayerRole_Ranger || !Player || !Player->IsPresent ||
            IsDeadPlayer(Player) || Player->CastSpell != PlayerSpell_RangerA)
        {
            continue;
        }
        float Progress = PlayerCastProgress(Player);
        bool32 Local = SlotIndex == AppState->LocalPlayerIndex;
        float Alpha = (Local ? 0.55f : 0.3f) * Minimum(1.f, 4.f * Progress);
        v2 Dir = RangerScreenAim(Player);
        v2 Side = V2(-Dir.Y, Dir.X);
        v2 Feet = BurstToScreen(V3(Player->Position.X, Player->Position.Y, Player->GroundZ), CameraOffset);
        v2 Far = Feet + PIERCE_RANGE * Dir;
        u32 RGB = (Slot->ClassFlags & RANGER_FLAG_DEADEYE) ? RANGER_FX_GOLD_RGB : RANGER_FX_TEAL_RGB;
        // NOTE(zoubir): the lane's edges, and its fill racing out with the draw
        u32 Edge = FxColor(Alpha, RGB);
        DrawFxStroke(RenderContext, Feet + PIERCE_WIDTH * Side, Far + PIERCE_WIDTH * Side, 2.f, 2.f,
                     Edge, FxColor(0.f, RGB));
        DrawFxStroke(RenderContext, Feet - PIERCE_WIDTH * Side, Far - PIERCE_WIDTH * Side, 2.f, 2.f,
                     Edge, FxColor(0.f, RGB));
        DrawFxStroke(RenderContext, Feet, Feet + Progress * PIERCE_RANGE * Dir, 2.f * PIERCE_WIDTH,
                     2.f * PIERCE_WIDTH, FxColor(0.16f * Alpha, RGB), FxColor(0.02f * Alpha, RGB));
        for(u32 Chevron = 0; Chevron < 6; Chevron++)
        {
            float Along = PIERCE_RANGE * DungeonFxFraction(0.166f * (float)Chevron + 0.8f * GetFxClock(AppState));
            float Show = Alpha * (Along < Progress * PIERCE_RANGE ? 1.f : 0.25f) * (1.f - Along / PIERCE_RANGE);
            v2 Tip = Feet + Along * Dir;
            DrawFxStroke(RenderContext, Tip - 10.f * Dir + 9.f * Side, Tip, 2.5f, 2.5f,
                         FxColor(Show, RGB), FxColor(Show, RGB));
            DrawFxStroke(RenderContext, Tip - 10.f * Dir - 9.f * Side, Tip, 2.5f, 2.5f,
                         FxColor(Show, RGB), FxColor(Show, RGB));
        }
    }
}
