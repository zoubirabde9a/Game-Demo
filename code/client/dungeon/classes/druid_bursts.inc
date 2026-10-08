// The Druid's bursts (SimBurst_DruidFirst on, sim/events.h) in BurstLooks
// (client/fx_bursts.cpp), CLASS_BURSTS rows, in druid_burst order
// (sim/dungeon/role_kits/druid.cpp); drawn by
// client/dungeon/classes/druid.cpp. A Moonfire and a Rejuvenation are
// sent again every DRUID_KEEP_SECONDS while they last, so each burst
// outlives the gap to the next; the roots play for their longest hold.
    {BurstShape_Role, 0.85f, 30.f, 0x003CC8A5, false, 0.f, BurstPose_Release},                   // Wrath
    {BurstShape_Role, DRUID_KEEP_SECONDS + 0.5f, 30.f, 0x00FFC8AA, false, 0.f, BurstPose_None},  // Moonfire
    {BurstShape_Role, STARFIRE_FALL + 0.6f, 60.f, 0x008CE1FF, false, 0.25f, BurstPose_None},     // Starfire
    {BurstShape_Role, DRUID_KEEP_SECONDS + 0.5f, 30.f, 0x003CC8A5, false, 0.f, BurstPose_None},  // Rejuvenation
    {BurstShape_Role, 0.9f, 40.f, 0x00BE96FF, false, 0.f, BurstPose_None},                       // Regrowth
    {BurstShape_Role, ROOTS_SECONDS_2 + 0.3f, ROOTS_RADIUS, 0x001E6E46, false, 0.f, BurstPose_Release}, // Roots
    {BurstShape_Role, 0.8f, TRANQUILITY_RADIUS, 0x003CC8A5, false, 0.f, BurstPose_None},         // Tranquility
    {BurstShape_Role, 0.7f, 30.f, 0x00BE96FF, false, 0.f, BurstPose_None},                       // Bloom full
