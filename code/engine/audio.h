#if !defined(AUDIO_H)
/* Audio state: playing_sound is one sound being mixed (its volume fade
   and playback position), and audio_state is the list of up to 64
   playing sounds plus the master volume. The mixer that uses them is in
   audio.cpp. */

#define AUDIO_H

struct playing_sound
{
    loaded_audio *Audio;
    v2 CurrentVolume;
    v2 DeltaVolume;
    v2 TargetVolume;

    float DeltaSample;
    
    float SamplesPlayed;
};

struct audio_state
{
    playing_sound PlayingSounds[64];
    u32 PlayingSoundsCount;

    v2 MasterVolume;
};

#endif
