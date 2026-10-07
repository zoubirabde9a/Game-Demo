/* Sound recipes (sounds.cpp): how each sound effect is made from the
   synth's parts (synth.cpp). Each fills Count samples at SFX_RATE; the
   finish (peak, fade) comes after, in RenderSoundEffect. The designs:

   Jump        a breath of air rising, on a soft step off the ground
   Dash        a fast whoosh, falling
   Blink       a bright chirp rising through a shimmer, with a tail
   Shield      a struck glass bell over a ring of air
   FireCast    a whump and a short roar of flame, with crackle
   Hit         a body blow: a falling thud with a slap on top
   Sword       a thin, quick swish with a ring of steel in it
   Kunai       a blade whistling past
   AreaCast    a low boom with rumble under it
   LevelUp     four bell notes up a major chord, ringing on
   WardBreak   glass shattering: a burst and many small high pings
   Rewind      a sweep that swells toward its end, as if played backward
   Taunt       a war horn growl, low and rough
   ShieldSlam  a heavy thud and the clang of a shield
   Heal        a soft chime gliding up a fifth
   Ward        a glassy shimmer closing in
   Sanctuary   a warm chord swelling and ringing out
   MeteorCast  fire gathering: a roar rising in pitch and loudness
   Explosion   a big blast: boom, roar and debris crackling down
   GiantFireball  a heavy roaring launch
   Combustion  a flare of flame bursting up */

// NOTE(zoubir): seconds at sample Index
inline float
SfxTime(u32 Index)
{
    float Result = (float)Index / (float)SFX_RATE;
    return Result;
}

internal void
SfxJump(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Air = {};
    sfx_filter Step = {};
    float Phase = 0.f;
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Breath = SfxBandPass(&Air, SfxNoise(R), SfxSweep(T, 0.16f, 650.f, 2400.f), 1.4f) *
            SfxEnvelope(T, 0.025f, 0.07f);
        float Tap = SfxSine(&Phase, SfxSweep(T, 0.05f, 210.f, 95.f)) * SfxEnvelope(T, 0.002f, 0.03f);
        float Scuff = SfxLowPass(&Step, SfxNoise(R), 1800.f) * SfxEnvelope(T, 0.001f, 0.015f);
        S[I] = 0.9f * Breath + 0.55f * Tap + 0.35f * Scuff;
    }
}

internal void
SfxDash(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Wind = {};
    sfx_filter Body = {};
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Noise = SfxNoise(R);
        float Whoosh = SfxBandPass(&Wind, Noise, SfxSweep(T, 0.24f, 3200.f, 420.f), 1.1f);
        float Low = SfxLowPass(&Body, Noise, 500.f);
        float Shape = SfxEnvelope(T, 0.045f, 0.09f);
        S[I] = (Whoosh + 0.4f * Low) * Shape;
    }
}

internal void
SfxBlink(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    float Carrier = 0.f;
    float Mod = 0.f;
    sfx_filter Air = {};
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Hz = SfxSweep(T, 0.18f, 520.f, 1900.f);
        // NOTE(zoubir): phase modulation by a partial a little over an
        // octave up, fading, so the start is metallic and the end pure
        float Depth = 2.5f * expf(-T / 0.12f);
        float Fm = SfxSine(&Mod, 2.01f * Hz) * Depth;
        float Tone = Sin(2.f * Pi32 * Carrier + Fm) * SfxEnvelope(T, 0.01f, 0.11f);
        Carrier += Hz / (float)SFX_RATE;
        Carrier -= floorf(Carrier);
        float Sparkle = SfxBandPass(&Air, SfxNoise(R), 3800.f, 1.2f) * SfxEnvelope(T - 0.05f, 0.03f, 0.08f);
        S[I] = 0.75f * Tone + 0.12f * Sparkle;
    }
    // NOTE(zoubir): a few glints along the way
    for(u32 Ping = 0; Ping < 6; Ping++)
    {
        float Start = 0.03f + 0.03f * (float)Ping + 0.01f * SfxRandom01(R);
        float Hz = 2000.f + 1800.f * SfxRandom01(R);
        for(u32 I = 0; I < Count; I++)
        {
            S[I] += 0.1f * SfxPartial(SfxTime(I) - Start, Hz, 0.05f);
        }
    }
    SfxAddReverb(Reverb, S, Count, 0.35f, 0.5f);
}

internal void
SfxShield(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    // NOTE(zoubir): the partials of a struck glass bowl, out of tune with
    // each other as real ones are
    float Hz[5] = {640.f, 1712.f, 3420.f, 5060.f, 2390.f};
    float Decay[5] = {0.5f, 0.32f, 0.2f, 0.12f, 0.25f};
    float Level[5] = {1.f, 0.6f, 0.35f, 0.2f, 0.3f};
    sfx_filter Air = {};
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Bell = 0.f;
        for(u32 P = 0; P < 5; P++)
        {
            Bell += Level[P] * SfxPartial(T, Hz[P] * (1.f + 0.004f * Sin(30.f * T)), Decay[P]);
        }
        float Ring = SfxBandPass(&Air, SfxNoise(R), SfxSweep(T, 0.3f, 1200.f, 4200.f), 2.f) *
            SfxEnvelope(T, 0.04f, 0.12f);
        S[I] = 0.6f * Bell + 0.35f * Ring;
    }
    SfxAddReverb(Reverb, S, Count, 0.3f, 0.55f);
}

internal void
SfxFireCast(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Roar = {};
    sfx_filter Crack = {};
    float Phase = 0.f;
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Flame = SfxLowPass(&Roar, SfxNoise(R), SfxSweep(T, 0.3f, 2600.f, 500.f), 1.2f) *
            SfxEnvelope(T, 0.02f, 0.14f);
        float Whump = SfxSine(&Phase, SfxSweep(T, 0.12f, 160.f, 55.f)) * SfxEnvelope(T, 0.004f, 0.07f);
        // NOTE(zoubir): crackle: rare clicks, filtered so they pop
        float Click = SfxRandom01(R) > 0.9975f ? SfxNoise(R) * 3.f : 0.f;
        float Crackle = SfxBandPass(&Crack, Click, 2800.f, 3.f) * SfxEnvelope(T, 0.01f, 0.2f);
        S[I] = SfxSaturate(0.9f * Flame + 0.8f * Whump + 0.6f * Crackle, 1.6f);
    }
}

internal void
SfxHit(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Slap = {};
    float Phase = 0.f;
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Thud = SfxSine(&Phase, SfxSweep(T, 0.09f, 150.f, 48.f)) * SfxEnvelope(T, 0.002f, 0.09f);
        float Top = SfxHighPass(&Slap, SfxNoise(R), 1200.f) * SfxEnvelope(T, 0.001f, 0.018f);
        S[I] = SfxSaturate(1.1f * Thud + 0.6f * Top, 2.2f);
    }
}

internal void
SfxSword(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Edge = {};
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Swish = SfxBandPass(&Edge, SfxNoise(R), SfxSweep(T, 0.14f, 5200.f, 1400.f), 1.6f) *
            SfxEnvelope(T, 0.03f, 0.05f);
        float Steel = 0.5f * SfxPartial(T - 0.02f, 2340.f, 0.06f) +
            0.3f * SfxPartial(T - 0.02f, 3810.f, 0.04f);
        S[I] = Swish + 0.25f * Steel;
    }
}

internal void
SfxKunai(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Whistle = {};
    float Phase = 0.f;
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Shape = SfxEnvelope(T, 0.02f, 0.07f);
        float Tone = SfxSine(&Phase, SfxSweep(T, 0.18f, 2900.f, 1800.f));
        float Air = SfxBandPass(&Whistle, SfxNoise(R), SfxSweep(T, 0.18f, 3400.f, 2000.f), 4.f);
        S[I] = (0.35f * Tone + 0.9f * Air) * Shape;
    }
}

internal void
SfxAreaCast(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Rumble = {};
    sfx_filter Front = {};
    float Phase = 0.f;
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Boom = SfxSine(&Phase, SfxSweep(T, 0.25f, 90.f, 36.f)) * SfxEnvelope(T, 0.006f, 0.2f);
        float Under = SfxLowPass(&Rumble, SfxNoise(R), 380.f, 0.9f) * SfxEnvelope(T, 0.02f, 0.25f);
        float Wave = SfxBandPass(&Front, SfxNoise(R), SfxSweep(T, 0.3f, 2000.f, 300.f)) *
            SfxEnvelope(T, 0.01f, 0.12f);
        S[I] = SfxSaturate(1.1f * Boom + 1.2f * Under + 0.4f * Wave, 1.8f);
    }
    SfxAddReverb(Reverb, S, Count, 0.18f, 0.4f);
}

// NOTE(zoubir): a bell note: the fundamental and two partials above it
inline float
SfxBellNote(float Seconds, float Hz, float Decay)
{
    float Result = SfxPartial(Seconds, Hz, Decay) + 0.4f * SfxPartial(Seconds, 2.f * Hz, 0.6f * Decay) +
        0.15f * SfxPartial(Seconds, 3.01f * Hz, 0.35f * Decay);
    return Result;
}

internal void
SfxLevelUp(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    // NOTE(zoubir): C5, E5, G5, C6
    float Notes[4] = {523.25f, 659.25f, 783.99f, 1046.5f};
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Sum = 0.f;
        for(u32 Note = 0; Note < 4; Note++)
        {
            Sum += SfxBellNote(T - 0.085f * (float)Note, Notes[Note], Note == 3 ? 0.6f : 0.3f);
        }
        S[I] = Sum;
    }
    SfxAddReverb(Reverb, S, Count, 0.35f, 0.6f);
}

internal void
SfxWardBreak(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Burst = {};
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        S[I] = SfxBandPass(&Burst, SfxNoise(R), 3200.f, 0.8f) * SfxEnvelope(T, 0.001f, 0.05f);
    }
    for(u32 Shard = 0; Shard < 22; Shard++)
    {
        float Start = 0.18f * SfxRandom01(R) * SfxRandom01(R);
        float Hz = 1700.f + 3300.f * SfxRandom01(R);
        float Level = 0.15f + 0.25f * SfxRandom01(R);
        float Decay = 0.03f + 0.06f * SfxRandom01(R);
        for(u32 I = 0; I < Count; I++)
        {
            S[I] += Level * SfxPartial(SfxTime(I) - Start, Hz, Decay);
        }
    }
    SfxAddReverb(Reverb, S, Count, 0.3f, 0.45f);
}

internal void
SfxRewind(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    float Length = (float)Count / (float)SFX_RATE;
    sfx_filter Sweep = {};
    float Phase = 0.f;
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Share = T / Length;
        // NOTE(zoubir): grows toward the end and stops short, as a sound
        // played backward does
        float Swell = Share * Share * Share * (1.f - Clamp01((Share - 0.94f) / 0.06f));
        float Air = SfxBandPass(&Sweep, SfxNoise(R), SfxSweep(T, Length, 3500.f, 600.f), 2.f);
        float Tone = SfxSine(&Phase, SfxSweep(T, Length, 900.f, 300.f));
        S[I] = (Air + 0.4f * Tone) * Swell;
    }
}

internal void
SfxTaunt(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Throat = {};
    sfx_filter Grit = {};
    float Phase = 0.f;
    float Second = 0.f;
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Hz = 92.f * (1.f + 0.04f * Sin(2.f * Pi32 * 6.f * T)) * SfxSweep(T, 0.08f, 0.85f, 1.f);
        float Horn = SfxSaw(&Phase, Hz) + 0.6f * SfxSaw(&Second, 1.503f * Hz);
        float Voiced = SfxLowPass(&Throat, Horn, 700.f + 500.f * SfxEnvelope(T, 0.05f, 0.2f), 1.8f);
        float Rough = SfxBandPass(&Grit, SfxNoise(R), 900.f, 0.8f) * 0.3f;
        S[I] = SfxSaturate((Voiced + Rough) * SfxEnvelope(T, 0.04f, 0.3f), 2.f);
    }
    SfxAddReverb(Reverb, S, Count, 0.2f, 0.5f);
}

internal void
SfxShieldSlam(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Dust = {};
    float Phase = 0.f;
    float Hz[4] = {380.f, 1050.f, 1870.f, 2730.f};
    float Decay[4] = {0.3f, 0.18f, 0.12f, 0.08f};
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Thud = SfxSine(&Phase, SfxSweep(T, 0.1f, 120.f, 40.f)) * SfxEnvelope(T, 0.002f, 0.12f);
        float Clang = 0.f;
        for(u32 P = 0; P < 4; P++)
        {
            Clang += SfxPartial(T, Hz[P], Decay[P]) / (1.f + (float)P);
        }
        float Crash = SfxLowPass(&Dust, SfxNoise(R), 2200.f) * SfxEnvelope(T, 0.001f, 0.06f);
        S[I] = SfxSaturate(1.1f * Thud + 0.5f * Clang + 0.5f * Crash, 1.8f);
    }
    SfxAddReverb(Reverb, S, Count, 0.2f, 0.45f);
}

internal void
SfxHeal(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    float Phase = 0.f;
    float Fifth = 0.f;
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Hz = SfxSweep(T, 0.18f, 660.f, 990.f);
        float Tone = SfxSine(&Phase, Hz) + 0.35f * SfxSine(&Fifth, 1.5f * Hz);
        S[I] = Tone * SfxEnvelope(T, 0.03f, 0.22f) + 0.3f * SfxBellNote(T - 0.12f, 1980.f, 0.2f);
    }
    SfxAddReverb(Reverb, S, Count, 0.35f, 0.55f);
}

internal void
SfxWard(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Air = {};
    float Hz[4] = {1180.f, 1770.f, 2950.f, 4120.f};
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Glass = 0.f;
        for(u32 P = 0; P < 4; P++)
        {
            Glass += SfxPartial(T - 0.02f * (float)P, Hz[P], 0.25f - 0.04f * (float)P) / (1.f + 0.5f * (float)P);
        }
        float Close = SfxBandPass(&Air, SfxNoise(R), SfxSweep(T, 0.15f, 5000.f, 1500.f), 1.5f) *
            SfxEnvelope(T, 0.06f, 0.05f);
        S[I] = 0.6f * Glass + 0.4f * Close;
    }
    SfxAddReverb(Reverb, S, Count, 0.3f, 0.5f);
}

internal void
SfxSanctuary(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    // NOTE(zoubir): an A major chord, each voice a pair of slightly
    // detuned sines so it breathes
    float Hz[3] = {440.f, 554.37f, 659.25f};
    float Phase[6] = {};
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Pad = 0.f;
        for(u32 V = 0; V < 3; V++)
        {
            Pad += SfxSine(&Phase[2 * V], Hz[V] * 0.997f) + SfxSine(&Phase[2 * V + 1], Hz[V] * 1.003f);
        }
        S[I] = Pad * SfxEnvelope(T, 0.18f, 0.35f) + 0.4f * SfxBellNote(T, 1760.f, 0.4f);
    }
    SfxAddReverb(Reverb, S, Count, 0.4f, 0.7f);
}

internal void
SfxMeteorCast(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    float Length = (float)Count / (float)SFX_RATE;
    sfx_filter Roar = {};
    float Phase = 0.f;
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Share = T / Length;
        float Rise = Share * Share * (1.f - Clamp01((Share - 0.9f) / 0.1f));
        float Fire = SfxLowPass(&Roar, SfxNoise(R), SfxSweep(T, Length, 250.f, 2400.f), 1.5f);
        float Hum = SfxSine(&Phase, SfxSweep(T, Length, 55.f, 140.f));
        S[I] = SfxSaturate((Fire + 0.6f * Hum) * (0.15f + 0.85f * Rise), 1.5f);
    }
}

internal void
SfxExplosion(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Body = {};
    sfx_filter Debris = {};
    float Phase = 0.f;
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Boom = SfxSine(&Phase, SfxSweep(T, 0.3f, 110.f, 32.f)) * SfxEnvelope(T, 0.003f, 0.22f);
        float Roar = SfxLowPass(&Body, SfxNoise(R), SfxSweep(T, 0.6f, 3000.f, 300.f), 0.9f) *
            SfxEnvelope(T, 0.005f, 0.3f);
        float Click = SfxRandom01(R) > 0.996f ? SfxNoise(R) * 2.5f : 0.f;
        float Crackle = SfxBandPass(&Debris, Click, 1800.f, 2.f) * SfxEnvelope(T - 0.08f, 0.05f, 0.35f);
        S[I] = SfxSaturate(1.2f * Boom + 1.1f * Roar + 0.7f * Crackle, 2.4f);
    }
    SfxAddReverb(Reverb, S, Count, 0.22f, 0.6f);
}

internal void
SfxGiantFireball(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Roar = {};
    sfx_filter Low = {};
    float Phase = 0.f;
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Noise = SfxNoise(R);
        float Flame = SfxBandPass(&Roar, Noise, SfxSweep(T, 0.4f, 1800.f, 380.f), 0.9f);
        float Weight = SfxLowPass(&Low, Noise, 260.f);
        float Push = SfxSine(&Phase, SfxSweep(T, 0.2f, 120.f, 60.f)) * SfxEnvelope(T, 0.005f, 0.1f);
        S[I] = SfxSaturate((Flame + 1.2f * Weight) * SfxEnvelope(T, 0.03f, 0.22f) + 0.8f * Push, 1.8f);
    }
}

internal void
SfxCombustion(float *S, u32 Count, sfx_random *R, sfx_reverb *Reverb)
{
    sfx_filter Flare = {};
    sfx_filter Crack = {};
    sfx_filter Body = {};
    for(u32 I = 0; I < Count; I++)
    {
        float T = SfxTime(I);
        float Noise = SfxNoise(R);
        float Fire = SfxBandPass(&Flare, Noise, SfxSweep(T, 0.25f, 300.f, 1800.f), 1.2f);
        float Low = SfxLowPass(&Body, Noise, 350.f);
        float Click = SfxRandom01(R) > 0.997f ? SfxNoise(R) * 2.f : 0.f;
        float Crackle = SfxBandPass(&Crack, Click, 2200.f, 2.f);
        S[I] = SfxSaturate((Fire + 0.8f * Low + 0.6f * Crackle) * SfxEnvelope(T, 0.08f, 0.15f), 1.6f);
    }
    SfxAddReverb(Reverb, S, Count, 0.15f, 0.4f);
}
