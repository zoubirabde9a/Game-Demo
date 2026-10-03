#if !defined(AUDIO_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

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
