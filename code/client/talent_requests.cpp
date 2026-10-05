/* Talent requests: the talents the player clicked in the talent panel
   (ui/talent_panel/), on their way to whoever runs the simulation.

   Offline the local simulation takes one a frame through
   player_input.Learn (keyboard_input.cpp). Online they ride in the
   talent field of the held buttons (NET_LEARN_SHIFT, net/protocol.h):
   each is held for TALENT_HOLD_SECONDS, so several inputs carry it and a
   lost packet loses nothing, then the field is let go for
   TALENT_GAP_SECONDS so the server sees the next one, even the same
   talent again, as new.

   The panel also leaves here the screen rectangle it covered last frame,
   so a click on it does not cast a fireball as well. */

#define TALENT_REQUEST_QUEUE 8
#define TALENT_HOLD_SECONDS 0.12f
#define TALENT_GAP_SECONDS 0.08f

struct talent_requests
{
    u32 Queue[TALENT_REQUEST_QUEUE]; // talent_id + 1, oldest first
    u32 Count;
    u32 Holding;  // what the field holds now, 0 for nothing
    float HoldLeft;
    float GapLeft;
    // NOTE(zoubir): the panel's rectangle in window pixels, last frame;
    // zero width while it is closed
    float PanelX, PanelY, PanelWidth, PanelHeight;
    bool32 PanelOpen;
    // NOTE(zoubir): the ability bar's plate with its badge and button,
    // last frame (ui/ability_bar.cpp): clicks there are not casts either
    float BarX, BarY, BarWidth, BarHeight;
};

internal talent_requests *
GetTalentRequests(app_state *AppState)
{
    if (!AppState->TalentRequests)
    {
        AppState->TalentRequests = AllocateStruct(&AppState->MemoryArena, talent_requests);
        *AppState->TalentRequests = {};
    }
    return AppState->TalentRequests;
}

internal void
RequestTalent(app_state *AppState, u32 Talent)
{
    talent_requests *Requests = GetTalentRequests(AppState);
    if (Requests->Count < TALENT_REQUEST_QUEUE && Talent < Talent_Count)
    {
        Requests->Queue[Requests->Count++] = Talent + 1;
    }
}

// NOTE(zoubir): every point back (sim/progression/talents.cpp ResetTalents)
internal void
RequestTalentReset(app_state *AppState)
{
    talent_requests *Requests = GetTalentRequests(AppState);
    if (Requests->Count < TALENT_REQUEST_QUEUE)
    {
        Requests->Queue[Requests->Count++] = TALENT_LEARN_RESET;
    }
}

internal u32
PopTalentRequest(talent_requests *Requests)
{
    u32 Result = 0;
    if (Requests->Count)
    {
        Result = Requests->Queue[0];
        for(u32 Index = 1; Index < Requests->Count; Index++)
        {
            Requests->Queue[Index - 1] = Requests->Queue[Index];
        }
        Requests->Count--;
    }
    return Result;
}

// NOTE(zoubir): offline, the talent the local simulation spends a point on
// this frame (player_input.Learn), 0 for none
internal u32
TakeOfflineTalentRequest(app_state *AppState)
{
    u32 Result = AppState->TalentRequests ?
        PopTalentRequest(AppState->TalentRequests) : 0;
    return Result;
}

// NOTE(zoubir): online, the talent field to OR into this frame's held
// buttons
internal u32
OnlineTalentBits(app_state *AppState, float DeltaTime)
{
    talent_requests *Requests = AppState->TalentRequests;
    if (!Requests)
    {
        return 0;
    }
    if (Requests->Holding)
    {
        Requests->HoldLeft -= DeltaTime;
        if (Requests->HoldLeft <= 0.f)
        {
            Requests->Holding = 0;
            Requests->GapLeft = TALENT_GAP_SECONDS;
        }
    }
    else if (Requests->GapLeft > 0.f)
    {
        Requests->GapLeft -= DeltaTime;
    }
    else if (Requests->Count)
    {
        Requests->Holding = PopTalentRequest(Requests);
        Requests->HoldLeft = TALENT_HOLD_SECONDS;
    }
    u32 Result = (Requests->Holding & NET_LEARN_MASK) << NET_LEARN_SHIFT;
    return Result;
}

inline bool32
MouseInBox(app_input *Input, float X, float Y, float Width, float Height)
{
    bool32 Result = (float)Input->MouseX >= X && (float)Input->MouseY >= Y &&
        (float)Input->MouseX < X + Width && (float)Input->MouseY < Y + Height;
    return Result;
}

// NOTE(zoubir): the mouse is over the open talent panel or the ability
// bar, so its buttons are the screen's and not the player's
internal bool32
TalentPanelHasMouse(app_state *AppState, app_input *Input)
{
    talent_requests *Requests = AppState->TalentRequests;
    bool32 Result = Requests &&
        ((Requests->PanelOpen &&
          MouseInBox(Input, Requests->PanelX, Requests->PanelY, Requests->PanelWidth,
                     Requests->PanelHeight)) ||
         MouseInBox(Input, Requests->BarX, Requests->BarY, Requests->BarWidth,
                    Requests->BarHeight));
    return Result;
}
