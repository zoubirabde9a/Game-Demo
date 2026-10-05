/* Player animation: which animation plays and which way the body faces
   (the aim, or a swing's or cast's direction while it lasts). */

// NOTE(zoubir): the keys point more than a right angle away from a
// running body, which is braking hard before it runs the new way
inline bool32
IsTurningBack(world_entity *Player)
{
    v2 Velocity = Player->Velocity.XY;
    bool32 Result = IsOnGround(Player) &&
        LengthSq(Velocity) > Square(PlayerStats.SkidSpeed) &&
        DotProduct(Velocity, Player->Direction) < 0.f;
    return Result;
}

// NOTE(zoubir): the animation for the player's state. The body faces the
// aim; a swing or cast keeps the facing it started with so its animation
// is not cut by the cursor moving. Moving pushes the way the keys point.
internal void
PickPlayerAnimation(world_entity *Player, player_tick *Tick)
{
    if (Player->State == EntityState_Stopping &&
        IsAnimationFinished(Player->AnimationSet, &Player->AnimationState,
                            AnimationType_Stop, *Tick->AnimationDirection))
    {
        Player->State = EntityState_Standing;
    }

    v2 Facing = GetPlayerAim(Player);
    if (Player->State == EntityState_Attacking ||
        Player->State == EntityState_Casting ||
        IsPlayerCasting(Player))
    {
        Facing = Player->CastingDirection;
    }
    *Tick->AnimationDirection = DominantFacing(Facing);

    if (Player->State == EntityState_Stopping)
    {
        Assert(!Tick->Move);
        *Tick->AnimationType = AnimationType_Stop;
    }
    else if (Player->State == EntityState_Attacking)
    {
        *Tick->AnimationType = AnimationType_Attack;
    }
    else if (Player->State == EntityState_Casting ||
             IsPlayerCasting(Player))
    {
        *Tick->AnimationType = AnimationType_Cast;
    }
    else if (Tick->Move && IsTurningBack(Player))
    {
        // NOTE(zoubir): reversing out of a run, the feet skid before they
        // run the new way
        *Tick->AnimationType = AnimationType_Stop;
    }
    else if (Tick->Move)
    {
        *Tick->AnimationType = AnimationType_Move;
    }
    else
    {
        *Tick->AnimationType = AnimationType_Stand;
    }
    if (Tick->Move)
    {
        Tick->DDPlayer.XY = Player->Direction;
    }
}
