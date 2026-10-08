/* Announcer sound recipes (sounds.cpp, played by ui/announcer/): the
   stings under the match's title cards and callouts.

   Announce   a deep gong struck under a low brass swell, ringing on
   Fight      a hard hit: a drum thump, a bright brass stab and a crack
   Countdown  a short wooden tick with a bell tone on top */

internal void
SfxAnnounce(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Swell = {};
    sfx_filter Strike = {};
    float Phase = 0.f;
    float Fifth = 0.f;
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        // NOTE(zoubir): the gong: inharmonic partials over a low A
        float Gong = SfxPartial(T, 55.f, 1.4f) + 0.7f * SfxPartial(T, 110.6f, 1.f) +
            0.45f * SfxPartial(T, 152.3f, 0.7f) + 0.3f * SfxPartial(T, 231.f, 0.45f) +
            0.2f * SfxPartial(T, 347.f, 0.3f);
        float Hit = SfxLowPass(&Strike, SfxNoise(R), 900.f) * SfxEnvelope(T, 0.002f, 0.04f);
        float Brass = SfxSaw(&Phase, 110.f) + 0.6f * SfxSaw(&Fifth, 164.8f);
        float Horn = SfxLowPass(&Swell, Brass, 300.f + 900.f * SfxEnvelope(T - 0.05f, 0.25f, 0.5f)) *
            SfxEnvelope(T - 0.05f, 0.25f, 0.6f);
        S[I] = SfxSaturate(0.9f * Gong + 0.6f * Hit + 0.35f * Horn, 1.4f);
    }
    SfxAddReverb(Reverb, S, Count, 0.3f, 0.7f);
}

internal void
SfxFight(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Crack = {};
    sfx_filter Bright = {};
    float Drum = 0.f;
    float Phase = 0.f;
    float Third = 0.f;
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Thump = SfxSine(&Drum, SfxSweep(T, 0.12f, 150.f, 48.f)) * SfxEnvelope(T, 0.002f, 0.12f);
        float Stab = SfxLowPass(&Bright, SfxSaw(&Phase, 220.f) + 0.7f * SfxSaw(&Third, 277.2f),
                                2600.f * SfxEnvelope(T, 0.01f, 0.12f) + 300.f, 1.2f) *
            SfxEnvelope(T, 0.008f, 0.18f);
        float Snap = SfxHighPass(&Crack, SfxNoise(R), 2500.f) * SfxEnvelope(T, 0.001f, 0.03f);
        S[I] = SfxSaturate(1.2f * Thump + 0.55f * Stab + 0.4f * Snap, 1.8f);
    }
    SfxAddReverb(Reverb, S, Count, 0.18f, 0.45f);
}

internal void
SfxCountdown(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Wood = {};
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Knock = SfxBandPass(&Wood, SfxNoise(R), 1400.f, 3.f) * SfxEnvelope(T, 0.001f, 0.02f);
        float Bell = SfxPartial(T, 880.f, 0.12f) + 0.3f * SfxPartial(T, 1760.f, 0.07f);
        S[I] = 0.8f * Knock + 0.5f * Bell;
    }
    SfxAddReverb(Reverb, S, Count, 0.12f, 0.3f);
}
