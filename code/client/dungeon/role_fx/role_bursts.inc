// The first three roles' later bursts (SimBurst_Dawnbreak on, sim/events.h)
// in BurstLooks (client/fx_bursts.cpp), one row each in their order; drawn
// by client/dungeon/role_fx/.
    {BurstShape_Role, 0.55f, DAWNBREAK_LENGTH, 0x0070E0FF, false, 0.15f, BurstPose_Release}, // Dawnbreak
    {BurstShape_Role, 0.5f, 40.f, 0x0080F0FF, false, 0.1f, BurstPose_Release},          // HolyStrike
    {BurstShape_Role, (1 + SHIELD_THROW_BOUNCES) * SHIELD_THROW_HOP_SECONDS + 0.25f, 30.f, 0x00D0E0F0,
     false, 0.f, BurstPose_Release},                                                        // ShieldThrow
    {BurstShape_Role, 0.5f, FLAME_WAVE_REACH, 0x002080FF, false, 0.3f, BurstPose_Release}, // FlameWave
    {BurstShape_Role, 0.5f, DETONATE_SPELL_REACH, 0x002080FF, false, 0.35f, BurstPose_Release}, // Detonate
    {BurstShape_Role, 0.35f, 30.f, 0x002080FF, false, 0.f, BurstPose_None},               // DetonateLink
    {BurstShape_Role, 0.6f, RALLYING_CRY_REACH, 0x0060E0FF, false, 0.3f, BurstPose_Release}, // RallyingCry
    {BurstShape_Role, 0.6f, DEMORALIZING_ROAR_REACH, 0x00A04070, false, 0.4f, BurstPose_Release}, // DemoralizingRoar
    {BurstShape_Role, 0.7f, 90.f, 0x0090F0B0, false, 0.f, BurstPose_Release},             // Prayer
    {BurstShape_Role, 0.6f, 40.f, 0x00FFF0E0, false, 0.f, BurstPose_Release},             // Purify
    {BurstShape_Role, 0.7f, 40.f, 0x00D0E0F0, false, 0.25f, BurstPose_Release},           // ShieldFlare
