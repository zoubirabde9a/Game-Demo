// world_entity fields used only by monsters, and by the shots and hazards
// they make. Included inside struct world_entity in entity.h, so they are
// read as Entity->AbilityPhase like any other field. Add new monster state
// here, not in entity.h. Types and limits are in monster_types.h.
    monster_kind MonsterKind;
    // NOTE(zoubir): multiplied with the sprite, 0 means untinted
    u32 Tint;
    // NOTE(zoubir): seconds until a monster can hit again
    float AttackCooldown;
    // NOTE(zoubir): monsters with no player in range stroll this way
    // until WanderTimer runs out, then pick a new direction (or rest)
    v2 WanderDirection;
    float WanderTimer;
    // NOTE(zoubir): the monster ability in progress, if any
    ability_phase AbilityPhase;
    u32 AbilityIndex;
    float AbilityTimer;
    float AbilityCooldowns[MAX_MONSTER_ABILITIES];
    // NOTE(zoubir): locked direction for charges, landing spots for
    // mortars and blinks
    v2 AbilityAim;
    v2 AbilityPoints[MAX_ABILITY_POINTS];
    u32 AbilityPointCount;
    bool32 AbilityHasHit;
    // NOTE(zoubir): monster_affix of an elite monster, and of the shots and
    // hazards it makes; 0 for ordinary ones. See sim/monster_affixes.cpp
    u32 EliteAffix;
    // NOTE(zoubir): entity slots are reused, so monsters that point at
    // each other (summons, heal targets) keep the slot and a serial that
    // is never reused. Serial 0 means not registered
    u32 MonsterSerial;
    u32 SummonerSlot;
    u32 SummonerSerial;
    u32 AbilityTargetSlot;
    u32 AbilityTargetSerial;
    // NOTE(zoubir): seconds left on the shell glint after a blocked hit;
    // facing for armored monsters lives in the shared Direction field
    float BlockFlash;
