/* Sword arcs: each new sword entity gets a sweep of dots around the
   player who swung it, along the swing direction, drawn over
   SWORD_ARC_SECONDS. Replicas do not know their owner, so the swinger is
   the nearest player when the sword first shows up, and the direction
   runs from that player to the sword. */

#define MAX_SWORD_ARCS 16
#define SWORD_ARC_SECONDS 0.2f
#define SWORD_ARC_RADIUS 30.f
// NOTE(zoubir): half the swept angle, about 70 degrees
#define SWORD_ARC_HALF_ANGLE 1.22f
#define SWORD_ARC_DOTS 14
// NOTE(zoubir): a sword this far or farther from every player has no
// known swinger and is skipped
#define SWORD_ARC_OWNER_RANGE 48.f
#define SWORD_ARC_RGB 0x00F0FFFF

struct sword_arc
{
    u32 SwordID;
    v2 Center;
    float Angle;
    float Age;
    // NOTE(zoubir): 1 or -1; swings alternate sides so a combo reads as
    // back-and-forth slashes
    float Side;
};

struct sword_arcs
{
    sword_arc Arcs[MAX_SWORD_ARCS];
    u32 Count;
    u32 SwingCount;
};

internal sword_arc *
FindSwordArc(sword_arcs *Fx, u32 SwordID)
{
    sword_arc *Result = 0;
    for(u32 Index = 0; Index < Fx->Count && !Result; Index++)
    {
        if (Fx->Arcs[Index].SwordID == SwordID)
        {
            Result = &Fx->Arcs[Index];
        }
    }
    return Result;
}

internal world_entity *
FindSwordSwinger(app_state *AppState, world_entity *Sword)
{
    world_entity *Result = 0;
    float Best = SWORD_ARC_OWNER_RANGE;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = Slot->Entity;
        if (!Slot->Active || !Player || !Player->IsPresent)
        {
            continue;
        }
        float Distance = Length(Sword->Position.XY - Player->Position.XY);
        if (Distance < Best)
        {
            Best = Distance;
            Result = Player;
        }
    }
    return Result;
}

inline bool32
IsSwordPresent(world *World, u32 SwordID)
{
    bool32 Result = false;
    for(u32 Index = 0; Index < World->EntityCount && !Result; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        Result = Entity->IsPresent && Entity->Type == EntityType_Sword &&
            Entity->ID == SwordID;
    }
    return Result;
}

// NOTE(zoubir): an arc stays listed while its sword exists, even once
// drawn, so the same sword never starts a second arc
internal void
UpdateSwordArcs(sword_arcs *Fx, app_state *AppState, float DeltaTime)
{
    world *World = &AppState->World;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Sword = &World->Entities[Index];
        if (!Sword->IsPresent || Sword->Type != EntityType_Sword ||
            FindSwordArc(Fx, Sword->ID) || Fx->Count >= MAX_SWORD_ARCS)
        {
            continue;
        }
        world_entity *Swinger = FindSwordSwinger(AppState, Sword);
        if (!Swinger)
        {
            continue;
        }
        v2 Dir = Sword->Position.XY - Swinger->Position.XY;
        sword_arc *Arc = &Fx->Arcs[Fx->Count++];
        Arc->SwordID = Sword->ID;
        Arc->Center = Swinger->Position.XY;
        Arc->Angle = LengthSq(Dir) > 0.0001f ? ATan2(Dir.Y, Dir.X) : 0.f;
        Arc->Age = 0.f;
        Arc->Side = (Fx->SwingCount++ & 1) ? -1.f : 1.f;
    }

    for(u32 Index = 0; Index < Fx->Count;)
    {
        sword_arc *Arc = &Fx->Arcs[Index];
        Arc->Age += DeltaTime;
        if (Arc->Age >= SWORD_ARC_SECONDS && !IsSwordPresent(World, Arc->SwordID))
        {
            *Arc = Fx->Arcs[--Fx->Count];
        }
        else
        {
            Index++;
        }
    }
}

// NOTE(zoubir): the leading edge sweeps across the arc in the first half
// of its time; dots behind it shrink and fade, like a blade's trail
internal void
DrawSwordArcs(render_context *RenderContext, sword_arcs *Fx, v3 CameraOffset)
{
    for(u32 Index = 0; Index < Fx->Count; Index++)
    {
        sword_arc *Arc = &Fx->Arcs[Index];
        float Progress = Arc->Age / SWORD_ARC_SECONDS;
        if (Progress >= 1.f)
        {
            continue;
        }
        float Lead = Minimum(1.f, 2.f * Progress);
        float Fade = 1.f - Progress;
        v2 Center = Arc->Center - CameraOffset.XY;
        for(u32 Dot = 0; Dot < SWORD_ARC_DOTS; Dot++)
        {
            float T = (float)Dot / (float)(SWORD_ARC_DOTS - 1);
            if (T > Lead)
            {
                break;
            }
            float Strength = Fade * (1.f - (Lead - T));
            if (Strength <= 0.f)
            {
                continue;
            }
            float Angle = Arc->Angle +
                Arc->Side * SWORD_ARC_HALF_ANGLE * (2.f * T - 1.f);
            v2 P = Center + SWORD_ARC_RADIUS * V2(Cos(Angle), Sin(Angle));
            u32 Color = ((u32)(255.f * Strength) << 24) | SWORD_ARC_RGB;
            DrawFxDot(RenderContext, P, 3.f + 3.f * Strength, Color);
        }
    }
}
