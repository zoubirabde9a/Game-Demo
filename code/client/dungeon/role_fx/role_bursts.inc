// The first three roles' later bursts (SimBurst_Dawnbreak on, sim/events.h)
// in BurstLooks (client/fx_bursts.cpp), one row each in their order; drawn
// by client/dungeon/role_fx/.
    {BurstShape_Role, 0.55f, DAWNBREAK_LENGTH, 0x0070E0FF, false, 0.15f, BurstPose_Release}, // Dawnbreak
    {BurstShape_Role, 0.5f, 40.f, 0x0080F0FF, false, 0.1f, BurstPose_Release},          // HolyStrike
    {BurstShape_Role, (1 + SHIELD_THROW_BOUNCES) * SHIELD_THROW_HOP_SECONDS + 0.25f, 30.f, 0x00D0E0F0,
     false, 0.f, BurstPose_Release},                                                        // ShieldThrow
    {BurstShape_Role, 0.5f, FLAME_WAVE_REACH, 0x002080FF, false, 0.3f, BurstPose_Release}, // FlameWave
