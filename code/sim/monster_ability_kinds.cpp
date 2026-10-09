/* Monster ability kinds (monster_kinds.cpp): what each kind of
   monster_ability does, in a word and a note. How each one runs is in
   monster_abilities.cpp and its folder; sim/monsters/README.md has the
   fields each kind reads. */

enum monster_ability_kind
{
    MonsterAbility_None,
    // NOTE(zoubir): hits every player within Radius of the monster
    MonsterAbility_Slam,
    // NOTE(zoubir): runs along the aim locked at windup start, hitting the
    // first player it touches; a wall cuts it short and stuns the monster
    MonsterAbility_Charge,
    // NOTE(zoubir): marks Count spots around the target during windup,
    // each blows up for Damage within Radius when the windup ends
    MonsterAbility_Mortar,
    // NOTE(zoubir): marks a spot behind the target during windup, then
    // appears there and strikes everything within Radius
    MonsterAbility_Blink,
    // NOTE(zoubir): throws Count shots fanned over Spread degrees along
    // the aim locked at windup start. Shots fly at Speed for Active
    // seconds, stop at walls and hit the first player within Radius
    MonsterAbility_Volley,
    // NOTE(zoubir): raises Count monsters of SummonKind on spots marked
    // during windup, while fewer than MaxActive of its summons live.
    // Summons crumble when their summoner dies
    MonsterAbility_Summon,
    // NOTE(zoubir): heals the most hurt ally within Radius by Heal; only
    // starts when an ally is below MEND_THRESHOLD of its health
    MonsterAbility_Mend,
    // NOTE(zoubir): digs in when the windup ends and stays underground
    // (immune) for Active seconds while a ripple tunnels toward the
    // target. The landing spot follows the target until the last
    // BURROW_LOCK_SHARE of Active, then locks and is marked; the monster
    // erupts there, hitting everything within Radius
    MonsterAbility_Burrow,
    // NOTE(zoubir): a blow nobody can step out of, dash through or jump
    // over: it lands on the player the monster is after when the windup
    // ends (by threat in a dungeon run), wherever they stand within
    // MaxRange, so a party wants a tank to take it, and a tank that
    // taunts during the windup takes it off a friend. Spread above 0
    // first blinks the monster to Spread from the victim when it is
    // farther than that. Hits only the victim; a ward still takes it.
    // Radius is only the size of the mark drawn round the victim
    MonsterAbility_Smite,
    // NOTE(zoubir): a ring of frost rolls out from the monster through the
    // Active time at Speed, out to Radius; Count rings, each Spread behind
    // the one before. A ring hits every player on the ground once as it
    // passes them, walls or not: the only way past is to jump it
    // (monster_abilities/waves_beams_shards.cpp)
    MonsterAbility_Wave,
    // NOTE(zoubir): a beam of light Speed long and 2 Radius wide from the
    // monster, cut short by the first wall. It shows during the windup,
    // then sweeps Spread degrees across the target through the Active
    // time, hitting each player for Damage as its edge crosses them. A
    // jump does not clear it; a pillar between it and you does
    MonsterAbility_Beam,
    // NOTE(zoubir): a gravity well opens Spread toward the target (0: at
    // the monster's feet) and drags every player within Radius toward it
    // at Speed (an acceleration) through the Active time, then collapses,
    // hitting everyone within InnerRadius of it. A jump does not help;
    // running against it, a dash or a blink does
    // (monster_abilities/wells_brands_mirrors.cpp)
    MonsterAbility_Pull,
    // NOTE(zoubir): brands the player within MaxRange farthest from the
    // monster; the brand follows them through the windup, then bursts on
    // them and every other player within Radius of them
    MonsterAbility_Brand,
    // NOTE(zoubir): a mirror held up through the Active time: hits on the
    // monster do nothing, and Spread of each one, at most Damage, is
    // turned back on whoever struck. Radius is the size drawn. Only in a
    // dungeon run (DungeonScaleDamage)
    MonsterAbility_Reflect,
    // NOTE(zoubir): Count circles of light of Radius, up to Spread from the
    // monster; when the windup ends every player outside all of them is
    // hit, past any dodge or jump
    MonsterAbility_Eclipse,
    MonsterAbility_Count
};
