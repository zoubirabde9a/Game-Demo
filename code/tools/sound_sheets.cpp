/* Writes every made sound effect (client/sounds/sounds.cpp) to
   build/sounds/<name>.wav, 16-bit mono at the game's rate, so sounds can
   be listened to without starting the game. Built and run by sounds.bat. */

#include <direct.h>
#include "../app.cpp"

// NOTE(zoubir): a RIFF/WAVE file: the header, then the samples
internal void
WriteWav(char *Path, i16 *Samples, u32 Count)
{
    FILE *File = fopen(Path, "wb");
    if (!File)
    {
        return;
    }
    u32 DataSize = Count * sizeof(i16);
    u32 RiffSize = 36 + DataSize;
    u32 FormatSize = 16;
    u16 Format = 1;
    u16 Channels = 1;
    u32 Rate = SFX_RATE;
    u32 ByteRate = SFX_RATE * sizeof(i16);
    u16 Align = sizeof(i16);
    u16 Bits = 16;
    fwrite("RIFF", 1, 4, File);
    fwrite(&RiffSize, 4, 1, File);
    fwrite("WAVEfmt ", 1, 8, File);
    fwrite(&FormatSize, 4, 1, File);
    fwrite(&Format, 2, 1, File);
    fwrite(&Channels, 2, 1, File);
    fwrite(&Rate, 4, 1, File);
    fwrite(&ByteRate, 4, 1, File);
    fwrite(&Align, 2, 1, File);
    fwrite(&Bits, 2, 1, File);
    fwrite("data", 1, 4, File);
    fwrite(&DataSize, 4, 1, File);
    fwrite(Samples, sizeof(i16), Count, File);
    fclose(File);
}

int main()
{
    _mkdir("sounds");
    sfx_reverb *Reverb = (sfx_reverb *)calloc(1, sizeof(sfx_reverb));
    for(u32 Index = 0; Index < ArrayCount(SoundEffects); Index++)
    {
        sound_effect *Effect = &SoundEffects[Index];
        u32 Count = SoundEffectSampleCount(Effect);
        float *Work = (float *)calloc(Count, sizeof(float));
        i16 *Samples = (i16 *)calloc(Count, sizeof(i16));
        RenderSoundEffect(Effect, Work, Reverb);
        // NOTE(zoubir): at its level in the mix, as the game plays it
        for(u32 Sample = 0; Sample < Count; Sample++)
        {
            float Value = Minimum(1.f, Maximum(-1.f, Effect->Gain * Work[Sample]));
            Samples[Sample] = (i16)(Value * 32767.f);
        }
        char Path[256];
        snprintf(Path, sizeof(Path), "sounds/%s.wav", Effect->Name);
        WriteWav(Path, Samples, Count);
        printf("%s\n", Path);
        free(Samples);
        free(Work);
    }
    free(Reverb);
    return 0;
}
