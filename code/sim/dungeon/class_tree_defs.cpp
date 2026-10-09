/* Every class's tree (class_tree.cpp, docs/class-trees.md): the two
   branches' names, what sits fixed in each slot and what each branch's
   wild slots roll from. CLASS_BRANCH lists a branch's fixed slots in
   order: the core talent, the spell pair, the fixed talents of tiers 2, 3
   and 4, and the capstone. CT is a talent of the class's catalog, RM a
   run talent (run_tree/run_mods.cpp).

   A pair whose second spell is not built yet holds a passive there, named
   in a note, until it is. */

// NOTE(zoubir): by player_role
global_variable class_tree_def ClassTrees[PlayerRole_Count] =
{
    // NOTE(zoubir): the Fire Mage. Cataclysm holds Wildfire's second spell
    // slot until Flame Wave is in
    {{"Wildfire", "Pyre"},
     {CLASS_BRANCH(CT(StrikerTalent_SearingHeat), CT(StrikerTalent_Fireguard), CT(StrikerTalent_Cataclysm),
                   CT(StrikerTalent_Wildfire), CT(StrikerTalent_MoltenGround), RM(Smoulder),
                   CT(StrikerTalent_Overload)),
      CLASS_BRANCH(CT(StrikerTalent_Pyromancer), CT(StrikerTalent_Combustion), CT(StrikerTalent_Detonate),
                   CT(StrikerTalent_Executioner), RM(KindledWrath), RM(FireWithin),
                   RM(PhoenixHeart))},
     {{CT(StrikerTalent_Kindling), RM(Pyroclasm), RM(RisingGlory), RM(Reprisal), RM(Feast),
       CT(StrikerTalent_HeatShield)},
      {CT(StrikerTalent_QuickenedFlame), RM(Cadence), RM(OpeningSalvo), RM(Leeching),
       CT(StrikerTalent_EmberMantle), RM(CinderSkin)}}},

    // NOTE(zoubir): the Bulwark. Provoke and Vengeance hold the second spell
    // slots until Rallying Cry and Demoralizing Roar are in
    {{"Bastion", "Vanguard"},
     {CLASS_BRANCH(CT(TankTalent_IronSkin), CT(TankTalent_LastStand), CT(TankTalent_Provoke),
                   CT(TankTalent_Bastion), RM(SteadyHeart), RM(Stoneform),
                   CT(TankTalent_Unbroken)),
      CLASS_BRANCH(CT(TankTalent_ShatterArmor), CT(TankTalent_Intercept), CT(TankTalent_Vengeance),
                   CT(TankTalent_Juggernaut), RM(Menacing), RM(ShieldBrother),
                   CT(TankTalent_Guardian))},
     {{CT(TankTalent_Fortitude), CT(TankTalent_PlateMastery), RM(Stubborn), RM(Lifeline),
       RM(LivingFortress)},
      {CT(TankTalent_BattleRhythm), RM(Spikes), RM(Rallying), RM(Menace), RM(Vigor),
       RM(Retaliation)}}},

    // NOTE(zoubir): the Mender. Quickening, Light Feet and Inner Light hold
    // the second spell slots until Prayer of Healing, Dawnbreak and Purify
    // are in
    {{"Sanctum", "Dawn"},
     {CLASS_BRANCH(CT(HealerTalent_DeepWard), CT(HealerTalent_Sanctuary), CT(HealerTalent_Quickening),
                   CT(HealerTalent_SteadfastWard), CT(HealerTalent_BlessedHands), RM(SpiritWard),
                   CT(HealerTalent_GuardianAngel)),
      CLASS_BRANCH(CT(HealerTalent_SwiftMending), CT(HealerTalent_LightFeet), CT(HealerTalent_InnerLight),
                   CT(HealerTalent_Renewal), CT(HealerTalent_Atonement), RM(Sanctified),
                   CT(HealerTalent_Miracle))},
     {{RM(Bountiful), RM(Hymn), RM(SharedSpoils), RM(Rallying), RM(Overflowing)},
      {RM(WarSong), RM(Receptive), RM(MendingTouch), RM(KeenEdge), RM(SecondBreath),
       RM(Serenity)}}},

    // NOTE(zoubir): the Ranger
    {{"Marksmanship", "Survival"},
     {CLASS_BRANCH(CT(RangerTalent_Marksman), CT(RangerTalent_RapidFire), CT(RangerTalent_KillShot),
                   CT(RangerTalent_Deadeye), RM(BigGame), RM(Patience),
                   RM(ApexPredator)),
      CLASS_BRANCH(CT(RangerTalent_Barrage), CT(RangerTalent_Disengage), CT(RangerTalent_ExplosiveTrap),
                   CT(RangerTalent_PinningVolley), CT(RangerTalent_HuntersNet), RM(Trailwise),
                   RM(Trophy))},
     {{CT(RangerTalent_KeenEye), CT(RangerTalent_SteadyHands), RM(KillingRhythm), RM(Finisher),
       RM(Leeching), CT(RangerTalent_Survivalist), CT(RangerTalent_LethalMark)},
      {RM(Cleaver), RM(Bloodrush), RM(Feast), RM(Toughened), RM(Vigor),
       CT(RangerTalent_FleetHunter)}}},

    // NOTE(zoubir): the Berserker. Bladestorm holds Carnage's second spell
    // slot until Rampage is in
    {{"Fury", "Carnage"},
     {CLASS_BRANCH(CT(BerserkerTalent_Brutality), CT(BerserkerTalent_Berserk), CT(BerserkerTalent_BattleShout),
                   CT(BerserkerTalent_Bloodthirst), CT(BerserkerTalent_Massacre), RM(CorneredBeast),
                   RM(UndyingFury)),
      CLASS_BRANCH(CT(BerserkerTalent_UnbridledWrath), CT(BerserkerTalent_Leap), CT(BerserkerTalent_Bladestorm),
                   CT(BerserkerTalent_SweepingStrikes), CT(BerserkerTalent_ShatteringLeap), RM(Savagery),
                   RM(RedMist))},
     {{CT(BerserkerTalent_BruteForce), CT(BerserkerTalent_Bloodlust), RM(Finisher), RM(Gorge),
       RM(Stubborn), CT(BerserkerTalent_ThickHide)},
      {CT(BerserkerTalent_Unyielding), RM(Reprisal), RM(Spikes), RM(Wayfarer), RM(Cadence),
       RM(Vigor)}}},

    // NOTE(zoubir): the Shadowblade. Cutthroat holds Assassination's
    // second spell slot until Garrote is in
    {{"Assassination", "Subtlety"},
     {CLASS_BRANCH(CT(ShadowbladeTalent_Venom), CT(ShadowbladeTalent_DeadlyThrow), CT(ShadowbladeTalent_Cutthroat),
                   CT(ShadowbladeTalent_Envenom), CT(ShadowbladeTalent_KnifeStorm), RM(Assassinate),
                   RM(DeathMark)),
      CLASS_BRANCH(CT(ShadowbladeTalent_Opportunist), CT(ShadowbladeTalent_Shadowstep), CT(ShadowbladeTalent_ShadowDance),
                   CT(ShadowbladeTalent_Relentless), RM(Ambush), RM(QuickHands),
                   CT(ShadowbladeTalent_KidneyShot))},
     {{CT(ShadowbladeTalent_Lethality), CT(ShadowbladeTalent_Siphon), RM(Cadence), RM(Giantslayer),
       RM(Unseen)},
      {CT(ShadowbladeTalent_Evasion), RM(Bloodrush), RM(Slip), RM(Shroud), RM(Stubborn),
       RM(KeenEdge)}}},

    // NOTE(zoubir): the Stormcaller. High Voltage holds Conduction's second
    // spell slot until Ball Lightning is in
    {{"Conduction", "Tempest"},
     {CLASS_BRANCH(CT(StormcallerTalent_Voltage), CT(StormcallerTalent_StaticField), CT(StormcallerTalent_HighVoltage),
                   CT(StormcallerTalent_Conductor), CT(StormcallerTalent_ArcField), RM(StormFront),
                   CT(StormcallerTalent_Stormbringer)),
      CLASS_BRANCH(CT(StormcallerTalent_Capacitor), CT(StormcallerTalent_LightningDash), CT(StormcallerTalent_EyeOfTheStorm),
                   CT(StormcallerTalent_LiveWire), RM(Grounded), CT(StormcallerTalent_Tailwind),
                   RM(Surge))},
     {{RM(StaticBuild), RM(RisingGlory), RM(Bloodrush), RM(Leeching),
       CT(StormcallerTalent_Grounding)},
      {RM(Giantslayer), RM(Cornered), RM(Wayfarer), RM(Quickened), RM(Toughened),
       CT(StormcallerTalent_Quickening)}}},

    // NOTE(zoubir): the Duelist. Flurry holds Bladework's second spell slot
    // until Disarm is in
    {{"Bladework", "Guard"},
     {CLASS_BRANCH(CT(DuelistTalent_Finesse), CT(DuelistTalent_PerfectForm), CT(DuelistTalent_Flurry),
                   CT(DuelistTalent_Precision), CT(DuelistTalent_Crescendo), RM(CoupDeGrace),
                   CT(DuelistTalent_Masterstroke)),
      CLASS_BRANCH(CT(DuelistTalent_Parade), CT(DuelistTalent_Riposte), CT(DuelistTalent_Feint),
                   CT(DuelistTalent_Bait), RM(Measured), RM(PerfectForm),
                   RM(Lifeline))},
     {{CT(DuelistTalent_KeenEdge), RM(Precision), RM(Giantslayer),
       RM(Panache), CT(DuelistTalent_QuickWrist), CT(DuelistTalent_Footwork)},
      {CT(DuelistTalent_Stamina), RM(Spikes), RM(Stubborn), RM(SecondBreath), RM(Feast),
       RM(Vigor)}}},

    // NOTE(zoubir): the Frost Mage. Cold Snap holds Winter's second spell
    // slot until Cone of Cold is in
    {{"Winter", "Shatter"},
     {CLASS_BRANCH(CT(FrostMageTalent_Permafrost), CT(FrostMageTalent_Blizzard), CT(FrostMageTalent_ColdSnap),
                   CT(FrostMageTalent_DeepFreeze), RM(ShatterPoint), RM(DeepWinter),
                   CT(FrostMageTalent_AbsoluteZero)),
      CLASS_BRANCH(CT(FrostMageTalent_Frostbite), CT(FrostMageTalent_FrozenOrb), CT(FrostMageTalent_IceBarrier),
                   CT(FrostMageTalent_FingersOfFrost), CT(FrostMageTalent_SplittingIce), RM(GlacialSkin),
                   RM(ColdCalculation))},
     {{RM(Cleaver), RM(Bloodrush), RM(RisingGlory), CT(FrostMageTalent_GlacialArmor), RM(Stubborn)},
      {CT(FrostMageTalent_IceShards), CT(FrostMageTalent_WintersGrace), RM(Cadence), RM(Giantslayer),
       RM(Leeching), RM(OpeningSalvo)}}},

    // NOTE(zoubir): the Druid. Swiftmend and Gift of the Wild hold the
    // second spell slots until Lifebloom and Starfall are in
    {{"Grove", "Moon"},
     {CLASS_BRANCH(CT(DruidTalent_Verdancy), CT(DruidTalent_Tranquility), CT(DruidTalent_Swiftmend),
                   CT(DruidTalent_WildGrowth), RM(Verdant), RM(WildBloom),
                   RM(HeartOfTheWild)),
      CLASS_BRANCH(CT(DruidTalent_NaturesWrath), CT(DruidTalent_EntanglingRoots), CT(DruidTalent_GiftOfTheWild),
                   CT(DruidTalent_Symbiosis), CT(DruidTalent_Eclipse), CT(DruidTalent_StarlitFury),
                   CT(DruidTalent_LunarBloom))},
     {{RM(Overflowing), RM(Receptive), RM(Hymn), CT(DruidTalent_Barkskin), CT(DruidTalent_Overgrowth)},
      {RM(Giantslayer), RM(KeenEdge), RM(WarSong), RM(Bloodrush), RM(Quickened),
       RM(Cadence), RM(Moonlit), RM(SharedSpoils)}}},
};
