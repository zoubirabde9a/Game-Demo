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
#include "crypt_brood_queen.cpp"
#include "crypt_hollow_king.cpp"
#include "depths_forgemaster.cpp"
#include "depths_cinder_wyrm.cpp"
#include "depths_ember_tyrant.cpp"
#include "vault_frost_colossus.cpp"
#include "vault_pale_witch.cpp"
#include "vault_rimeheart.cpp"
#include "rift_sentinel.cpp"
#include "rift_yeti.cpp"
#include "rift_wisp.cpp"
#include "rift_pylon.cpp"
#include "rift_grondmaw.cpp"
#include "rift_prism.cpp"
#include "rift_everwinter.cpp"
