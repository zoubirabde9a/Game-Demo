/* Player cooldowns as a list: the abilities whose cooldown the HUD shows,
   in one fixed order, so the server can send a player its own (snapshot
   Cooldowns[]) and the client can put them back where the HUD reads them.
   Movement abilities first, then area abilities, one each in table
   order. A row added to either table gets a bar; the protocol's
   NET_COOLDOWN_COUNT grows with it. */

#define PLAYER_COOLDOWN_COUNT (PlayerMove_Count + PLAYER_AREA_ABILITY_COUNT)

// NOTE(zoubir): the field holding cooldown Index, and its full length
internal float *
PlayerCooldown(world_entity *Player, u32 Index, float *Full)
{
    if (Index < PlayerMove_Count)
    {
        *Full = PlayerMovements[Index].Cooldown;
        return &Player->MovementCooldowns[Index];
    }
    u32 Area = Index - PlayerMove_Count;
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
