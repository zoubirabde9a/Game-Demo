/* World tick (online.cpp): the frame's one world update. Online the
   server's snapshot drives the world through the replicas; offline the
   local simulation does; it switches between the two when the
   connection comes up or ends. Desktop only: the browser build has its
   own RunWorldTick in online.cpp, which only simulates. */

// NOTE(zoubir): in client/rewind_fx/rewind_fx.cpp, included later
internal void ReadRewindsFromSnapshot(app_state *AppState, replica_table *Replicas,
                                      net_snapshot *Snapshot);

// NOTE(zoubir): the frame's one world update. Online the server's
// snapshot drives the world; offline the local simulation does. Switches
// between the two when the connection comes up or ends.
internal void
RunWorldTick(app_state *AppState, memory_arena *Arena, float DeltaTime)
{
    online_session *Online = AppState->Online;
    if (IsOnline(Online) && Online->Client.HasSnapshot)
    {
        net_snapshot *Snapshot = &Online->Client.Snapshot;
        // NOTE(zoubir): joining a server on another map, or a map vote
        // there moving everyone: build its ground first, and the replicas
        // again from this snapshot. Terrain is never sent, both sides
        // generate it from the id
        u32 MapId = Snapshot->MapId;
        if (MapId < MapId_Count && MapId != AppState->World.MapId)
        {
            RebuildWorldForMap(AppState, Arena, MapId);
            ZeroSize(&Online->Replicas, sizeof(Online->Replicas));
            Online->Prediction = {};
        }
        bool32 NewSnapshot = !Online->Replicas.Active ||
            Snapshot->Tick != Online->Replicas.LastAppliedTick;
        if (NewSnapshot)
        {
            // NOTE(zoubir): inputs waiting on the server already arrived,
            // so they are not part of the round trip
            RecordSnapshotQuality(&Online->Quality, Snapshot->Tick,
                                  Online->Client.InputTick,
                                  Snapshot->InputTick + Snapshot->InputBuffered);
            NotePacingSnapshot(&Online->Pacing, Snapshot->InputBuffered);
        }
        SyncReplicas(AppState, Arena, &Online->Replicas, Snapshot, DeltaTime,
                     Online->Client.PlayerIndex);
        // NOTE(zoubir): the server's sounds go where the local game's go;
        // PlaySimEvents plays them after this tick
        for(u32 Index = 0; NewSnapshot && Index < Snapshot->SoundCount; Index++)
        {
            if (Snapshot->Sounds[Index] < AssetType_Count)
            {
                EmitSound(&AppState->Events,
                          (asset_type_id)Snapshot->Sounds[Index], V3(0.f));
            }
        }
        for(u32 Index = 0; NewSnapshot && Index < Snapshot->KillCount; Index++)
        {
            net_kill *Kill = &Snapshot->Kills[Index];
            EmitKill(&AppState->Events, Kill->Killer, Kill->Victim,
                     Kill->KillerMonster);
        }
        for(u32 Index = 0; NewSnapshot && Index < Snapshot->BurstCount; Index++)
        {
            net_burst *Burst = &Snapshot->Bursts[Index];
            if (!IsBurstPredictedHere(Burst->Kind, Burst->Slot,
                                      Online->Client.PlayerIndex))
            {
                EmitBurst(&AppState->Events, (sim_burst)Burst->Kind,
                          Burst->Slot, V3(Burst->X, Burst->Y, Burst->Z),
                          (float)Burst->Angle * (Pi32 / 128.f));
            }
        }
        // NOTE(zoubir): before prediction, which leaves a player a time
        // rewind froze where the server has it (client/rewind_fx/)
        if (NewSnapshot)
        {
            ReadRewindsFromSnapshot(AppState, &Online->Replicas, Snapshot);
        }
        PredictLocalPlayer(AppState, Arena, &Online->Prediction, NewSnapshot,
                           Snapshot->InputTick, Online->NewTicks,
                           OnlinePacingBlend(&Online->Pacing), DeltaTime);
        return;
    }
    if (Online && Online->Replicas.Active)
    {
        LeaveReplicaWorld(AppState, Arena, &Online->Replicas);
        Online->Prediction = {};
        // NOTE(zoubir): the history was of the world before the server's
        ResetTimeRewind(AppState->Rewind);
    }
    SimulateTick(AppState, Arena, DeltaTime);
    ApplyDeveloperStart(AppState, Arena);
    ApplyDeveloperMarks(AppState);
    ApplyDeveloperBossHealth(AppState);
}
