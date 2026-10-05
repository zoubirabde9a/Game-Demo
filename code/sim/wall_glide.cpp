/* Wall glide: a player who runs or dashes into a wall, a rock or a unit
   at a slant keeps going along it at the speed they had, instead of
   keeping only the part of the speed that was already along the face.
   Sliding by projection alone threw most of a dash away on a glancing
   touch (a dash at 45 degrees left at 0.7 of its speed, at 30 degrees off
   the face 0.5), and walking diagonally along a wall crawled.

   The slower the slant, the more of the speed is turned along the face;
   from WALL_GLIDE_TURN on, all of it. Square on, nearly all of it is
   still lost, so a wall still stops a player who runs straight at it.
   MoveEntity calls GlideAlongWall after a blocking hit has removed the
   part of the move and the velocity into the face, and before the corner
   slip. Thrown bodies bounce instead (impacts.cpp) and a blink stops at
   the first wall, so neither glides. */

// NOTE(zoubir): how many times the share of the speed already along the
// face is kept, at most all of it. At 2, a hit 30 degrees or more off
// square keeps its whole speed; 10 degrees off keeps 0.35 of it, where
// sliding alone kept 0.17
#define WALL_GLIDE_TURN 2.f

// NOTE(zoubir): scales Slid, what is left of Full after the part along
// Normal was removed, back up toward Full's length
inline v2
GlideLength(v2 Full, v2 Slid)
{
    v2 Result = Slid;
    float FullLength = Length(Full);
    float SlidLength = Length(Slid);
    if (SlidLength > 0.0001f && FullLength > SlidLength)
    {
        float Keep = Minimum(1.f, WALL_GLIDE_TURN * SlidLength / FullLength);
        float NewLength = Maximum(SlidLength, Keep * FullLength);
        Result = (NewLength / SlidLength) * Slid;
    }
    return Result;
}

// NOTE(zoubir): Velocity is what the entity had before the hit and
// Blocked what was left of the move; Entity->Velocity and *Delta are
// both already slid along the face with Normal
internal void
GlideAlongWall(world_entity *Entity, v3 Normal, v3 Velocity, v3 Blocked,
               v3 *Delta)
{
    if (Entity->Type != EntityType_Player || Entity->Phasing ||
        Normal.Z != 0.f)
    {
        return;
    }
    Entity->Velocity.XY = GlideLength(Velocity.XY, Entity->Velocity.XY);
    Delta->XY = GlideLength(Blocked.XY, Delta->XY);
}
