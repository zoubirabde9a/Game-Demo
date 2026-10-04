// The ability bar's slots, left to right: the player_button, the name
// shown on hover, the function that paints its icon (one file each in
// this folder, listed in ability_icons.cpp) and the slot's accent colour.
// ABILITY_GAP() puts a wider space between groups. One entry per line:
// the file merges line by line (merge=union in .gitattributes).
ABILITY_ICON(PlayerButton_Cast, "Fireball", PaintFireballIcon, 255, 136, 40)
ABILITY_ICON(PlayerButton_Attack, "Sword", PaintSwordIcon, 200, 214, 236)
ABILITY_GAP()
ABILITY_ICON(PlayerButton_Shockwave, "Shockwave", PaintShockwaveIcon, 170, 130, 255)
ABILITY_ICON(PlayerButton_Push, "Push", PaintPushIcon, 90, 230, 170)
ABILITY_ICON(PlayerButton_Launch, "Launch", PaintLaunchIcon, 255, 200, 60)
ABILITY_ICON(PlayerButton_Slam, "Slam", PaintSlamIcon, 245, 80, 55)
ABILITY_GAP()
ABILITY_ICON(PlayerButton_Blink, "Blink", PaintBlinkIcon, 240, 110, 230)
ABILITY_ICON(PlayerButton_Dash, "Dash", PaintDashIcon, 100, 230, 245)
ABILITY_ICON(PlayerButton_Jump, "Jump", PaintJumpIcon, 140, 205, 255)
