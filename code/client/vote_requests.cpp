/* Vote requests: a map asked for or a vote answered in the Escape menu
   (ui/options_menu.cpp), on its way to whoever runs the simulation
   (sim/map_vote.cpp). Offline the local simulation takes it through
   player_input.Vote (app.cpp), even with the menu open. Online it rides
   in the vote field of the held buttons (NET_VOTE_SHIFT, net/protocol.h),
   held and let go like a talent request (talent_requests.cpp), so a lost
   packet loses nothing. One at a time: a newer request replaces one not
   yet sent. */

internal void
RequestVote(app_state *AppState, u32 Request)
{
    AppState->VoteRequest = Request;
}

// NOTE(zoubir): offline, the request the local simulation takes this
// frame, 0 for none
internal u32
TakeOfflineVoteRequest(app_state *AppState)
{
    u32 Result = AppState->VoteRequest;
    AppState->VoteRequest = 0;
    return Result;
}

// NOTE(zoubir): online, the vote field to OR into this frame's held
// buttons
internal u32
OnlineVoteBits(app_state *AppState, float DeltaTime)
{
    if (AppState->VoteHolding)
    {
        AppState->VoteHoldLeft -= DeltaTime;
        if (AppState->VoteHoldLeft <= 0.f)
        {
            AppState->VoteHolding = 0;
            AppState->VoteGapLeft = TALENT_GAP_SECONDS;
        }
    }
    else if (AppState->VoteGapLeft > 0.f)
    {
        AppState->VoteGapLeft -= DeltaTime;
    }
    else if (AppState->VoteRequest)
    {
        AppState->VoteHolding = AppState->VoteRequest;
        AppState->VoteRequest = 0;
        AppState->VoteHoldLeft = TALENT_HOLD_SECONDS;
    }
    u32 Result = (AppState->VoteHolding & NET_VOTE_MASK) << NET_VOTE_SHIFT;
    return Result;
}
