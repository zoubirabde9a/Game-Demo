// The Stormcaller's bursts (SimBurst_StormcallerFirst on, sim/events.h) in BurstLooks
// (client/fx_bursts.cpp), CLASS_BURSTS rows, in stormcaller_burst order
// (sim/dungeon/role_kits/stormcaller.cpp); drawn by
// client/dungeon/classes/stormcaller.cpp. A Static Field's dome is drawn for
// as long as its ClassFlags bit is up, so its burst only plays the landing.
    {BurstShape_Role, 0.34f, 40.f, 0x0050DCFA, false, 0.05f, BurstPose_Release},   // Stormcaller Bolt
    {BurstShape_Role, 0.6f, STATIC_FIELD_RADIUS, 0x0050DCFA, false, 0.1f, BurstPose_Release}, // Static Field
    {BurstShape_Role, 0.9f, THUNDERCLAP_SPLASH_RADIUS, 0x0050DCFA, false, 0.5f, BurstPose_Release}, // Thunderclap
    {BurstShape_Role, 0.45f, 30.f, 0x0050DCFA, false, 0.12f, BurstPose_None},       // Sky Bolt
    {BurstShape_Role, 0.9f, OVERLOAD_RADIUS, 0x0050DCFA, false, 0.7f, BurstPose_Release}, // Overload
    {BurstShape_Role, 0.5f, 40.f, 0x0050DCFA, false, 0.15f, BurstPose_Release},     // Lightning Dash
    {BurstShape_Role, 0.6f, 40.f, 0x0050DCFA, false, 0.f, BurstPose_None},          // Supercharged
    {BurstShape_Role, 0.5f, 30.f, 0x00808080, false, 0.f, BurstPose_None},          // no Charge
