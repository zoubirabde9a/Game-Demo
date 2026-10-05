/* Player casts: the one wind-up every player spell that takes time goes
   through. Pressing such a spell does not fire it: the ability that owns
   it spends the cooldown and calls StartPlayerCast. While the cast runs
   the player shows the cast pose, keeps the aim it pressed with
   (CastingDirection), walks at the spell's MoveScale and, for a Hover
   spell, holds its height. When CastLeft runs out, UpdatePlayerCast
   (player_update/casts.cpp) hands the spell back to its owner, which
   fires it.

   One cast at a time: a spell pressed during another cast is ignored. A
   dash or a blink, a stun, death or a rewind freezing the caster cuts the
   cast short (CancelPlayerCast) and the cooldown stays spent.

   Clients draw a bar over every caster (client/cast_bars.cpp) from
   CastSpell and CastLeft, which snapshots carry (net_snapshot.Casts).

   A new spell with a wind-up is a row in PlayerSpells (and its name in
   player_spell), a case in FinishPlayerCast, and a call to
   StartPlayerCast where its key is read instead of firing at once. */

enum player_spell
{
    PlayerSpell_None,
    PlayerSpell_Shockwave,
    PlayerSpell_Push,
    PlayerSpell_Launch,
    PlayerSpell_Slam,
    PlayerSpell_Blink,
    PlayerSpell_FrostNova,
    PlayerSpell_GravityWell,
    // NOTE(zoubir): the rewinds stay last, in rewind_kind order
    // (time_rewind/rewind_abilities.cpp RewindSpell)
    PlayerSpell_RewindSelf,
    PlayerSpell_RewindBubble,
    PlayerSpell_RewindWorld,
    PlayerSpell_Count
};

struct player_spell_cast
{
    // NOTE(zoubir): seconds from the press to the spell going off
    float CastTime;
    // NOTE(zoubir): share of the walk speed left while it winds up
    float MoveScale;
    // NOTE(zoubir): the caster hangs in the air instead of falling
    bool32 Hover;
    // NOTE(zoubir): drawn over the cast bar
    char *Name;
};

global_variable player_spell_cast PlayerSpells[PlayerSpell_Count] =
{
    {0.f, 1.f, false, ""},
    // NOTE(zoubir): Shockwave (E), Push (R) and Launch (A), the area
    // abilities (player_abilities/area_abilities.cpp)
    {0.3f, 0.35f, false, "Shockwave"},
    {0.2f, 0.35f, false, "Push"},
    {0.4f, 0.35f, false, "Launch"},
    // NOTE(zoubir): Slam (C): the player hangs in the air, then dives
    // (player_abilities/movement_abilities.cpp)
    {0.25f, 0.35f, true, "Slam"},
    // NOTE(zoubir): Blink (F): the jump goes to where the cursor is when
    // the cast ends (player_abilities/movement_abilities.cpp)
    {0.5f, 0.35f, false, "Blink"},
    // NOTE(zoubir): Frost Nova (G) and Gravity Well (T), area abilities
    // only the talent tree unlocks
    {0.25f, 0.35f, false, "Frost Nova"},
    {0.35f, 0.35f, false, "Gravity Well"},
    // NOTE(zoubir): the time rewinds (time_rewind/rewind_abilities.cpp):
    // the hold and the playback follow the cast
    {0.5f, 0.5f, false, "Rewind"},
    {0.5f, 0.5f, false, "Rewind bubble"},
    {0.5f, 0.5f, false, "Rewind world"},
};
static_assert(PlayerSpell_Count <= 16, "Spell is 4 bits on the wire");

inline bool32
IsPlayerCasting(world_entity *Player)
{
    bool32 Result = Player->CastSpell != PlayerSpell_None;
    return Result;
}

// NOTE(zoubir): the caller has checked the key and spent the cooldown
inline void
StartPlayerCast(world_entity *Player, player_spell Spell, v2 Aim)
{
    Player->CastSpell = Spell;
    Player->CastLeft = PlayerSpells[Spell].CastTime;
    Player->CastingDirection = Aim;
    Player->AnimationState.SlotIndex = 0;
}

inline void
CancelPlayerCast(world_entity *Player)
{
    Player->CastSpell = PlayerSpell_None;
    Player->CastLeft = 0.f;
}

// NOTE(zoubir): how much of the wind-up is done, 0..1, for the cast bar
inline float
PlayerCastProgress(world_entity *Player)
{
    float CastTime = Player->CastSpell < PlayerSpell_Count ?
        PlayerSpells[Player->CastSpell].CastTime : 0.f;
    float Result = CastTime > 0.f ? 1.f - Player->CastLeft / CastTime : 1.f;
    Result = Maximum(0.f, Minimum(1.f, Result));
    return Result;
}
