/* Hero looks for the later classes (hero_looks.cpp has the first three).
   Their hands are empty: each class's own look draws its weapon over the
   sprite, moving with its spells (client/dungeon/classes/). So a skin
   here is the body, the clothes and what is on the head:
   - Ranger: a green hooded tunic over leather.
   - Berserker: bare arms, a fur vest with fur on the shoulders, a red
     sash, a band with two horns, red hair and beard.
   - Shadowblade: dark leather, a black hood and a mask over the face.
   - Stormcaller: a navy robe with gold trim, pale hair standing on end.
   - Duelist: a magenta doublet with white sleeves, black breeches, a
     cavalier's hat with a white feather, a moustache.
   - Frost Mage: an ice-blue robe, a pale hood rimmed in fur, a white
     beard.
   - Druid: a brown robe trimmed in leaf green, long grey hair and beard;
     its antlers are its own look's. */

#define HERO_RAMP(R0, G0, B0, R1, G1, B1, R2, G2, B2, R3, G3, B3) \
    Ramp(ART_RGB(R0, G0, B0), ART_RGB(R1, G1, B1), ART_RGB(R2, G2, B2), ART_RGB(R3, G3, B3))

// NOTE(zoubir): what every later class shares: skin, leather, no weapon
internal hero_look
EmptyHandedLook(void)
{
    hero_look Look = {};
    Look.Skin = HeroSkin;
    Look.Hair = HeroLeather;
    Look.Trim = HeroGold;
    Look.Belt = HeroLeather;
    Look.Boots = HeroLeather;
    Look.HatBand = HeroGold;
    Look.Shaft = HeroWood;
    Look.Head = HeroGold;
    Look.ShieldRim = HeroSteel;
    Look.ShieldField = HeroSteel;
    Look.Weapon = HeroWeapon_None;
    Look.Offhand = HeroOffhand_None;
    return Look;
}

internal hero_look
RangerLook(void)
{
    hero_look Look = EmptyHandedLook();
    Look.Cloth = HERO_RAMP(24, 54, 36, 38, 92, 58, 62, 138, 84, 110, 184, 120);
    Look.Trim = HeroLeather;
    Look.Limb = HERO_RAMP(52, 40, 28, 92, 70, 46, 132, 104, 68, 170, 140, 98);
    Look.Legs = HERO_RAMP(40, 30, 24, 70, 54, 38, 104, 82, 56, 140, 112, 80);
    Look.Hat = HERO_RAMP(20, 44, 32, 32, 76, 50, 52, 116, 76, 88, 160, 106);
    Look.HatBand = HERO_RAMP(16, 34, 24, 26, 58, 40, 40, 90, 60, 66, 124, 84);
    Look.Glow = ART_RGB(90, 210, 170);
    Look.GlowCore = ART_RGB(220, 255, 240);
    Look.Headgear = HeroHeadgear_Hood;
    return Look;
}

internal hero_look
BerserkerLook(void)
{
    hero_look Look = EmptyHandedLook();
    Look.Hair = HERO_RAMP(90, 30, 16, 150, 54, 24, 204, 90, 40, 240, 140, 80);
    Look.Cloth = HERO_RAMP(60, 40, 28, 104, 74, 50, 150, 114, 80, 196, 160, 120);
    Look.Trim = HERO_RAMP(90, 14, 20, 150, 28, 32, 205, 50, 50, 240, 110, 100);
    Look.Limb = HeroSkin;
    Look.Legs = HERO_RAMP(36, 26, 22, 66, 46, 36, 98, 70, 52, 132, 98, 72);
    Look.Boots = Look.Cloth;
    Look.HatBand = HERO_RAMP(36, 38, 46, 70, 74, 86, 110, 114, 128, 160, 164, 176);
    Look.Shaft = HERO_RAMP(120, 104, 80, 180, 164, 130, 222, 210, 180, 250, 244, 224);
    Look.Glow = ART_RGB(225, 50, 50);
    Look.GlowCore = ART_RGB(255, 200, 180);
    Look.Beard = true;
    Look.Pauldrons = true;
    Look.Headgear = HeroHeadgear_Horns;
    return Look;
}

internal hero_look
ShadowbladeLook(void)
{
    hero_look Look = EmptyHandedLook();
    Look.Cloth = HERO_RAMP(24, 20, 32, 46, 38, 60, 72, 60, 92, 110, 94, 138);
    Look.Trim = HERO_RAMP(60, 30, 110, 100, 60, 180, 150, 110, 240, 200, 170, 255);
    Look.Belt = HERO_RAMP(20, 16, 24, 40, 32, 46, 62, 50, 70, 90, 76, 100);
    Look.Limb = HERO_RAMP(20, 18, 28, 38, 34, 50, 60, 54, 76, 92, 84, 112);
    Look.Boots = Look.Belt;
    Look.Hat = HERO_RAMP(18, 14, 26, 36, 28, 52, 60, 46, 86, 96, 76, 130);
    Look.HatBand = Look.Trim;
    Look.Glow = ART_RGB(170, 110, 255);
    Look.GlowCore = ART_RGB(230, 210, 255);
    Look.Mask = true;
    Look.Headgear = HeroHeadgear_Hood;
    return Look;
}

internal hero_look
StormcallerLook(void)
{
    hero_look Look = EmptyHandedLook();
    Look.Hair = HERO_RAMP(150, 140, 100, 210, 200, 150, 240, 234, 190, 255, 252, 230);
    Look.Cloth = HERO_RAMP(16, 22, 56, 28, 42, 100, 48, 70, 150, 84, 110, 200);
    Look.Limb = Look.Cloth;
    Look.Belt = HeroGold;
    Look.Glow = ART_RGB(250, 220, 80);
    Look.GlowCore = ART_RGB(255, 250, 210);
    Look.Robe = true;
    Look.SpikyHair = true;
    return Look;
}

internal hero_look
DuelistLook(void)
{
    hero_look Look = EmptyHandedLook();
    Look.Hair = HERO_RAMP(20, 16, 18, 40, 30, 32, 66, 50, 50, 96, 76, 72);
    Look.Cloth = HERO_RAMP(80, 20, 50, 140, 40, 90, 200, 80, 140, 240, 140, 190);
    Look.Trim = HERO_RAMP(170, 160, 150, 220, 214, 204, 244, 240, 232, 255, 255, 250);
    Look.Limb = HERO_RAMP(150, 150, 160, 210, 210, 215, 240, 240, 240, 255, 255, 255);
    Look.Legs = HERO_RAMP(20, 20, 28, 40, 40, 52, 70, 70, 86, 110, 110, 130);
    Look.Boots = HERO_RAMP(16, 12, 14, 34, 26, 26, 56, 44, 40, 84, 68, 60);
    Look.Hat = HERO_RAMP(30, 16, 28, 60, 30, 52, 92, 48, 80, 130, 76, 112);
    Look.ShieldField = HERO_RAMP(180, 140, 160, 230, 200, 215, 250, 230, 240, 255, 255, 255);
    Look.Glow = ART_RGB(240, 110, 170);
    Look.GlowCore = ART_RGB(255, 220, 236);
    Look.Mustache = true;
    Look.Headgear = HeroHeadgear_Cavalier;
    return Look;
}

internal hero_look
FrostMageLook(void)
{
    hero_look Look = EmptyHandedLook();
    Look.Hair = HERO_RAMP(150, 156, 166, 206, 212, 220, 236, 240, 246, 255, 255, 255);
    Look.Cloth = HERO_RAMP(70, 100, 140, 140, 180, 220, 200, 228, 250, 240, 250, 255);
    Look.Trim = HERO_RAMP(60, 80, 120, 110, 140, 180, 170, 196, 226, 220, 236, 250);
    Look.Limb = Look.Cloth;
    Look.Belt = Look.Trim;
    Look.Hat = HERO_RAMP(60, 90, 140, 110, 160, 210, 170, 210, 245, 225, 242, 255);
    Look.HatBand = HERO_RAMP(170, 180, 190, 220, 226, 232, 245, 248, 250, 255, 255, 255);
    Look.Glow = ART_RGB(150, 215, 255);
    Look.GlowCore = ART_RGB(240, 250, 255);
    Look.Robe = true;
    Look.Beard = true;
    Look.Headgear = HeroHeadgear_Hood;
    return Look;
}

internal hero_look
DruidLook(void)
{
    hero_look Look = EmptyHandedLook();
    Look.Hair = HERO_RAMP(70, 66, 60, 120, 114, 104, 170, 164, 150, 214, 208, 194);
    Look.Cloth = HERO_RAMP(50, 34, 20, 90, 62, 36, 130, 94, 56, 170, 130, 84);
    Look.Trim = HERO_RAMP(40, 80, 20, 80, 130, 40, 130, 180, 60, 180, 220, 110);
    Look.Limb = Look.Cloth;
    Look.Belt = Look.Trim;
    Look.Glow = ART_RGB(165, 200, 60);
    Look.GlowCore = ART_RGB(236, 255, 200);
    Look.Robe = true;
    Look.LongHair = true;
    Look.Beard = true;
    return Look;
}
