/* Every class's tree (class_tree.cpp, docs/class-trees.md): the two
   branches' names, what sits fixed in each slot and what each branch's
   wild slots roll from. CLASS_BRANCH lists a branch's fixed slots in
   order: the core talent, the spell pair, the fixed talents of tiers 2, 3
   and 4, and the capstone. CT is a talent of the class's catalog, RM a
   run talent (run_tree/run_mods.cpp). */

// NOTE(zoubir): by player_role
global_variable class_tree_def ClassTrees[PlayerRole_Count] =
{
    // NOTE(zoubir): the Fire Mage
    {{"Wildfire", "Pyre"},
     {CLASS_BRANCH(CT(StrikerTalent_SearingHeat), CT(StrikerTalent_Meteor), CT(StrikerTalent_Fireguard),
                   CT(StrikerTalent_Wildfire), CT(StrikerTalent_MoltenGround), RM(Smoulder),
                   CT(StrikerTalent_Overload)),
      CLASS_BRANCH(CT(StrikerTalent_Pyromancer), CT(StrikerTalent_Combustion), CT(StrikerTalent_Detonate),
                   CT(StrikerTalent_Executioner), RM(KindledWrath), RM(FireWithin),
                   RM(PhoenixHeart))},
     {{CT(StrikerTalent_Kindling), RM(Pyroclasm), RM(RisingGlory), RM(Reprisal), RM(Feast),
       CT(StrikerTalent_HeatShield)},
      {CT(StrikerTalent_QuickenedFlame), RM(Cadence), RM(OpeningSalvo), RM(Leeching),
       CT(StrikerTalent_EmberMantle), RM(CinderSkin), CT(StrikerTalent_Cataclysm)}}},

    // NOTE(zoubir): the Bulwark
    {{"Bastion", "Vanguard"},
     {CLASS_BRANCH(CT(TankTalent_IronSkin), CT(TankTalent_LastStand), CT(TankTalent_Taunt),
                   CT(TankTalent_Bastion), RM(SteadyHeart), RM(Stoneform),
                   CT(TankTalent_Unbroken)),
      CLASS_BRANCH(CT(TankTalent_ShatterArmor), CT(TankTalent_ShieldCharge), CT(TankTalent_ShieldThrow),
                   CT(TankTalent_Juggernaut), RM(Menacing), RM(ShieldBrother),
                   RM(Retaliation))},
     {{CT(TankTalent_Provoke), CT(TankTalent_Fortitude), CT(TankTalent_PlateMastery), RM(Stubborn),
       RM(Lifeline), RM(LivingFortress)},
      {CT(TankTalent_Vengeance), CT(TankTalent_BattleRhythm), RM(Spikes), RM(Rallying),
       RM(Menace), RM(Vigor)}}},

    // NOTE(zoubir): the Mender
    {{"Sanctum", "Dawn"},
     {CLASS_BRANCH(CT(HealerTalent_DeepWard), CT(HealerTalent_Sanctuary), CT(HealerTalent_Radiance),
                   CT(HealerTalent_SteadfastWard), CT(HealerTalent_BlessedHands), RM(SpiritWard),
                   CT(HealerTalent_GuardianAngel)),
      CLASS_BRANCH(CT(HealerTalent_SwiftMending), CT(HealerTalent_HolyFire), CT(HealerTalent_SmiteBolt),
                   CT(HealerTalent_Renewal), RM(Serenity), RM(Sanctified),
                   CT(HealerTalent_Miracle))},
     {{CT(HealerTalent_Quickening), RM(Bountiful), RM(Hymn), RM(SharedSpoils), RM(Rallying),
       CT(HealerTalent_InnerLight)},
      {CT(HealerTalent_LightFeet), RM(WarSong), RM(Receptive), RM(MendingTouch), RM(KeenEdge),
       RM(SecondBreath)}}},

    // NOTE(zoubir): the Ranger
    {{"Marksmanship", "Survival"},
     {CLASS_BRANCH(CT(RangerTalent_Marksman), CT(RangerTalent_RapidFire), CT(RangerTalent_KillShot),
                   CT(RangerTalent_Deadeye), RM(BigGame), RM(Patience),
                   RM(ApexPredator)),
      CLASS_BRANCH(CT(RangerTalent_Barrage), CT(RangerTalent_Volley), CT(RangerTalent_Disengage),
                   CT(RangerTalent_PinningVolley), CT(RangerTalent_HuntersNet), RM(Trailwise),
                   RM(Trophy))},
     {{CT(RangerTalent_KeenEye), CT(RangerTalent_SteadyHands), RM(KillingRhythm), RM(Finisher),
       RM(Leeching), CT(RangerTalent_Survivalist), CT(RangerTalent_LethalMark)},
      {CT(RangerTalent_FleetHunter), RM(Cleaver), RM(Bloodrush), RM(Feast), RM(Toughened),
       RM(Vigor)}}},

    // NOTE(zoubir): the Berserker
    {{"Fury", "Carnage"},
     {CLASS_BRANCH(CT(BerserkerTalent_Brutality), CT(BerserkerTalent_Execute), CT(BerserkerTalent_Berserk),
                   CT(BerserkerTalent_Bloodthirst), CT(BerserkerTalent_Massacre), RM(CorneredBeast),
                   RM(UndyingFury)),
      CLASS_BRANCH(CT(BerserkerTalent_UnbridledWrath), CT(BerserkerTalent_Leap), CT(BerserkerTalent_BattleShout),
                   CT(BerserkerTalent_SweepingStrikes), CT(BerserkerTalent_ShatteringLeap), RM(Savagery),
                   RM(RedMist))},
     {{CT(BerserkerTalent_BruteForce), CT(BerserkerTalent_Bloodlust), RM(Finisher), RM(Gorge),
       RM(Stubborn), CT(BerserkerTalent_ThickHide)},
      {CT(BerserkerTalent_Unyielding), RM(Reprisal), RM(Spikes), RM(Wayfarer), RM(Cadence),
       RM(Vigor), CT(BerserkerTalent_Bladestorm)}}},

    // NOTE(zoubir): the Shadowblade
    {{"Assassination", "Subtlety"},
     {CLASS_BRANCH(CT(ShadowbladeTalent_Venom), CT(ShadowbladeTalent_FanOfKnives), CT(ShadowbladeTalent_DeadlyThrow),
                   CT(ShadowbladeTalent_Envenom), CT(ShadowbladeTalent_KnifeStorm), RM(Assassinate),
                   RM(DeathMark)),
      CLASS_BRANCH(CT(ShadowbladeTalent_Opportunist), CT(ShadowbladeTalent_Shadowstep), CT(ShadowbladeTalent_ShadowDance),
                   CT(ShadowbladeTalent_Relentless), RM(Ambush), RM(QuickHands),
                   CT(ShadowbladeTalent_KidneyShot))},
     {{CT(ShadowbladeTalent_Lethality), CT(ShadowbladeTalent_Siphon), RM(Cadence), RM(Giantslayer),
       RM(Unseen), CT(ShadowbladeTalent_Cutthroat)},
      {CT(ShadowbladeTalent_Evasion), RM(Bloodrush), RM(Slip), RM(Shroud), RM(Stubborn),
       RM(KeenEdge)}}},

    // NOTE(zoubir): the Stormcaller
    {{"Conduction", "Tempest"},
     {CLASS_BRANCH(CT(StormcallerTalent_Voltage), CT(StormcallerTalent_ChainLightning), CT(StormcallerTalent_StaticField),
                   CT(StormcallerTalent_Conductor), CT(StormcallerTalent_ArcField), RM(StormFront),
                   CT(StormcallerTalent_Stormbringer)),
      CLASS_BRANCH(CT(StormcallerTalent_Capacitor), CT(StormcallerTalent_LightningDash), CT(StormcallerTalent_EyeOfTheStorm),
                   CT(StormcallerTalent_LiveWire), RM(Grounded), CT(StormcallerTalent_Tailwind),
                   RM(Surge))},
     {{CT(StormcallerTalent_HighVoltage), RM(StaticBuild), RM(RisingGlory), RM(Bloodrush),
       RM(Leeching), CT(StormcallerTalent_Grounding)},
      {RM(Giantslayer), RM(Cornered), RM(Wayfarer), RM(Quickened), RM(Toughened),
       CT(StormcallerTalent_Quickening)}}},

    // NOTE(zoubir): the Duelist. Footwork holds Guard's second spell slot
    // until Feint is in
    {{"Bladework", "Guard"},
     {CLASS_BRANCH(CT(DuelistTalent_Finesse), CT(DuelistTalent_PerfectForm), CT(DuelistTalent_Lunge),
                   CT(DuelistTalent_Precision), CT(DuelistTalent_Crescendo), RM(CoupDeGrace),
                   CT(DuelistTalent_Masterstroke)),
      CLASS_BRANCH(CT(DuelistTalent_Parade), CT(DuelistTalent_Riposte), CT(DuelistTalent_Footwork),
                   CT(DuelistTalent_Bait), RM(Measured), RM(PerfectForm),
                   RM(Lifeline))},
     {{CT(DuelistTalent_KeenEdge), CT(DuelistTalent_Flurry), RM(Precision), RM(Giantslayer),
       RM(Panache), CT(DuelistTalent_QuickWrist)},
      {CT(DuelistTalent_Stamina), RM(Spikes), RM(Stubborn), RM(SecondBreath), RM(Feast),
       RM(Vigor)}}},

    // NOTE(zoubir): the Frost Mage
    {{"Winter", "Shatter"},
     {CLASS_BRANCH(CT(FrostMageTalent_Permafrost), CT(FrostMageTalent_FrostNova), CT(FrostMageTalent_Blizzard),
                   CT(FrostMageTalent_DeepFreeze), RM(ShatterPoint), RM(DeepWinter),
                   CT(FrostMageTalent_AbsoluteZero)),
      CLASS_BRANCH(CT(FrostMageTalent_Frostbite), CT(FrostMageTalent_FrozenOrb), CT(FrostMageTalent_IceBarrier),
                   CT(FrostMageTalent_FingersOfFrost), CT(FrostMageTalent_SplittingIce), RM(GlacialSkin),
                   RM(ColdCalculation))},
     {{CT(FrostMageTalent_ColdSnap), RM(Cleaver), RM(Bloodrush), RM(RisingGlory),
       CT(FrostMageTalent_GlacialArmor), RM(Stubborn)},
      {CT(FrostMageTalent_IceShards), CT(FrostMageTalent_WintersGrace), RM(Cadence), RM(Giantslayer),
       RM(Leeching), RM(OpeningSalvo)}}},

    // NOTE(zoubir): the Druid
    {{"Grove", "Moon"},
     {CLASS_BRANCH(CT(DruidTalent_Verdancy), CT(DruidTalent_Rejuvenation), CT(DruidTalent_Tranquility),
                   CT(DruidTalent_WildGrowth), CT(DruidTalent_Overgrowth), RM(Verdant),
                   RM(HeartOfTheWild)),
      CLASS_BRANCH(CT(DruidTalent_NaturesWrath), CT(DruidTalent_Starfire), CT(DruidTalent_Moonfire),
                   CT(DruidTalent_Symbiosis), RM(Moonlit), CT(DruidTalent_StarlitFury),
                   RM(SharedSpoils))},
     {{CT(DruidTalent_GiftOfTheWild), CT(DruidTalent_Swiftmend), RM(Overflowing), RM(Receptive),
       RM(Hymn), CT(DruidTalent_Barkskin)},
      {RM(Giantslayer), RM(KeenEdge), RM(WarSong), RM(Bloodrush), RM(Quickened),
       RM(Cadence)}}},
};
