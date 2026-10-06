/* The numbers that tune body poses (body_pose.cpp): how far each squash,
   stretch, tilt and flash goes, and how fast it eases. Change these to
   retune the feel; the code reads only these names. */

#define BODY_POSE_SLOTS 4096
// NOTE(zoubir): vertical speed at which the stretch is full
#define BODY_STRETCH_SPEED 900.f
#define BODY_STRETCH_MAX 0.16f
// NOTE(zoubir): ground speed at which a body starts to stretch sideways
// (a player's full run is 260, a dash 1440, a Push throws at 750), and
// where the stretch is full
#define BODY_RUSH_SPEED 300.f
#define BODY_RUSH_FULL_SPEED 700.f
#define BODY_RUSH_MAX 0.2f
// NOTE(zoubir): a body that moves farther than this in one frame was put
// somewhere (a respawn, a map change), not thrown: its pose starts over.
// A blink is at most PLAYER_AIM_REACH (320)
#define BODY_TELEPORT_DISTANCE 400.f
// NOTE(zoubir): a fall faster than this squashes on landing; the squash is
// full at BODY_SQUASH_FULL_SPEED
#define BODY_SQUASH_MIN_SPEED 120.f
#define BODY_SQUASH_FULL_SPEED 450.f
#define BODY_SQUASH_DEPTH 0.28f
// NOTE(zoubir): a jump in vertical speed this big within a frame, in the
// air, is a kick upward
#define BODY_POP_KICK 120.f
#define BODY_POP_HEIGHT 0.22f
// NOTE(zoubir): per second, how fast squash and pop wear off
#define BODY_POSE_RECOVERY 7.f
#define BODY_SPIN_SECONDS 0.32f
// NOTE(zoubir): radians of lean at full run speed (BODY_LEAN_SPEED), and
// how fast any tilt eases toward where it should be, per second
#define BODY_LEAN_MAX 0.12f
#define BODY_LEAN_SPEED 180.f
#define BODY_TILT_EASE 14.f
// NOTE(zoubir): a run starts when the ground speed passes RUN_START and
// stops below RUN_STOP; starting squashes this much (of a full landing)
#define BODY_RUN_START_SPEED 120.f
#define BODY_RUN_STOP_SPEED 40.f
#define BODY_RUN_START_SQUASH 0.45f
// NOTE(zoubir): a hit's flash lasts this many seconds; it is white while
// above BODY_HIT_WHITE (the first two frames at 30 a second), then red
#define BODY_HIT_FLASH_SECONDS 0.2f
#define BODY_HIT_WHITE 0.65f
#define BODY_HIT_SQUASH 0.55f
// NOTE(zoubir): a hit tilts the body this far away from the blow, back
// upright within FLINCH_SECONDS. A body knocked this fast sideways at the
// hit flinches the way it was thrown
#define BODY_FLINCH_ANGLE 0.3f
#define BODY_FLINCH_SECONDS 0.25f
#define BODY_FLINCH_KNOCK_SPEED 40.f
// NOTE(zoubir): a hit thrown closer to straight up or down the screen
// than this (the cosine of its angle) has no side to flinch to
#define BODY_FLINCH_SIDEWAYS 0.2f
// NOTE(zoubir): the local player landing a hit of HITSTOP_MIN_DAMAGE or
// more holds its own sprite this long, and nudges the camera this many
// world units toward the blow and back within NUDGE_SECONDS
#define BODY_ATTACK_HOLD_SECONDS 0.05f
#define BODY_NUDGE_DISTANCE 4.f
#define BODY_NUDGE_SECONDS 0.12f
// NOTE(zoubir): a body that leaves the ground within KNOCK_SECONDS of a
// hit (or stunned) was thrown: it tips back up to TUMBLE_ANGLE, fully at
// TUMBLE_HEIGHT above the ground
#define BODY_KNOCK_SECONDS 0.3f
#define BODY_TUMBLE_ANGLE 1.1f
#define BODY_TUMBLE_HEIGHT 60.f
// NOTE(zoubir): a cast crouches fully in this long, this deep; letting go
// pops by this share of how far it crouched. A charge with no release
// lets go by itself after BODY_WINDUP_MAX_SECONDS (a release lost with
// its snapshot)
#define BODY_WINDUP_SECONDS 0.15f
#define BODY_WINDUP_DEPTH 0.14f
#define BODY_WINDUP_RELEASE 0.7f
#define BODY_WINDUP_MAX_SECONDS 0.6f
// NOTE(zoubir): an idle body's breath: how much taller at the top, breaths
// a second, and the ground speed under which a body counts as still
#define BODY_BREATH_DEPTH 0.03f
#define BODY_BREATH_RATE 0.45f
#define BODY_STILL_SPEED 10.f
// NOTE(zoubir): how thin a turn squeezes the body, and how long it lasts
#define BODY_TURN_SQUEEZE 0.25f
#define BODY_TURN_SECONDS 0.1f
// NOTE(zoubir): a stunned body's sway, in radians and turns per second
#define BODY_DIZZY_ANGLE 0.16f
#define BODY_DIZZY_SPEED 2.2f
// NOTE(zoubir): faster than FOOTSTEP_SPEED on the ground a unit puffs dust
// every FOOTSTEP_SECONDS, faster than TRAIL_SPEED every TRAIL_SECONDS.
// Only within DUST_RANGE of the local player, so far-off crowds do not
// fill the burst pool
#define BODY_FOOTSTEP_SPEED 80.f
#define BODY_FOOTSTEP_SECONDS 0.24f
#define BODY_TRAIL_SPEED 340.f
#define BODY_TRAIL_SECONDS 0.06f
#define BODY_DUST_RANGE 1100.f
