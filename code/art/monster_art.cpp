/* Monster art: the drawing half of every monster file. Reads
   sim/monsters/monster_list.inc in MONSTER_ART_PASS, so each kind's
   DrawMonster_<Name> (and any helpers it uses) is compiled here and never
   in the game rules the server builds. */

typedef void monster_draw_function(sprite_canvas *Canvas, monster_pose Pose);

#define MONSTER_ART_PASS
#include "../sim/monsters/monster_list.inc"
#undef MONSTER_ART_PASS

global_variable monster_draw_function *MonsterDrawFunctions[MonsterKind_Count] =
{
#define MONSTER(Name) DrawMonster_##Name,
#define MONSTER_NAME_PASS
#include "../sim/monsters/monster_list.inc"
#undef MONSTER_NAME_PASS
#undef MONSTER
};
