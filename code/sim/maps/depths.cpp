/* Ember Depths: the second level of the co-op dungeon (sim/dungeon/,
   docs/dungeon-depths.md), reached by clearing the Sunken Crypt. A
   buried forge under the crypt, flooded with magma. Seven rooms joined
   by short corridors: the top row runs east from the Cinder Stair
   (where the party arrives) through the Slag Pits to the Anvil Hall; a
   passage drops south to the Glasswing Hollow, which runs back west to
   the round lair of Wyrm's Gullet; from there a passage drops to the
   Ashfall Bridge over a lake of lava, which runs east to the Throne of
   Embers.

   DepthsRooms works like CryptRooms (sim/maps/crypt.cpp): a digit is
   the room a tile belongs to, a letter the gate between two rooms, '.'
   neither. Drawn by a script, like the other hand-made maps. */
#if defined(MAP_NAME_PASS)
MAP(Depths)
#else

global_variable char *DepthsLayout[] =
{
    "################################################################################################",
    "################################################################################################",
    "#######################bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb#####bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb########",
    "#######################bbXbbbbbbbbbbbbbbbbbbbbbbbbbbb#####bLbbbbbbbbbbbbbbbbbbbbbbbbbbLb########",
    "#######################bbbbbbLLLLLbbbbbbbbbbbbbbbbbbb#####bLbbbbbbbbbbbb##bbbbbbbbbbbbLb########",
    "#######################bbbbbLLLLLLLbbbbbbbbbbbbbbbbbb#####bLbbbb##bbbbbb##bbbbbb##bbbbLb########",
    "##bbbbbbbbbbbbbbbb#####bbbbbLLLLLLLbbbbb##bbbbbbbbbbb#####bLbbbb##bbbbbbbbbbbbbb##bbbbLb########",
    "##bcbbbbbbbbbbbbcb#####bbbbbLLLLLLLbbbbb##bbbbbb##bbb#####bLbbbbbbbbbbbbbbbbbbbbbbbbbbLb########",
    "##bbbbbbbbbbbbbbbb#####bbbbbbLLLLLbbbbbbbbbbbbbb##bbb#####bLbbbbbbbbbbbbbbbbbbbbbbbbbbLb########",
    "##bbbsbbsbbsbbsbbb#####bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb#####bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb########",
    "##bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb########",
    "##bbbbbbbhbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb##bbbbbbbbbbbbb##bbbbbbbb^bbbbbbbbbbb##bbb########",
    "##bbbbbbbhbbbbbbbbbbbbbbbbbb##bbbbbbbbbbbbbbbb##bbbbbbbbbbbbb##bbbbbbbbbbb^bbbbbbbb##bbb########",
    "##bbbbbbbbbbbbbbbbbbbbbbbbbb##bbbbbLLLbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb########",
    "##bbbsbbsbbsbbsbbb#####bbbbbbbbbbbLLLLLbbbbbbbbbbbbbb#####bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb########",
    "##bbbbbbbbbbbbbbbb#####bbbbbbbbbbbLLLLLbbbLLLLLbbbbbb#####bLbbbbbbbbbbbbbbbbbbbbbbbbbbLb########",
    "##bbbbbbbbbbbbbbcb#####bbbbbbbbbbbLLLLLbLLLLLLLLLbbbb#####bLbbbbbbbbbbbbbbbbbbbbbbbbbbLb########",
    "##bbbbbbbbbbbbbbbb#####bbbbbbbbbbbbLLLbbLLLLLLLLLbbbb#####bLbbbb##bbbbbbbbbbbbbb##bbbbLb########",
    "#######################bbbbbbb##bbbbbbbbLLLLLLLLLbbbb#####bLbbbb##bbbbbb##bbbbbb##bbbbLb########",
    "#######################bbbbbbb##bbbbbbbbbbLLLLLbbbbbb#####bLbbbbbbbbbbbb##bbbbbbbbbbbbLb########",
    "#######################bbbbbbbbbbbbbbbbXbbbbbbbbbbbXb#####bLbbbbbbbbbbbbbbbbbbbbbbbbbbLb########",
    "#######################bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb#####bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb########",
    "########################################################################bbbb####################",
    "########################################################################bbbb####################",
    "########################################################################bbbb####################",
    "########################################################################bbbb####################",
    "########################################################################bbbb####################",
    "#######################bLLLLbLLLLbLLL##############aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa########",
    "#####################LbLLbbbbbbbbbbLLbL############aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa########",
    "###################LLbbbbbbbbbbbbbbbbbLLL##########aaaaaaaaaaaXXaaaaaaaaaaaaaadaaaaaaaaa########",
    "#################LLLbbbbbbbbbbbbbbbbbbbbbLL########aaadaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa########",
    "################LLLbbbbbbbbbbbbbbbbbbbbbbLLL#######aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaXXa########",
    "################LLbbbbbbbbbbbbbbbbbbbbbbbbLb#######aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa########",
    "###############LLbbbbbbXbbbbbbbbbbbbXbbbbbbLL######aaaaaaaaaaaaaaadaaaaaaaaaaa##aaaaaaaa########",
    "##############LLbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb#####aaaaaaaaaaaaaaaaaaaaaaaaaaa##aaaaaaaa########",
    "##############Lbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbaaaaaaaaaaaaaaaaaqqqqqaaaaaaaaaaaaaaa########",
    "##############bLbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbaaaaaaaaaaaaaaaaqqqqqqqaaaaaaadaaaaaa########",
    "##############Lbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbaaaaadaaaaaaa##aqqqqqqqaaaaaaaaaaaaaa########",
    "##############Lbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbaaaaaaaaaaaaa##aqqqqqqqaaaaaaaaaaaaaa########",
    "##############LLbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb#####aaaaaaaaaaaaaaaaaqqqqqaaaaaaaaaaaaaaa########",
    "##############LbbbbbbbbbbbbbbbbbbbbbbbbbbbbbLb#####aaaaaaaaaaaaaaaaaaaaaaaaXXaaaaaaaaaaa########",
    "##############bLbbbbbbbbbbbbbbbbbbbbbbbbbbbbbL#####aaaaaaaaadaaaaaaaaaaaaaaaaaaaaaaaaaaa########",
    "###############LLbbbbbbXbbbbbbbbbbbbXbbbbbbbL######aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa########",
    "################LbbbbbbbbbbbbbbbbbbbbbbbbbbL#######aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaadaaa########",
    "################bLLbbbbbbbbbbbbbbbbbbbbbbbLL#######aaaaaaaXXaaaaaaaaaadaaaaaaaaaaaaaaaaa########",
    "#################LLLbbbbbbbbbbbbbbbbbbbbbLL########aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa########",
    "###################bLLbbbbbbbbbbbbbbbbLbL##########aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa########",
    "#####################LLbbbbbbbbbbbbLLLb#########################################################",
    "#######################bbbbbbLLLbLLLL###########################################################",
    "########################bbbb####################################################################",
    "########################bbbb#######################bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb###",
    "########################bbbb#######################bbLLLbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbLLLbb###",
    "########################bbbb#######################bLLLLLbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbLLLLLb###",
    "########################bbbb#######################bbLLLbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbLLLbb###",
    "##LLLLLLLLLLLLLLLLLLLLbbbbbbbbLLLLLLbbbbbbLLLL#####bbbbbbb##bbbbbb##bbbbbbbb##bbbbbb##bbbbbbb###",
    "##LLLLLLLLLLLLLLLLLLLLbbbbbbbbLLLLLLbbbbbbLLLL#####bbbbbbb##bbbbbb##bbbbbbbb##bbbbbb##bbbbbbb###",
    "##LLLLLLLLLLLLLLLLLLLLbbbbbbbbLLLLLLbbbbbbLLLL#####bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb###",
    "##LLLLLLLLLLLLLLLLLLLLbbbbbbbbLLLLLLbbbbbbLLLL#####bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb###",
    "##bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb#####bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbXXXbb###",
    "##bbbbbbbbbbbbbbbbbbbbbbbbbbbbbXbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbXXXbb###",
    "##bbbbbbXbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbXXXbb###",
    "##bbbbbbbbbbbbbbbbXbbbbbbbbbbbbbbbbbbbbbXbbbbbbbbbbbbbbbbbbbbbbbbbbbbb^bbbbbbbbbbbbbbbbbXXXbb###",
    "##bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb^bbbbbbbbbbbbbbbXXXbb###",
    "##bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb#####bbbbbbbbbbbbbbbbbbb^bbbbbbbbbbbbbbbbbXXXbb###",
    "##LLLLLLLbbbbbbLLLLLLLLLLLLLLLLLLbbbbbbbbLLLLL#####bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbXXXbb###",
    "##LLLLLLLbbbbbbLLLLLLLLLLLLLLLLLLbbbbbbbbLLLLL#####bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbXXXbb###",
    "##LLLLLLLbbbbbbLLLLLLLLLLLLLLLLLLbbbbbbbbLLLLL#####bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb###",
    "##LLLLLLLbbbbbbLLLLLLLLLLLLLLLLLLbbbbbbbbLLLLL#####bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb###",
    "###################################################bbbbbbb##bbbbbb##bbbbbbbb##bbbbbb##bbbbbbb###",
    "###################################################bbbbbbb##bbbbbb##bbbbbbbb##bbbbbb##bbbbbbb###",
    "###################################################bbLLLbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbLLLbb###",
    "###################################################bLLLLLbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbLLLLLb###",
    "###################################################bbLLLbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbLLLbb###",
    "###################################################bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb###",
    "################################################################################################",
    "################################################################################################",
};

global_variable char *DepthsRooms[] =
{
    "................................................................................................",
    "................................................................................................",
    ".......................222222222222222222222222222222.....333333333333333333333333333333........",
    ".......................222222222222222222222222222222.....333333333333333333333333333333........",
    ".......................222222222222222222222222222222.....333333333333333333333333333333........",
    ".......................222222222222222222222222222222.....333333333333333333333333333333........",
    "..1111111111111111.....222222222222222222222222222222.....333333333333333333333333333333........",
    "..1111111111111111.....222222222222222222222222222222.....333333333333333333333333333333........",
    "..1111111111111111.....222222222222222222222222222222.....333333333333333333333333333333........",
    "..1111111111111111.....222222222222222222222222222222.....333333333333333333333333333333........",
    "..1111111111111111..A..222222222222222222222222222222..B..333333333333333333333333333333........",
    "..1111111111111111..A..222222222222222222222222222222..B..333333333333333333333333333333........",
    "..1111111111111111..A..222222222222222222222222222222..B..333333333333333333333333333333........",
    "..1111111111111111..A..222222222222222222222222222222..B..333333333333333333333333333333........",
    "..1111111111111111.....222222222222222222222222222222.....333333333333333333333333333333........",
    "..1111111111111111.....222222222222222222222222222222.....333333333333333333333333333333........",
    "..1111111111111111.....222222222222222222222222222222.....333333333333333333333333333333........",
    "..1111111111111111.....222222222222222222222222222222.....333333333333333333333333333333........",
    ".......................222222222222222222222222222222.....333333333333333333333333333333........",
    ".......................222222222222222222222222222222.....333333333333333333333333333333........",
    ".......................222222222222222222222222222222.....333333333333333333333333333333........",
    ".......................222222222222222222222222222222.....333333333333333333333333333333........",
    "................................................................................................",
    "................................................................................................",
    "........................................................................CCCC....................",
    "................................................................................................",
    "................................................................................................",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555..D..4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555..D..4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555..D..4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555..D..4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555.....4444444444444444444444444444444444444........",
    "..............55555555555555555555555555555555..................................................",
    "..............55555555555555555555555555555555..................................................",
    "................................................................................................",
    "...................................................777777777777777777777777777777777777777777...",
    "........................EEEE.......................777777777777777777777777777777777777777777...",
    "...................................................777777777777777777777777777777777777777777...",
    "...................................................777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666.....777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666.....777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666.....777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666.....777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666.....777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666..F..777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666..F..777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666..F..777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666..F..777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666.....777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666.....777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666.....777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666.....777777777777777777777777777777777777777777...",
    "..66666666666666666666666666666666666666666666.....777777777777777777777777777777777777777777...",
    "...................................................777777777777777777777777777777777777777777...",
    "...................................................777777777777777777777777777777777777777777...",
    "...................................................777777777777777777777777777777777777777777...",
    "...................................................777777777777777777777777777777777777777777...",
    "...................................................777777777777777777777777777777777777777777...",
    "...................................................777777777777777777777777777777777777777777...",
    "................................................................................................",
    "................................................................................................",
};

internal void
DefineMap_Depths(map_def *Map)
{
    Map->Name = "Ember Depths";
    Map->Kind = MapKind_Bounded;
    Map->Seed = 9127;
    Map->Layout = DepthsLayout;
    Map->Width = 96;
    Map->Height = ArrayCount(DepthsLayout);
    Map->Outside = TerrainKind_StoneWall;
    Map->MonsterPopulation = 0;
    Map->Dungeon = true;
}

#endif
