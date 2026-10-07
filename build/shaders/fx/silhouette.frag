// Silhouette: a sprite's shape in one flat colour, for glows round a unit
// (client/draw_entities/windup_glow.cpp). The texture's own colours are
// dropped; only its alpha is kept, so a dark sprite glows as bright as a
// pale one.
//   colour rgb: the colour
//   colour a:   its strength

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform sampler2D Sprite;

void main()
{
    // flipped like texture_shading.vert flips a sprite's V
    float Shape = TEXTURE(Sprite, vec2(fragmentUV.x, 1.0 - fragmentUV.y)).a;
    FragColor = vec4(fragmentColor.rgb, fragmentColor.a * Shape);
}
