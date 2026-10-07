// One line per monster kind. Order does not matter. Each file is read in
// three passes: MONSTER_NAME_PASS (the kind's name), the plain pass (stats
// and abilities, sim/monster_kinds.cpp) and MONSTER_ART_PASS (drawing,
// art/monster_art.cpp, client only). Git merges this file
// with merge=union (see .gitattributes), so two people adding a monster at
// once both keep their line. Read README.md in this folder first.
#include "brute.cpp"
#include "bat.cpp"
#include "ravager.cpp"
#include "toad.cpp"
#include "shade.cpp"
#include "imp.cpp"
#include "spider.cpp"
#include "slime.cpp"
#include "necromancy.cpp"
#include "warden.cpp"
#include "warlord.cpp"
#include "lurker.cpp"
#include "crypt_gravecaller.cpp"
