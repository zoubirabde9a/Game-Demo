// The ability bar's slots, left to right (a slot shows once the player
// has the ability, sim/progression/talents.cpp): the player_button, the name
// shown on hover, the function that paints its icon (one file each in
// this folder, listed in ability_icons.cpp) and the slot's accent colour.
// ABILITY_GAP() puts a wider space between groups. One entry per line:
// the file merges line by line (merge=union in .gitattributes).
ABILITY_ICON(PlayerButton_Cast, "Fireball", PaintFireballIcon, 255, 136, 40)
ABILITY_ICON(PlayerButton_Attack, "Sword", PaintSwordIcon, 235, 235, 245)
ABILITY_ICON(PlayerButton_Launch, "Launch", PaintLaunchIcon, 255, 200, 60)
ABILITY_ICON(PlayerButton_Kunai, "Kunai", PaintKunaiIcon, 200, 230, 255)
ABILITY_ICON(PlayerButton_Shockwave, "Shockwave", PaintShockwaveIcon, 170, 220, 255)
ABILITY_ICON(PlayerButton_Push, "Push", PaintPushIcon, 190, 230, 255)
ABILITY_ICON(PlayerButton_FrostNova, "Frost Nova", PaintFrostNovaIcon, 150, 225, 255)
ABILITY_ICON(PlayerButton_GravityWell, "Gravity Well", PaintGravityWellIcon, 180, 100, 255)
ABILITY_GAP()
ABILITY_ICON(PlayerButton_Shield, "Shield", PaintShieldIcon, 120, 200, 255)
ABILITY_ICON(PlayerButton_Blink, "Blink", PaintBlinkIcon, 240, 110, 230)
ABILITY_ICON(PlayerButton_Slam, "Slam", PaintSlamIcon, 255, 170, 90)
ABILITY_ICON(PlayerButton_Jump, "Jump", PaintJumpIcon, 140, 205, 255)
