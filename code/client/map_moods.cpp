/* Map moods: how each map is colour graded by the world grade shader
   (build/shaders/fx/world_grade.frag), so the maps feel apart at a glance:
   the Ashen Wastes hot and smoky, Frostbite Keep cold, the Verdant Wilds
   lush and golden under drifting sun shafts, the crypt dark, lit by the
   lanterns its players carry.
   One look per map, picked by name (MoodFor) since MapId follows the
   order of sim/maps/map_list.inc; a map without one gets DefaultMood. */

struct map_mood
{
    v3 Shadow;        // added to dark pixels, fading out toward light ones
    float Saturation; // 1 leaves colour as it is
    v3 Light;         // added to light pixels, fading out toward dark ones
    float Exposure;   // the whole world multiplied by this
    float Contrast;   // how much of the S-curve is mixed in, 0..1
    float Sky;        // strength of the ground patches and cloud shadows;
                      // 0 indoors
    float Lantern;    // strength of the light every living player carries
                      // (world_lights.cpp); 0 where the day is light enough
    float SunShafts;  // strength of the warm beams drifting over the ground;
                      // 0 indoors and under smoke
};

global_variable map_mood DefaultMood = {{-0.012f, 0.f, 0.022f}, 1.14f, {0.022f, 0.010f, -0.014f}, 1.f, 0.30f, 1.f, 0.f, 0.5f};
global_variable map_mood KeepMood = {{-0.020f, 0.f, 0.050f}, 1.00f, {0.f, 0.010f, 0.020f}, 0.97f, 0.25f, 1.f, 0.f, 0.35f};
global_variable map_mood WastesMood = {{0.020f, -0.010f, 0.030f}, 1.08f, {0.050f, 0.015f, -0.030f}, 0.92f, 0.40f, 0.6f, 0.f, 0.f};
global_variable map_mood WildsMood = {{-0.015f, 0.010f, 0.020f}, 1.20f, {0.030f, 0.025f, -0.010f}, 1.f, 0.30f, 1.2f, 0.f, 1.f};
// NOTE(zoubir): the crypt is dark, and lit by the lanterns its players
// carry and whatever burns or casts in it
global_variable map_mood CryptMood = {{-0.010f, 0.f, 0.040f}, 0.85f, {0.020f, 0.010f, 0.f}, 0.55f, 0.45f, 0.f, 0.9f, 0.f};

internal map_mood *
MoodFor(u32 MapId)
{
    map_mood *Result = &DefaultMood;
    switch(MapId)
    {
        case MapId_Keep: Result = &KeepMood; break;
        case MapId_Wastes: Result = &WastesMood; break;
        case MapId_Wilds: Result = &WildsMood; break;
        case MapId_Crypt: Result = &CryptMood; break;
        default: break;
    }
    return Result;
}
