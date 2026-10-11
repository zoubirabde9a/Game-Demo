/* Sight: a monster starts no ability on a player who cannot see it.
   Many abilities reach farther than the screen shows (Bile Barrage 380,
   the window 266 above and below the player), so a monster could shell
   a player from off screen, and a player standing still (behind the Play
   screen, or just respawned) took hits from something it never saw. Now
   a monster outside the target's view keeps walking in, as it does out
   of range, and casts once it is on screen. The bite needs no rule: it
   only lands at arm's length.

   Dungeon levels keep the old rule: their fights were tuned without it
   (tools/dungeon_balance.cpp measured the party's deaths moving with
   it, both ways), and a monster there chases by threat from anywhere.

   The view is the client's (client/camera.cpp reads WORLD_VIEW_HEIGHT
   from here): WORLD_VIEW_HEIGHT units tall, and as wide as a 16:9
   window, the default one. The server never sees a window, so a wider
   window shows a little more than the rule allows and a narrower one
   less. */

// NOTE(zoubir): world units the screen shows from top to bottom; the
// camera zooms each window to this
#define WORLD_VIEW_HEIGHT 532.f
#define WORLD_VIEW_WIDTH (WORLD_VIEW_HEIGHT * 16.f / 9.f)
// NOTE(zoubir): the monster's middle must be this far inside the edge,
// so most of its body shows. Keep half the view height minus this above
// every kind's StopRange (230 at most), or a monster that stops straight
// above a player would wait there for good
#define MONSTER_SIGHT_MARGIN 24.f

// NOTE(zoubir): true when Monster stands inside the screen of a player at
// Player's position, the camera centred on the player
inline bool32
IsInPlayerView(world_entity *Player, world_entity *Monster)
{
    v2 Offset = Monster->Position.XY - Player->Position.XY;
    bool32 Result =
        Absolute(Offset.X) <= 0.5f * WORLD_VIEW_WIDTH - MONSTER_SIGHT_MARGIN &&
        Absolute(Offset.Y) <= 0.5f * WORLD_VIEW_HEIGHT - MONSTER_SIGHT_MARGIN;
    return Result;
}

// NOTE(zoubir): from UpdateMonsterAbilities: whether Monster may start an
// ability on Player from where it stands
inline bool32
SightAllowsCast(world *World, world_entity *Player, world_entity *Monster)
{
    bool32 Result = IsDungeonMap(World->MapId) || IsInPlayerView(Player, Monster);
    return Result;
}
