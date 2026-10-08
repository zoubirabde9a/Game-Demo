/* Experience: what a player earns, and the level it makes.

   Killing a player       XP_PLAYER_KILL, plus XP_PER_LEVEL_GAP for each
                          level the victim has over the killer (less for
                          each it has under), kept within
                          XP_PLAYER_KILL_MIN..XP_PLAYER_KILL_MAX
   Killing a monster      XP_MONSTER_KILL, a token next to a player
   Being in the match     XP_PER_SECOND, alive or dead, so a player who
                          keeps losing still levels

   Level L takes XpToReach(L) in all: 80 for level 2, then each level
   costs 20 more than the last; level 20, the most, takes 4940. At two a
   second, time alone reaches level 2 in 40 s and level 10 in about 20
   minutes; kills get there far sooner. Every level after the first is a
   talent point (talents.cpp).

   In a dungeon run a player earns one level per room the party clears,
   starting from level 1: the cap is DUNGEON_LEVELS_AHEAD plus the rooms
   with monsters cleared, so waiting around cannot farm levels.
   Experience past the cap stops one point short of the next level, which
   comes the moment the next room is cleared. The cap sits below what
   time alone would give at 90 s a room (level 7 leaving the Crypt where
   the trickle reaches 9), so a dungeon party is a little lower than the
   bosses were first tuned for. */

#define XP_PLAYER_KILL 100
#define XP_PER_LEVEL_GAP 15
#define XP_PLAYER_KILL_MIN 50
#define XP_PLAYER_KILL_MAX 250
#define XP_MONSTER_KILL 5
#define XP_PER_SECOND 2
#define PLAYER_MAX_LEVEL 20
// NOTE(zoubir): the first level's cost and how much more each costs
#define XP_FIRST_LEVEL 80
#define XP_LEVEL_STEP 20
// NOTE(zoubir): the most a dungeon player can be is this plus the rooms
// with monsters cleared: level 1 until the first fight is won
#define DUNGEON_LEVELS_AHEAD 1

// NOTE(zoubir): experience needed in all to be Level
inline u32
XpToReach(u32 Level)
{
    u32 Steps = Level > 1 ? Level - 1 : 0;
    u32 Result = XP_FIRST_LEVEL * Steps +
        (Steps > 1 ? XP_LEVEL_STEP * Steps * (Steps - 1) / 2 : 0);
    return Result;
}

inline u32
LevelForXp(u32 Xp)
{
    u32 Result = 1;
    while (Result < PLAYER_MAX_LEVEL && Xp >= XpToReach(Result + 1))
    {
        Result++;
    }
    return Result;
}

// NOTE(zoubir): how far the player is from its level to the next, 0..1;
// 1 at the top level
inline float
LevelProgress(u32 Xp)
{
    u32 Level = LevelForXp(Xp);
    if (Level >= PLAYER_MAX_LEVEL)
    {
        return 1.f;
    }
    u32 From = XpToReach(Level);
    u32 To = XpToReach(Level + 1);
    float Result = (float)(Xp - From) / (float)(To - From);
    return Result;
}

// NOTE(zoubir): the highest level a player can reach now:
// PLAYER_MAX_LEVEL, or in a dungeon run DUNGEON_LEVELS_AHEAD past the
// rooms cleared
inline u32
LevelCap(app_state *AppState)
{
    u32 Result = PLAYER_MAX_LEVEL;
    if (AppState->Dungeon)
    {
        Result = Minimum(Result, DUNGEON_LEVELS_AHEAD + AppState->DungeonRoomsCleared);
    }
    return Result;
}

inline u32
PlayerKillXp(u32 KillerLevel, u32 VictimLevel)
{
    i32 Result = XP_PLAYER_KILL +
        XP_PER_LEVEL_GAP * ((i32)VictimLevel - (i32)KillerLevel);
    Result = Maximum(XP_PLAYER_KILL_MIN, Minimum(XP_PLAYER_KILL_MAX, Result));
    return (u32)Result;
}

// NOTE(zoubir): Amount more experience for Slot; a level reached lights
// the player up for everyone (SimBurst_LevelUp)
internal void
AwardXp(app_state *AppState, player_slot *Slot, u32 Amount)
{
    // NOTE(zoubir): under a dungeon's cap, one point short of the level
    // past it; never taken back below what the player has
    u32 Level = LevelCap(AppState);
    u32 Cap = Level < PLAYER_MAX_LEVEL ? XpToReach(Level + 1) - 1 :
        XpToReach(PLAYER_MAX_LEVEL);
    Slot->Xp = Maximum(Slot->Xp, Minimum(Cap, Slot->Xp + Amount));
    Level = LevelForXp(Slot->Xp);
    if (Level > Slot->Level)
    {
        Slot->Level = Level;
        world_entity *Player = Slot->Entity;
        if (Player && Player->IsPresent)
        {
            EmitBurst(&AppState->Events, SimBurst_LevelUp,
                      (u8)Player->PlayerIndex, Player->Position);
            EmitSound(&AppState->Events, AssetType_SfxLevelUp, Player->Position);
        }
    }
}

// NOTE(zoubir): DamageEntity (entity.cpp) on a kill: what the killer
// earns, and the talents a player kill sets off (Pyre, Momentum)
internal void
AwardKill(app_state *AppState, player_slot *Killer, world_entity *Target)
{
    if (Target->Type == EntityType_Monster)
    {
        AwardXp(AppState, Killer, XP_MONSTER_KILL);
        return;
    }
    player_slot *Victim = &AppState->Players[Target->PlayerIndex];
    AwardXp(AppState, Killer, PlayerKillXp(Killer->Level, Victim->Level));
    world_entity *Player = Killer->Entity;
    if (Player && Player->IsPresent)
    {
        if (Killer->Ranks[Talent_Pyre])
        {
            Player->ActionCooldowns[PlayerAction_FireBall] = 0.f;
        }
        if (Killer->Ranks[Talent_Momentum])
        {
            RefundOnKill(Player);
        }
    }
}
