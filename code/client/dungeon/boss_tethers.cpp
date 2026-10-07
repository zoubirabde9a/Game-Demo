/* Boss tethers (sim/dungeon/boss_scripts.cpp): adds a boss calls back into
   itself are tied to it by a stream of motes running from the add to the
   boss, so the party sees which monsters to kill before they heal it, and
   not only the countdown on the HUD. The stream turns the alarm colour
   and beats in the last BOSS_TETHER_ALARM_SECONDS, and an add that erupts
   on the party when it returns (the Hollow King's champion) is always
   drawn in it.

   Which adds return is read from BossEvents for the shown boss's kind and
   the time left from the run's shown add seconds, both of which a client
   has online, so it needs nothing new in the snapshots. Every living
   monster of a returning kind in the fight's room is tethered.

   Entry point: DrawBossTethers, once a frame from screen_pass.inc. */

#define BOSS_TETHER_MOTES 12
#define BOSS_TETHER_ALARM_SECONDS 5
#define BOSS_TETHER_COLOR UI_RGBA(190, 170, 255, 230)
#define BOSS_TETHER_ALARM_COLOR UI_RGBA(255, 90, 70, 235)

// NOTE(zoubir): whether adds of Kind return into a boss of BossKind, and
// whether they erupt when they do; a row with no affix takes an add of
// that kind whatever affix it rolled
internal bool32
IsReturningAdd(u32 BossKind, u32 Kind, u32 Affix, bool32 *Erupts)
{
    bool32 Result = false;
    *Erupts = false;
    for(u32 Index = 0; Index < ArrayCount(BossEvents); Index++)
    {
        boss_event *Event = &BossEvents[Index];
        if ((u32)Event->Boss == BossKind && (u32)Event->AddKind == Kind &&
            Event->MergeSeconds > 0.f && (!Event->Affix || Event->Affix == Affix))
        {
            Result = true;
            *Erupts = *Erupts || Event->BurstShare > 0.f;
        }
    }
    return Result;
}

internal void
DrawBossTethers(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    dungeon_run *Run = AppState->Dungeon;
    if (!Run || Run->ShownBossKind >= MonsterKind_Count || !Run->Clock.ShownAddSeconds)
    {
        return;
    }
    world *World = &AppState->World;
    world_entity *Boss = 0;
    for(u32 Index = 0; Index < World->EntityCount && !Boss; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster && Entity->Hp > 0.f &&
            (u32)Entity->MonsterKind == Run->ShownBossKind)
        {
            Boss = Entity;
        }
    }
    if (!Boss)
    {
        return;
    }
    float Clock = GetFxClock(AppState);
    bool32 Alarm = Run->Clock.ShownAddSeconds <= BOSS_TETHER_ALARM_SECONDS;
    v2 BossAt = Boss->Position.XY - CameraOffset.XY;
    BossAt.Y -= 0.5f * Boss->Dimensions.Y;
    u32 BossRoom = RoomAtPosition(World, Boss->Position.XY);
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Add = &World->Entities[Index];
        bool32 Erupts;
        if (!Add->IsPresent || Add->Type != EntityType_Monster || Add->Hp <= 0.f ||
            Add == Boss ||
            !IsReturningAdd(Run->ShownBossKind, (u32)Add->MonsterKind, Add->EliteAffix, &Erupts) ||
            RoomAtPosition(World, Add->Position.XY) != BossRoom)
        {
            continue;
        }
        bool32 Hot = Alarm || Erupts;
        u32 Color = Hot ? BOSS_TETHER_ALARM_COLOR : BOSS_TETHER_COLOR;
        float Beat = Hot ? 0.6f + 0.4f * Absolute(Sin(6.f * Clock)) : 1.f;
        v2 AddAt = Add->Position.XY - CameraOffset.XY;
        AddAt.Y -= 0.5f * Add->Dimensions.Y;
        // NOTE(zoubir): motes run from the add into the boss, faster as
        // the return gets close
        float Speed = Alarm ? 1.4f : 0.6f;
        for(u32 Mote = 0; Mote < BOSS_TETHER_MOTES; Mote++)
        {
            float T = fmodf((float)Mote / (float)BOSS_TETHER_MOTES + Speed * Clock, 1.f);
            v2 P = Lerp2(AddAt, T, BossAt);
            // NOTE(zoubir): a little arc, so the stream reads as drawn in
            P.Y -= 18.f * Sin(Pi32 * T);
            float Size = 3.f + 3.f * Sin(Pi32 * T);
            DrawFilledRectangle(RenderContext, P.X - 0.5f * Size, P.Y - 0.5f * Size,
                                Size, Size, WithAlpha(Color, Beat), 0.f);
        }
    }
}
