/* Sound tests (client/sounds/): every made sound effect renders, is not
   silent, never clips, has no number gone wrong in it, comes out the same
   twice, and has its own asset type. Included by sim_tests.cpp, which
   calls RunSoundTests. */

internal void
TestSoundEffectsRender()
{
    static sfx_reverb Reverb;
    for(u32 Index = 0; Index < ArrayCount(SoundEffects); Index++)
    {
        sound_effect *Effect = &SoundEffects[Index];
        u32 Count = SoundEffectSampleCount(Effect);
        Check(Count > SFX_RATE / 10 && Count < 2 * SFX_RATE);
        Check(Effect->Type >= AssetType_PackCount && Effect->Type < AssetType_Count);
        Check(FindSoundEffect(Effect->Type) == Effect);
        Check(Effect->Gain > 0.f && Effect->Gain <= 1.f);
        float *First = (float *)calloc(Count, sizeof(float));
        float *Second = (float *)calloc(Count, sizeof(float));
        Check(RenderSoundEffect(Effect, First, &Reverb));
        RenderSoundEffect(Effect, Second, &Reverb);
        float Peak = 0.f;
        double Energy = 0.0;
        bool32 Same = true;
        for(u32 Sample = 0; Sample < Count; Sample++)
        {
            float Value = First[Sample];
            Check(Value == Value);
            Peak = Maximum(Peak, Absolute(Value));
            Energy += (double)Value * Value;
            Same &= Value == Second[Sample];
        }
        Check(Peak > 0.8f && Peak <= SFX_PEAK + 0.001f);
        Check(Energy / (double)Count > 1e-4);
        Check(Same);
        // NOTE(zoubir): it ends in silence, so it never stops on a click
        Check(Absolute(First[Count - 1]) < 0.01f);
        free(First);
        free(Second);
    }
}

internal void
RunSoundTests()
{
    TestSoundEffectsRender();
}
