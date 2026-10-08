/* Bot rift dangers (server/bots.cpp, after DodgeDangers): how a bot lives
   through the Aurora Rift's frost waves and beams
   (sim/monster_abilities/waves_beams_shards.cpp), which a danger circle
   cannot describe.

   A frost wave cannot be walked out of, so a bot on the ground jumps as
   a ring is about to reach it: early enough to be clear of the ground
   when it passes, late enough to still be in the air.

   A beam sweeps an arc. Through the windup a bot inside the arc walks to
   the nearer way out: behind where the beam starts, past where it ends,
   or out of its reach. Once the beam fires, behind the start is through
   the beam, so only the other two are left. */

// NOTE(zoubir): a jump clears the ground about this long after the press
// and stays clear about this long after that (CLEARS_GROUND_HEIGHT)
#define BOT_JUMP_RISE_SECONDS 0.07f
#define BOT_JUMP_CLEAR_SECONDS 0.25f
// NOTE(zoubir): how far past the beam's edge a bot wants to stand
#define BOT_BEAM_MARGIN 40.f

// NOTE(zoubir): whether a ring of a wave reaches Self while a jump
// pressed now would carry it over
internal bool32
BotShouldJumpWave(world_entity *Monster, monster_ability *Ability, world_entity *Self)
{
    float Distance = Length(Self->Position.XY - Monster->Position.XY);
    if (Distance > Ability->Radius || Ability->Speed <= 0.f)
    {
        return false;
    }
    float Elapsed = AbilityActiveElapsed(Monster, Ability);
    if (Monster->AbilityPhase == AbilityPhase_Windup)
    {
        Elapsed = -Monster->AbilityTimer;
    }
    u32 Rings = Maximum(1u, Ability->Count);
    for(u32 Ring = 0; Ring < Rings; Ring++)
    {
        float Front = Ability->Speed * Elapsed - (float)Ring * Ability->Spread;
        float Arrives = (Distance - Front) / Ability->Speed;
        if (Arrives > BOT_JUMP_RISE_SECONDS &&
            Arrives < BOT_JUMP_RISE_SECONDS + 0.5f * BOT_JUMP_CLEAR_SECONDS)
        {
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): the way Self walks out of a beam's arc, zero when it is
// out of it already
internal v2
BotBeamEscape(world *World, world_entity *Monster, monster_ability *Ability, world_entity *Self)
{
    v2 From = Monster->Position.XY;
    v2 Way = Self->Position.XY - From;
    float Distance = Length(Way);
    float Reach = BeamReach(World, From, Distance > 0.f ? Way * (1.f / Distance) : V2(1.f, 0.f),
                            Ability);
    if (Distance <= 0.f || Distance > Reach + BOT_BEAM_MARGIN)
    {
        return V2(0.f);
    }
    float Turn = BeamTurn(Monster);
    float Sweep = Ability->Spread * (Pi32 / 180.f);
    float Edge = (Ability->Radius + BOT_BEAM_MARGIN) / Maximum(Distance, 1.f);
    // NOTE(zoubir): how far round the arc Self stands from where the beam
    // is now, and from where it ends
    v2 Now = BeamDirection(Monster, Ability);
    float Ahead = AngleAhead(Now, Way, Turn);
    float Swept = Monster->AbilityPhase == AbilityPhase_Active ?
        AbilityActiveElapsed(Monster, Ability) / Maximum(Ability->Active, 0.01f) : 0.f;
    float Left = (1.f - Minimum(1.f, Swept)) * Sweep;
    if (Ahead < -Edge || Ahead > Left + Edge)
    {
        return V2(0.f);
    }
    v2 Out = Way * (1.f / Distance);
    v2 Along = Turn * V2(-Out.Y, Out.X);
    float PastEnd = (Left + Edge - Ahead) * Distance;
    float OutOfReach = Reach + BOT_BEAM_MARGIN - Distance;
    float BehindStart = Monster->AbilityPhase == AbilityPhase_Windup ?
        (Ahead + Edge) * Distance : 1.0e9f;
    v2 Result = Along;
    float Best = PastEnd;
    if (OutOfReach < Best)
    {
        Best = OutOfReach;
        Result = Out;
    }
    if (BehindStart < Best)
    {
        Result = -Along;
    }
    return Result;
}

// NOTE(zoubir): the keys of Held changed so Self jumps a wave about to
// reach it and walks out of a beam's sweep
internal u32
DodgeRiftDangers(app_state *AppState, world_entity *Self, u32 Held)
{
    world *World = &AppState->World;
    bool32 Grounded = !IsClearOfGround(Self);
    v2 Escape = V2(0.f);
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Monster = &World->Entities[Index];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            (Monster->AbilityPhase != AbilityPhase_Windup &&
             Monster->AbilityPhase != AbilityPhase_Active))
        {
            continue;
        }
        monster_def *Def = GetMonsterDef(Monster->MonsterKind);
        if (Monster->AbilityIndex >= Def->AbilityCount)
        {
            continue;
        }
        monster_ability *Ability = &Def->Abilities[Monster->AbilityIndex];
        if (Ability->Kind == MonsterAbility_Wave && Grounded &&
            BotShouldJumpWave(Monster, Ability, Self))
        {
            Held |= NetButton_Jump;
        }
        if (Ability->Kind == MonsterAbility_Beam && LengthSq(Escape) <= 0.f)
        {
            Escape = BotBeamEscape(World, Monster, Ability, Self);
        }
    }
    if (LengthSq(Escape) > 0.f)
    {
        Held = (Held & ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down)) |
            NetButtonsToward(Escape);
    }
    return Held;
}
