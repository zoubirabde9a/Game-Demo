/* Resource refusal tests (sim/dungeon/role_kits/class_refusal.cpp),
   included by dungeon_tests.cpp after the class tests, whose crypts they
   borrow: every spell a class gates on its resource, pressed short of
   it, raises the shared refusal bits the HUD reads (a Berserker's
   Whirlwind and Execute, a Stormcaller's Thunderclap, a Shadowblade's
   Eviscerate and Deadly Throw); one press is one refusal however long
   the press is kept, mashing gives at most one every CLASS_REFUSAL_GAP,
   and the notice goes after CLASS_REFUSED_SECONDS. */

// NOTE(zoubir): one press of Button in slot 0; true when it raised a
// fresh refusal (the notice up, the beat flipped)
internal bool32
PressIsRefused(crypt_world *Crypt, u32 Button)
{
    player_slot *Slot = &Crypt->AppState->Players[0];
    u32 Before = Slot->ClassFlags & CLASS_FLAG_REFUSAL_BEAT;
    PressOnce(Crypt, 0, Button);
    bool32 Result = (Slot->ClassFlags & CLASS_FLAG_REFUSED) &&
        (Slot->ClassFlags & CLASS_FLAG_REFUSAL_BEAT) != Before;
    return Result;
}

internal void
TestShortPressesAreRefused()
{
    // NOTE(zoubir): Berserker: Whirlwind with no Rage; Execute with a foe
    // in reach and no Rage
    {
        crypt_world Crypt = BerserkerCrypt();
        player_slot *Slot = &Crypt.AppState->Players[0];
        Check(BerserkerRage(Slot) == 0);
        Check(PressIsRefused(&Crypt, PlayerButton_Push));
        Check(Slot->Entity->CastSpell == 0);
        TickCrypt(&Crypt, (u32)(60.f * CLASS_REFUSED_SECONDS) + 40);
        Check(!(Slot->ClassFlags & CLASS_FLAG_REFUSED));
        BerserkerDummy(&Crypt, V3(60.f, 0.f, 0.f));
        Check(PressIsRefused(&Crypt, PlayerButton_Shockwave));
        Check(Slot->Entity->CastSpell == 0);
        DestroyCryptWorld(&Crypt);
    }
    // NOTE(zoubir): Stormcaller: Thunderclap short of its Charge
    {
        crypt_world Crypt = CreateStormWorld();
        player_slot *Slot = &Crypt.AppState->Players[0];
        SetStormCharge(Slot, THUNDERCLAP_MIN_CHARGE - 1.f);
        Check(PressIsRefused(&Crypt, PlayerButton_Shockwave));
        Check(Slot->Entity->CastSpell == 0);
        DestroyCryptWorld(&Crypt);
    }
    // NOTE(zoubir): Shadowblade: Eviscerate and Deadly Throw with no
    // combo points
    {
        crypt_world Crypt = CreateCryptWorld(1);
        player_slot *Slot = ReadyShadowblade(&Crypt);
        Slot->ClassMeter = 0;
        Check(PressIsRefused(&Crypt, PlayerButton_Shockwave));
        TickCrypt(&Crypt, (u32)(60.f * CLASS_REFUSAL_GAP) + 2);
        Slot->ClassMeter = 0;
        Check(PressIsRefused(&Crypt, PlayerButton_Cast));
        DestroyCryptWorld(&Crypt);
    }
}

// NOTE(zoubir): Ticks ticks in slot 0, pressing Button every Every ticks
// (0 for never); how many times the beat flipped, and whether the notice
// stayed up the whole time
internal u32
CountRefusals(crypt_world *Crypt, u32 Button, u32 Every, u32 Ticks, bool32 *AlwaysUp)
{
    player_slot *Slot = &Crypt->AppState->Players[0];
    u32 Result = 0;
    *AlwaysUp = true;
    for(u32 Tick = 0; Tick < Ticks; Tick++)
    {
        u32 Before = Slot->ClassFlags & CLASS_FLAG_REFUSAL_BEAT;
        if (Every && Tick % Every == 0)
        {
            PressOnce(Crypt, 0, Button);
        }
        else
        {
            TickCrypt(Crypt, 1);
        }
        Result += (Slot->ClassFlags & CLASS_FLAG_REFUSAL_BEAT) != Before ? 1 : 0;
        *AlwaysUp = *AlwaysUp && (Slot->ClassFlags & CLASS_FLAG_REFUSED);
    }
    return Result;
}

internal void
TestRefusalsDoNotSpam()
{
    crypt_world Crypt = BerserkerCrypt();
    player_slot *Slot = &Crypt.AppState->Players[0];
    bool32 Up = false;
    // NOTE(zoubir): one press, kept and tried again each tick for a
    // while (key_presses.cpp): one refusal
    Check(CountRefusals(&Crypt, PlayerButton_Push, 60, 60, &Up) == 1);
    TickCrypt(&Crypt, 90);
    Check(!(Slot->ClassFlags & CLASS_FLAG_REFUSED));

    // NOTE(zoubir): mashed every three ticks for two seconds: the notice
    // stays up, and the beat flips no more than once a gap
    u32 Mashed = CountRefusals(&Crypt, PlayerButton_Push, 3, 120, &Up);
    u32 Most = (u32)(2.f / CLASS_REFUSAL_GAP) + 1;
    Check(Mashed >= Most - 2 && Mashed <= Most);
    Check(Up);
    TickCrypt(&Crypt, (u32)(60.f * CLASS_REFUSED_SECONDS) + 2);
    Check(!(Slot->ClassFlags & CLASS_FLAG_REFUSED));

    // NOTE(zoubir): nothing pressed, nothing refused
    u8 Flags = Slot->ClassFlags;
    RefuseForResource(Slot);
    Check(Slot->ClassFlags == Flags);
    DestroyCryptWorld(&Crypt);
}

internal void
RunResourceRefusalTests()
{
    TestShortPressesAreRefused();
    TestRefusalsDoNotSpam();
}
