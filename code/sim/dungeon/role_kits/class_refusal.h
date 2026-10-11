/* Class refusals (role_kits/class_refusal.cpp), the part sim/player.h
   needs before the rest (through sim/dungeon/class_states.h): the two
   ClassFlags bits every class shares to say a spell was pressed without
   the resource it costs, and what each player keeps for them
   (player_slot.Refusal). Classes use bits 0x1 to 0x20 for their own
   flags; these two are the top of the byte and mean the same for all. */

// NOTE(zoubir): ClassFlags bits for every class: a spell was refused for
// want of the class's resource in the last CLASS_REFUSED_SECONDS (the HUD
// shows "Not enough <resource>"); and a bit that flips on each fresh
// refusal, at most once in CLASS_REFUSAL_GAP, so a client hears each
// press once however the snapshots fall (ui/resource_refusal.cpp)
#define CLASS_FLAG_REFUSAL_BEAT 0x40
#define CLASS_FLAG_REFUSED 0x80
#define CLASS_FLAGS_SHARED (CLASS_FLAG_REFUSAL_BEAT | CLASS_FLAG_REFUSED)
#define CLASS_REFUSED_SECONDS 1.f
#define CLASS_REFUSAL_GAP 0.33f

struct class_refusal_slot
{
    // NOTE(zoubir): seconds the HUD keeps saying so, and before another
    // refusal may flip the beat
    float Seconds;
    float Gap;
    // NOTE(zoubir): a role key was pressed since the last refusal, so the
    // next one is fresh; a kept press tried again each tick is not
    // (role_abilities/key_presses.cpp)
    bool32 Armed;
};

// NOTE(zoubir): a class's own ClassFlags with the shared bits Old holds
// kept, for a class that sets its flags whole
inline u8
KeepSharedClassFlags(u8 Old, u32 Flags)
{
    u8 Result = (u8)((Flags & ~(u32)CLASS_FLAGS_SHARED) | (Old & CLASS_FLAGS_SHARED));
    return Result;
}
