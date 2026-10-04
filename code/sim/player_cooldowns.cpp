/* Player cooldowns as a list: the abilities whose cooldown the HUD shows,
   in one fixed order, so the server can send a player its own (snapshot
   Cooldowns[]) and the client can put them back where the HUD reads them.
   Area abilities come last, one each, in table order. A new ability with
   a cooldown bar outside that table adds a case here; either way the
   protocol's NET_COOLDOWN_COUNT grows. */

#define PLAYER_COOLDOWN_COUNT (3 + PLAYER_AREA_ABILITY_COUNT)

// NOTE(zoubir): the field holding cooldown Index, and its full length
internal float *
PlayerCooldown(world_entity *Player, u32 Index, float *Full)
{
    switch (Index)
    {
        case 0: *Full = PLAYER_DASH_COOLDOWN; return &Player->DashCooldown;
        case 1: *Full = PLAYER_SHOCKWAVE_COOLDOWN; return &Player->ShockwaveCooldown;
        case 2: *Full = PLAYER_BLINK_COOLDOWN; return &Player->BlinkCooldown;
    }
    u32 Area = Index - 3;
    if (Area < PLAYER_AREA_ABILITY_COUNT)
    {
        *Full = PlayerAreaAbilities[Area].Cooldown;
        return &Player->AreaCooldowns[Area];
    }
    *Full = 0.f;
    return 0;
}

// NOTE(zoubir): a cooldown as one byte, 0..255 of its full length
inline u8
CooldownToByte(float Seconds, float Full)
{
    float Share = (Full > 0.f) ? Seconds / Full : 0.f;
    if (!(Share > 0.f)) return 0;
    if (Share >= 1.f) return 255;
    return (u8)(Share * 255.f + 0.5f);
}

inline float
CooldownFromByte(u8 Byte, float Full)
{
    return (float)Byte / 255.f * Full;
}
