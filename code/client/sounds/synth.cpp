/* Sound synth (sounds.cpp): the parts every sound effect is built from,
   at SFX_RATE samples a second, one channel, as floats about -1..1.
   White noise from a seeded generator, so a sound comes out the same on
   every launch; a state-variable filter whose cutoff may move every
   sample; envelopes and pitch sweeps; soft clipping; a small room reverb
   (four damped combs into two all-passes, after Schroeder and Moorer)
   for the sounds that want a tail; and the finish every sound gets:
   DC removed, the last few milliseconds faded, the peak brought to
   SFX_PEAK. The recipes using them are in recipes.cpp. */

#define SFX_RATE 44100
// NOTE(zoubir): every finished sound peaks here (about -1 dBFS); how loud
// it plays against the others is its gain in SoundEffects (sounds.cpp)
#define SFX_PEAK 0.89f
#define SFX_FADE_SECONDS 0.008f

struct sfx_random
{
    u32 State;
};

// NOTE(zoubir): white noise, -1..1 (xorshift32)
inline float
SfxNoise(sfx_random *Random)
{
    u32 X = Random->State;
    X ^= X << 13;
    X ^= X >> 17;
    X ^= X << 5;
    Random->State = X;
    float Result = (float)(X >> 8) * (2.f / 16777216.f) - 1.f;
    return Result;
}

// NOTE(zoubir): 0..1
inline float
SfxRandom01(sfx_random *Random)
{
    float Result = 0.5f * (SfxNoise(Random) + 1.f);
    return Result;
}

// NOTE(zoubir): a state-variable filter (the trapezoidal form, stable
// while its cutoff moves): Run gives low, band and high pass at once
struct sfx_filter
{
    float Ic1;
    float Ic2;
    float Low;
    float Band;
    float High;
};

inline void
SfxFilterRun(sfx_filter *Filter, float In, float Cutoff, float Resonance)
{
    float Hz = Minimum(Maximum(Cutoff, 20.f), 0.45f * (float)SFX_RATE);
    float G = tanf(Pi32 * Hz / (float)SFX_RATE);
    float K = 1.f / Maximum(Resonance, 0.3f);
    float A1 = 1.f / (1.f + G * (G + K));
    float A2 = G * A1;
    float A3 = G * A2;
    float V3 = In - Filter->Ic2;
    float V1 = A1 * Filter->Ic1 + A2 * V3;
    float V2 = Filter->Ic2 + A2 * Filter->Ic1 + A3 * V3;
    Filter->Ic1 = 2.f * V1 - Filter->Ic1;
    Filter->Ic2 = 2.f * V2 - Filter->Ic2;
    Filter->Low = V2;
    Filter->Band = V1;
    Filter->High = In - K * V1 - V2;
}

inline float
SfxLowPass(sfx_filter *Filter, float In, float Cutoff, float Resonance = 0.707f)
{
    SfxFilterRun(Filter, In, Cutoff, Resonance);
    return Filter->Low;
}

inline float
SfxBandPass(sfx_filter *Filter, float In, float Cutoff, float Resonance = 0.707f)
{
    SfxFilterRun(Filter, In, Cutoff, Resonance);
    return Filter->Band;
}

inline float
SfxHighPass(sfx_filter *Filter, float In, float Cutoff, float Resonance = 0.707f)
{
    SfxFilterRun(Filter, In, Cutoff, Resonance);
    return Filter->High;
}

// NOTE(zoubir): rises over Attack seconds on a quarter sine, then falls
// by e every Decay seconds
inline float
SfxEnvelope(float Seconds, float Attack, float Decay)
{
    float Result = 0.f;
    if (Seconds < 0.f)
    {
        Result = 0.f;
    }
    else if (Seconds < Attack)
    {
        Result = Sin(0.5f * Pi32 * Seconds / Attack);
    }
    else
    {
        Result = expf(-(Seconds - Attack) / Maximum(Decay, 0.0001f));
    }
    return Result;
}

// NOTE(zoubir): From to To over Seconds along a curve that sounds even
// (equal steps in pitch, not in hertz), then holds at To
inline float
SfxSweep(float Seconds, float Length, float From, float To)
{
    float Share = Clamp01(Seconds / Maximum(Length, 0.0001f));
    float Result = From * powf(To / From, Share);
    return Result;
}

// NOTE(zoubir): a sine whose frequency may change every sample: Phase
// keeps its place in turns
inline float
SfxSine(float *Phase, float Hz)
{
    float Result = Sin(2.f * Pi32 * *Phase);
    *Phase += Hz / (float)SFX_RATE;
    *Phase -= floorf(*Phase);
    return Result;
}

// NOTE(zoubir): a soft-edged sawtooth (the sine series cut at its tenth
// partial, so it does not alias up high)
inline float
SfxSaw(float *Phase, float Hz)
{
    float Result = 0.f;
    float Turn = 2.f * Pi32 * *Phase;
    for(u32 Partial = 1; Partial <= 10; Partial++)
    {
        Result += Sin(Turn * (float)Partial) / (float)Partial;
    }
    *Phase += Hz / (float)SFX_RATE;
    *Phase -= floorf(*Phase);
    return 0.55f * Result;
}

// NOTE(zoubir): loud parts rounded off instead of cut, like tape
inline float
SfxSaturate(float In, float Drive)
{
    float Result = tanhf(Drive * In) / tanhf(Drive);
    return Result;
}

// NOTE(zoubir): a struck metal or glass partial: a sine at Hz, Seconds
// into its ring, fading by e every Decay seconds
inline float
SfxPartial(float Seconds, float Hz, float Decay)
{
    float Result = 0.f;
    if (Seconds >= 0.f)
    {
        float Attack = Minimum(1.f, Seconds / 0.002f);
        Result = Attack * Sin(2.f * Pi32 * Hz * Seconds) * expf(-Seconds / Decay);
    }
    return Result;
}

#define SFX_COMBS 4
#define SFX_ALLPASSES 2
#define SFX_COMB_MOST 1800
#define SFX_ALLPASS_MOST 600

// NOTE(zoubir): Schroeder's delay lengths at 44.1 kHz, Freeverb's tuning
global_variable u32 SfxCombLengths[SFX_COMBS] = {1557, 1617, 1491, 1422};
global_variable u32 SfxAllpassLengths[SFX_ALLPASSES] = {556, 225};

struct sfx_reverb
{
    float Combs[SFX_COMBS][SFX_COMB_MOST];
    float CombStore[SFX_COMBS];
    u32 CombAt[SFX_COMBS];
    float Allpasses[SFX_ALLPASSES][SFX_ALLPASS_MOST];
    u32 AllpassAt[SFX_ALLPASSES];
};

// NOTE(zoubir): Samples get Wet of a small room's reverb on top; Room is
// 0..1, how long the tail rings
internal void
SfxAddReverb(sfx_reverb *Reverb, float *Samples, u32 Count, float Wet, float Room)
{
    *Reverb = {};
    float Feedback = 0.7f + 0.28f * Room;
    float Damp = 0.3f;
    for(u32 Index = 0; Index < Count; Index++)
    {
        float In = 0.015f * Samples[Index];
        float Out = 0.f;
        for(u32 Comb = 0; Comb < SFX_COMBS; Comb++)
        {
            float *Line = Reverb->Combs[Comb];
            u32 At = Reverb->CombAt[Comb];
            float Delayed = Line[At];
            Reverb->CombStore[Comb] = Delayed * (1.f - Damp) + Reverb->CombStore[Comb] * Damp;
            Line[At] = In + Reverb->CombStore[Comb] * Feedback;
            Reverb->CombAt[Comb] = (At + 1) % SfxCombLengths[Comb];
            Out += Delayed;
        }
        for(u32 Pass = 0; Pass < SFX_ALLPASSES; Pass++)
        {
            float *Line = Reverb->Allpasses[Pass];
            u32 At = Reverb->AllpassAt[Pass];
            float Delayed = Line[At];
            Line[At] = Out + 0.5f * Delayed;
            Out = Delayed - Out;
            Reverb->AllpassAt[Pass] = (At + 1) % SfxAllpassLengths[Pass];
        }
        Samples[Index] += Wet * 3.f * Out;
    }
}

// NOTE(zoubir): every sound's last step: DC taken out, the tail faded so
// it never ends on a click, the peak set to SFX_PEAK. Returns false for a
// sound with nothing in it
internal bool32
SfxFinish(float *Samples, u32 Count)
{
    float Previous = 0.f;
    float Held = 0.f;
    float Pole = 1.f - 2.f * Pi32 * 20.f / (float)SFX_RATE;
    float Peak = 0.f;
    u32 Fade = (u32)(SFX_FADE_SECONDS * (float)SFX_RATE);
    for(u32 Index = 0; Index < Count; Index++)
    {
        float In = Samples[Index];
        Held = In - Previous + Pole * Held;
        Previous = In;
        float Out = Held;
        if (Index + Fade >= Count)
        {
            Out *= (float)(Count - Index) / (float)Fade;
        }
        if (Index < 32)
        {
            Out *= (float)Index / 32.f;
        }
        Samples[Index] = Out;
        Peak = Maximum(Peak, Absolute(Out));
    }
    bool32 Result = Peak > 0.0001f && Peak == Peak;
    if (Result)
    {
        float Scale = SFX_PEAK / Peak;
        for(u32 Index = 0; Index < Count; Index++)
        {
            Samples[Index] *= Scale;
        }
    }
    return Result;
}
