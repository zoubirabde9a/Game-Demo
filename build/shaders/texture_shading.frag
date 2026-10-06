#version 130
// Sprites, tiles, icons and text. Pixel art is sampled so each texel stays
// a crisp square but its edges are anti-aliased over one screen pixel: the
// texture is filtered linearly (engine/asset/memory_and_upload.cpp) and the
// UV is pulled to the texel's centre everywhere except a band one screen
// pixel wide at each texel border. Sprites then move smoothly at sub-pixel
// positions instead of jumping a whole texel, and keep no jagged stairs.
// Where the texture is drawn at its size or smaller, the band covers the
// whole texel and this is plain linear filtering.

in vec2 fragmentPosition;
in vec4 fragmentColor;
in vec2 fragmentUV;

out vec4 color;

uniform sampler2D mySampler;

void main() {
    vec2 Size = vec2(textureSize(mySampler, 0));
    vec2 Texel = fragmentUV * Size;
    vec2 Seam = floor(Texel + 0.5);
    vec2 Band = clamp(fwidth(Texel), 1e-4, 1.0);
    Texel = Seam + clamp((Texel - Seam) / Band, -0.5, 0.5);
    color = fragmentColor * texture(mySampler, Texel / Size);
}
