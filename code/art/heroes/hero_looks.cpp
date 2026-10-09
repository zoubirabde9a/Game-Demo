/* Hero looks (hero_rig.cpp): how each class is dressed. The Fire Mage
   wears a red robe, a short beard and a wizard's hat and carries a staff with an ember
   orb; the Bulwark is in steel plate with a blue tabard, a plumed helm,
   a sword and a kite shield; the Mender wears a white robe with gold
   trim and a circlet over short fair hair, and carries a staff with a green light. */

global_variable color_ramp HeroSkin =
{{ART_RGB(122, 70, 58), ART_RGB(196, 126, 98), ART_RGB(236, 176, 140), ART_RGB(252, 214, 182)}};
global_variable color_ramp HeroGold =
{{ART_RGB(110, 66, 20), ART_RGB(182, 122, 32), ART_RGB(232, 182, 62), ART_RGB(255, 232, 146)}};
global_variable color_ramp HeroLeather =
{{ART_RGB(44, 26, 22), ART_RGB(84, 50, 36), ART_RGB(124, 80, 52), ART_RGB(162, 114, 76)}};
global_variable color_ramp HeroWood =
{{ART_RGB(52, 30, 20), ART_RGB(92, 56, 32), ART_RGB(134, 88, 50), ART_RGB(172, 126, 74)}};
global_variable color_ramp HeroSteel =
{{ART_RGB(42, 46, 62), ART_RGB(92, 102, 124), ART_RGB(154, 166, 186), ART_RGB(224, 232, 244)}};

internal hero_look
FireMageLook(void)
{
    hero_look Look = {};
    Look.Skin = HeroSkin;
    Look.Hair = Ramp(ART_RGB(32, 20, 26), ART_RGB(62, 36, 36), ART_RGB(96, 56, 46), ART_RGB(132, 86, 62));
    Look.Cloth = Ramp(ART_RGB(72, 16, 22), ART_RGB(134, 30, 32), ART_RGB(188, 56, 40), ART_RGB(228, 98, 58));
    Look.Trim = HeroGold;
    Look.Belt = HeroLeather;
    Look.Limb = Look.Cloth;
    Look.Boots = HeroLeather;
    Look.Hat = Ramp(ART_RGB(56, 14, 22), ART_RGB(108, 24, 30), ART_RGB(156, 42, 36), ART_RGB(200, 74, 48));
    Look.HatBand = HeroGold;
    Look.Shaft = HeroWood;
    Look.Head = Ramp(ART_RGB(176, 52, 10), ART_RGB(240, 116, 28), ART_RGB(255, 186, 68), ART_RGB(255, 240, 178));
    Look.ShieldRim = HeroSteel;
    Look.ShieldField = Look.Cloth;
    Look.Glow = ART_RGB(255, 132, 36);
    Look.GlowCore = ART_RGB(255, 236, 150);
    Look.Robe = true;
    Look.Beard = true;
    Look.Headgear = HeroHeadgear_WizardHat;
    Look.Weapon = HeroWeapon_Staff;
    Look.Offhand = HeroOffhand_None;
    return Look;
}

internal hero_look
BulwarkLook(void)
{
    hero_look Look = {};
    Look.Skin = HeroSkin;
    Look.Hair = HeroLeather;
    Look.Cloth = HeroSteel;
    Look.Trim = Ramp(ART_RGB(20, 34, 90), ART_RGB(36, 68, 150), ART_RGB(70, 118, 212), ART_RGB(132, 180, 252));
    Look.Belt = HeroLeather;
    Look.Limb = HeroSteel;
    Look.Boots = Ramp(ART_RGB(32, 34, 46), ART_RGB(64, 70, 88), ART_RGB(104, 112, 132), ART_RGB(160, 170, 190));
    Look.Hat = HeroSteel;
    Look.HatBand = HeroGold;
    Look.Shaft = Ramp(ART_RGB(70, 80, 100), ART_RGB(150, 162, 184), ART_RGB(210, 220, 234), ART_RGB(250, 252, 255));
    Look.Head = HeroGold;
    Look.ShieldRim = HeroSteel;
    Look.ShieldField = Look.Trim;
    Look.Glow = ART_RGB(140, 196, 255);
    Look.GlowCore = ART_RGB(236, 246, 255);
    Look.Robe = false;
    Look.Pauldrons = true;
    Look.Headgear = HeroHeadgear_Helmet;
    Look.Weapon = HeroWeapon_Sword;
    Look.Offhand = HeroOffhand_Shield;
    return Look;
}

internal hero_look
MenderLook(void)
{
    hero_look Look = {};
    Look.Skin = HeroSkin;
    Look.Hair = Ramp(ART_RGB(140, 94, 40), ART_RGB(202, 150, 62), ART_RGB(240, 202, 104), ART_RGB(255, 238, 172));
    Look.Cloth = Ramp(ART_RGB(118, 124, 134), ART_RGB(188, 194, 198), ART_RGB(232, 236, 232), ART_RGB(255, 255, 250));
    Look.Trim = HeroGold;
    Look.Belt = Ramp(ART_RGB(26, 86, 50), ART_RGB(46, 146, 80), ART_RGB(96, 206, 120), ART_RGB(170, 250, 182));
    Look.Limb = Look.Cloth;
    Look.Boots = HeroLeather;
    Look.Hat = HeroGold;
    Look.HatBand = HeroGold;
    Look.Shaft = Ramp(ART_RGB(120, 100, 70), ART_RGB(200, 180, 140), ART_RGB(236, 222, 186), ART_RGB(255, 248, 226));
    Look.Head = Ramp(ART_RGB(30, 120, 60), ART_RGB(80, 200, 110), ART_RGB(160, 250, 170), ART_RGB(236, 255, 236));
    Look.ShieldRim = HeroSteel;
    Look.ShieldField = Look.Belt;
    Look.Glow = ART_RGB(130, 240, 150);
    Look.GlowCore = ART_RGB(240, 255, 220);
    Look.Robe = true;
    Look.Headgear = HeroHeadgear_Circlet;
    Look.Weapon = HeroWeapon_Staff;
    Look.Offhand = HeroOffhand_None;
    return Look;
}
