/* Cast effects: how a player winding up a spell (sim/player_casts.cpp)
   looks, from the press to the spell going off.

   - a circle of light opens on the ground under the caster, in the
     spell's colour, its rim filling clockwise as the cast runs and its
     glyphs turning faster toward the end (build/shaders/fx/cast_sigil.frag,
     drawn by DrawEntity just under the sprite, so the body stands on it);
   - light gathers in the caster's hand: motes spiral into a growing orb,
     sparks rise off the circle and the body glows
     (player_fx/cast_glow.cpp);
   - the caster lights the ground round it (world_lights.cpp, CastLightOf);
   - the body crouches for the whole cast and pops up when it ends
     (SetBodyWindup, body_pose.cpp);
   - when the cast goes off the circle flares out wide, a flash and a
     ring burst from the hand; when it is cut short (a dash, a stun,
     death) the circle shrinks away and the light puffs out grey.

   Read only from CastSpell and CastLeft, which snapshots carry and
   prediction steps for the local player, so it looks the same offline
   and online. A cast counts as gone off when it ended with at least
   CAST_FX_RELEASED of its wind-up seen: online the last snapshot of a
   short cast can come a little before its end.

   The rewinds have their own clock sigil (rewind_fx/), so their row in
   CastLooks draws nothing. A new spell with a wind-up gets a row there.

   Entry points: UpdateCastFx once a frame (from DrawPlayerAbilityFx),
   DrawCastSigil from DrawEntity, DrawCastGlows over the world. */

#define CAST_FX_RELEASED 0.6f
#define CAST_FX_RELEASE_SECONDS 0.4f
#define CAST_FX_FIZZLE_SECONDS 0.35f
// NOTE(zoubir): the circle opens over this long instead of popping in
#define CAST_FX_OPEN_SECONDS 0.12f
// NOTE(zoubir): the circle is drawn flat on the ground: this much as tall
// as it is wide, like the ring under every player
#define CAST_SIGIL_FLATTEN 0.55f

enum cast_figure
{
    CastFigure_Arcane, // two triangles in a star
    CastFigure_Frost,  // six branching spokes
    CastFigure_Fire,   // a five-pointed star with flames on its rim
    CastFigure_Gravity, // three arms spiralling in
};

struct cast_look
{
    // NOTE(zoubir): 0 draws nothing
    float Radius;
    v3 Color;
    cast_figure Figure;
};

// NOTE(zoubir): one row per player_spell, in its order
global_variable cast_look CastLooks[PlayerSpell_Count] =
{
    {0.f, {}, CastFigure_Arcane},                          // None
    {38.f, {0.70f, 0.82f, 1.00f}, CastFigure_Arcane},      // Shockwave, pale steel
    {36.f, {0.35f, 0.95f, 1.00f}, CastFigure_Arcane},      // Push, cyan
    {40.f, {1.00f, 0.70f, 0.22f}, CastFigure_Fire},        // Launch, amber
    {38.f, {1.00f, 0.42f, 0.18f}, CastFigure_Fire},        // Slam, orange
    {30.f, {0.90f, 0.40f, 1.00f}, CastFigure_Arcane},      // Blink, magenta
    {42.f, {0.55f, 0.90f, 1.00f}, CastFigure_Frost},       // Frost Nova, ice
    {42.f, {0.68f, 0.38f, 1.00f}, CastFigure_Gravity},     // Gravity Well, violet
    {50.f, {1.00f, 0.48f, 0.12f}, CastFigure_Fire},        // Meteor, fire
    {50.f, {1.00f, 0.32f, 0.08f}, CastFigure_Fire},        // Giant Fireball, deep fire
    {0.f, {}, CastFigure_Arcane},                          // Rewind
    {0.f, {}, CastFigure_Arcane},                          // Rewind bubble
    {0.f, {}, CastFigure_Arcane},                          // Rewind world
};

struct cast_fx_slot
{
    u32 EntityId;
    // NOTE(zoubir): the cast running, and how far it was last frame
    u32 Spell;
    float Progress;
    // NOTE(zoubir): seconds since it started
    float Age;
    // NOTE(zoubir): the cast that ended: the spell, where, how long ago,
    // and whether it went off or was cut short
    u32 EndedSpell;
    float EndedAge;
    bool32 Fizzled;
    v3 EndedAt;
    v2 EndedHand;
    float EndedProgress;
};

struct cast_fx
{
    cast_fx_slot Slots[MAX_PLAYERS];
    float Clock;
};

inline cast_look *
CastLookOf(u32 Spell)
{
    cast_look *Result = Spell < PlayerSpell_Count && CastLooks[Spell].Radius > 0.f ?
        &CastLooks[Spell] : 0;
    return Result;
}

// NOTE(zoubir): Color times Scale, clamped, with Alpha (0..1)
inline u32
CastRGBA(v3 Color, float Scale, float Alpha)
{
    u32 Result = UI_RGBA((u32)(255.f * Clamp01(Color.X * Scale)),
                         (u32)(255.f * Clamp01(Color.Y * Scale)),
                         (u32)(255.f * Clamp01(Color.Z * Scale)),
                         (u32)(255.f * Clamp01(Alpha)));
    return Result;
}

// NOTE(zoubir): which way the caster's hand points: the aim the cast
// started with, for bodies this machine simulates; replicas never start
// a cast here, so they keep a zero one and use the way they face
internal v2
CastHandDirection(world_entity *Entity)
{
    v2 Result = Entity->CastingDirection;
    if (LengthSq(Result) < 0.01f)
    {
        switch(Entity->AnimationDirection)
        {
            case AnimationDirection_Up: Result = V2(0.f, -1.f); break;
            case AnimationDirection_Left: Result = V2(-1.f, 0.f); break;
            case AnimationDirection_Right: Result = V2(1.f, 0.f); break;
            default: Result = V2(0.f, 1.f); break;
        }
    }
    else
    {
        Result *= 1.f / Length(Result);
    }
    return Result;
}

internal cast_fx_slot *
CastFxSlotOf(app_state *AppState, world_entity *Entity)
{
    cast_fx *Fx = AppState->CastFx;
    cast_fx_slot *Result = 0;
    if (Fx && Entity->Type == EntityType_Player && Entity->PlayerIndex < MAX_PLAYERS &&
        Fx->Slots[Entity->PlayerIndex].EntityId == Entity->ID)
    {
        Result = &Fx->Slots[Entity->PlayerIndex];
    }
    return Result;
}

internal void
EndCastFx(app_state *AppState, world_entity *Entity, cast_fx_slot *Slot)
{
    if (CastLookOf(Slot->Spell))
    {
        Slot->EndedSpell = Slot->Spell;
        Slot->EndedAge = 0.f;
        Slot->Fizzled = Slot->Progress < CAST_FX_RELEASED || IsDeadPlayer(Entity);
        Slot->EndedAt = Entity->Position;
        Slot->EndedHand = CastHandDirection(Entity);
        Slot->EndedProgress = Slot->Progress;
        SetBodyWindup(AppState, Entity, false);
    }
    Slot->Spell = PlayerSpell_None;
    Slot->Progress = 0.f;
}

internal void
UpdateCastFx(app_state *AppState, float DeltaTime)
{
    if (!AppState->CastFx)
    {
        AppState->CastFx = AllocateStruct(&AppState->MemoryArena, cast_fx);
        *AppState->CastFx = {};
    }
    cast_fx *Fx = AppState->CastFx;
    Fx->Clock += DeltaTime;
    bool32 Seen[MAX_PLAYERS] = {};
    world *World = &AppState->World;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (!Entity->IsPresent || Entity->Type != EntityType_Player ||
            Entity->PlayerIndex >= MAX_PLAYERS)
        {
            continue;
        }
        cast_fx_slot *Slot = &Fx->Slots[Entity->PlayerIndex];
        Seen[Entity->PlayerIndex] = true;
        if (Slot->EntityId != Entity->ID)
        {
            *Slot = {};
            Slot->EntityId = Entity->ID;
        }
        u32 Spell = IsPlayerCasting(Entity) && Entity->CastSpell < PlayerSpell_Count ?
            Entity->CastSpell : PlayerSpell_None;
        if (Spell != Slot->Spell)
        {
            EndCastFx(AppState, Entity, Slot);
            Slot->Spell = Spell;
            Slot->Age = 0.f;
        }
        if (Spell != PlayerSpell_None)
        {
            Slot->Age += DeltaTime;
            Slot->Progress = PlayerCastProgress(Entity);
            // NOTE(zoubir): held crouched the whole cast, however long;
            // EndCastFx lets go
            if (CastLookOf(Spell))
            {
                SetBodyWindup(AppState, Entity, true);
            }
        }
        Slot->EndedAge += DeltaTime;
    }
    for(u32 Index = 0; Index < MAX_PLAYERS; Index++)
    {
        if (!Seen[Index])
        {
            Fx->Slots[Index] = {};
        }
    }
}

// NOTE(zoubir): the circle under Entity, at Feet on screen (on the ground
// under it, raised ground included), sorted just under its ring
internal void
DrawCastSigil(render_context *RenderContext, app_state *AppState,
              world_entity *Entity, v2 Feet, float SortingValue)
{
    cast_fx_slot *Slot = CastFxSlotOf(AppState, Entity);
    render_program Program = RenderContext->Programs[Shader_CastSigil];
    if (!Slot || Program.ID == RenderContext->TextureProgram.ID)
    {
        return;
    }
    cast_look *Look = CastLookOf(Slot->Spell);
    float Progress = Slot->Progress;
    float Size = 0.f;
    float Fade = 0.f;
    if (Look)
    {
        float Open = Clamp01(Slot->Age / CAST_FX_OPEN_SECONDS);
        Size = 2.f * Look->Radius * (0.6f + 0.4f * Open) * (1.f + 0.08f * Progress);
        Fade = Open;
    }
    else if ((Look = CastLookOf(Slot->EndedSpell)) != 0)
    {
        Progress = Slot->EndedProgress;
        if (Slot->Fizzled)
        {
            float T = Clamp01(Slot->EndedAge / CAST_FX_FIZZLE_SECONDS);
            Size = 2.f * Look->Radius * (1.f - 0.7f * T);
            Fade = (1.f - T) * 0.6f;
        }
        else
        {
            // NOTE(zoubir): it flares out wide and bright, then is gone
            float T = Clamp01(Slot->EndedAge / CAST_FX_RELEASE_SECONDS);
            Size = 2.f * Look->Radius * (1.08f + 0.7f * sqrtf(T));
            Fade = (1.f - T) * (1.f - T) * 1.6f;
            Progress = 1.f;
        }
    }
    if (!Look || Fade <= 0.f)
    {
        return;
    }
    v3 Color = Slot->Fizzled && !Slot->Spell ? V3(0.55f, 0.55f, 0.6f) : Look->Color;
    float Figure = 2.f * (float)Look->Figure;
    float Height = Size * CAST_SIGIL_FLATTEN;
    BeginBatch(RenderContext, 0, SortingValue - 0.03f, Program);
    RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend = RenderBlend_Additive;
    RenderQuadTexture(RenderContext, Feet.X - 0.5f * Size, Feet.Y - 0.5f * Height,
                      Size, Height, V4(Figure, 1.f, Figure + 1.f, 0.f),
                      CastRGBA(Color, Fade, Progress), 0.f);
    EndBatch(RenderContext);
}

// NOTE(zoubir): the light a caster throws on the ground round it, for
// world_lights.cpp: false when it is not casting a spell with a look
internal bool32
CastLightOf(app_state *AppState, world_entity *Entity, v3 *Color, float *Radius,
            float *Strength)
{
    cast_fx_slot *Slot = CastFxSlotOf(AppState, Entity);
    cast_look *Look = Slot ? CastLookOf(Slot->Spell) : 0;
    bool32 Result = false;
    if (Look)
    {
        *Color = Look->Color;
        *Radius = 2.4f * Look->Radius * (0.8f + 0.4f * Slot->Progress);
        *Strength = 0.25f + 0.55f * Slot->Progress;
        Result = true;
    }
    else if (Slot && !Slot->Fizzled && CastLookOf(Slot->EndedSpell) &&
             Slot->EndedAge < CAST_FX_RELEASE_SECONDS)
    {
        float Left = 1.f - Slot->EndedAge / CAST_FX_RELEASE_SECONDS;
        *Color = CastLooks[Slot->EndedSpell].Color;
        *Radius = 4.f * CastLooks[Slot->EndedSpell].Radius;
        *Strength = 1.1f * Left * Left;
        Result = true;
    }
    return Result;
}
