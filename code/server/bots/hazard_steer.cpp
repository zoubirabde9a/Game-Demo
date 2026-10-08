/* Hazard steering (server/bots.cpp): a bot walks straight at what it
   wants, so on a map with lava or pits it would walk into them (the
   Ember Depths' lava lake beside the Ashfall causeway). Before a bot's
   movement keys go out, SteerAroundHazards looks a few steps ahead
   along the way it means to walk; if the ground there hurts
   (IsHazardAt, sim/terrain/ground_queries.cpp), it tries the ways
   either side of it, nearest first, and walks the first clear one, or
   stands still when none is. When the bot was walking at what it
   chases, it follows the way round over the map's tiles instead
   (bots/bot_paths.cpp), so it is not stopped at the shore of a lake
   with its target on the far side. A bot already standing in it heads
   for the nearest clear ground.

   Between fights a bot that sees no monster walks to the middle of the
   room the party has to clear next (BotWalkToNextRoom) along the same
   tile paths, instead of wandering until something comes into sight. */

#include "bot_paths.cpp"

// NOTE(zoubir): how far ahead a bot looks, in how many checks, and how
// far either side of its middle (about its body's half width): a bot
// running at full speed needs most of this to turn
#define BOT_HAZARD_LOOK 120.f
#define BOT_HAZARD_CHECKS 5
#define BOT_HAZARD_SIDE 18.f
// NOTE(zoubir): how far a bot in a hazard looks for clear ground
#define BOT_HAZARD_ESCAPE 192.f

#define BOT_MOVE_BUTTONS (NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down)

// NOTE(zoubir): the way the movement keys Buttons walk, one long
inline v2
BotKeysWay(u32 Buttons)
{
    v2 Result = V2((Buttons & NetButton_Right ? 1.f : 0.f) - (Buttons & NetButton_Left ? 1.f : 0.f),
                   (Buttons & NetButton_Down ? 1.f : 0.f) - (Buttons & NetButton_Up ? 1.f : 0.f));
    if (LengthSq(Result) > 0.5f)
    {
        Result = Result * (1.f / Length(Result));
    }
    return Result;
}

// NOTE(zoubir): whether a body walking from From along Way stays off
// hazards for BOT_HAZARD_LOOK, its middle and both its sides
internal bool32
BotWayIsClear(world *World, v2 From, v2 Way)
{
    v2 Side = V2(-Way.Y, Way.X);
    for(u32 Check = 1; Check <= BOT_HAZARD_CHECKS; Check++)
    {
        v2 P = From + (BOT_HAZARD_LOOK * (float)Check / (float)BOT_HAZARD_CHECKS) * Way;
        for(i32 Lane = -1; Lane <= 1; Lane++)
        {
            v2 Q = P + ((float)Lane * BOT_HAZARD_SIDE) * Side;
            if (IsHazardAt(World, V3(Q.X, Q.Y, 0.f)))
            {
                return false;
            }
        }
    }
    return true;
}

// NOTE(zoubir): Way turned by Angle radians
inline v2
BotTurn(v2 Way, float Angle)
{
    float C = Cos(Angle);
    float S = Sin(Angle);
    v2 Result = V2(C * Way.X - S * Way.Y, S * Way.X + C * Way.Y);
    return Result;
}

// NOTE(zoubir): in a dungeon run, the middle of the room being fought or
// the next one to clear, until Self is this close to it; false for none
#define BOT_ROOM_ARRIVED 96.f

internal bool32
BotNextRoomGoal(app_state *AppState, world_entity *Self, v2 *Goal)
{
    dungeon_run *Run = AppState->Dungeon;
    if (!IsDungeon(AppState) || !Run || IsDeadPlayer(Self))
    {
        return false;
    }
    u32 Room = Run->FightingRoom ? Run->FightingRoom : NextRoomToClear(Run->RoomStates, Run->RoomCount);
    if (!Room)
    {
        return false;
    }
    // NOTE(zoubir): on into the room, not just over its edge, so its
    // monsters come into sight
    *Goal = Run->RoomMiddle[Room].XY;
    bool32 Result = LengthSq(*Goal - Self->Position.XY) > Square(BOT_ROOM_ARRIVED);
    return Result;
}

// NOTE(zoubir): the way toward the next room (BotNextRoomGoal), round
// what hurts; false for none
internal bool32
BotWalkToNextRoom(app_state *AppState, world_entity *Self, v2 *Way)
{
    v2 Goal;
    if (!BotNextRoomGoal(AppState, Self, &Goal))
    {
        return false;
    }
    v2 Straight = Goal - Self->Position.XY;
    if (LengthSq(Straight) < 1.f)
    {
        return false;
    }
    if (!BotPathWay(&AppState->World, Self->Position.XY, Goal, Way))
    {
        *Way = Straight * (1.f / Length(Straight));
    }
    return true;
}

// NOTE(zoubir): the movement keys of Held changed to keep Self out of
// lava and pits on its way to Goal (0 for none); the other keys stay
internal u32
SteerAroundHazards(app_state *AppState, world_entity *Self, u32 Held, world_entity *Goal)
{
    world *World = &AppState->World;
    v2 From = Self->Position.XY;
    v2 Way = BotKeysWay(Held);
    bool32 InHazard = IsHazardAt(World, V3(From.X, From.Y, 0.f));
    u32 Result = Held;
    if (InHazard)
    {
        // NOTE(zoubir): out by the shortest way of eight
        float Best = BOT_HAZARD_ESCAPE + 1.f;
        v2 BestWay = Way;
        for(u32 Index = 0; Index < 8; Index++)
        {
            v2 Try = V2(Cos(0.25f * Pi32 * (float)Index), Sin(0.25f * Pi32 * (float)Index));
            for(float Out = 16.f; Out < Best; Out += 16.f)
            {
                v2 P = From + Out * Try;
                if (!IsHazardAt(World, V3(P.X, P.Y, 0.f)))
                {
                    Best = Out;
                    BestWay = Try;
                    break;
                }
            }
        }
        if (Best <= BOT_HAZARD_ESCAPE)
        {
            Result = (Held & ~(u32)BOT_MOVE_BUTTONS) | NetButtonsToward(BestWay);
        }
        return Result;
    }
    if (LengthSq(Way) < 0.5f)
    {
        return Result;
    }
    if (BotWayIsClear(World, From, Way))
    {
        return Result;
    }
    Result &= ~(u32)BOT_MOVE_BUTTONS;
    // NOTE(zoubir): walking at its target, or to the next room with none
    // in sight, the bot takes the way round; a bot backing off or
    // kiting does not
    v2 GoalPoint = Goal ? Goal->Position.XY : From;
    bool32 HasGoal = Goal ? DotProduct(Way, DirectionTo(GoalPoint - From)) > 0.5f :
        BotNextRoomGoal(AppState, Self, &GoalPoint);
    v2 PathWay;
    if (HasGoal && BotPathWay(World, From, GoalPoint, &PathWay))
    {
        Result |= NetButtonsToward(PathWay);
        return Result;
    }
    float Turns[] = {0.25f, -0.25f, 0.5f, -0.5f, 0.75f, -0.75f};
    for(u32 Index = 0; Index < ArrayCount(Turns); Index++)
    {
        // NOTE(zoubir): checked as the keys will walk it, eight ways
        u32 Keys = NetButtonsToward(BotTurn(Way, Turns[Index] * Pi32));
        if (BotWayIsClear(World, From, BotKeysWay(Keys)))
        {
            Result |= Keys;
            break;
        }
    }
    return Result;
}
