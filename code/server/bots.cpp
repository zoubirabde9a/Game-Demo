/* Bot players: the server fills player slots no human is using with bots,
   so a lone player has someone to fight and online play can be tried with
   one client. A bot is a player slot whose input comes from BotThink
   instead of the network: it sends the same held buttons and aim a client
   would, through GameApplyInput, so every rule (cooldowns, damage, score)
   is the simulation's own. A human joining takes the slot over.

   The brain is small on purpose: chase the nearest living player or
   monster in sight, swing the sword up close, throw fireballs from range,
   dash now and then, and wander when nothing is near. Included by
   sim_game.cpp; GameKeepBots runs it every tick. */

#define BOT_SIGHT 700.f
#define BOT_SWORD_RANGE 70.f
#define BOT_FIREBALL_RANGE 380.f

// NOTE(zoubir): the movement keys a player would hold to go along
// Direction (8 ways)
inline u32
NetButtonsToward(v2 Direction)
{
    u32 Result = 0;
    if (Direction.X > 0.38f) Result |= NetButton_Right;
    if (Direction.X < -0.38f) Result |= NetButton_Left;
    if (Direction.Y > 0.38f) Result |= NetButton_Down;
    if (Direction.Y < -0.38f) Result |= NetButton_Up;
    return Result;
}

struct bot_brain
{
    bool32 Active;
    u32 Random;
    u32 Held;          // buttons held this tick
    float AttackWait;  // seconds until the next attack
    float WanderLeft;  // seconds until a new wander direction
    v2 Wander;
};

inline u32
BotRandom(bot_brain *Bot)
{
    u32 X = Bot->Random;
    X ^= X << 13; X ^= X >> 17; X ^= X << 5;
    Bot->Random = X;
    return X;
}

// NOTE(zoubir): the nearest living player (not itself) or monster within
// BOT_SIGHT, or 0
internal world_entity *
BotFindTarget(app_state *AppState, world_entity *Self)
{
    world *World = &AppState->World;
    world_entity *Best = 0;
    float BestSq = Square(BOT_SIGHT);
    for (u32 Index = 0; Index < World->EntityCount; ++Index)
    {
        world_entity *Other = &World->Entities[Index];
        if (!Other->IsPresent || Other == Self) continue;
        if (Other->Type != EntityType_Player && Other->Type != EntityType_Monster) continue;
        if (IsDeadPlayer(Other)) continue;
        float DistanceSq = LengthSq(Other->Position.XY - Self->Position.XY);
        if (DistanceSq < BestSq)
        {
            BestSq = DistanceSq;
            Best = Other;
        }
    }
    return Best;
}

// NOTE(zoubir): one tick of a bot's thinking, as the input a client would send
internal net_input
BotThink(bot_brain *Bot, app_state *AppState, world_entity *Self, u32 Tick, float Dt)
{
    net_input Input = {};
    Input.Tick = Tick;
    // Buttons are pressed for one tick, then let go, so each one fires once.
    u32 Held = 0;
    Bot->AttackWait -= Dt;
    Bot->WanderLeft -= Dt;

    v2 Direction = {};
    world_entity *Target = Self ? BotFindTarget(AppState, Self) : 0;
    if (Target)
    {
        v2 Delta = Target->Position.XY - Self->Position.XY;
        float Distance = Length(Delta);
        Direction = (Distance > 0.001f) ? Delta * (1.f / Distance) : V2(1.f, 0.f);
        if (Distance > BOT_SWORD_RANGE * 0.6f) Held |= NetButtonsToward(Direction);
        if (Bot->AttackWait <= 0.f)
        {
            if (Distance < BOT_SWORD_RANGE)
            {
                Held |= NetButton_Sword;
                Bot->AttackWait = 0.4f + (BotRandom(Bot) % 40) / 100.f;
            }
            else if (Distance < BOT_FIREBALL_RANGE)
            {
                Held |= NetButton_Fireball;
                Bot->AttackWait = 0.9f + (BotRandom(Bot) % 60) / 100.f;
            }
        }
        if (BotRandom(Bot) % 400 == 0) Held |= NetButton_Dash;
        // NOTE(zoubir): badly hurt, a bot may rewind itself to before the
        // hits; up close, now and then, it rewinds the fight around it.
        // Never the whole world: on a live server that is the players' call
        if (Self->Hp < 0.35f * Self->MaxHp && BotRandom(Bot) % 60 == 0)
        {
            Held |= NetButton_RewindSelf;
        }
        if (Distance < BOT_SWORD_RANGE * 2.f && BotRandom(Bot) % 900 == 0)
        {
            Held |= NetButton_RewindBubble;
        }
    }
    else
    {
        if (Bot->WanderLeft <= 0.f)
        {
            u32 Pick = BotRandom(Bot) % 5;
            v2 Ways[] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            Bot->Wander = Ways[Pick];
            Bot->WanderLeft = 1.5f + (BotRandom(Bot) % 150) / 100.f;
        }
        Direction = Bot->Wander;
        Held |= NetButtonsToward(Direction);
    }

    // A press needs the button up the tick before; drop repeats.
    Held &= ~(Bot->Held & (NetButton_Sword | NetButton_Fireball | NetButton_Dash |
                           NetButton_RewindSelf | NetButton_RewindBubble));
    Bot->Held = Held;
    Input.Buttons = Held;
    Input.AimX = Direction.X;
    Input.AimY = Direction.Y;
    return Input;
}
