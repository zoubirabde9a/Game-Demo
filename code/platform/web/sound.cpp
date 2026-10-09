/* Browser sound: the game mixes 44.1 kHz stereo once a frame, as on
   Windows, and the page plays it through Web Audio. Each frame tops up a
   queue on the JavaScript side to about WEB_SOUND_AHEAD seconds ahead of
   what is playing; an audio callback drains it, and plays silence if it
   runs dry. Browsers start audio only after the player clicks or presses
   a key, so the first sounds wait for that. */

#define WEB_SOUND_RATE 44100
#define WEB_SOUND_AHEAD 0.06f
// NOTE(zoubir): one frame's mix at most, rounded up to a multiple of 8
#define WEB_SOUND_MAX_SAMPLES 8192

EM_JS(void, WebStartSound, (int Rate), {
    var Audio;
    try
    {
        Audio = new (window.AudioContext || window.webkitAudioContext)({sampleRate: Rate});
    }
    catch (Failed)
    {
        return;
    }
    var Sound = Module.gameSound = {Audio: Audio, Left: new Float32Array(Rate), Right: new Float32Array(Rate),
                                    Read: 0, Write: 0};
    var Node = Audio.createScriptProcessor(1024, 0, 2);
    Node.onaudioprocess = function(Event)
    {
        var Left = Event.outputBuffer.getChannelData(0);
        var Right = Event.outputBuffer.getChannelData(1);
        var Size = Sound.Left.length;
        for (var Index = 0; Index < Left.length; Index++)
        {
            if (Sound.Read != Sound.Write)
            {
                Left[Index] = Sound.Left[Sound.Read];
                Right[Index] = Sound.Right[Sound.Read];
                Sound.Read = (Sound.Read + 1) % Size;
            }
            else
            {
                Left[Index] = 0;
                Right[Index] = 0;
            }
        }
    };
    Node.connect(Audio.destination);
    Sound.Node = Node;
    var Resume = function() { if (Audio.state != "running") Audio.resume(); };
    window.addEventListener("pointerdown", Resume, true);
    window.addEventListener("keydown", Resume, true);
});

// NOTE(zoubir): how many samples the queue wants to reach WEB_SOUND_AHEAD;
// 0 while the browser has not started the audio
EM_JS(int, WebSoundWanted, (int Rate, float Ahead), {
    var Sound = Module.gameSound;
    if (!Sound || Sound.Audio.state != "running") return 0;
    var Size = Sound.Left.length;
    var Queued = (Sound.Write - Sound.Read + Size) % Size;
    var Wanted = Math.floor(Rate * Ahead) - Queued;
    return Wanted > 0 ? Wanted : 0;
});

EM_JS(void, WebQueueSound, (const short *Samples, int Count), {
    Samples = Number(Samples);
    var Sound = Module.gameSound;
    if (!Sound) return;
    var Size = Sound.Left.length;
    var At = Samples >> 1;
    for (var Index = 0; Index < Count; Index++)
    {
        var Next = (Sound.Write + 1) % Size;
        if (Next == Sound.Read) break;
        Sound.Left[Sound.Write] = HEAP16[At + 2 * Index] / 32768;
        Sound.Right[Sound.Write] = HEAP16[At + 2 * Index + 1] / 32768;
        Sound.Write = Next;
    }
});

global_variable i16 *WebSoundSamples;

internal void
WebInitSound()
{
    // NOTE(zoubir): stereo, padded as app_sound_output_buffer asks
    WebSoundSamples = (i16 *)calloc(2 * WEB_SOUND_MAX_SAMPLES + 16, sizeof(i16));
    WebStartSound(WEB_SOUND_RATE);
}

internal void
WebWriteFrameSound(thread_context *Thread, app_memory *Memory)
{
    int Wanted = WebSoundWanted(WEB_SOUND_RATE, WEB_SOUND_AHEAD);
    if (Wanted <= 0 || !WebSoundSamples) return;
    if (Wanted > WEB_SOUND_MAX_SAMPLES) Wanted = WEB_SOUND_MAX_SAMPLES;
    app_sound_output_buffer Buffer = {};
    Buffer.SamplesPerSecond = WEB_SOUND_RATE;
    Buffer.SampleCount = Align8(Wanted);
    Buffer.Samples = WebSoundSamples;
    AppGetSoundSamples(Thread, Memory, &Buffer);
    WebQueueSound(WebSoundSamples, Buffer.SampleCount);
}
