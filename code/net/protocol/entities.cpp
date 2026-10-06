/* Entities on the wire (protocol.cpp): one net_entity_state as bytes. The
   type and three flags share a byte, the small fields (facing, animation,
   affix, status, ability) are packed into bits, positions are sent from
   the snapshot's origin in fixed point, and height, velocity and a fresh
   hit only when they are there. */

// NOTE: the type byte keeps the type in its low 5 bits; the top three say
// whether a fresh hit, height and velocity follow. Most things stand on
// the ground, many stand still and few were just hit, so those 9 bytes
// are usually left out.
#define NET_ENTITY_TYPE_MASK 0x1f
#define NET_ENTITY_HAS_HIT 0x20
#define NET_ENTITY_HAS_Z 0x40
#define NET_ENTITY_MOVING 0x80

// Whether a value is still nonzero once quantized to Steps per unit.
inline bool32
NetNonZero(float Value, float Steps)
{
    float Scaled = Value * Steps;
    return Scaled >= 0.5f || Scaled <= -0.5f;
}

// A position on the ground, sent as its distance from the snapshot's
// origin (the viewer) in 1/8 units. Sent as is, positions past 4096 units
// from the map's middle, where infinite maps take players, were all cut
// to 4096; near the viewer they keep their precision anywhere.
internal void
NetPosition(net_stream *S, float *Value, float Origin)
{
    float Relative = *Value - Origin;
    NetFixed16(S, &Relative, NET_POSITION_STEPS);
    *Value = Relative + Origin;
}

internal void
NetSerializeEntity(net_stream *S, net_entity_state *E)
{
    NetU16(S, &E->Id);
    u8 TypeAndFlags = 0;
    if (S->Writing)
    {
        if (E->Type > NET_ENTITY_TYPE_MASK) S->Failed = true;
        TypeAndFlags = (u8)(E->Type & NET_ENTITY_TYPE_MASK);
        // A unit thrown up this tick can still be at height 0; its speed
        // goes with the height, so the flag counts either.
        if (NetNonZero(E->Z, NET_POSITION_STEPS) || NetNonZero(E->VelZ, NET_VELOCITY_STEPS))
        {
            TypeAndFlags |= NET_ENTITY_HAS_Z;
        }
        if (NetNonZero(E->VelX, NET_VELOCITY_STEPS) || NetNonZero(E->VelY, NET_VELOCITY_STEPS))
        {
            TypeAndFlags |= NET_ENTITY_MOVING;
        }
        if (E->Hit)
        {
            TypeAndFlags |= NET_ENTITY_HAS_HIT;
        }
    }
    NetU8(S, &TypeAndFlags);
    E->Type = TypeAndFlags & NET_ENTITY_TYPE_MASK;
    // Small fields are packed; out-of-range values are cut to their bits.
    // Status's first three bits ride in Extra; Look's top bit says the
    // other eight follow in a byte of their own, so a unit that is only
    // burning, poisoned or slowed costs nothing more.
    u8 Look = (u8)((E->Facing & 3) | ((E->Animation & 15) << 2) | ((E->Flash & 1) << 6) |
                   ((E->Status >> 3) ? 0x80 : 0));
    NetU8(S, &Look);
    E->Facing = Look & 3;
    E->Animation = (Look >> 2) & 15;
    E->Flash = (Look >> 6) & 1;
    NetU8(S, &E->Variant);
    u8 Extra = (u8)((E->Affix & 7) | ((E->Status & 7) << 3) | ((E->Ability & 3) << 6));
    NetU8(S, &Extra);
    E->Affix = Extra & 7;
    E->Ability = (Extra >> 6) & 3;
    u8 MoreStatus = (u8)(E->Status >> 3);
    if (Look & 0x80)
    {
        NetU8(S, &MoreStatus);
    }
    else
    {
        MoreStatus = 0;
    }
    E->Status = (u16)(((Extra >> 3) & 7) | (MoreStatus << 3));
    NetI16(S, &E->Health);
    NetPosition(S, &E->X, S->OriginX);
    NetPosition(S, &E->Y, S->OriginY);
    if (TypeAndFlags & NET_ENTITY_HAS_Z)
    {
        NetFixed16(S, &E->Z, NET_POSITION_STEPS);
        NetFixed16(S, &E->VelZ, NET_VELOCITY_STEPS);
    }
    else
    {
        E->Z = E->VelZ = 0.f;
    }
    if (TypeAndFlags & NET_ENTITY_MOVING)
    {
        NetFixed16(S, &E->VelX, NET_VELOCITY_STEPS);
        NetFixed16(S, &E->VelY, NET_VELOCITY_STEPS);
    }
    else
    {
        E->VelX = E->VelY = 0.f;
    }
    E->Hit = (TypeAndFlags & NET_ENTITY_HAS_HIT) ? 1 : 0;
    if (E->Hit)
    {
        NetU8(S, &E->HitStop);
        NetU8(S, &E->HitAngle);
        u8 By = (u8)((E->HitBy & 15) | ((E->HitThrown & 1) << 7));
        NetU8(S, &By);
        E->HitBy = By & 15;
        E->HitThrown = (By >> 7) & 1;
    }
    else
    {
        E->HitStop = E->HitAngle = E->HitBy = E->HitThrown = 0;
    }
}
