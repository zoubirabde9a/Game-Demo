// The Ranger's bursts (SimBurst_RangerFirst on, sim/events.h) in BurstLooks
// (client/fx_bursts.cpp), CLASS_BURSTS rows; drawn by
// client/dungeon/classes/ranger.cpp. The lasting ones play for the whole
// of what they show: a Volley's draw and rain (Barrage's longest); a mark
// and a trap are sent again every RANGER_KEEP_SECONDS while they last, so
// each burst outlives the gap to the next.
    {BurstShape_Role, 0.62f, 30.f, 0x00AAD25A, false, 0.04f, BurstPose_Release},  // Ranger Arrow
    {BurstShape_Role, VOLLEY_DRAW + VOLLEY_SECONDS + BARRAGE_SECONDS + 0.4f, VOLLEY_RADIUS,
     0x00AAD25A, false, 0.f, BurstPose_Release},                                    // Ranger Volley
    {BurstShape_Role, PIERCE_RANGE / PIERCE_SPEED + 0.5f, 40.f, 0x00AAD25A, false, 0.32f,
     BurstPose_Release},                                                            // Ranger Pierce
    {BurstShape_Role, RANGER_KEEP_SECONDS + 0.5f, 30.f, 0x00AAD25A, false, 0.f, BurstPose_None},    // Ranger Mark
    {BurstShape_Role, 0.5f, 40.f, 0x00AAD25A, false, 0.1f, BurstPose_Release},        // Ranger Leap
    {BurstShape_Role, RANGER_KEEP_SECONDS + 0.5f, TRAP_RADIUS, 0x00AAD25A, false, 0.f, BurstPose_None}, // Ranger Trap
    {BurstShape_Role, 0.6f, 40.f, 0x00AAD25A, false, 0.2f, BurstPose_None},           // Ranger TrapSnap
    {BurstShape_Role, 0.7f, 30.f, 0x00AAD25A, false, 0.f, BurstPose_None},            // Ranger FocusFull
