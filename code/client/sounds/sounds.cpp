/* Sound effects: every action's sound, loaded at startup from a recorded
   file in sfx/ when there is one (sound_files.cpp), else made by code
   instead of read from asset_1.zas (synth.cpp has the parts, recipes.cpp
   the designs). SoundEffects lists each one: its asset type (the id the
   simulation emits, engine/asset_type_id.h), its recipe, its length, how
   loud it plays against the others, and how much its pitch may wander
   from one play to the next so a repeated sound does not sound stamped.

   Entry points: AddSoundEffects (once, after InitializeAssets) renders
   them all into the permanent arena and loads them as audio assets;
   PlayGameSound (play_events.cpp) plays one at its level and pitch.
   A new sound is a type in asset_type_id.h, a recipe and a row here,
   and sfx/<its name>.wav for a recorded one. */

#include "synth.cpp"
#include "recipes.cpp"
#include "announcer_recipes.cpp"
#include "sound_files.cpp"

typedef void sfx_recipe(float *Samples, u32 Count, sfx_random *Random, sfx_reverb *Reverb);

struct sound_effect
{
    asset_type_id Type;
    sfx_recipe *Recipe;
    float Seconds;
    // NOTE(zoubir): its volume in the mix, 0..1
    float Gain;
    // NOTE(zoubir): each play goes up to this share higher or lower
    float PitchJitter;
    char *Name;
};

global_variable sound_effect SoundEffects[] =
{
    {AssetType_SfxJump,          SfxJump,          0.22f, 0.32f, 0.06f, "jump"},
    {AssetType_SfxDash,          SfxDash,          0.32f, 0.42f, 0.05f, "dash"},
    {AssetType_SfxBlink,         SfxBlink,         0.6f,  0.45f, 0.03f, "blink"},
    {AssetType_SfxShield,        SfxShield,        0.9f,  0.42f, 0.02f, "shield"},
    {AssetType_SfxFireCast,      SfxFireCast,      0.45f, 0.45f, 0.06f, "fire_cast"},
    {AssetType_SfxHit,           SfxHit,           0.3f,  0.5f,  0.08f, "hit"},
    {AssetType_SfxSword,         SfxSword,         0.22f, 0.38f, 0.07f, "sword"},
    {AssetType_SfxKunai,         SfxKunai,         0.25f, 0.36f, 0.05f, "kunai"},
    {AssetType_SfxAreaCast,      SfxAreaCast,      0.7f,  0.55f, 0.04f, "area_cast"},
    {AssetType_SfxLevelUp,       SfxLevelUp,       1.3f,  0.5f,  0.f,   "level_up"},
    {AssetType_SfxWardBreak,     SfxWardBreak,     0.7f,  0.45f, 0.04f, "ward_break"},
    {AssetType_SfxRewind,        SfxRewind,        0.55f, 0.48f, 0.02f, "rewind"},
    {AssetType_SfxTaunt,         SfxTaunt,         0.7f,  0.5f,  0.04f, "taunt"},
    {AssetType_SfxShieldSlam,    SfxShieldSlam,    0.7f,  0.55f, 0.04f, "shield_slam"},
    {AssetType_SfxHeal,          SfxHeal,          0.8f,  0.36f, 0.02f, "heal"},
    {AssetType_SfxWard,          SfxWard,          0.7f,  0.38f, 0.03f, "ward"},
    {AssetType_SfxSanctuary,     SfxSanctuary,     1.4f,  0.42f, 0.f,   "sanctuary"},
    {AssetType_SfxMeteorCast,    SfxMeteorCast,    1.f,   0.42f, 0.02f, "meteor_cast"},
    {AssetType_SfxExplosion,     SfxExplosion,     1.2f,  0.65f, 0.05f, "explosion"},
    {AssetType_SfxGiantFireball, SfxGiantFireball, 0.6f,  0.52f, 0.04f, "giant_fireball"},
    {AssetType_SfxCombustion,    SfxCombustion,    0.6f,  0.48f, 0.03f, "combustion"},
    {AssetType_SfxAnnounce,      SfxAnnounce,      1.9f,  0.6f,  0.f,   "announce"},
    {AssetType_SfxFight,         SfxFight,         0.6f,  0.55f, 0.f,   "fight"},
    {AssetType_SfxCountdown,     SfxCountdown,     0.35f, 0.4f,  0.f,   "countdown"},
};

// NOTE(zoubir): the row for Type, 0 for a sound read from the pack
internal sound_effect *
FindSoundEffect(asset_type_id Type)
{
    for(u32 Index = 0; Index < ArrayCount(SoundEffects); Index++)
    {
        if (SoundEffects[Index].Type == Type)
        {
            return &SoundEffects[Index];
        }
    }
    return 0;
}

inline u32
SoundEffectSampleCount(sound_effect *Effect)
{
    u32 Result = (u32)(Effect->Seconds * (float)SFX_RATE);
    return Result;
}

// NOTE(zoubir): Effect's samples into Out (SoundEffectSampleCount of
// them), finished; the seed is the type, so a sound is the same on every
// launch. Returns false when it came out silent
internal bool32
RenderSoundEffect(sound_effect *Effect, float *Out, sfx_reverb *Reverb)
{
    u32 Count = SoundEffectSampleCount(Effect);
    for(u32 Index = 0; Index < Count; Index++)
    {
        Out[Index] = 0.f;
    }
    sfx_random Random = {0x9E3779B9u ^ ((u32)Effect->Type * 2654435761u)};
    Effect->Recipe(Out, Count, &Random, Reverb);
    bool32 Result = SfxFinish(Out, Count);
    return Result;
}

internal void
AddSoundEffects(assets *Assets, memory_arena *Permanent, memory_arena *Temporary)
{
    temporary_memory Scratch = BeginTemporaryMemory(Temporary);
    sfx_reverb *Reverb = AllocateStruct(Temporary, sfx_reverb);
    u32 Longest = 0;
    for(u32 Index = 0; Index < ArrayCount(SoundEffects); Index++)
    {
        Longest = Maximum(Longest, SoundEffectSampleCount(&SoundEffects[Index]));
    }
    float *Work = AllocateArray(Temporary, Longest, float);
    for(u32 Index = 0; Index < ArrayCount(SoundEffects); Index++)
    {
        sound_effect *Effect = &SoundEffects[Index];
        ReserveGeneratedAssets(Assets, Effect->Type, 1);
        u32 Count = 0;
        i16 *Samples = LoadSoundFile(Effect->Name, Permanent, &Count);
        if (!Samples)
        {
            RenderSoundEffect(Effect, Work, Reverb);
            Count = SoundEffectSampleCount(Effect);
            Samples = AllocateArray(Permanent, Count, i16);
            for(u32 Sample = 0; Sample < Count; Sample++)
            {
                float Value = Minimum(1.f, Maximum(-1.f, Work[Sample]));
                Samples[Sample] = (i16)(Value * 32767.f);
            }
        }
        AddGeneratedAudio(Assets, {Effect->Type, 0}, Samples, Count);
    }
    EndTemporaryMemory(Scratch);
}
