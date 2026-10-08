/* Announcer: tells the players what is happening in the match, as it
   happens, with short cinematic cards over the game. Three sizes:

   - Title cards (film bars slide in, a big title lands in the middle):
     the game is ready on a new map, a dungeon boss steps in, a level is
     cleared, the party wipes.
   - Callouts (a big word punched in over the player, no bars): the round
     countdown and "Fight!", first blood, double kills and killing sprees,
     a room cleared, a boss defeated.
   - Toasts (one line under the kill feed): a player joined or left, a
     vote opened, passed or failed, an ally is down.

   Everything is read from state that offline and online play fill the
   same way (the kill feed, the scores, the round break, the vote, the
   dungeon run's Shown fields and room states), so nothing goes on the
   wire. The watchers (match_watch.cpp, dungeon_watch.cpp) push cards;
   banners.cpp draws them. Entry point: DrawAnnouncer, once a frame from
   client/screen_pass.inc. GAME_ANNOUNCE=<card> (see ForceAnnouncement)
   shows one card at start, for screenshots. */

#define ANNOUNCE_QUEUE_SIZE 4
#define ANNOUNCE_TOAST_COUNT 5
// NOTE(zoubir): a queued callout older than this is no news any more
#define ANNOUNCE_MAX_WAIT 4.f

enum announce_style
{
    AnnounceStyle_Title,
    AnnounceStyle_Callout,
};

// NOTE(zoubir): a higher one cuts a lower one short; equal ones queue
enum announce_priority
{
    AnnouncePriority_Intro = 1,
    AnnouncePriority_Medal,
    AnnouncePriority_Event,
    AnnouncePriority_Big,
    AnnouncePriority_Title,
    AnnouncePriority_Countdown,
};

struct announce_card
{
    u32 Style;
    u32 Priority;
    float Seconds;
    float Age;
    // NOTE(zoubir): seconds spent waiting in the queue
    float Waited;
    u32 Color;
    // NOTE(zoubir): announce_icon (icons/announce_icons.cpp), drawn above
    // the title
    u32 Icon;
    asset_type_id Sound;
    char Kicker[48];
    char Title[64];
    char Detail[128];
};

struct announce_toast
{
    float Age;
    u32 Color;
    u32 Icon;
    char Text[96];
};

struct announcer
{
    // NOTE(zoubir): the big face for titles, made on first use; 0 when
    // the font file is missing (then the title font stands in)
    font *Display;
    bool32 DisplayTried;
    // NOTE(zoubir): the icons' texture, painted on first use, 0 until then
    u32 IconAtlas;
    // NOTE(zoubir): GAME_ANNOUNCE=icons (forced.cpp)
    bool32 ShowIconSheet;
    float Clock;

    bool32 Showing;
    announce_card Now;
    announce_card Queue[ANNOUNCE_QUEUE_SIZE];
    u32 QueueCount;
    announce_toast Toasts[ANNOUNCE_TOAST_COUNT];
    u32 ToastCount;

    // NOTE(zoubir): match_watch.cpp
    bool32 Started;
    u32 MapId;
    bool32 WasOnline;
    float TitleDue;
    float Warmup;
    bool32 LostServer;
    // NOTE(zoubir): people_watch.cpp: a human in each slot, its last
    // name seen, the seconds left waiting for a new one's name, and who
    // is down
    bool32 SlotHuman[MAX_PLAYERS];
    char SlotName[MAX_PLAYERS][16];
    float JoinWait[MAX_PLAYERS];
    bool32 SlotDown[MAX_PLAYERS];
    bool32 VoteWasOpen;
    u32 VoteMapSeen;
    u32 VoteYesSeen;
    u32 VotePlayersSeen;
    float VoteResultDue;
    u32 Round;
    float LastRoundBreak;
    i32 LastCountdown;
    u32 RoundPlayers;
    u32 RoundWins[MAX_PLAYERS];
    bool32 FinalBlowSeen;
    bool32 FinalTwoShown;
    u32 FeedSeen;
    bool32 FirstBloodDone;
    u32 Streak[MAX_PLAYERS];
    u32 MultiKills;
    float LastKillAt;
    // NOTE(zoubir): the slot leading on kills, MAX_PLAYERS for none yet
    u32 Leader;

    // NOTE(zoubir): dungeon_watch.cpp
    struct dungeon_run *Run;
    u8 RoomStates[DUNGEON_MAX_ROOMS + 1];
    u32 FightingRoom;
    bool32 Fighting;
    float FightDue;
    u32 FightBoss;
    bool32 BossLowShown;
    u32 Wipes;
    bool32 LevelDone;
};

internal announcer *
GetAnnouncer(app_state *AppState)
{
    if (!AppState->Announcer)
    {
        AppState->Announcer = AllocateStruct(&AppState->MemoryArena, announcer);
        ZeroSize(AppState->Announcer, sizeof(announcer));
    }
    return AppState->Announcer;
}

// NOTE(zoubir): a card to fill in and push; Seconds by style
inline announce_card
MakeCard(u32 Style, u32 Priority, u32 Icon, u32 Color, char *Kicker, char *Title,
         char *Detail)
{
    announce_card Card = {};
    Card.Style = Style;
    Card.Priority = Priority;
    Card.Icon = Icon;
    Card.Seconds = Style == AnnounceStyle_Title ? 3.4f : 1.7f;
    Card.Color = Color;
    Card.Sound = AssetType_Count;
    snprintf(Card.Kicker, sizeof(Card.Kicker), "%s", Kicker ? Kicker : "");
    snprintf(Card.Title, sizeof(Card.Title), "%s", Title ? Title : "");
    snprintf(Card.Detail, sizeof(Card.Detail), "%s", Detail ? Detail : "");
    return Card;
}

internal void
StartCard(app_state *AppState, announcer *Announcer, announce_card *Card)
{
    Announcer->Now = *Card;
    Announcer->Now.Age = 0.f;
    Announcer->Showing = true;
    if (Card->Sound != AssetType_Count)
    {
        PlayGameSound(AppState, Card->Sound);
    }
}

// NOTE(zoubir): shows the card now when it outranks the one showing,
// else queues it behind the others (a full queue drops the newest)
internal void
PushCard(app_state *AppState, announce_card Card)
{
    announcer *Announcer = GetAnnouncer(AppState);
    if (!Announcer->Showing || Card.Priority > Announcer->Now.Priority)
    {
        // NOTE(zoubir): a card cut short before anyone could read it
        // comes back after (the map's intro is only a greeting)
        announce_card *Cut = &Announcer->Now;
        if (Announcer->Showing && Cut->Priority != AnnouncePriority_Intro &&
            Cut->Age < 0.5f * Cut->Seconds && Announcer->QueueCount < ANNOUNCE_QUEUE_SIZE)
        {
            Cut->Age = 0.f;
            Cut->Waited = 0.f;
            Cut->Sound = AssetType_Count;
            Announcer->Queue[Announcer->QueueCount++] = *Cut;
        }
        StartCard(AppState, Announcer, &Card);
    }
    else if (Announcer->QueueCount < ANNOUNCE_QUEUE_SIZE)
    {
        Announcer->Queue[Announcer->QueueCount++] = Card;
    }
}

// NOTE(zoubir): waits for the card showing to end whatever the ranks:
// the next beat of the same moment ("Round 1" after the map's title)
internal void
PushCardAfter(app_state *AppState, announce_card Card)
{
    announcer *Announcer = GetAnnouncer(AppState);
    if (!Announcer->Showing)
    {
        StartCard(AppState, Announcer, &Card);
    }
    else if (Announcer->QueueCount < ANNOUNCE_QUEUE_SIZE)
    {
        Announcer->Queue[Announcer->QueueCount++] = Card;
    }
}

internal void
PushToast(app_state *AppState, u32 Icon, u32 Color, char *Text)
{
    announcer *Announcer = GetAnnouncer(AppState);
    u32 Keep = Minimum(Announcer->ToastCount, (u32)ANNOUNCE_TOAST_COUNT - 1);
    for(u32 Index = Keep; Index > 0; Index--)
    {
        Announcer->Toasts[Index] = Announcer->Toasts[Index - 1];
    }
    announce_toast *Toast = &Announcer->Toasts[0];
    Toast->Age = 0.f;
    Toast->Color = Color;
    Toast->Icon = Icon;
    snprintf(Toast->Text, sizeof(Toast->Text), "%s", Text);
    Announcer->ToastCount = Keep + 1;
}

// NOTE(zoubir): ages the card showing and moves the queue along
internal void
AdvanceCards(app_state *AppState, announcer *Announcer, float DeltaTime)
{
    for(u32 Index = 0; Index < Announcer->QueueCount;)
    {
        Announcer->Queue[Index].Waited += DeltaTime;
        if (Announcer->Queue[Index].Waited > ANNOUNCE_MAX_WAIT)
        {
            Announcer->Queue[Index] = Announcer->Queue[--Announcer->QueueCount];
        }
        else
        {
            Index++;
        }
    }
    if (Announcer->Showing)
    {
        Announcer->Now.Age += DeltaTime;
        if (Announcer->Now.Age >= Announcer->Now.Seconds)
        {
            Announcer->Showing = false;
        }
    }
    if (!Announcer->Showing && Announcer->QueueCount)
    {
        // NOTE(zoubir): the highest first, the oldest of those
        u32 Best = 0;
        for(u32 Index = 1; Index < Announcer->QueueCount; Index++)
        {
            if (Announcer->Queue[Index].Priority > Announcer->Queue[Best].Priority)
            {
                Best = Index;
            }
        }
        announce_card Card = Announcer->Queue[Best];
        for(u32 Index = Best; Index + 1 < Announcer->QueueCount; Index++)
        {
            Announcer->Queue[Index] = Announcer->Queue[Index + 1];
        }
        Announcer->QueueCount--;
        StartCard(AppState, Announcer, &Card);
    }
    for(u32 Index = 0; Index < Announcer->ToastCount; Index++)
    {
        Announcer->Toasts[Index].Age += DeltaTime;
    }
}

#define ANNOUNCE_WARMUP_SECONDS 2.f
#define ANNOUNCE_TITLE_DELAY 0.4f
#define ANNOUNCE_JOIN_NAME_WAIT 1.5f
#define ANNOUNCE_MULTI_KILL_SECONDS 4.f
#define ANNOUNCE_COUNTDOWN_FROM 3
#define ANNOUNCE_LEAD_MIN_KILLS 2

#define ANNOUNCE_COLOR_GOLD UI_RGBA(255, 214, 96, 255)
#define ANNOUNCE_COLOR_RED UI_RGBA(255, 96, 72, 255)
#define ANNOUNCE_COLOR_BLUE UI_RGBA(120, 190, 255, 255)
#define ANNOUNCE_COLOR_VIOLET UI_RGBA(200, 140, 255, 255)

#include "icons/announce_icons.cpp"
#include "banners.cpp"
#include "people_watch.cpp"
#include "kill_watch.cpp"
#include "match_watch.cpp"
#include "dungeon_watch.cpp"
#include "forced.cpp"

internal void
DrawAnnouncer(render_context *RenderContext, app_state *AppState,
              u32 WindowWidth, u32 WindowHeight, float DeltaTime)
{
    announcer *Announcer = GetAnnouncer(AppState);
    Announcer->Clock += DeltaTime;
    WatchMatch(AppState, Announcer, DeltaTime);
    WatchDungeon(AppState, Announcer, DeltaTime);
    // NOTE(zoubir): the final blow has the screen to itself; cards wait
    if (FinalBlowLeft(AppState) <= 0.f)
    {
        AdvanceCards(AppState, Announcer, DeltaTime);
        DrawAnnounceCard(RenderContext, AppState, Announcer, WindowWidth, WindowHeight);
    }
    DrawAnnounceToasts(RenderContext, AppState, Announcer, WindowWidth);
    if (Announcer->ShowIconSheet)
    {
        DrawIconSheet(RenderContext, Announcer, WindowWidth, WindowHeight);
    }
}
