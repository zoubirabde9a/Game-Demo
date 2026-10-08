// The Shadowblade's bursts (SimBurst_ShadowbladeFirst on, sim/events.h) in BurstLooks
// (client/fx_bursts.cpp), CLASS_BURSTS rows, in shadowblade_burst order
// (sim/dungeon/role_kits/shadowblade_defs.cpp); drawn by
// client/dungeon/classes/shadowblade.cpp. The last two rows are spare.
    {BurstShape_Role, 0.38f, 70.f, 0x00FF6EAA, false, 0.06f, BurstPose_Release}, // Twin Strike
    {BurstShape_Role, 0.55f, 60.f, 0x00FF6EAA, false, 0.18f, BurstPose_Release}, // Shadowstep
    {BurstShape_Role, 0.8f, 80.f, 0x00FF6EAA, false, 0.3f, BurstPose_Release},   // Shadow Dance
    {BurstShape_Role, 0.45f, FAN_OF_KNIVES_RADIUS, 0x00FF6EAA, false, 0.32f, BurstPose_Release}, // Fan of Knives
    {BurstShape_Role, 0.6f, 50.f, 0x00FFC8E6, false, 0.45f, BurstPose_Release},  // Eviscerate
    {BurstShape_Role, 0.6f, 30.f, 0x002030FF, false, 0.f, BurstPose_None},       // no combo points
    {BurstShape_Role, 0.1f, 1.f, 0, false, 0.f, BurstPose_None},                 // spare
    {BurstShape_Role, 0.1f, 1.f, 0, false, 0.f, BurstPose_None},                 // spare
