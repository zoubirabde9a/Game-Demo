/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: zoubir $
   ======================================================================== */
#include "audio.h"

internal void
InitializeAudio(audio_state *AudioState)
{
    AudioState->MasterVolume = V2(1.f, 1.f);
}

internal playing_sound *
PlaySound(app_state *AppState, asset_id ID)
{
    assets *Assets = &AppState->Assets;
    audio_state *AudioState = &AppState->AudioState;
    playing_sound *Result = 0;
    loaded_audio *Audio = GetAudio(Assets, AppState, ID);
    if (Audio)
    {
        Assert(AudioState->PlayingSoundsCount < ArrayCount(AudioState->PlayingSounds));
        playing_sound *NewSound =
            &AudioState->PlayingSounds[AudioState->PlayingSoundsCount++];
        NewSound->Audio = Audio;
        NewSound->CurrentVolume = NewSound->TargetVolume =
            V2(1.f, 1.f);
        NewSound->DeltaVolume = V2(0.f, 0.f);
        NewSound->DeltaSample = 1.f;
        NewSound->SamplesPlayed = 0;
        Result = NewSound;
    }

    return Result;
}

internal void
ChangeVolume(playing_sound *Sound, float FadeInDurationInSeconds,
             v2 Volume)
{
    if (FadeInDurationInSeconds <= 0.f)
    {
        Sound->CurrentVolume = Sound->TargetVolume = Volume;
        Assert(Sound->DeltaVolume.X == 0.f);
        Assert(Sound->DeltaVolume.Y == 0.f);
    }
    else
    {
        Sound->TargetVolume = Volume;
        float OneOverFade = 1.f / FadeInDurationInSeconds;
        Sound->DeltaVolume =
            (Sound->TargetVolume - Sound->CurrentVolume) *
            OneOverFade;
    }
}

internal void
ChangePitch(playing_sound *Sound, float DeltaSample)
{
    Sound->DeltaSample = DeltaSample;
}

internal void
OutputAudio(audio_state *AudioState,
            app_sound_output_buffer *SoundBuffer, memory_arena *Arena)
{
    Assert((SoundBuffer->SampleCount & 3) == 0);
    u32 ChunkCount = SoundBuffer->SampleCount / 4;
    
    __m128 *Channel0 = AllocateArray(Arena, ChunkCount, __m128, 16);
    __m128 *Channel1 = AllocateArray(Arena, ChunkCount, __m128, 16);
    float SecondsPerSample =
        1.f / (float)SoundBuffer->SamplesPerSecond;
#define OutputChannelCount 2

//    ZeroArray(Channel0, SampleCount, float);
//    ZeroArray(Channel1, SampleCount, float);

        __m128 Zero = _mm_set1_ps(0.f);
        __m128 One = _mm_set1_ps(1.f);
    {

        __m128 *Dest0 = Channel0;
        __m128 *Dest1 = Channel1;
        for(u32 SampleIndex = 0;
            SampleIndex < ChunkCount;
            SampleIndex++)
        {
            _mm_store_ps((float *)Dest0++, Zero);
            _mm_store_ps((float *)Dest1++, Zero);
///            *Dest0++ = Zero4Wide;
//            *Dest1++ = Zero4Wide;
        }
    }
    
    for(u32 PlayingIndex = 0;
        PlayingIndex < AudioState->PlayingSoundsCount;
        )
    {
        bool32 SoundFinished = false;
        playing_sound *ThisSound = &AudioState->PlayingSounds[PlayingIndex];
        loaded_audio *LoadedAudio = ThisSound->Audio;
        u32 Channels = LoadedAudio->Channels;
        
//        i16 *Src = Data + (i32)ThisSound->SamplesPlayed * Channels;
//        u32 ThisSoundSampleCount8 = LoadedAudio->SampleCount * 8;

        v2 Volume = ThisSound->CurrentVolume;
        v2 DeltaVolume = ThisSound->DeltaVolume * SecondsPerSample;
        v2 DeltaVolumeChunk = 4.f * DeltaVolume;
        float DeltaSample = ThisSound->DeltaSample * 1.0f;
        float DeltaSampleChunk = 4.f * DeltaSample;

        // Channel 0
        __m128 MasterVolume0 = _mm_set1_ps(AudioState->MasterVolume.Data[0]);
        __m128 Volume0 = _mm_setr_ps(Volume.Data[0] + 0.f * DeltaVolume.Data[0],
                                     Volume.Data[0] + 1.f * DeltaVolume.Data[0],
                                     Volume.Data[0] + 2.f * DeltaVolume.Data[0],
                                     Volume.Data[0] + 3.f * DeltaVolume.Data[0]);        
        __m128 DeltaVolumeChunk0 = _mm_set1_ps(DeltaVolumeChunk.Data[0]);
        __m128 DeltaVolume0 = _mm_set1_ps(DeltaVolume.Data[0]);
        
        // Channel 1
        __m128 MasterVolume1 = _mm_set1_ps(AudioState->MasterVolume.Data[1]);
        __m128 Volume1 = _mm_setr_ps(Volume.Data[1] + 0.f * DeltaVolume.Data[1],
                                     Volume.Data[1] + 1.f * DeltaVolume.Data[1],
                                     Volume.Data[1] + 2.f * DeltaVolume.Data[1],
                                     Volume.Data[1] + 3.f * DeltaVolume.Data[1]);        
        __m128 DeltaVolumeChunk1 = _mm_set1_ps(DeltaVolumeChunk.Data[1]);
        __m128 DeltaVolume1 = _mm_set1_ps(DeltaVolume.Data[1]);
        
        
        __m128 *Dest0 = Channel0;
        __m128 *Dest1 = Channel1;

        Assert(ThisSound->SamplesPlayed >= 0.f);

        u32 ChunksToMix = ChunkCount;
        
        float FloatChunksRemainingInSound =
            (LoadedAudio->SampleCount -
             RoundFloatToI32(ThisSound->SamplesPlayed)) /
            DeltaSampleChunk;
        
        u32 ChunksRemainingInSound =
            RoundFloatToU32(FloatChunksRemainingInSound);
        
        if (ChunksToMix >= ChunksRemainingInSound)
        {
            ChunksToMix = ChunksRemainingInSound;
            SoundFinished = true;
        }

        u32 VolumeEndsAt[OutputChannelCount] = {};
        for(u32 ChannelIndex = 0;
            ChannelIndex < ArrayCount(VolumeEndsAt);
            ChannelIndex++)
        {
            if (DeltaVolumeChunk.Data[ChannelIndex] != 0.f)
            {
                float Diff =
                    ThisSound->TargetVolume.Data[ChannelIndex] -
                    Volume.Data[ChannelIndex];
                // 0.125f = 1/8
                u32 VolumeChunkCount =
                    (u32)((Diff / DeltaVolumeChunk.Data[ChannelIndex]) + 0.5f);
                
                if (ChunksToMix > VolumeChunkCount)
                {
                    ChunksToMix = VolumeChunkCount;
                    VolumeEndsAt[ChannelIndex] = ChunksToMix;
                }
            }                                   
        }

        //TODO(zoubir): handle stereo
        float BeginSamplePosition = ThisSound->SamplesPlayed;
        float EndSamplePosition = BeginSamplePosition + ChunksToMix * DeltaSampleChunk;
        float LoopIndexC =
            (EndSamplePosition - BeginSamplePosition) / ChunksToMix;

        __m128 ChunksToMix4Wide = _mm_set1_ps((float)ChunksToMix);
        
        __m128 BeginVolume0 = _mm_load_ps((float *)&Volume0);
        __m128 BeginVolume1 = _mm_load_ps((float *)&Volume1);
        __m128 EndVolume0 = _mm_add_ps(BeginVolume0, _mm_mul_ps(ChunksToMix4Wide, DeltaVolumeChunk0));
        __m128 EndVolume1 = _mm_add_ps(BeginVolume1, _mm_mul_ps(ChunksToMix4Wide, DeltaVolumeChunk1));

        Assert(ChunksToMix > 0);
        
        __m128 LoopIndexVolume0C =
            _mm_div_ps(_mm_sub_ps(EndVolume0, BeginVolume0), ChunksToMix4Wide);
        __m128 LoopIndexVolume1C =
            _mm_div_ps(_mm_sub_ps(EndVolume1, BeginVolume1), ChunksToMix4Wide);        
        
        for(u32 LoopIndex = 0;
            LoopIndex < ChunksToMix;
            LoopIndex++)
        {
            float SamplePosition = BeginSamplePosition +
                LoopIndexC * (float)LoopIndex;
            __m128 LoopIndex4Wide = _mm_set1_ps((float)LoopIndex);
            __m128 VolumeValue0 = _mm_add_ps(BeginVolume0, _mm_mul_ps(LoopIndexVolume0C, LoopIndex4Wide));
            __m128 VolumeValue1 = _mm_add_ps(BeginVolume1, _mm_mul_ps(LoopIndexVolume1C, LoopIndex4Wide));            
#if 1
            __m128 SamplePos = _mm_setr_ps(SamplePosition + 0.f * DeltaSample,
                                           SamplePosition + 1.f * DeltaSample,
                                           SamplePosition + 2.f * DeltaSample,
                                           SamplePosition + 3.f * DeltaSample);
            __m128i SampleIndex = _mm_cvttps_epi32(SamplePos);
            __m128 Frac = _mm_sub_ps(SamplePos, _mm_cvtepi32_ps(SampleIndex));
            
            __m128 SampleValueF =
                _mm_setr_ps(LoadedAudio->Samples[0][((i32 *)&SampleIndex)[0]],
                            LoadedAudio->Samples[0][((i32 *)&SampleIndex)[1]],
                            LoadedAudio->Samples[0][((i32 *)&SampleIndex)[2]],
                            LoadedAudio->Samples[0][((i32 *)&SampleIndex)[3]]);

            __m128 SampleValueC =
                _mm_setr_ps(LoadedAudio->Samples[0][((i32 *)&SampleIndex)[0] + 1],
                            LoadedAudio->Samples[0][((i32 *)&SampleIndex)[1] + 1],
                            LoadedAudio->Samples[0][((i32 *)&SampleIndex)[2] + 1],
                            LoadedAudio->Samples[0][((i32 *)&SampleIndex)[3] + 1]);

            __m128 SampleValue = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(One, Frac), SampleValueF),
                                            _mm_mul_ps(Frac, SampleValueC));
#else
            __m128 SampleValue = _mm_setr_ps(LoadedAudio->Samples[0][RoundFloatToU32(SamplePosition + 0.f * DeltaSample)],
                                              LoadedAudio->Samples[0][RoundFloatToU32(SamplePosition + 1.f * DeltaSample)],
                                              LoadedAudio->Samples[0][RoundFloatToU32(SamplePosition + 2.f * DeltaSample)],
                                              LoadedAudio->Samples[0][RoundFloatToU32(SamplePosition + 3.f * DeltaSample)]);
            #endif
            __m128 D0 = _mm_load_ps((float *)&Dest0[0]);
            __m128 D1 = _mm_load_ps((float *)&Dest1[0]);

            D0 = _mm_add_ps(D0, _mm_mul_ps(_mm_mul_ps(MasterVolume0, VolumeValue0),
                                         SampleValue));
            D1 = _mm_add_ps(D1, _mm_mul_ps(_mm_mul_ps(MasterVolume1, VolumeValue1),
                                         SampleValue));
            
            _mm_store_ps((float *)&Dest0[0], D0);
            _mm_store_ps((float *)&Dest1[0], D1);

            Dest0++;
            Dest1++;
        }

        Volume0 = _mm_load_ps((float *)&EndVolume0);
        Volume1 = _mm_load_ps((float *)&EndVolume1);
        
        ThisSound->CurrentVolume.Data[0] = ((float*)&Volume0)[0];
        ThisSound->CurrentVolume.Data[1] = ((float*)&Volume1)[1];
        // TODO(zoubir): not correct yet, need to truncate
        // the loop
        
        for(u32 ChannelIndex = 0;
            ChannelIndex < ArrayCount(VolumeEndsAt);
            ChannelIndex++)
        {
            if (ChunksToMix == VolumeEndsAt[ChannelIndex])
            {
                ThisSound->CurrentVolume.Data[ChannelIndex] =
                    ThisSound->TargetVolume.Data[ChannelIndex];
                ThisSound->DeltaVolume.Data[ChannelIndex] = 0.f;
            }
        }
        
        ThisSound->SamplesPlayed = EndSamplePosition;
        
        if (SoundFinished)
        {
            AudioState->PlayingSounds[PlayingIndex] =
                AudioState->PlayingSounds[(AudioState->PlayingSoundsCount--) - 1];
        }
        else
        {
            PlayingIndex++;
        }
    }

    __m128 *Source0 = Channel0;
    __m128 *Source1 = Channel1;

    __m128i *SampleOut = (__m128i *)SoundBuffer->Samples;
    for(u32 SampleIndex = 0;
        SampleIndex < ChunkCount;
        SampleIndex++)
    {
//    __m128 MaxS16 = _mm_set1_ps(32767.f);
//    __m128 MinS16 = _mm_set1_ps(-32768.f);

        __m128i L = _mm_cvtps_epi32(*Source0++);
        __m128i R = _mm_cvtps_epi32(*Source1++);

        __m128i LR0 = _mm_unpacklo_epi32(L, R);
        __m128i LR1 = _mm_unpackhi_epi32(L, R);
                
        __m128i S01 = _mm_packs_epi32(LR0, LR1);
        
        *SampleOut++ = S01;
    }

}


