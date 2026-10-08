/* Key presses (role_abilities.cpp): UseRoleAbilities, the one place a
   class key turns into a cast. A key pressed while another spell winds
   up (a Meteor, a Whirlwind, an Execute) used to be dropped, so the key
   seemed not to work; now the press is kept until the wind-up ends and
   goes off then, one key at a time, the newest press winning. A key that
   cannot cast for its own reasons (short of Rage, no foe in reach) is
   tried again each tick for ROLE_QUEUE_SECONDS. A key still cooling down
   is as before: pressed inside ROLE_EARLY_PRESS_SECONDS of ready it
   casts, the rest added to the next cooldown, and pressed earlier it is
   dropped, so a double click does not swing twice. */

// NOTE(zoubir): a class key pressed with this much of its cooldown left
// still casts, the rest added to the next cooldown, so the rate holds.
// The same 0.25 s the game's sword and fireball keep a press for
// (PLAYER_ACTION_LINGER): without it a click a moment early was lost, and
// a Shadowblade's half-second Twin Strike dropped most of a player's clicks
#define ROLE_EARLY_PRESS_SECONDS 0.25f
// NOTE(zoubir): how long a kept press waits once no spell winds up; the
// clock stops during a wind-up
#define ROLE_QUEUE_SECONDS 0.5f

// NOTE(zoubir): Key's spell for Slot's class, now; true when it cast
internal bool32
CastRoleKey(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
            world_entity *Player, u32 Key)
{
    bool32 Result = false;
    switch(Slot->Role)
    {
        case PlayerRole_Tank: Result = CastTankKey(AppState, World, Arena, Slot, Player, Key); break;
        case PlayerRole_Healer: Result = CastHealerKey(AppState, Slot, Player, Key); break;
        case PlayerRole_Damage: Result = CastStrikerKey(AppState, Slot, Player, Key); break;
        default: Result = CastClassKey(AppState, World, Arena, Slot, Player, Key); break;
    }
    return Result;
}

// NOTE(zoubir): from UpdatePlayer, before the game's abilities
internal void
UseRoleAbilities(app_state *AppState, world *World, memory_arena *Arena,
                 player_slot *Slot, float DeltaTime)
{
    world_entity *Player = Slot->Entity;
    for(u32 Key = 0; Key < ROLE_KEYS; Key++)
    {
        Slot->RoleCooldowns[Key] = Maximum(0.f, Slot->RoleCooldowns[Key] - DeltaTime);
    }
    Slot->ShieldWallSeconds = Maximum(0.f, Slot->ShieldWallSeconds - DeltaTime);
    bool32 Casting = IsPlayerCasting(Player);
    for(u32 Key = 0; Key < ROLE_KEYS; Key++)
    {
        if (!RoleOwnsKey(AppState, Slot, Key))
        {
            continue;
        }
        u32 Button = RoleKeys[Key];
        bool32 Pressed = WasPressed(&Slot->Input, Button);
        Slot->Input.Pressed &= ~Button;
        Slot->Input.ServerPressed &= ~Button;
        // NOTE(zoubir): a client predicting its own player starts only the
        // wind-ups (the pose, the slowdown, the bar); the rest waits for
        // the server
        if (Pressed && !(Slot->Predicted && !RoleKeyWindsUp(Slot, Key)) &&
            !IsDeadPlayer(Player) && RoleSpellLearned(Slot, Key) &&
            (Casting || Slot->RoleCooldowns[Key] <= ROLE_EARLY_PRESS_SECONDS))
        {
            Player->QueuedRoleKey = Key + 1;
            Player->QueuedRoleSeconds = ROLE_QUEUE_SECONDS;
        }
    }
    if (!Player->QueuedRoleKey)
    {
        return;
    }
    u32 Key = Player->QueuedRoleKey - 1;
    if (IsDeadPlayer(Player) || !RoleOwnsKey(AppState, Slot, Key) || !RoleSpellLearned(Slot, Key))
    {
        Player->QueuedRoleKey = 0;
        return;
    }
    // NOTE(zoubir): one cast at a time: the key waits while a spell winds up
    if (!Casting && Slot->RoleCooldowns[Key] <= ROLE_EARLY_PRESS_SECONDS)
    {
        float Early = Slot->RoleCooldowns[Key];
        if (CastRoleKey(AppState, World, Arena, Slot, Player, Key))
        {
            // NOTE(zoubir): a cast may give part of its cooldown back
            // (CastRefund)
            Slot->RoleCooldowns[Key] = Maximum(0.f, Early + RoleSpellCooldown(Slot, Key) - Slot->CastRefund);
            Slot->CastRefund = 0.f;
            Player->QueuedRoleKey = 0;
            return;
        }
    }
    if (!Casting)
    {
        Player->QueuedRoleSeconds -= DeltaTime;
        if (Player->QueuedRoleSeconds <= 0.f)
        {
            Player->QueuedRoleKey = 0;
        }
    }
}
