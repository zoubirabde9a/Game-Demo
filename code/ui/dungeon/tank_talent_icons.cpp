/* Bulwark talent icons (role_talent_icons.cpp, included just above
   TankTalentIconPainters): the pictures for the tank's slot 8 and slot 11
   talents, Juggernaut and Unbroken. */

// NOTE(zoubir): Juggernaut: Shield Charge with the "sooner" badge
internal void PaintJuggernautIcon(icon_canvas *C) { PaintRoleShieldChargeIcon(C); IconBadgeSooner(C); }

// NOTE(zoubir): Unbroken: a cracked shield still standing in a gold glow,
// a last sliver of red health under it
internal void
PaintUnbrokenIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.46f), 0.5f, IconColor(255, 210, 110, 150));
    v2 Shield[5] = {V2(0.28f, 0.12f), V2(0.72f, 0.12f), V2(0.72f, 0.56f), V2(0.5f, 0.8f),
                    V2(0.28f, 0.56f)};
    IconPolygon(Canvas, Shield, 5, Gradient(IconColor(240, 242, 248), IconColor(105, 112, 135),
                                            V2(0.5f, 0.12f), V2(0.5f, 0.8f)));
    v2 Field[5] = {V2(0.33f, 0.18f), V2(0.67f, 0.18f), V2(0.67f, 0.54f), V2(0.5f, 0.73f),
                   V2(0.33f, 0.54f)};
    IconPolygon(Canvas, Field, 5, Gradient(IconColor(90, 120, 200), IconColor(30, 45, 100),
                                           V2(0.5f, 0.18f), V2(0.5f, 0.73f)));
    // NOTE(zoubir): the crack, a dark zigzag from the rim down the field
    v4 Crack = IconColor(20, 22, 32);
    IconCapsule(Canvas, V2(0.46f, 0.12f), V2(0.54f, 0.28f), 0.022f, Solid(Crack));
    IconCapsule(Canvas, V2(0.54f, 0.28f), V2(0.44f, 0.42f), 0.022f, Solid(Crack));
    IconCapsule(Canvas, V2(0.44f, 0.42f), V2(0.53f, 0.58f), 0.02f, Solid(Crack));
    IconCapsule(Canvas, V2(0.53f, 0.58f), V2(0.49f, 0.68f), 0.016f, Solid(Crack));
    // NOTE(zoubir): the health bar, nearly empty
    IconCapsule(Canvas, V2(0.24f, 0.9f), V2(0.76f, 0.9f), 0.035f, Solid(IconColor(30, 20, 24)));
    IconCapsule(Canvas, V2(0.24f, 0.9f), V2(0.3f, 0.9f), 0.025f, Solid(IconColor(240, 70, 70)));
    IconSparkle(Canvas, V2(0.76f, 0.2f), 0.06f, Solid(IconColor(255, 240, 190)));
}
