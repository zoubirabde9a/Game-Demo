// Bloom (client/world_bloom.cpp): the glow round fire, lava, magic and
// flashes, made at a quarter of the screen's size so it costs a sixteenth
// and reaches far. Every pass draws into the bottom-left quarter of a
// texture as big as the screen, so no viewport change is needed:
//   Pass.x 0, bright:  reads the world at full size and keeps what is
//                      bright and coloured, four samples to a pixel
//   Pass.x 1, blur:    reads the quarter and blurs it along Pass.yz,
//                      nine bilinear samples covering seventeen pixels
//   Pass.x 2, add:     reads the quarter over the whole window and adds
//                      it, Pass.w strong, warmed a little
// Screen = the texture's width and height in pixels.

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform sampler2D Source;
uniform vec4 Screen;
uniform vec4 Pass;

// NOTE(zoubir): how much of a pixel glows: the brightest channel past a
// soft knee; pale pixels (snow, sand, stone) need a far higher peak, so
// only coloured light and true flashes glow
vec3 Bright(vec3 C)
{
    float Peak = max(C.r, max(C.g, C.b));
    float Low = min(C.r, min(C.g, C.b));
    float Saturation = (Peak - Low) / max(Peak, 0.001);
    float Knee = mix(0.97, 0.70, smoothstep(0.12, 0.45, Saturation));
    return C * smoothstep(Knee, Knee + 0.28, Peak);
}

// NOTE(zoubir): the quarter, never past its own edge, so the blur does
// not pull in the rest of the texture
vec3 Quarter(vec2 Pixel)
{
    vec2 Most = 0.25 * Screen.xy - 0.5;
    vec2 UV = clamp(Pixel, vec2(0.5), Most) / Screen.xy;
    return TEXTURE(Source, UV).rgb;
}

void main()
{
    vec2 Pixel = gl_FragCoord.xy;
    vec3 C = vec3(0.0);
    if (Pass.x < 0.5)
    {
        vec2 Centre = Pixel * 4.0;
        C += Bright(TEXTURE(Source, (Centre + vec2(-1.0, -1.0)) / Screen.xy).rgb);
        C += Bright(TEXTURE(Source, (Centre + vec2(1.0, -1.0)) / Screen.xy).rgb);
        C += Bright(TEXTURE(Source, (Centre + vec2(-1.0, 1.0)) / Screen.xy).rgb);
        C += Bright(TEXTURE(Source, (Centre + vec2(1.0, 1.0)) / Screen.xy).rgb);
        FragColor = vec4(C * 0.25, 1.0);
    }
    else if (Pass.x < 1.5)
    {
        // NOTE(zoubir): a Gaussian over seventeen pixels, each pair of
        // taps folded into one bilinear sample between them
        vec2 Step = Pass.yz;
        C += Quarter(Pixel) * 0.1964825501511404;
        C += (Quarter(Pixel + Step * 1.411764705882353) +
              Quarter(Pixel - Step * 1.411764705882353)) * 0.2969069646728344;
        C += (Quarter(Pixel + Step * 3.2941176470588234) +
              Quarter(Pixel - Step * 3.2941176470588234)) * 0.09447039785044732;
        C += (Quarter(Pixel + Step * 5.176470588235294) +
              Quarter(Pixel - Step * 5.176470588235294)) * 0.010381362401148057;
        FragColor = vec4(C, 1.0);
    }
    else
    {
        C = Quarter(Pixel * 0.25) * Pass.w * vec3(1.0, 0.93, 0.82);
        FragColor = vec4(C, 1.0);
    }
}
