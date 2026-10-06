/* Cliff colours: how each terrain kind's cliff faces and steps look
   (terrain_art.cpp draws them), one row per kind in terrain_kind order. */

// NOTE(zoubir): one row per terrain kind, in terrain_kind order. Faces are
// darker than the tops they hang from, so height reads at a glance
global_variable terrain_cliff TerrainCliffs[TerrainKind_Count] =
{
    // NOTE(zoubir): grass and dirt banks show earth
    {CliffPattern_Earth, {{ART_RGB(54, 36, 24), ART_RGB(84, 58, 38),
                           ART_RGB(112, 80, 52), ART_RGB(140, 104, 70)}}, ART_RGB(150, 140, 124), CliffDetail_None},
    {CliffPattern_Earth, {{ART_RGB(62, 42, 28), ART_RGB(92, 66, 42),
                           ART_RGB(120, 88, 58), ART_RGB(150, 116, 78)}}, ART_RGB(166, 156, 136), CliffDetail_None},
    {CliffPattern_Earth, {{ART_RGB(34, 24, 16), ART_RGB(52, 38, 26),
                           ART_RGB(72, 54, 36), ART_RGB(96, 74, 52)}}, ART_RGB(110, 122, 112), CliffDetail_None},
    // NOTE(zoubir): water held up by wet rock
    {CliffPattern_Rock, {{ART_RGB(34, 46, 58), ART_RGB(52, 68, 82),
                          ART_RGB(72, 92, 106), ART_RGB(100, 124, 138)}}, ART_RGB(150, 200, 220), CliffDetail_Glints},
    {CliffPattern_Rock, {{ART_RGB(24, 32, 46), ART_RGB(38, 50, 66),
                          ART_RGB(56, 70, 88), ART_RGB(80, 98, 116)}}, ART_RGB(120, 170, 210), CliffDetail_Glints},
    // NOTE(zoubir): rock
    {CliffPattern_Rock, {{ART_RGB(50, 48, 46), ART_RGB(76, 72, 68),
                          ART_RGB(102, 98, 92), ART_RGB(132, 126, 118)}}, ART_RGB(160, 154, 146), CliffDetail_None},
    // NOTE(zoubir): ash and basalt show dark rock with the odd ember
    {CliffPattern_Rock, {{ART_RGB(36, 34, 34), ART_RGB(54, 50, 50),
                          ART_RGB(74, 70, 68), ART_RGB(98, 92, 88)}}, ART_RGB(230, 110, 40), CliffDetail_Embers},
    {CliffPattern_Rock, {{ART_RGB(16, 14, 18), ART_RGB(28, 24, 30),
                          ART_RGB(42, 38, 44), ART_RGB(62, 56, 62)}}, ART_RGB(230, 100, 30), CliffDetail_Embers},
    {CliffPattern_Rock, {{ART_RGB(12, 10, 14), ART_RGB(22, 20, 24),
                          ART_RGB(36, 32, 38), ART_RGB(54, 48, 54)}}, ART_RGB(220, 90, 30), CliffDetail_Embers},
    // NOTE(zoubir): lava: crust with glowing seams
    {CliffPattern_Rock, {{ART_RGB(26, 14, 12), ART_RGB(46, 22, 16),
                          ART_RGB(68, 32, 22), ART_RGB(94, 46, 28)}}, ART_RGB(255, 150, 40), CliffDetail_Embers},
    // NOTE(zoubir): snow and ice show icy rock
    {CliffPattern_Ice, {{ART_RGB(62, 74, 94), ART_RGB(90, 106, 130),
                         ART_RGB(122, 142, 168), ART_RGB(164, 186, 210)}}, ART_RGB(220, 240, 255), CliffDetail_None},
    {CliffPattern_Ice, {{ART_RGB(56, 90, 118), ART_RGB(80, 124, 156),
                         ART_RGB(112, 160, 190), ART_RGB(160, 206, 228)}}, ART_RGB(236, 250, 255), CliffDetail_None},
    // NOTE(zoubir): stone floors and walls show masonry
    {CliffPattern_Masonry, {{ART_RGB(54, 50, 48), ART_RGB(80, 76, 72),
                             ART_RGB(104, 100, 94), ART_RGB(130, 126, 118)}}, ART_RGB(150, 146, 138), CliffDetail_None},
    {CliffPattern_Masonry, {{ART_RGB(40, 38, 42), ART_RGB(60, 58, 64),
                             ART_RGB(84, 80, 86), ART_RGB(110, 106, 112)}}, ART_RGB(130, 126, 132), CliffDetail_None},
    // NOTE(zoubir): the hazards and boons (terrain_hazard_art.cpp): a pit
    // shows black earth, a spring wet rock, the rest earth
    {CliffPattern_Earth, {{ART_RGB(10, 8, 10), ART_RGB(22, 18, 18),
                           ART_RGB(36, 28, 26), ART_RGB(54, 42, 36)}}, ART_RGB(80, 70, 60), CliffDetail_None},
    {CliffPattern_Rock, {{ART_RGB(30, 54, 58), ART_RGB(46, 80, 84),
                          ART_RGB(66, 108, 110), ART_RGB(96, 140, 140)}}, ART_RGB(170, 250, 230), CliffDetail_Glints},
    {CliffPattern_Earth, {{ART_RGB(40, 30, 20), ART_RGB(62, 46, 30),
                           ART_RGB(86, 64, 42), ART_RGB(112, 86, 58)}}, ART_RGB(120, 130, 80), CliffDetail_None},
    {CliffPattern_Earth, {{ART_RGB(30, 30, 18), ART_RGB(48, 46, 28),
                           ART_RGB(68, 64, 38), ART_RGB(90, 86, 52)}}, ART_RGB(130, 150, 70), CliffDetail_None},
    {CliffPattern_Earth, {{ART_RGB(62, 42, 28), ART_RGB(92, 66, 42),
                           ART_RGB(120, 88, 58), ART_RGB(150, 116, 78)}}, ART_RGB(190, 160, 255), CliffDetail_Glints},
};
