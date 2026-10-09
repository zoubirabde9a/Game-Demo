/* Second tree icons (sim/dungeon/run_tree/): one per run talent, picked by
   what its first effect does from the pictures the classes and the stat
   talents already have, so Execute looks like the Berserker's Execute
   wherever it rolls. A keystone carries a gold badge. Included by
   talent_icons.cpp after the role icons, whose painters it borrows. */

// NOTE(zoubir): by run_effect
global_variable talent_icon_painter *RunEffectIconPainters[RunEffect_Count] =
{
    0,
    PaintStatDamageIcon,
    PaintStatArmorIcon,
    PaintStatVitalityIcon,
    PaintStatHasteIcon,
    PaintStatHealingIcon,
    PaintStatSwiftnessIcon,
    PaintStatLifestealIcon,
    PaintBerserkerExecuteIcon,
    PaintOpportunistIcon,
    PaintRangerHuntersMarkIcon,
    PaintBerserkerCleaveIcon,
    PaintUnbridledWrathIcon,
    PaintPrecisionIcon,
    PaintBerserkerBerserkIcon,
    PaintUnbrokenIcon,
    PaintBerserkerBloodthirstIcon,
    PaintMomentumIcon,
    PaintRoleShieldBashIcon,
    PaintRenewalIcon,
    PaintRoleSanctuaryIcon,
    PaintRoleRadianceIcon,
    PaintRoleTauntIcon,
    PaintRoleMendingBoltIcon,
    PaintRoleWardIcon,
    PaintStormcallerThunderclapIcon,
    PaintRoleCombustionIcon,
    PaintSecondWindIcon,
    PaintDruidWildGrowthIcon,
};

internal void
PaintRunModIcon(icon_canvas *Canvas, u32 Mod)
{
    run_mod_def *Def = &RunModDefs[Mod < RunMod_Count ? Mod : 0];
    talent_icon_painter *Paint = RunEffectIconPainters[Def->Effect[0]];
    // NOTE(zoubir): less threat is a step into the shadows, not a war cry
    if (Def->Effect[0] == RunEffect_Threat && Def->PerRank[0] < 0.f)
    {
        Paint = PaintShadowstepIcon;
    }
    if (Paint)
    {
        Paint(Canvas);
    }
    if (Def->Keystone)
    {
        IconBadge(Canvas, IconColor(255, 205, 80));
    }
}
