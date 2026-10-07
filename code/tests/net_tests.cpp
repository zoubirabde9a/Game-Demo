/* Packet format tests: every packet type survives a write and read, and
   malformed datagrams are rejected. Builds on its own, without the game,
   which also proves code/net has no dependency on rendering or assets.
   Run test.bat from the repo root; exit code 0 means every check passed. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../net/protocol.cpp"
#include "../net/connections.cpp"
#include "../net/socket.cpp"

global_variable int TestFailures;
global_variable int TestChecks;

#define Check(Expression) CheckImpl((Expression) != 0, #Expression, __LINE__)

internal void
CheckImpl(bool Passed, const char *Expression, int Line)
{
    TestChecks++;
    if (!Passed)
    {
        TestFailures++;
        printf("  FAILED net_tests.cpp(%d): %s\n", Line, Expression);
    }
}

internal net_packet
RoundTrip(net_packet *In, u32 *SizeOut)
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    net_packet Out = {};
    u32 Size = NetWritePacket(In, Buffer, sizeof(Buffer));
    Check(Size > 0);
    Check(NetReadPacket(Buffer, Size, &Out));
    if (SizeOut) *SizeOut = Size;
    return Out;
}

internal net_packet
FullSnapshot()
{
    net_packet P = {};
    P.Header = {NetPacket_Snapshot, 7, 3};
    P.Snapshot.Tick = 123456;
    P.Snapshot.InputTick = 0xfedcba98;
    P.Snapshot.Count = NET_MAX_SNAPSHOT_ENTITIES;
    for (u16 Index = 0; Index < NET_MAX_SNAPSHOT_ENTITIES; ++Index)
    {
        net_entity_state *E = &P.Snapshot.Entities[Index];
        E->Id = (u16)(1000 + Index);
        E->Type = (u8)(Index % 5);
        E->Facing = 1;
        E->Animation = 2;
        E->Affix = (u8)(Index % 5);
        E->Status = (u8)(Index % 8);
        E->Ability = (u8)(Index % 3);
        E->Variant = (u8)(Index % 7);
        E->Health = (i16)(Index - 10);
        E->X = 12.5f * Index;
        E->Y = -3.25f;
        E->VelX = 0.1f;
        E->VelY = -900.0f;
    }

    // Worst case: every ability slot used, every one with all its points.
    P.Snapshot.AbilityCount = NET_MAX_SNAPSHOT_ABILITIES;
    for (u8 Index = 0; Index < NET_MAX_SNAPSHOT_ABILITIES; ++Index)
    {
        net_ability_state *A = &P.Snapshot.Abilities[Index];
        A->EntityIndex = (u8)(NET_MAX_SNAPSHOT_ENTITIES - 1 - Index);
        A->Phase = 1;
        A->Ability = Index;
        A->TimeLeft = 0.75f;
        A->AimX = 0.6f;
        A->AimY = -0.8f;
        A->PointCount = NET_MAX_ABILITY_POINTS;
        for (u32 Point = 0; Point < NET_MAX_ABILITY_POINTS; ++Point)
        {
            A->PointX[Point] = 100.0f * Point + 0.125f;
            A->PointY[Point] = 2000.0f;
        }
    }

    // ...and every player slot's score.
    P.Snapshot.ScoreCount = NET_MAX_SNAPSHOT_SCORES;
    for (u8 Index = 0; Index < NET_MAX_SNAPSHOT_SCORES; ++Index)
    {
        net_score *Score = &P.Snapshot.Scores[Index];
        Score->Slot = Index;
        Score->Kills = (u16)(Index * 3);
        Score->Deaths = 65535;
        Score->MonsterKills = (u16)(1000 + Index);
        Score->Level = (u8)(Index * 2 + 1);
        Score->Ward = Index & 1;
        Score->Dungeon = (u8)(0x91 + Index);
    }
    P.Snapshot.Xp = 4321;
    for (u8 Index = 0; Index < NET_TALENT_COUNT; ++Index)
    {
        P.Snapshot.TalentRanks[Index] = (u8)(Index % 4);
    }

    // ...and every facing slot...
    P.Snapshot.FacingCount = NET_MAX_SNAPSHOT_FACINGS;
    for (u8 Index = 0; Index < NET_MAX_SNAPSHOT_FACINGS; ++Index)
    {
        P.Snapshot.Facings[Index].EntityIndex = Index;
        P.Snapshot.Facings[Index].Angle = (u8)(Index * 32 + 1);
    }
    P.Snapshot.SoundCount = NET_MAX_SNAPSHOT_SOUNDS;
    for (u8 Index = 0; Index < NET_MAX_SNAPSHOT_SOUNDS; ++Index)
    {
        P.Snapshot.Sounds[Index] = (u8)(200 + Index);
    }
    // One kill: with more kill slots full as well it no longer fits in a
    // datagram (TestOverfullSnapshotIsTrimmed).
    P.Snapshot.KillCount = 1;
    for (u8 Index = 0; Index < NET_MAX_SNAPSHOT_KILLS; ++Index)
    {
        P.Snapshot.Kills[Index].Killer = Index;
        P.Snapshot.Kills[Index].Victim = (u8)(Index + 1);
        P.Snapshot.Kills[Index].KillerMonster = 0xFF;
    }
    P.Snapshot.Stagger = 0x5a;
    P.Snapshot.RoundBreak = 0x63;
    P.Snapshot.VoteMap = 2;
    P.Snapshot.VoteBy = 5;
    P.Snapshot.VoteSeconds = 27;
    P.Snapshot.VoteYes = 3;
    P.Snapshot.VoteNo = 1;
    P.Snapshot.OwnVote = 2;
    P.Snapshot.MapId = 2;
    P.Snapshot.HasDungeon = 1;
    P.Snapshot.FightingRoom = 5;
    P.Snapshot.RoomsCleared = 0x0f;
    P.Snapshot.Wipes = 3;
    P.Snapshot.BossKind = 14;
    P.Snapshot.BossHealth = 200;
    P.Snapshot.FoesLeft = 4;
    P.Snapshot.BossClock = 37;
    P.Snapshot.SanctuaryCount = NET_MAX_SANCTUARIES;
    for (u32 Index = 0; Index < NET_MAX_SANCTUARIES; ++Index)
    {
        P.Snapshot.SanctuaryX[Index] = (i16)(800 * Index - 3);
        P.Snapshot.SanctuaryY[Index] = (i16)(1500 - 300 * Index);
        P.Snapshot.SanctuaryTenths[Index] = (u8)(41 + Index);
    }
    P.Snapshot.InfernoCount = NET_MAX_INFERNOS;
    for (u32 Index = 0; Index < NET_MAX_INFERNOS; ++Index)
    {
        P.Snapshot.InfernoX[Index] = (i16)(-600 * Index + 7);
        P.Snapshot.InfernoY[Index] = (i16)(250 * Index - 9);
        P.Snapshot.InfernoTenths[Index] = (u8)(0x80 | 0x40 | Index);
    }
    // One burst, for the same reason.
    P.Snapshot.BurstCount = 1;
    P.Snapshot.Bursts[0].Kind = 2;
    P.Snapshot.Bursts[0].Slot = 0xFF;
    P.Snapshot.Bursts[0].Angle = 200;
    P.Snapshot.Bursts[0].X = -40.5f;
    P.Snapshot.Bursts[0].Y = 812.25f;
    P.Snapshot.Bursts[0].Z = 33.f;
    // One rewind, for the same reason.
    P.Snapshot.RewindCount = 1;
    P.Snapshot.Rewinds[0].Slot = 6;
    P.Snapshot.Rewinds[0].Kind = 1;
    P.Snapshot.Rewinds[0].Phase = 2;
    P.Snapshot.Rewinds[0].PhaseLeft = 0.36f;
    P.Snapshot.Rewinds[0].X = 512.5f;
    P.Snapshot.Rewinds[0].Y = -77.25f;
    P.Snapshot.Rewinds[0].Radius = 160.f;
    P.Snapshot.Rewinds[0].Frozen[0] = 0x81;
    P.Snapshot.Rewinds[0].Frozen[5] = 0x04; // entity 42, the last of 43
    // Every player winding up a spell, pointing at the last (farthest)
    // entities so a trimmed snapshot has to drop them.
    P.Snapshot.CastCount = NET_MAX_SNAPSHOT_CASTS;
    for (u32 Index = 0; Index < NET_MAX_SNAPSHOT_CASTS; ++Index)
    {
        net_player_cast *Cast = &P.Snapshot.Casts[Index];
        Cast->EntityIndex = (u8)(NET_MAX_SNAPSHOT_ENTITIES - 1 - Index);
        Cast->Spell = (u8)(1 + Index % 7);
        Cast->Done = (u8)(Index * 30 + 5);
    }

    // ...and the longest name.
    P.Snapshot.NameSlot = NET_MAX_SNAPSHOT_SCORES - 1;
    snprintf(P.Snapshot.Name, NET_NAME_SIZE, "%s", "ABCDEFGHIJKLMNO");
    return P;
}

internal void
TestConnectRoundTrip()
{
    net_packet In = {};
    In.Header = {NetPacket_ConnectAccepted, 65535, 1};
    In.ConnectAccepted = {0xdeadbeef, 3, 99};
    net_packet Out = RoundTrip(&In, 0);
    Check(Out.Header.Type == NetPacket_ConnectAccepted);
    Check(Out.Header.Sequence == 65535);
    Check(Out.Header.Ack == 1);
    Check(Out.ConnectAccepted.ClientSalt == 0xdeadbeef);
    Check(Out.ConnectAccepted.PlayerIndex == 3);
    Check(Out.ConnectAccepted.ServerTick == 99);
}

internal void
TestNamesRoundTripAndAreCleaned()
{
    net_packet In = {};
    In.Header.Type = NetPacket_ConnectRequest;
    In.ConnectRequest.ClientSalt = 5;
    snprintf(In.ConnectRequest.Name, NET_NAME_SIZE, "Zoubir");
    net_packet Out = RoundTrip(&In, 0);
    Check(strcmp(Out.ConnectRequest.Name, "Zoubir") == 0);

    // Control characters and bytes above 126 become '?'.
    In.ConnectRequest.Name[1] = '\t';
    In.ConnectRequest.Name[2] = (char)200;
    Out = RoundTrip(&In, 0);
    Check(strcmp(Out.ConnectRequest.Name, "Z??bir") == 0);

    // A forged length longer than the buffer is refused.
    u8 Buffer[NET_MAX_PACKET_SIZE];
    u32 Size = NetWritePacket(&In, Buffer, sizeof(Buffer));
    Check(Size > 0);
    Buffer[Size - 7] = NET_NAME_SIZE; // the name's length byte
    net_packet Bad;
    Check(!NetReadPacket(Buffer, Size, &Bad));

    // No name at all is fine.
    In.ConnectRequest.Name[0] = 0;
    Out = RoundTrip(&In, 0);
    Check(Out.ConnectRequest.Name[0] == 0);
}

internal void
TestPackedEntityFields()
{
    net_packet In = {};
    In.Header.Type = NetPacket_Snapshot;
    In.Snapshot.Count = 2;
    In.Snapshot.NameSlot = NET_NO_NAME_SLOT;
    net_entity_state *A = &In.Snapshot.Entities[0];
    A->Facing = 3; A->Animation = 6; A->Affix = 4; A->Status = 5; A->Ability = 2;
    net_entity_state *B = &In.Snapshot.Entities[1];
    B->Facing = 1; B->Animation = 15; B->Affix = 7; B->Status = 15; B->Ability = 3;
    net_packet Out = RoundTrip(&In, 0);
    net_entity_state *OutA = &Out.Snapshot.Entities[0];
    net_entity_state *OutB = &Out.Snapshot.Entities[1];
    Check(OutA->Facing == 3 && OutA->Animation == 6);
    Check(OutA->Affix == 4 && OutA->Status == 5 && OutA->Ability == 2);
    Check(OutB->Facing == 1 && OutB->Animation == 15);
    Check(OutB->Affix == 7 && OutB->Status == 15 && OutB->Ability == 3);

    // NOTE(zoubir): every status bit (falling, rooted, ...) survives, and
    // the second status byte is sent only when one of its bits is set
    B->Status = 0x7ff;
    u8 Buffer[NET_MAX_PACKET_SIZE];
    u32 WithMore = NetWritePacket(&In, Buffer, sizeof(Buffer));
    Out = RoundTrip(&In, 0);
    Check(Out.Snapshot.Entities[1].Status == 0x7ff);
    B->Status = 7;
    u32 WithoutMore = NetWritePacket(&In, Buffer, sizeof(Buffer));
    Check(WithMore == WithoutMore + 1);
    B->Status = 15;

    // Values wider than their bits are cut, not spilled into neighbours.
    A->Facing = 5; A->Affix = 9; A->Status = 0; A->Ability = 0;
    Out = RoundTrip(&In, 0);
    Check(Out.Snapshot.Entities[0].Facing == 1);
    Check(Out.Snapshot.Entities[0].Animation == 6);
    Check(Out.Snapshot.Entities[0].Affix == 1);
    Check(Out.Snapshot.Entities[0].Status == 0);

    // NOTE(zoubir): a fresh hit's three bytes go with the unit that has
    // one; without the bit they are not sent, whatever they hold
    A->Hit = 1; A->HitStop = 75; A->HitAngle = 200; A->HitBy = 3; A->HitThrown = 1;
    B->Hit = 0; B->HitStop = 9; B->HitAngle = 9; B->HitBy = 9; B->HitThrown = 1;
    u32 Size = 0;
    Out = RoundTrip(&In, &Size);
    OutA = &Out.Snapshot.Entities[0];
    OutB = &Out.Snapshot.Entities[1];
    Check(OutA->Hit == 1 && OutA->HitStop == 75 && OutA->HitAngle == 200);
    Check(OutA->HitBy == 3 && OutA->HitThrown == 1);
    Check(OutA->Facing == 1 && OutA->Affix == 1);
    Check(OutB->Hit == 0 && OutB->HitStop == 0 && OutB->HitAngle == 0);
    Check(OutB->HitBy == 0 && OutB->HitThrown == 0);
    A->Hit = 0;
    u32 SizeWithout = 0;
    RoundTrip(&In, &SizeWithout);
    Check(Size == SizeWithout + 3);
}

internal void
TestInputRoundTrip()
{
    net_packet In = {};
    In.Header = {NetPacket_Input, 1, 2};
    In.Input.Count = 2;
    In.Input.Inputs[0] = {50, NetButton_Left | NetButton_Shockwave, 0.5f, -1.0f};
    In.Input.Inputs[1] = {49, NetButton_Jump, 3.0f, -7.0f}; // aim out of range
    net_packet Out = RoundTrip(&In, 0);
    Check(Out.Input.Count == 2);
    Check(Out.Input.Inputs[0].Tick == 50);
    Check(Out.Input.Inputs[0].Buttons == (NetButton_Left | NetButton_Shockwave));
    Check(Out.Input.Inputs[0].AimX > 0.4999f && Out.Input.Inputs[0].AimX < 0.5001f);
    Check(Out.Input.Inputs[0].AimY == -1.0f);
    Check(Out.Input.Inputs[1].AimX == 1.0f);
    Check(Out.Input.Inputs[1].AimY == -1.0f);
}

// Every list full at once is more than one datagram: writing it plainly
// fails, and NetWriteSnapshotFitting leaves out the last (farthest)
// entities, and what pointed at them, until it fits.
internal void
TestOverfullSnapshotIsTrimmed()
{
    net_packet P = FullSnapshot();
    P.Snapshot.KillCount = NET_MAX_SNAPSHOT_KILLS;
    // The worst case: every entity in the air, moving and just hit, so
    // none of its fields is left out.
    for (u32 Index = 0; Index < NET_MAX_SNAPSHOT_ENTITIES; ++Index)
    {
        P.Snapshot.Entities[Index].Z = 7.5f;
        P.Snapshot.Entities[Index].Hit = 1;
    }
    static u8 Buffer[NET_MAX_PACKET_SIZE];
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) == 0);
    u32 Dropped = 0;
    u32 Size = NetWriteSnapshotFitting(&P, Buffer, sizeof(Buffer), &Dropped);
    Check(Size > 0 && Size <= NET_MAX_PACKET_SIZE);
    printf("  overfull snapshot: %u of %u entities left out\n", Dropped,
           NET_MAX_SNAPSHOT_ENTITIES);
    // NOTE(zoubir): an airborne entity is 20 bytes since vertical speed
    // travels with its height (it was 18, and 1 or 2 were left out; then
    // 3), 23 with a fresh hit (now 6)
    Check(Dropped >= 1 && Dropped <= 7);
    Check(P.Snapshot.Count == NET_MAX_SNAPSHOT_ENTITIES - Dropped);
    static net_packet Out;
    Check(NetReadPacket(Buffer, Size, &Out));
    Check(Out.Snapshot.KillCount == NET_MAX_SNAPSHOT_KILLS);
    for (u32 Index = 0; Index < Out.Snapshot.AbilityCount; ++Index)
    {
        Check(Out.Snapshot.Abilities[Index].EntityIndex < Out.Snapshot.Count);
    }
    Check(Out.Snapshot.CastCount < NET_MAX_SNAPSHOT_CASTS);
    for (u32 Index = 0; Index < Out.Snapshot.CastCount; ++Index)
    {
        Check(Out.Snapshot.Casts[Index].EntityIndex < Out.Snapshot.Count);
    }
    // One that already fits is written as it is.
    P = FullSnapshot();
    P.Snapshot.Count = 10;
    P.Snapshot.AbilityCount = 0;
    P.Snapshot.FacingCount = 0;
    P.Snapshot.CastCount = 0;
    Check(NetWriteSnapshotFitting(&P, Buffer, sizeof(Buffer), &Dropped) > 0);
    Check(Dropped == 0 && P.Snapshot.Count == 10);
}

internal void
TestFullSnapshotFits()
{
    net_packet In = FullSnapshot();
    u32 Size = 0;
    net_packet Out = RoundTrip(&In, &Size);
    Check(Size <= NET_MAX_PACKET_SIZE);
    Check(Out.Snapshot.Tick == 123456);
    Check(Out.Snapshot.InputTick == 0xfedcba98);
    Check(Out.Snapshot.Stagger == 0x5a);
    Check(Out.Snapshot.RoundBreak == 0x63);
    Check(Out.Snapshot.VoteMap == 2 && Out.Snapshot.VoteBy == 5 &&
          Out.Snapshot.VoteSeconds == 27 && Out.Snapshot.VoteYes == 3 &&
          Out.Snapshot.VoteNo == 1 && Out.Snapshot.OwnVote == 2);
    Check(Out.Snapshot.MapId == 2);
    Check(Out.Snapshot.RewindCount == 1);
    net_rewind *Rewind = &Out.Snapshot.Rewinds[0];
    Check(Rewind->Slot == 6 && Rewind->Kind == 1 && Rewind->Phase == 2);
    Check(Rewind->PhaseLeft > 0.355f && Rewind->PhaseLeft < 0.365f);
    Check(Rewind->X == 512.5f && Rewind->Y == -77.25f && Rewind->Radius == 160.f);
    Check(Rewind->Frozen[0] == 0x81 && Rewind->Frozen[5] == 0x04);
    Check(Out.Snapshot.CastCount == NET_MAX_SNAPSHOT_CASTS);
    net_player_cast *LastCast = &Out.Snapshot.Casts[NET_MAX_SNAPSHOT_CASTS - 1];
    Check(LastCast->EntityIndex == NET_MAX_SNAPSHOT_ENTITIES - NET_MAX_SNAPSHOT_CASTS);
    Check(LastCast->Spell == 1 + (NET_MAX_SNAPSHOT_CASTS - 1) % 7);
    Check(LastCast->Done == (NET_MAX_SNAPSHOT_CASTS - 1) * 30 + 5);
    Check(Out.Snapshot.Count == NET_MAX_SNAPSHOT_ENTITIES);
    net_entity_state *Last = &Out.Snapshot.Entities[NET_MAX_SNAPSHOT_ENTITIES - 1];
    Check(Last->Id == 1000 + NET_MAX_SNAPSHOT_ENTITIES - 1);
    Check(Out.Snapshot.Entities[0].Health == -10);
    Check(Last->Variant == (NET_MAX_SNAPSHOT_ENTITIES - 1) % 7);
    Check(Last->Affix == (NET_MAX_SNAPSHOT_ENTITIES - 1) % 5);
    Check(Last->Status == (NET_MAX_SNAPSHOT_ENTITIES - 1) % 8);
    Check(Last->Ability == (NET_MAX_SNAPSHOT_ENTITIES - 1) % 3);
    Check(Last->VelY == -900.0f);
    Check(Last->X == 12.5f * (NET_MAX_SNAPSHOT_ENTITIES - 1)); // multiples of 1/8 are exact
    Check(Out.Snapshot.Entities[3].Y == -3.25f);

    Check(Out.Snapshot.AbilityCount == NET_MAX_SNAPSHOT_ABILITIES);
    Check(Out.Snapshot.ScoreCount == NET_MAX_SNAPSHOT_SCORES);
    net_score *LastScore = &Out.Snapshot.Scores[NET_MAX_SNAPSHOT_SCORES - 1];
    Check(LastScore->Slot == NET_MAX_SNAPSHOT_SCORES - 1);
    Check(LastScore->Kills == 3 * (NET_MAX_SNAPSHOT_SCORES - 1));
    Check(LastScore->Deaths == 65535);
    Check(LastScore->MonsterKills == 1000 + NET_MAX_SNAPSHOT_SCORES - 1);
    Check(LastScore->Level == 2 * (NET_MAX_SNAPSHOT_SCORES - 1) + 1);
    Check(LastScore->Ward == ((NET_MAX_SNAPSHOT_SCORES - 1) & 1));
    Check(Out.Snapshot.Xp == 4321);
    for (u32 Index = 0; Index < NET_TALENT_COUNT; ++Index)
    {
        Check(Out.Snapshot.TalentRanks[Index] == Index % 4);
    }
    Check(Out.Snapshot.NameSlot == NET_MAX_SNAPSHOT_SCORES - 1);
    Check(Out.Snapshot.FacingCount == NET_MAX_SNAPSHOT_FACINGS);
    Check(Out.Snapshot.SoundCount == NET_MAX_SNAPSHOT_SOUNDS);
    Check(Out.Snapshot.KillCount == 1);
    Check(Out.Snapshot.Kills[0].Victim == 1);
    Check(Out.Snapshot.Kills[0].KillerMonster == 0xFF);
    Check(Out.Snapshot.BurstCount == 1);
    Check(Out.Snapshot.Bursts[0].Kind == 2 && Out.Snapshot.Bursts[0].Slot == 0xFF);
    Check(Out.Snapshot.Bursts[0].Angle == 200);
    Check(Out.Snapshot.Bursts[0].X == -40.5f && Out.Snapshot.Bursts[0].Y == 812.25f);
    Check(Out.Snapshot.Bursts[0].Z == 33.f);
    Check(Out.Snapshot.Sounds[NET_MAX_SNAPSHOT_SOUNDS - 1] == 200 + NET_MAX_SNAPSHOT_SOUNDS - 1);
    Check(Out.Snapshot.Facings[NET_MAX_SNAPSHOT_FACINGS - 1].Angle ==
          (NET_MAX_SNAPSHOT_FACINGS - 1) * 32 + 1);
    Check(strcmp(Out.Snapshot.Name, "ABCDEFGHIJKLMNO") == 0);
    net_ability_state *A = &Out.Snapshot.Abilities[2];
    Check(A->EntityIndex == NET_MAX_SNAPSHOT_ENTITIES - 3);
    Check(A->Phase == 1 && A->Ability == 2);
    Check(A->TimeLeft == 0.75f);
    Check(A->AimX > 0.5999f && A->AimX < 0.6001f);
    Check(A->PointCount == NET_MAX_ABILITY_POINTS);
    Check(A->PointX[3] == 300.125f && A->PointY[3] == 2000.0f);
}

internal void
TestFixedPointPrecisionAndClamping()
{
    net_packet In = {};
    In.Header.Type = NetPacket_Snapshot;
    In.Snapshot.Count = 3;
    In.Snapshot.Entities[0].X = 1234.56f;      // rounds to the nearest 1/8
    In.Snapshot.Entities[0].VelX = -77.3f;     // rounds to the nearest 1/4
    In.Snapshot.Entities[1].X = 99999.0f;      // beyond +-4096 of the viewer: clamped
    In.Snapshot.Entities[1].VelY = -99999.0f;  // beyond +-8192: clamped
    float NotANumber = 0.0f;
    NotANumber = NotANumber / NotANumber;
    In.Snapshot.Entities[2].Z = NotANumber;    // NaN is sent as 0
    net_packet Out = RoundTrip(&In, 0);

    float X = Out.Snapshot.Entities[0].X;
    Check(X > 1234.56f - 0.0626f && X < 1234.56f + 0.0626f);
    float VelX = Out.Snapshot.Entities[0].VelX;
    Check(VelX > -77.3f - 0.126f && VelX < -77.3f + 0.126f);
    Check(Out.Snapshot.Entities[1].X > 4095.0f && Out.Snapshot.Entities[1].X < 4096.0f);
    Check(Out.Snapshot.Entities[1].VelY < -8191.0f && Out.Snapshot.Entities[1].VelY > -8192.0f);
    Check(Out.Snapshot.Entities[2].Z == 0.0f);

    // Positions go from the viewer (the snapshot's origin), so far out on
    // an infinite map they keep their 1/8 unit; only what is more than
    // 4096 from the viewer is cut. The viewer's own body is exact.
    net_packet Far = {};
    Far.Header.Type = NetPacket_Snapshot;
    Far.Snapshot.OriginX = 10000.3f;
    Far.Snapshot.OriginY = -7000.9f;
    Far.Snapshot.Count = 1;
    Far.Snapshot.Entities[0].X = 10012.3f;
    Far.Snapshot.Entities[0].Y = -7004.1f;
    Far.Snapshot.HasOwnBody = 1;
    Far.Snapshot.OwnPosition[0] = 10000.3f;
    Far.Snapshot.OwnVelocity[1] = -123.456f;
    net_packet Back = RoundTrip(&Far, 0);
    Check(Back.Snapshot.Entities[0].X > 10012.3f - 0.0626f &&
          Back.Snapshot.Entities[0].X < 10012.3f + 0.0626f);
    Check(Back.Snapshot.Entities[0].Y > -7004.1f - 0.0626f &&
          Back.Snapshot.Entities[0].Y < -7004.1f + 0.0626f);
    Check(Back.Snapshot.HasOwnBody == 1);
    Check(Back.Snapshot.OwnPosition[0] == 10000.3f);
    Check(Back.Snapshot.OwnVelocity[1] == -123.456f);
}

internal void
TestRejectsAbilityForMissingEntity()
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    net_packet P = {};
    P.Header.Type = NetPacket_Snapshot;
    P.Snapshot.Count = 2;
    P.Snapshot.AbilityCount = 1;
    P.Snapshot.Abilities[0].EntityIndex = 2; // only 0 and 1 exist
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) == 0);

    P.Snapshot.Abilities[0].EntityIndex = 1;
    P.Snapshot.Abilities[0].PointCount = NET_MAX_ABILITY_POINTS + 1;
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) == 0);

    P.Snapshot.Abilities[0].PointCount = 1;
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) > 0);

    P.Snapshot.AbilityCount = NET_MAX_SNAPSHOT_ABILITIES + 1;
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) == 0);

    // Facings must point at an entity that is in the snapshot too.
    P.Snapshot.AbilityCount = 0;
    P.Snapshot.FacingCount = 1;
    P.Snapshot.Facings[0].EntityIndex = 2;
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) == 0);
    P.Snapshot.Facings[0].EntityIndex = 1;
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) > 0);
    P.Snapshot.FacingCount = NET_MAX_SNAPSHOT_FACINGS + 1;
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) == 0);
}

internal void
TestRejectsBadPackets()
{
    // NOTE: one byte spare for the trailing-garbage case below; the
    // largest snapshot fills NET_MAX_PACKET_SIZE exactly
    u8 Buffer[NET_MAX_PACKET_SIZE + 1];
    net_packet P = FullSnapshot();
    net_packet Out;

    // Too small a buffer to write into.
    Check(NetWritePacket(&P, Buffer, 100) == 0);

    u32 Size = NetWritePacket(&P, Buffer, sizeof(Buffer));
    Check(Size > 0);

    // Truncated anywhere.
    bool32 AnyTruncatedAccepted = false;
    for (u32 Cut = 0; Cut < Size; ++Cut)
    {
        if (NetReadPacket(Buffer, Cut, &Out)) AnyTruncatedAccepted = true;
    }
    Check(!AnyTruncatedAccepted);

    // Trailing garbage.
    Buffer[Size] = 0;
    Check(!NetReadPacket(Buffer, Size + 1, &Out));

    // Wrong protocol id.
    Buffer[0] ^= 0xff;
    Check(!NetReadPacket(Buffer, Size, &Out));
    Buffer[0] ^= 0xff;
    Check(NetReadPacket(Buffer, Size, &Out));

    // Unknown packet type.
    Buffer[4] = NetPacket_Count;
    Check(!NetReadPacket(Buffer, Size, &Out));
    Buffer[4] = NetPacket_Snapshot;

    // Entity count above the limit. Count sits after the 13-byte header,
    // the 4-byte tick, the 4-byte input tick, the 1-byte input buffered
    // and the two 4-byte origin floats.
    Buffer[30] = (u8)(NET_MAX_SNAPSHOT_ENTITIES + 1);
    Check(!NetReadPacket(Buffer, Size, &Out));

    // Input batches must hold 1..NET_MAX_INPUTS_PER_PACKET inputs.
    net_packet Input = {};
    Input.Header.Type = NetPacket_Input;
    Check(NetWritePacket(&Input, Buffer, sizeof(Buffer)) == 0);
    Input.Input.Count = NET_MAX_INPUTS_PER_PACKET + 1;
    Check(NetWritePacket(&Input, Buffer, sizeof(Buffer)) == 0);
}

internal void
TestSequenceWraparound()
{
    Check(NetSequenceNewer(1, 0));
    Check(!NetSequenceNewer(0, 1));
    Check(!NetSequenceNewer(5, 5));
    Check(NetSequenceNewer(2, 65530));
    Check(!NetSequenceNewer(65530, 2));
}

internal net_packet
ConnectRequest(u32 Salt)
{
    net_packet P = {};
    P.Header.Type = NetPacket_ConnectRequest;
    P.ConnectRequest.ClientSalt = Salt;
    return P;
}

internal net_packet
InputPacket(u16 Sequence, u32 NewestTick, u8 Count)
{
    net_packet P = {};
    P.Header = {NetPacket_Input, Sequence, 0};
    P.Input.Count = Count;
    for (u8 Index = 0; Index < Count; ++Index)
    {
        P.Input.Inputs[Index].Tick = NewestTick - Index;
    }
    return P;
}

internal void
TestClientsJoinAndRejoin()
{
    net_server_clients Clients = {};
    net_address A = {0x7f000001, 4000};
    net_address B = {0x7f000001, 4001};

    net_packet Request = ConnectRequest(111);
    net_receive_result R = NetServerReceive(&Clients, A, &Request, 10);
    Check(R.Event == NetReceive_Joined);
    Check(R.SlotIndex == 0);
    Check(R.HasReply && R.Reply.Header.Type == NetPacket_ConnectAccepted);
    Check(R.Reply.ConnectAccepted.ClientSalt == 111);
    Check(R.Reply.ConnectAccepted.ServerTick == 10);

    // The client did not hear back and asks again: same slot, no new player.
    R = NetServerReceive(&Clients, A, &Request, 11);
    Check(R.Event == NetReceive_Rejoined);
    Check(R.SlotIndex == 0);
    Check(R.Reply.Header.Sequence == 1);

    net_packet Other = ConnectRequest(222);
    R = NetServerReceive(&Clients, B, &Other, 12);
    Check(R.Event == NetReceive_Joined);
    Check(R.SlotIndex == 1);

    // Same address with a new salt means the client restarted; it gets a fresh slot.
    net_packet Restart = ConnectRequest(333);
    R = NetServerReceive(&Clients, A, &Restart, 13);
    Check(R.Event == NetReceive_Joined);
    Check(R.SlotIndex == 0);
    Check(Clients.Slots[0].Salt == 333);
}

internal void
TestServerFullDenies()
{
    net_server_clients Clients = {};
    for (u16 Index = 0; Index < NET_MAX_CLIENTS; ++Index)
    {
        net_packet Request = ConnectRequest(Index);
        NetServerReceive(&Clients, {1, Index}, &Request, 0);
    }
    net_packet Late = ConnectRequest(99);
    net_receive_result R = NetServerReceive(&Clients, {2, 0}, &Late, 0);
    Check(R.Event == NetReceive_Denied);
    Check(R.Reply.Header.Type == NetPacket_ConnectDenied);
    Check(R.Reply.ConnectDenied.Reason == NetDeny_ServerFull);
    Check(R.Reply.ConnectDenied.ClientSalt == 99);
}

internal void
TestInputsAppliedOnceInOrder()
{
    net_server_clients Clients = {};
    net_address A = {5, 5};
    net_packet Request = ConnectRequest(1);
    NetServerReceive(&Clients, A, &Request, 0);

    net_packet First = InputPacket(1, 3, 3); // ticks 3, 2, 1
    net_receive_result R = NetServerReceive(&Clients, A, &First, 0);
    Check(R.Event == NetReceive_Inputs);
    Check(R.NewInputCount == 3);
    Check(R.NewInputs[0].Tick == 1 && R.NewInputs[2].Tick == 3);

    net_packet Overlap = InputPacket(2, 5, 4); // ticks 5, 4, 3, 2
    R = NetServerReceive(&Clients, A, &Overlap, 0);
    Check(R.NewInputCount == 2);
    Check(R.NewInputs[0].Tick == 4 && R.NewInputs[1].Tick == 5);
    Check(Clients.Slots[0].NewestReceived == 2);

    // A late, reordered packet brings nothing new and does not move the ack back.
    R = NetServerReceive(&Clients, A, &First, 0);
    Check(R.Event == NetReceive_Ignored);
    Check(Clients.Slots[0].NewestReceived == 2);

    // Strangers cannot inject inputs.
    R = NetServerReceive(&Clients, {6, 6}, &Overlap, 0);
    Check(R.Event == NetReceive_Ignored);
    Check(R.NewInputCount == 0);
}

internal void
TestClientsLeaveAndTimeOut()
{
    net_server_clients Clients = {};
    net_packet Request = ConnectRequest(1);
    NetServerReceive(&Clients, {1, 1}, &Request, 0);
    NetServerReceive(&Clients, {2, 2}, &Request, 0);

    net_packet Bye = {};
    Bye.Header.Type = NetPacket_Disconnect;
    net_receive_result R = NetServerReceive(&Clients, {1, 1}, &Bye, 0);
    Check(R.Event == NetReceive_Left);
    Check(!Clients.Slots[0].Connected);

    Check(NetServerAdvance(&Clients, NET_CLIENT_TIMEOUT * 0.5f) == 0);
    net_packet Ping = InputPacket(1, 1, 1);
    NetServerReceive(&Clients, {2, 2}, &Ping, 0); // hearing from it resets the clock
    Check(NetServerAdvance(&Clients, NET_CLIENT_TIMEOUT * 0.9f) == 0);
    Check(NetServerAdvance(&Clients, NET_CLIENT_TIMEOUT * 0.2f) == (1u << 1));
    Check(!Clients.Slots[1].Connected);
}

internal void
TestParseAddress()
{
    net_address A = {};
    Check(NetParseAddress("127.0.0.1:27015", &A));
    Check(A.Ip == 0x7f000001 && A.Port == 27015);
    Check(NetParseAddress("255.255.255.255:65535", &A));
    Check(A.Ip == 0xffffffff && A.Port == 65535);
    Check(!NetParseAddress("127.0.0.1", &A));
    Check(!NetParseAddress("127.0.0:80", &A));
    Check(!NetParseAddress("256.0.0.1:80", &A));
    Check(!NetParseAddress("1.2.3.4:65536", &A));
    Check(!NetParseAddress("1..3.4:80", &A));
    Check(!NetParseAddress("1.2.3.4:", &A));
    Check(!NetParseAddress("1.2.3.4:80x", &A));
    Check(!NetParseAddress("", &A));
}

// Sends a real packet between two sockets over the loopback interface.
internal void
TestLoopbackPacket()
{
    Check(NetSocketsStartup());
    net_socket Server = NetOpenSocket(0);
    net_socket Client = NetOpenSocket(0);
    Check(Server.Open && Client.Open);
    u16 ServerPort = NetSocketPort(&Server);
    Check(ServerPort != 0);

    u8 Buffer[NET_MAX_PACKET_SIZE];
    net_address From = {};
    Check(NetReceiveFrom(&Server, &From, Buffer, sizeof(Buffer)) == 0); // nothing yet, and no blocking

    net_packet Request = {};
    Request.Header.Type = NetPacket_ConnectRequest;
    Request.ConnectRequest.ClientSalt = 4242;
    u32 Size = NetWritePacket(&Request, Buffer, sizeof(Buffer));
    Check(NetSendTo(&Client, {0x7f000001, ServerPort}, Buffer, Size));

    // Loopback delivery is not instant; poll briefly.
    u32 Received = 0;
    for (int Try = 0; Try < 1000000 && !Received; ++Try)
    {
        Received = NetReceiveFrom(&Server, &From, Buffer, sizeof(Buffer));
    }
    net_packet Got = {};
    Check(Received == Size);
    Check(NetReadPacket(Buffer, Received, &Got));
    Check(Got.ConnectRequest.ClientSalt == 4242);
    Check(From.Ip == 0x7f000001 && From.Port == NetSocketPort(&Client));

    NetCloseSocket(&Client);
    NetCloseSocket(&Server);
    Check(!Server.Open);
    NetSocketsShutdown();
}

// With Strict, a join request only takes a slot (or restarts one)
// with the cookie the server sent to that address; strangers get a
// challenge and cost no slot, and a forged restart of a connected player
// from its address does nothing.
internal void
TestJoiningNeedsTheCookie()
{
    static net_server_clients Clients;
    Clients = {};
    Clients.Strict = true;
    Clients.Secret = 0x1234567;
    net_address A = {0x7f000001, 4000};
    net_address B = {0x7f000001, 4001};

    net_packet Request = {};
    Request.Header.Type = NetPacket_ConnectRequest;
    Request.ConnectRequest.ClientSalt = 77;
    net_receive_result R = NetServerReceive(&Clients, A, &Request, 100);
    Check(R.Event == NetReceive_Ignored);
    Check(R.HasReply && R.Reply.Header.Type == NetPacket_ConnectChallenge);
    Check(R.Reply.ConnectChallenge.ClientSalt == 77);
    u32 Cookie = R.Reply.ConnectChallenge.Cookie;
    Check(Cookie != 0);
    for (u32 Index = 0; Index < NET_MAX_CLIENTS; ++Index) Check(!Clients.Slots[Index].Connected);

    // A wrong cookie, or A's cookie used from B, is challenged again.
    Request.ConnectRequest.Cookie = Cookie ^ 4;
    Check(NetServerReceive(&Clients, A, &Request, 101).Event == NetReceive_Ignored);
    Request.ConnectRequest.Cookie = Cookie;
    Check(NetServerReceive(&Clients, B, &Request, 101).Event == NetReceive_Ignored);

    // The right one joins, also a window later.
    R = NetServerReceive(&Clients, A, &Request, 100 + 512);
    Check(R.Event == NetReceive_Joined);
    Check(R.Reply.Header.Type == NetPacket_ConnectAccepted);
    u32 Slot = R.SlotIndex;

    // A forged restart from A (new salt, no cookie) leaves the slot alone.
    net_packet Forged = {};
    Forged.Header.Type = NetPacket_ConnectRequest;
    Forged.ConnectRequest.ClientSalt = 99;
    R = NetServerReceive(&Clients, A, &Forged, 700);
    Check(R.Event == NetReceive_Ignored);
    Check(Clients.Slots[Slot].Connected && Clients.Slots[Slot].Salt == 77);

    // After joining, packets must carry the slot's token: a forged input
    // or goodbye from A's address without it is ignored.
    net_packet Input = {};
    Input.Header = {NetPacket_Input, 5, 0, 12345};
    Input.Input.Count = 1;
    Input.Input.Inputs[0].Tick = 1;
    Input.Input.Inputs[0].Buttons = NetButton_Left;
    Check(NetServerReceive(&Clients, A, &Input, 701).Event == NetReceive_Ignored);
    net_packet Bye = {};
    Bye.Header = {NetPacket_Disconnect, 6, 0, 12345};
    Check(NetServerReceive(&Clients, A, &Bye, 702).Event == NetReceive_Ignored);
    Check(Clients.Slots[Slot].Connected);
    Input.Header.Token = 77;
    Check(NetServerReceive(&Clients, A, &Input, 703).Event == NetReceive_Inputs);

    // Two windows on, the old cookie no longer works.
    Request.ConnectRequest.ClientSalt = 78;
    Request.ConnectRequest.Cookie = Cookie;
    Check(NetServerReceive(&Clients, B, &Request, 100 + 3 * 512).Event == NetReceive_Ignored);
}

// The info query: round trips, and a request is never smaller than the
// largest reply, so it cannot be used to multiply traffic.
internal void
TestInfoQueryRoundTripsAndIsNotAnAmplifier()
{
    static net_packet Request, Reply, Out;
    Request = {};
    Request.Header.Type = NetPacket_InfoRequest;
    Request.InfoRequest.Nonce = 0xabcdef01;
    static u8 Buffer[NET_MAX_PACKET_SIZE];
    u32 RequestSize = NetWritePacket(&Request, Buffer, sizeof(Buffer));
    Check(RequestSize > 0);
    Check(NetReadPacket(Buffer, RequestSize, &Out));
    Check(Out.InfoRequest.Nonce == 0xabcdef01);
    // A request without its padding is rejected.
    Check(!NetReadPacket(Buffer, RequestSize - 1, &Out));

    Reply = {};
    Reply.Header.Type = NetPacket_InfoReply;
    Reply.InfoReply.Nonce = 0xabcdef01;
    Reply.InfoReply.ContentId = 0x25519fd6;
    Reply.InfoReply.MapId = 3;
    Reply.InfoReply.PlayerCount = NET_MAX_SNAPSHOT_SCORES;
    Reply.InfoReply.MaxPlayers = NET_MAX_SNAPSHOT_SCORES;
    // NOTE: the longest server name, so the size check below covers it
    snprintf(Reply.InfoReply.ServerName, NET_SERVER_NAME_SIZE, "%s", "ABCDEFGHIJKLMNOPQRSTUVW");
    Reply.InfoReply.NameCount = NET_MAX_SNAPSHOT_SCORES;
    for (u32 Index = 0; Index < NET_MAX_SNAPSHOT_SCORES; ++Index)
    {
        snprintf(Reply.InfoReply.Names[Index], NET_NAME_SIZE, "ABCDEFGHIJKLMN%u", Index);
    }
    u32 ReplySize = NetWritePacket(&Reply, Buffer, sizeof(Buffer));
    Check(ReplySize > 0);
    Check(RequestSize >= ReplySize);
    Check(NetReadPacket(Buffer, ReplySize, &Out));
    Check(Out.InfoReply.NameCount == NET_MAX_SNAPSHOT_SCORES);
    Check(strcmp(Out.InfoReply.Names[7], "ABCDEFGHIJKLMN7") == 0);
    Check(Out.InfoReply.ContentId == 0x25519fd6 && Out.InfoReply.MapId == 3);
    Check(strcmp(Out.InfoReply.ServerName, "ABCDEFGHIJKLMNOPQRSTUVW") == 0);
}

// Another build of the game is told apart from noise, and the notice a
// server sends it reads back on any version and is never a packet.
internal void
TestVersionNotice()
{
    static u8 Buffer[NET_MAX_PACKET_SIZE];
    static net_packet Request, Out;
    Request = {};
    Request.Header.Type = NetPacket_ConnectRequest;
    u32 Size = NetWritePacket(&Request, Buffer, sizeof(Buffer));
    Check(Size > 0);
    Check(!NetIsOtherVersion(Buffer, Size));
    // NOTE: the id's low byte (first on the wire) is the version letter
    Buffer[0] = (u8)(Buffer[0] - 1);
    Check(NetIsOtherVersion(Buffer, Size));
    Check(!NetReadPacket(Buffer, Size, &Out));
    Buffer[3] = 'X';
    Check(!NetIsOtherVersion(Buffer, Size));
    Check(!NetIsOtherVersion(Buffer, 3));

    u8 Notice[NET_VERSION_NOTICE_SIZE];
    NetWriteVersionNotice(Notice);
    u32 ServerProtocol = 0;
    Check(NetReadVersionNotice(Notice, sizeof(Notice), &ServerProtocol));
    Check(ServerProtocol == NET_PROTOCOL_ID);
    Check(!NetReadVersionNotice(Notice, sizeof(Notice) - 1, &ServerProtocol));
    Check(!NetReadPacket(Notice, sizeof(Notice), &Out));
    Check(Size > NET_VERSION_NOTICE_SIZE);
}

// Height and velocity are left out when zero: a still entity on the ground
// costs 12 bytes, a jumping, moving one 20, and both read back exactly.
internal void
TestStillEntitiesAreSmaller()
{
    static net_packet P, Out;
    static u8 Buffer[NET_MAX_PACKET_SIZE];
    P = {};
    P.Header.Type = NetPacket_Snapshot;
    P.Snapshot.NameSlot = NET_NO_NAME_SLOT;
    P.Snapshot.Count = 1;
    P.Snapshot.Entities[0].Id = 5;
    P.Snapshot.Entities[0].Type = 4;
    P.Snapshot.Entities[0].X = 100.f;
    P.Snapshot.Entities[0].Y = 200.f;
    u32 Still = NetWritePacket(&P, Buffer, sizeof(Buffer));
    Check(NetReadPacket(Buffer, Still, &Out));
    Check(Out.Snapshot.Entities[0].Type == 4 && Out.Snapshot.Entities[0].Z == 0.f);
    Check(Out.Snapshot.Entities[0].VelX == 0.f && Out.Snapshot.Entities[0].X == 100.f);

    P.Snapshot.Entities[0].Flash = 1;
    P.Snapshot.Entities[0].Z = 12.5f;
    P.Snapshot.Entities[0].VelX = -30.25f;
    u32 Moving = NetWritePacket(&P, Buffer, sizeof(Buffer));
    Check(Moving == Still + 8);
    Check(NetReadPacket(Buffer, Moving, &Out));
    Check(Out.Snapshot.Entities[0].Z == 12.5f && Out.Snapshot.Entities[0].VelX == -30.25f);
    Check(Out.Snapshot.Entities[0].VelY == 0.f && Out.Snapshot.Entities[0].Type == 4);
    Check(Out.Snapshot.Entities[0].Flash == 1);

    // A unit thrown up this tick, still at height 0, keeps its speed.
    P.Snapshot.Entities[0].Z = 0.f;
    P.Snapshot.Entities[0].VelZ = 420.f;
    u32 Thrown = NetWritePacket(&P, Buffer, sizeof(Buffer));
    Check(NetReadPacket(Buffer, Thrown, &Out));
    Check(Out.Snapshot.Entities[0].VelZ == 420.f && Out.Snapshot.Entities[0].Z == 0.f);
    P.Snapshot.Entities[0].VelZ = 0.f;

    // A type past 6 bits cannot be sent.
    P.Snapshot.Entities[0].Type = 64;
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) == 0);
}

// Fuzz: the server reads datagrams from the whole internet. Random bytes,
// and real packets of every type with bytes flipped, cut or extended, must
// be rejected or decode to a packet that writes back out; never crash,
// never read past the datagram. linux_check.ps1 runs this under
// AddressSanitizer, which turns any stray read into a failure.
internal u32
FuzzRandom(u32 *State)
{
    u32 X = *State;
    X ^= X << 13; X ^= X >> 17; X ^= X << 5;
    *State = X;
    return X;
}

internal void
TestFuzzedPacketsAreSafe()
{
    static net_packet Seeds[5];
    for (u32 Index = 0; Index < 5; ++Index) Seeds[Index] = {};
    Seeds[0] = FullSnapshot();
    Seeds[1].Header.Type = NetPacket_ConnectRequest;
    Seeds[1].ConnectRequest.ClientSalt = 77;
    snprintf(Seeds[1].ConnectRequest.Name, NET_NAME_SIZE, "%s", "Fuzzy");
    Seeds[2].Header.Type = NetPacket_Input;
    Seeds[2].Input.Count = NET_MAX_INPUTS_PER_PACKET;
    Seeds[3].Header.Type = NetPacket_InfoReply;
    Seeds[3].InfoReply.NameCount = 3;
    Seeds[4].Header.Type = NetPacket_ConnectChallenge;

    static u8 Valid[5][NET_MAX_PACKET_SIZE];
    u32 ValidSize[5];
    for (u32 Index = 0; Index < 5; ++Index)
    {
        ValidSize[Index] = NetWritePacket(&Seeds[Index], Valid[Index], NET_MAX_PACKET_SIZE);
        Check(ValidSize[Index] > 0);
    }

    // NOTE: the datagram lives in a buffer of exactly its size, so a read
    // past its end is a read past the buffer (caught by AddressSanitizer)
    static net_packet Out;
    static u8 Rewritten[NET_MAX_PACKET_SIZE];
    u32 State = 0x2545f491u;
    u32 Accepted = 0, WritesBack = 0;
    for (u32 Round = 0; Round < 300000; ++Round)
    {
        u8 *Datagram;
        u32 Size;
        u32 Kind = FuzzRandom(&State) % 4;
        u32 Seed = FuzzRandom(&State) % 5;
        if (Kind == 0)
        {
            Size = FuzzRandom(&State) % 64;
        }
        else
        {
            Size = ValidSize[Seed];
            if (Kind == 2) Size = FuzzRandom(&State) % (Size + 1);
            if (Kind == 3 && Size < NET_MAX_PACKET_SIZE) Size += 1 + FuzzRandom(&State) % 8;
            if (Size > NET_MAX_PACKET_SIZE) Size = NET_MAX_PACKET_SIZE;
        }
        Datagram = (u8 *)malloc(Size ? Size : 1);
        for (u32 Byte = 0; Byte < Size; ++Byte)
        {
            Datagram[Byte] = (Kind == 0 || Byte >= ValidSize[Seed]) ?
                (u8)FuzzRandom(&State) : Valid[Seed][Byte];
        }
        // Keep the protocol id most of the time so the body gets read.
        if (Kind != 0 && Size >= 4 && FuzzRandom(&State) % 8)
        {
            for (u32 Flip = 1 + FuzzRandom(&State) % 4; Flip > 0; --Flip)
            {
                u32 At = 4 + FuzzRandom(&State) % (Size > 4 ? Size - 4 : 1);
                if (At < Size) Datagram[At] ^= (u8)(1u << (FuzzRandom(&State) % 8));
            }
        }
        if (NetReadPacket(Datagram, Size, &Out))
        {
            ++Accepted;
            if (NetWritePacket(&Out, Rewritten, sizeof(Rewritten)) > 0) ++WritesBack;
        }
        free(Datagram);
    }
    Check(Accepted > 0);
    Check(WritesBack == Accepted);
    printf("  fuzz: 300000 datagrams, %u accepted, all of them write back\n", Accepted);
}

// The wire layout is pinned: one packet of every type, with every field
// set, is written and its bytes hashed. A change to what goes on the wire
// must come with a new NET_PROTOCOL_ID, or old and new builds would
// accept each other and misread every packet. When this fails, bump
// NET_PROTOCOL_ID in protocol.h (if you have not), then copy the two
// printed values into NET_GOLDEN_PROTOCOL_ID and NET_GOLDEN_LAYOUT below.
// Changing only the test packets (FullSnapshot) also moves the hash;
// then the id stays and only NET_GOLDEN_LAYOUT is updated. Two branches
// that both change the layout conflict on these lines, which is the point.
#define NET_GOLDEN_PROTOCOL_ID 0x47444d67u
#define NET_GOLDEN_LAYOUT 0x9cfca42au

internal u32
HashBytes(u32 Hash, u8 *Bytes, u32 Count)
{
    for (u32 Index = 0; Index < Count; ++Index)
    {
        Hash = (Hash ^ Bytes[Index]) * 16777619u;
    }
    return Hash;
}

// NOTE(zoubir): a dungeon run's block (sim/dungeon/), sanctuaries and all,
// comes back as it went
internal void
TestDungeonBlockRoundTrip()
{
    static net_packet P;
    P = {};
    P.Header = {NetPacket_Snapshot, 1, 2};
    P.Snapshot.Tick = 99;
    P.Snapshot.NameSlot = NET_NO_NAME_SLOT;
    P.Snapshot.VoteMap = NET_NO_VOTE;
    P.Snapshot.HasDungeon = 1;
    P.Snapshot.FightingRoom = 7;
    P.Snapshot.RoomsCleared = 0x3f;
    P.Snapshot.Wipes = 9;
    P.Snapshot.BossKind = NET_NO_BOSS;
    P.Snapshot.BossHealth = 1;
    P.Snapshot.FoesLeft = 12;
    P.Snapshot.BossClock = NET_BOSS_ENRAGED;
    P.Snapshot.SanctuaryCount = NET_MAX_SANCTUARIES;
    for (u32 Index = 0; Index < NET_MAX_SANCTUARIES; ++Index)
    {
        P.Snapshot.SanctuaryX[Index] = (i16)(1000 * Index - 5);
        P.Snapshot.SanctuaryY[Index] = (i16)(3000 - 700 * Index);
        P.Snapshot.SanctuaryTenths[Index] = (u8)(10 + Index);
    }
    P.Snapshot.InfernoCount = NET_MAX_INFERNOS;
    for (u32 Index = 0; Index < NET_MAX_INFERNOS; ++Index)
    {
        P.Snapshot.InfernoX[Index] = (i16)(-2000 + 900 * Index);
        P.Snapshot.InfernoY[Index] = (i16)(40 * Index);
        P.Snapshot.InfernoTenths[Index] = (u8)((Index & 1) ? 0x40 | (6 - Index) : 31 + Index);
    }
    static u8 Buffer[NET_MAX_PACKET_SIZE];
    u32 Size = NetWritePacket(&P, Buffer, sizeof(Buffer));
    Check(Size > 0);
    static net_packet Out;
    Out = {};
    Check(NetReadPacket(Buffer, Size, &Out));
    Check(Out.Snapshot.HasDungeon == 1 && Out.Snapshot.FightingRoom == 7);
    Check(Out.Snapshot.RoomsCleared == 0x3f && Out.Snapshot.Wipes == 9);
    Check(Out.Snapshot.BossKind == NET_NO_BOSS && Out.Snapshot.FoesLeft == 12);
    Check(Out.Snapshot.BossClock == NET_BOSS_ENRAGED);
    Check(Out.Snapshot.SanctuaryCount == NET_MAX_SANCTUARIES);
    for (u32 Index = 0; Index < NET_MAX_SANCTUARIES; ++Index)
    {
        Check(Out.Snapshot.SanctuaryX[Index] == P.Snapshot.SanctuaryX[Index]);
        Check(Out.Snapshot.SanctuaryY[Index] == P.Snapshot.SanctuaryY[Index]);
        Check(Out.Snapshot.SanctuaryTenths[Index] == P.Snapshot.SanctuaryTenths[Index]);
    }
    Check(Out.Snapshot.InfernoCount == NET_MAX_INFERNOS);
    for (u32 Index = 0; Index < NET_MAX_INFERNOS; ++Index)
    {
        Check(Out.Snapshot.InfernoX[Index] == P.Snapshot.InfernoX[Index]);
        Check(Out.Snapshot.InfernoY[Index] == P.Snapshot.InfernoY[Index]);
        Check(Out.Snapshot.InfernoTenths[Index] == P.Snapshot.InfernoTenths[Index]);
    }
    // NOTE(zoubir): more infernos than there is room for is refused: the
    // count sits before the last own-body byte and NET_MAX_INFERNOS rows
    // of 5 bytes
    Buffer[Size - 1 - NET_MAX_INFERNOS * 5 - 1] = NET_MAX_INFERNOS + 1;
    Check(!NetReadPacket(Buffer, Size, &Out) || Out.Snapshot.InfernoCount <= NET_MAX_INFERNOS);
    Size = NetWritePacket(&P, Buffer, sizeof(Buffer));
    // NOTE(zoubir): and so are more sanctuaries: before the infernos and
    // NET_MAX_SANCTUARIES rows of 5 bytes
    Buffer[Size - 1 - NET_MAX_INFERNOS * 5 - 1 - NET_MAX_SANCTUARIES * 5 - 1] = NET_MAX_SANCTUARIES + 1;
    Check(!NetReadPacket(Buffer, Size, &Out) || Out.Snapshot.SanctuaryCount <= NET_MAX_SANCTUARIES);
}

internal void
TestWireLayoutIsPinned()
{
    static net_packet Packets[9];
    for (u32 Index = 0; Index < 9; ++Index) Packets[Index] = {};
    Packets[0].Header = {NetPacket_ConnectRequest, 1, 2};
    Packets[0].ConnectRequest.ClientSalt = 0x12345678;
    Packets[0].ConnectRequest.ContentId = 0x9abcdef0;
    Packets[0].ConnectRequest.Cookie = 0x2468ace1;
    snprintf(Packets[0].ConnectRequest.Name, NET_NAME_SIZE, "%s", "Layout");
    Packets[1].Header = {NetPacket_ConnectAccepted, 3, 4};
    Packets[1].ConnectAccepted.ClientSalt = 0x12345678;
    Packets[1].ConnectAccepted.PlayerIndex = 5;
    Packets[1].ConnectAccepted.ServerTick = 777;
    Packets[1].ConnectAccepted.MapId = 2;
    snprintf(Packets[1].ConnectAccepted.ServerName, NET_SERVER_NAME_SIZE, "%s", "Layout Arena");
    Packets[2].Header = {NetPacket_ConnectDenied, 5, 6};
    Packets[2].ConnectDenied.ClientSalt = 0x12345678;
    Packets[2].ConnectDenied.Reason = 1;
    Packets[3].Header = {NetPacket_Disconnect, 7, 8};
    Packets[4].Header = {NetPacket_Input, 9, 10};
    Packets[4].Input.Count = NET_MAX_INPUTS_PER_PACKET;
    for (u32 Index = 0; Index < NET_MAX_INPUTS_PER_PACKET; ++Index)
    {
        Packets[4].Input.Inputs[Index].Tick = 100 - Index;
        Packets[4].Input.Inputs[Index].Buttons = 0x10101 * Index;
        Packets[4].Input.Inputs[Index].AimX = 0.25f;
        Packets[4].Input.Inputs[Index].AimY = -0.5f;
    }
    Packets[5] = FullSnapshot();
    Packets[6].Header = {NetPacket_ConnectChallenge, 11, 12};
    Packets[6].ConnectChallenge.ClientSalt = 0x12345678;
    Packets[6].ConnectChallenge.Cookie = 0x13579bdf;
    Packets[7].Header = {NetPacket_InfoRequest, 13, 14};
    Packets[7].InfoRequest.Nonce = 0x0badf00d;
    Packets[8].Header = {NetPacket_InfoReply, 15, 16};
    Packets[8].InfoReply.Nonce = 0x0badf00d;
    Packets[8].InfoReply.ContentId = 0x25519fd6;
    Packets[8].InfoReply.MapId = 1;
    Packets[8].InfoReply.PlayerCount = 2;
    Packets[8].InfoReply.MaxPlayers = 8;
    snprintf(Packets[8].InfoReply.ServerName, NET_SERVER_NAME_SIZE, "%s", "Layout Arena");
    Packets[8].InfoReply.NameCount = 2;
    snprintf(Packets[8].InfoReply.Names[0], NET_NAME_SIZE, "%s", "Gary");
    snprintf(Packets[8].InfoReply.Names[1], NET_NAME_SIZE, "%s", "Player 2");

    u32 Hash = 2166136261u;
    for (u32 Index = 0; Index < 9; ++Index)
    {
        static u8 Buffer[NET_MAX_PACKET_SIZE];
        u32 Size = NetWritePacket(&Packets[Index], Buffer, sizeof(Buffer));
        Check(Size > 0);
        Hash = HashBytes(Hash, (u8 *)&Size, sizeof(Size));
        Hash = HashBytes(Hash, Buffer, Size);
    }
    bool32 Pinned = (NET_PROTOCOL_ID == NET_GOLDEN_PROTOCOL_ID &&
                     Hash == NET_GOLDEN_LAYOUT);
    if (!Pinned)
    {
        if (NET_PROTOCOL_ID == NET_GOLDEN_PROTOCOL_ID)
        {
            printf("  the wire layout changed but NET_PROTOCOL_ID did not: bump it in protocol.h\n");
        }
        printf("  then set NET_GOLDEN_PROTOCOL_ID 0x%08xu and NET_GOLDEN_LAYOUT 0x%08xu in net_tests.cpp\n",
               NET_PROTOCOL_ID, Hash);
    }
    Check(Pinned);
}

int
main()
{
    TestConnectRoundTrip();
    TestPackedEntityFields();
    TestNamesRoundTripAndAreCleaned();
    TestInputRoundTrip();
    TestFullSnapshotFits();
    TestOverfullSnapshotIsTrimmed();
    TestFixedPointPrecisionAndClamping();
    TestRejectsAbilityForMissingEntity();
    TestRejectsBadPackets();
    TestSequenceWraparound();
    TestClientsJoinAndRejoin();
    TestServerFullDenies();
    TestInputsAppliedOnceInOrder();
    TestClientsLeaveAndTimeOut();
    TestParseAddress();
    TestLoopbackPacket();
    TestJoiningNeedsTheCookie();
    TestInfoQueryRoundTripsAndIsNotAnAmplifier();
    TestVersionNotice();
    TestStillEntitiesAreSmaller();
    TestFuzzedPacketsAreSafe();
    TestDungeonBlockRoundTrip();
    TestWireLayoutIsPinned();

    printf("net tests: %d checks, %d failed\n", TestChecks, TestFailures);
    return TestFailures ? 1 : 0;
}
