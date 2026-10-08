/* Experience: what a player earns, and the level it makes.

   Killing a player       XP_PLAYER_KILL, plus XP_PER_LEVEL_GAP for each
                          level the victim has over the killer (less for
                          each it has under), kept within
                          XP_PLAYER_KILL_MIN..XP_PLAYER_KILL_MAX
   Killing a monster      XP_MONSTER_KILL, a token next to a player
   Being in the match     XP_PER_SECOND, alive or dead, so a player who
                          keeps losing still levels

   Level L takes XpToReach(L) in all: 80 for level 2, then each level
   costs 20 more than the last; level 20, the most in a duel, takes 4940,
   and level 30, the most in a dungeon, 10440. At two a second, time alone
   reaches level 2 in 40 s and level 10 in about 20 minutes; kills get
   there far sooner. Every level after the first is a talent point
   (talents.cpp).

   In a dungeon run a player earns one level per room the party clears,
   on top of the level it started the run at (RunStartLevel): the cap is
   that plus the rooms with monsters cleared, so waiting around cannot
   farm levels. A new run keeps the level and talents a player already
   has (StartNextRoundMap, setup.cpp), so a party that clears the Crypt
   again goes on climbing. Experience past the cap stops one point short
   of the next level, which comes the moment the next room is cleared.
   From level 1 the cap sits below what time alone would give at 90 s a
   room (level 7 leaving the Crypt where the trickle reaches 9), so a
   fresh party is a little lower than the bosses were first tuned for. */

#define XP_PLAYER_KILL 100
#define XP_PER_LEVEL_GAP 15
#define XP_PLAYER_KILL_MIN 50
#define XP_PLAYER_KILL_MAX 250
#define XP_MONSTER_KILL 5
#define XP_PER_SECOND 2
// NOTE(zoubir): the most any player can be, in a dungeon run; a duel
// stops at DUEL_MAX_LEVEL (TopLevel)
#define PLAYER_MAX_LEVEL 30
#define DUEL_MAX_LEVEL 20
// NOTE(zoubir): the first level's cost and how much more each costs
#define XP_FIRST_LEVEL 80
#define XP_LEVEL_STEP 20

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

// NOTE(zoubir): the most a player can be in this mode: PLAYER_MAX_LEVEL
// in a dungeon run, DUEL_MAX_LEVEL anywhere else
inline u32
TopLevel(app_state *AppState)
{
    u32 Result = AppState->Dungeon ? PLAYER_MAX_LEVEL : DUEL_MAX_LEVEL;
    return Result;
}

// NOTE(zoubir): how far the player is from its level to the next, 0..1;
// 1 at Top, the top level
inline float
LevelProgress(u32 Xp, u32 Top = PLAYER_MAX_LEVEL)
{
    u32 Level = LevelForXp(Xp);
    if (Level >= Top)
    {
        return 1.f;
    }
    u32 From = XpToReach(Level);
    u32 To = XpToReach(Level + 1);
    float Result = (float)(Xp - From) / (float)(To - From);
    return Result;
}

// NOTE(zoubir): the highest level Slot can reach now: TopLevel, or in a
// dungeon run one past its RunStartLevel for each room with monsters
// cleared (level 1 until the first fight is won, for a fresh player)
inline u32
LevelCap(app_state *AppState, player_slot *Slot)
{
    u32 Result = TopLevel(AppState);
    if (AppState->Dungeon)
    {
        u32 Start = Maximum(1u, Slot->RunStartLevel);
        Result = Minimum(Result, Start + AppState->DungeonRoomsCleared);
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
    u32 Level = LevelCap(AppState, Slot);
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
