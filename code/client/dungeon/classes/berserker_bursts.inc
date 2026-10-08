// The Berserker's bursts (SimBurst_BerserkerFirst on, sim/events.h) in BurstLooks
// (client/fx_bursts.cpp), CLASS_BURSTS rows; drawn by
// client/dungeon/classes/berserker.cpp. The Cleave's body swing is started
// there (SetBodySwing), so its rows have no pose.
    {BurstShape_Role, 0.42f, CLEAVE_REACH, 0x003232E1, false, 0.12f, BurstPose_None},    // Cleave
    {BurstShape_Role, 0.42f, CLEAVE_REACH, 0x003232E1, false, 0.12f, BurstPose_None},    // CleaveBack
    {BurstShape_Role, 2.f * AXE_THROW_RANGE / AXE_THROW_SPEED + 0.15f, 20.f, 0x003232E1, false, 0.05f,
     BurstPose_Release},                                                                  // AxeThrow
    {BurstShape_Role, 0.6f, 40.f, 0x003232E1, false, 0.2f, BurstPose_Release},           // Bloodthirst
    {BurstShape_Role, LEAP_SECONDS + 0.05f, LEAP_RADIUS, 0x003232E1, false, 0.1f, BurstPose_Release}, // Leap
    {BurstShape_Role, 0.75f, LEAP_RADIUS, 0x003232E1, false, 0.75f, BurstPose_None},      // Slam
    {BurstShape_Role, 0.7f, 60.f, 0x003232E1, false, 0.8f, BurstPose_None},               // Execute
    {BurstShape_Role, 0.9f, 160.f, 0x003232E1, false, 0.45f, BurstPose_Release},          // Berserk
