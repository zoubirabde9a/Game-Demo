/* Stormcaller keep tests (tests/stormcaller_tests.cpp, which has the
   helpers): the keep bursts a Static Field and a Ball Lightning send
   while they last, so a client that came near late draws them; a keep's
   variant round trip; and Ball Lightning's zaps as bolts from the ball. */

// NOTE(zoubir): the keep bursts of Kind in the queue, the newest in *Last;
// empties the queue
internal u32
TakeStormKeeps(app_state *AppState, u32 Kind, sim_event *Last)
{
    u32 Result = 0;
    sim_burst Burst = ClassBurst(SimBurst_StormcallerFirst, StormcallerBurst_Supercharged);
    for(u32 Index = 0; Index < AppState->Events.Count; Index++)
    {
        sim_event *Event = &AppState->Events.Events[Index];
        u32 Variant = StormcallerBurstVariant(Event->Position);
        if (Event->Type == SimEvent_Burst && Event->Burst == Burst && Variant &&
            StormcallerKeepKind(Variant) == Kind)
        {
            Result++;
            *Last = *Event;
        }
    }
    AppState->Events.Count = 0;
    return Result;
}

// NOTE(zoubir): a keep's variant carries its kind, seconds left and
// radius. A Static Field says again every STORMCALLER_KEEP_SECONDS where
// it is, so a client too far for its landing still draws it; a Ball
// Lightning does from its cast on, with its way and the seconds it has
// left, and its zaps are bolts from the ball
internal void
TestStormcallerKeeps()
{
    u32 Variant = StormcallerKeepVariant(StormcallerKeep_Ball, 2.96f, 82.5f);
    Check(StormcallerKeepKind(Variant) == StormcallerKeep_Ball);
    Check(Absolute(StormcallerKeepSeconds(Variant) - 3.f) < 0.01f);
    Check(Absolute(StormcallerKeepRadius(Variant) - 82.5f) <= 2.f);
    Check(StormcallerBurstVariant(StormcallerBurstSpot(V3(0.f, 0.f, 40.f), Variant)) == Variant);
    Check(StormcallerBoltKind(StormcallerBoltVariant(StormcallerBolt_Ball, 120.f)) == StormcallerBolt_Ball);

    crypt_world Crypt = CreateStormWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    storm_dummies Dummies = {};
    PressOnce(&Crypt, 0, PlayerButton_Push);
    stormcaller_field *Field = &AppState->Dungeon->Stormcaller.Fields[0];
    Check(Field->Seconds > 0.f);
    AppState->Events.Count = 0;
    u32 Keeps = 0;
    sim_event Last = {};
    for(u32 Tick = 0; Tick < (u32)(60.f * STATIC_FIELD_SECONDS) + 10; Tick++)
    {
        StormTick(&Crypt, &Dummies, 1);
        Keeps += TakeStormKeeps(AppState, StormcallerKeep_Field, &Last);
    }
    // NOTE(zoubir): one a second after the landing; the last may fall on
    // the tick the field ends
    u32 Seconds = (u32)(STATIC_FIELD_SECONDS / STORMCALLER_KEEP_SECONDS);
    Check(Keeps + 1 >= Seconds && Keeps <= Seconds);
    Check(Length(StormcallerBurstPlace(Last.Position).XY - Field->Position.XY) < 0.01f);
    Check(Absolute(StormcallerKeepRadius(StormcallerBurstVariant(Last.Position)) - STATIC_FIELD_RADIUS) <= 2.f);
    DestroyCryptWorld(&Crypt);

    Crypt = CreateStormWorld();
    AppState = Crypt.AppState;
    Slot = &AppState->Players[0];
    Slot->Entity->Aim = V2(1.f, 0.f);
    Dummies = {};
    StormDummy(&Crypt, &Dummies, V3(150.f, 0.f, 0.f));
    AppState->Events.Count = 0;
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Keeps = TakeStormKeeps(AppState, StormcallerKeep_Ball, &Last);
    Check(Keeps == 1);
    Check(Absolute(StormcallerKeepSeconds(StormcallerBurstVariant(Last.Position)) - BALL_LIGHTNING_SECONDS) < 0.11f);
    Check(Absolute(Last.Angle) < 0.01f);
    u32 Zaps = 0;
    sim_burst Bolt = ClassBurst(SimBurst_StormcallerFirst, StormcallerBurst_Bolt);
    for(u32 Tick = 0; Tick < (u32)(60.f * BALL_LIGHTNING_SECONDS) + 10; Tick++)
    {
        StormTick(&Crypt, &Dummies, 1);
        for(u32 Index = 0; Index < AppState->Events.Count; Index++)
        {
            sim_event *Event = &AppState->Events.Events[Index];
            Zaps += Event->Type == SimEvent_Burst && Event->Burst == Bolt &&
                StormcallerBoltKind(StormcallerBurstVariant(Event->Position)) == StormcallerBolt_Ball;
        }
        Keeps += TakeStormKeeps(AppState, StormcallerKeep_Ball, &Last);
    }
    Seconds = (u32)(BALL_LIGHTNING_SECONDS / STORMCALLER_KEEP_SECONDS);
    Check(Keeps >= Seconds && Keeps <= Seconds + 1);
    Check(Zaps > 0);
    DestroyCryptWorld(&Crypt);
}
