// The Frost Mage's bursts (SimBurst_FrostMageFirst on, sim/events.h) in
// BurstLooks (client/fx_bursts.cpp), CLASS_BURSTS rows, in frostmage_burst
// order (sim/dungeon/role_kits/frostmage.cpp); drawn by
// client/dungeon/classes/frostmage.cpp. The lasting ones play for the
// whole of what they show: a Blizzard's fall, an orb's roll, and a frozen
// foe's ice for the longest a Deep Freeze nova holds.
    {BurstShape_Role, FROSTBOLT_RANGE / FROSTBOLT_SPEED + 0.4f, 30.f, 0x00FFD796, false, 0.04f, BurstPose_Release}, // Frostbolt
    {BurstShape_Role, BLIZZARD_SECONDS + 0.45f, BLIZZARD_RADIUS, 0x00FFD796, false, 0.f, BurstPose_Release},       // Blizzard
    {BurstShape_Role, GLACIAL_SPIKE_RANGE / GLACIAL_SPIKE_SPEED + 0.55f, 40.f, 0x00FFD796, false, 0.3f,
     BurstPose_Release},                                                                                           // Glacial Spike
    {BurstShape_Role, 0.6f, FROST_NOVA_RADIUS, 0x00FFD796, false, 0.2f, BurstPose_Release},                       // Frost Nova
    {BurstShape_Role, 0.45f, 40.f, 0x00FFD796, false, 0.f, BurstPose_None},                                        // Ice Barrier
    {BurstShape_Role, FROZEN_ORB_SECONDS + 0.35f, FROZEN_ORB_RADIUS, 0x00FFD796, false, 0.f, BurstPose_Release},   // Frozen Orb
    {BurstShape_Role, FROST_NOVA_ROOT + 4.f * DEEP_FREEZE_SECONDS + 0.2f, 30.f, 0x00FFD796, false, 0.f,
     BurstPose_None},                                                                                              // frozen foe
    {BurstShape_Role, 0.7f, 30.f, 0x00FFF4E0, false, 0.f, BurstPose_None},                                         // five Icicles
