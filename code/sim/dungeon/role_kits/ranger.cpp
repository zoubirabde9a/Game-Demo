/* The Ranger's kit (role_abilities.cpp, through class_kits.cpp): a bow.
   Numbers, talents and the spell table are role_kits/ranger_defs.cpp; its
   state is role_kits/ranger.h.

   X  Quick Shot: an arrow at the foe aimed at, the filler; it puts
      Hunter's Mark on that foe: the foe takes MARK_SHARE more from the
      Ranger for MARK_SECONDS, and hits on it build Focus
      (ranger/shots.cpp).
   A  Volley: arrows rain on the circle at the cursor (ranger/ground.cpp);
      Pinning Volley keeps what it struck slowed longer.
   R  Piercing Shot: a one-second draw (PlayerSpell_RangerA), then a heavy
      arrow through every foe in a long line; it spends all the Focus
      for more damage, and with Deadeye a full Focus crits.
   C  Disengage (tree): a leap back that leaves a snare trap; with the
      capstone, Hunter's Net, the snare roots every foe near it.
   V  Rapid Fire (tree): a two-second channel (PlayerSpell_RangerB) that
      looses RAPID_FIRE_ARROWS arrows at a foe while the Ranger walks
      slowly.
   W  Kill Shot (tree): an arrow at the foe under the Ranger's mark, far
      harder on one nearly dead; a kill brings it back at once.

   Quick Shot and Piercing Shot are its base kit; the tree gives one of
   Rapid Fire and Kill Shot (Marksmanship) and one of Volley and
   Disengage (Survival), so it casts four (class_tree_defs.cpp).

   The rotation: Quick Shot the boss to mark it and fill Focus,
   spend Focus on a Piercing Shot lined up through the pack, Volley the
   pack. Every hit lands when its arrow arrives, and clients fly the
   arrows from the bursts (client/dungeon/classes/ranger.cpp). Focus goes
   to clients as ClassMeter, and what the HUD and the looks need as
   ClassFlags (RANGER_FLAG_*). */

// NOTE(zoubir): the class's bursts, SimBurst_RangerFirst + n; the rows
// are client/dungeon/classes/ranger_bursts.inc
enum ranger_burst
{
    RangerBurst_Arrow,     // an arrow arriving at Position along Angle (RangerArrow_*)
    RangerBurst_Volley,    // a Volley's circle at Position, variant 1 with Barrage
    RangerBurst_Pierce,    // a Piercing Shot leaving Position along Angle
    RangerBurst_Mark,      // Hunter's Mark landing on the foe at Position
    RangerBurst_Leap,      // Disengage taking off at Position, along Angle
    RangerBurst_Trap,      // a snare trap set at Position, for its life
    RangerBurst_TrapSnap,  // the snare at Position springing
    RangerBurst_FocusFull, // Focus coming full on the Ranger at Position
};

// NOTE(zoubir): what a hit is, for OnRangerHit and the arrows in flight
enum ranger_shot
{
    RangerShot_None,
    RangerShot_Quick,
    RangerShot_Rapid,
    RangerShot_Volley,
    RangerShot_Pierce,
    RangerShot_Trap,
    RangerShot_Kill,
};

// NOTE(zoubir): an arrow burst's look
enum ranger_arrow_variant
{
    RangerArrow_Quick,
    RangerArrow_Miss,
    RangerArrow_Rapid,
};

// NOTE(zoubir): a burst carries a small number too (an arrow's variant,
// a Piercing Shot's power, Barrage, a mark or a trap sent again), as
// whole steps of RANGER_BURST_STEP added to its height. Online a burst's
// angle shrinks to a byte (server/event_relay.cpp) but its position goes
// whole; no burst is drawn that high, and RangerBurstPlace takes it off
#define RANGER_BURST_STEP 4096.f

inline v3
RangerBurstSpot(v3 Position, u32 Variant)
{
    v3 Result = Position;
    Result.Z += RANGER_BURST_STEP * (float)Variant;
    return Result;
}

inline u32
RangerBurstVariant(v3 Position)
{
    float Steps = floorf((Position.Z + 0.5f * RANGER_BURST_STEP) / RANGER_BURST_STEP);
    u32 Result = Steps > 0.f ? (u32)Steps : 0;
    return Result;
}

// NOTE(zoubir): where the burst really is
inline v3
RangerBurstPlace(v3 Position)
{
    v3 Result = Position;
    Result.Z -= RANGER_BURST_STEP * (float)RangerBurstVariant(Position);
    return Result;
}

#include "ranger/shots.cpp"
#include "ranger/ground.cpp"

// NOTE(zoubir): whether Key only starts a wind-up when pressed (the cast
// runs through sim/player_casts.cpp, then FinishRangerCast fires it)
internal bool32
RangerKeyWindsUp(u32 Key)
{
    return Key == 1 || Key == 3;
}

// NOTE(zoubir): Key pressed, off cooldown and learned; true when it cast,
// which spends the cooldown
internal bool32
CastRangerKey(app_state *AppState, world *World, memory_arena *Arena, player_slot *Slot,
           world_entity *Player, u32 Key)
{
    switch(Key)
    {
        case 0:
        {
            return CastVolley(AppState, Slot, Player);
        } break;

        case 1:
        {
            StartPlayerCast(Player, PlayerSpell_RangerA, GetPlayerAim(Player));
            if (!Slot->Predicted)
            {
                EmitSound(&AppState->Events, AssetType_SfxMeteorCast, Player->Position);
            }
        } break;

        case 2:
        {
            CastDisengage(AppState, Slot, Player);
        } break;

        case 3:
        {
            // NOTE(zoubir): the foe is picked again at each arrow when
            // this one is gone; a client only starts the channel
            StartPlayerCast(Player, PlayerSpell_RangerB, GetPlayerAim(Player));
            Slot->Ranger.RapidTimer = PlayerSpells[PlayerSpell_RangerB].CastTime /
                (float)RAPID_FIRE_ARROWS;
            world_entity *Foe = Slot->Predicted ? 0 : RangerTarget(AppState, Slot, Player, RAPID_FIRE_RANGE);
            Slot->Ranger.RapidSlot = Foe ? (u32)(Foe - World->Entities) : 0;
            Slot->Ranger.RapidSerial = Foe ? Foe->MonsterSerial : 0;
        } break;

        case 4:
        {
            // NOTE(zoubir): only at the Ranger's own marked foe; none in
            // reach, no shot and no cooldown
            world_entity *Foe = RangerMarkedFoe(AppState, Slot);
            if (!Foe || Length(Foe->Position.XY - Player->Position.XY) > KILL_SHOT_RANGE)
            {
                return false;
            }
            ShootRangerArrow(AppState, Player, Foe, RangerShot_Kill, KILL_SHOT_DAMAGE,
                             RangerArrow_Quick);
            EmitSound(&AppState->Events, AssetType_SfxKunai, Player->Position);
        } break;

        case 5:
        {
            world_entity *Foe = RangerTarget(AppState, Slot, Player, QUICK_SHOT_RANGE);
            if (Foe)
            {
                KeepRangerMarkOn(AppState, Slot, Foe, Player->Position.XY);
            }
            ShootRangerArrow(AppState, Player, Foe, RangerShot_Quick, QUICK_SHOT_DAMAGE,
                             RangerArrow_Quick);
            EmitSound(&AppState->Events, AssetType_SfxKunai, Player->Position);
        } break;

        default:
        {
            return false;
        } break;
    }
    return true;
}

// NOTE(zoubir): the foe Rapid Fire shoots at now: its own while it lives,
// else the one the Ranger aims at
internal world_entity *
RapidFireFoe(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world_entity *Result = FindMonsterBySerial(&AppState->World, Slot->Ranger.RapidSlot,
                                               Slot->Ranger.RapidSerial);
    if (!Result || Result->Hp <= 0.f ||
        Length(Result->Position.XY - Player->Position.XY) > RAPID_FIRE_RANGE)
    {
        Result = RangerTarget(AppState, Slot, Player, RAPID_FIRE_RANGE);
        Slot->Ranger.RapidSlot = Result ? (u32)(Result - AppState->World.Entities) : 0;
        Slot->Ranger.RapidSerial = Result ? Result->MonsterSerial : 0;
    }
    return Result;
}

internal void
LooseRapidFireArrow(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world_entity *Foe = RapidFireFoe(AppState, Slot, Player);
    ShootRangerArrow(AppState, Player, Foe, RangerShot_Rapid, RAPID_FIRE_DAMAGE, RangerArrow_Rapid);
    EmitSound(&AppState->Events, AssetType_SfxKunai, Player->Position);
}

// NOTE(zoubir): Piercing Shot: the heavy arrow along the aim, every foe in
// its line struck as it passes; the Focus is spent
internal void
LoosePiercingShot(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    v2 Dir = NormalizeOr(Player->Aim, NormalizeOr(Player->CastingDirection, V2(1.f, 0.f)));
    float Focus = Slot->Ranger.Focus;
    bool32 Crit = Focus >= RANGER_FOCUS_MOST &&
        RoleRank(Slot, PlayerRole_Ranger, RangerTalent_Deadeye) > 0;
    float Damage = (PIERCE_DAMAGE + PIERCE_PER_FOCUS * Focus) * (Crit ? DEADEYE_SCALE : 1.f);
    Slot->Ranger.Focus = 0.f;
    u32 Room = RangerShotRoom(AppState, Player);
    v2 From = Player->Position.XY;
    world_entity *Struck[RANGER_MAX_ARROWS];
    float Alongs[RANGER_MAX_ARROWS];
    u32 Count = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount && Count < RANGER_MAX_ARROWS; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            RoomAtPosition(World, Monster->Position.XY) != Room)
        {
            continue;
        }
        v2 Offset = Monster->Position.XY - From;
        float Along = DotProduct(Offset, Dir);
        float Across = Absolute(DotProduct(Offset, V2(-Dir.Y, Dir.X)));
        if (Along > 0.f && Along < PIERCE_RANGE && Across < PIERCE_WIDTH + 0.5f * Monster->Dimensions.X)
        {
            Struck[Count] = Monster;
            Alongs[Count] = Along;
            Count++;
        }
    }
    // NOTE(zoubir): each foe after the first takes PIERCE_FALLOFF of what
    // the one before it took, so the shot is a boss's first and a line's
    // second
    for(u32 Index = 0; Index < Count; Index++)
    {
        float Share = 1.f;
        for(u32 Other = 0; Other < Count; Other++)
        {
            Share *= Alongs[Other] < Alongs[Index] ? PIERCE_FALLOFF : 1.f;
        }
        LooseRangerArrow(AppState, Player->PlayerIndex, Struck[Index], RangerShot_Pierce, Share * Damage,
                         Alongs[Index] / PIERCE_SPEED, Dir);
    }
    // NOTE(zoubir): the power rides along: Focus in tenths, 11 for a crit
    u32 Power = Crit ? 11 : (u32)(Focus / 10.f + 0.5f);
    v3 Start = ChestOf(Player);
    Start.XY += 20.f * Dir;
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_RangerFirst, RangerBurst_Pierce),
              (u8)Player->PlayerIndex, RangerBurstSpot(Start, Power), ATan2(Dir.Y, Dir.X));
    EmitSound(&AppState->Events, AssetType_SfxKunai, Player->Position);
    if (Crit || Focus >= 50.f)
    {
        EmitSound(&AppState->Events, AssetType_SfxGiantFireball, Player->Position);
    }
}

// NOTE(zoubir): a wind-up of this class is over
internal void
FinishRangerCast(app_state *AppState, player_slot *Slot, world_entity *Player, player_spell Spell)
{
    if (Spell == PlayerSpell_RangerA)
    {
        LoosePiercingShot(AppState, Slot, Player);
    }
    else if (Spell == PlayerSpell_RangerB)
    {
        LooseRapidFireArrow(AppState, Slot, Player);
    }
}

// NOTE(zoubir): any hit of this class's player on a monster dealt Damage
internal void
OnRangerHit(app_state *AppState, player_slot *Attacker, world_entity *Target,
         world_entity *Source, float Damage)
{
    float Focus = RangerShotFocus(Attacker->Ranger.Hitting);
    if (Focus > 0.f && IsRangerMarked(AppState, Attacker, Target))
    {
        AddRangerFocus(AppState, Attacker, Focus);
    }
}

// NOTE(zoubir): the share of a hit on Target the player deals, and of a
// hit on it the player takes, after its talents and buffs
internal float
RangerDealtScale(player_slot *Slot, world_entity *Target)
{
    float Result = 1.f + MARKSMAN_SHARE * (float)RoleRank(Slot, PlayerRole_Ranger, RangerTalent_Marksman);
    ranger_slot *Ranger = &Slot->Ranger;
    if (Target && Target->Type == EntityType_Monster && Ranger->MarkSeconds > 0.f &&
        Ranger->MarkSerial == Target->MonsterSerial)
    {
        bool32 Lethal = RoleRank(Slot, PlayerRole_Ranger, RangerTalent_LethalMark) > 0;
        Result *= 1.f + MARK_SHARE + (Lethal ? LETHAL_MARK_SHARE : 0.f);
    }
    return Result;
}

internal float
RangerTakenScale(player_slot *Slot, world_entity *Player)
{
    return 1.f;
}

// NOTE(zoubir): Key's cooldown and a ground spell's radius after talents,
// from the table's Base
internal float
RangerSpellCooldown(player_slot *Slot, u32 Key, float Base)
{
    return Base;
}

internal float
RangerSpellRadius(player_slot *Slot, u32 Key, float Base)
{
    float Result = Base;
    if (Key == 0 && RoleRank(Slot, PlayerRole_Ranger, RangerTalent_Barrage))
    {
        Result *= BARRAGE_RADIUS;
    }
    return Result;
}

// NOTE(zoubir): once a tick for each Ranger: the mark, the channel's
// arrows, Focus draining between fights, and what clients see of it
internal void
UpdateRangerSlot(app_state *AppState, dungeon_run *Run, player_slot *Slot, float DeltaTime)
{
    ranger_slot *Ranger = &Slot->Ranger;
    world_entity *Player = Slot->Entity;
    UpdateRangerMark(AppState, Slot, DeltaTime);
    bool32 Rapid = Player && !IsDeadPlayer(Player) && Player->CastSpell == PlayerSpell_RangerB;
    if (Rapid)
    {
        Ranger->RapidTimer -= DeltaTime;
        // NOTE(zoubir): the last arrow goes as the cast ends (FinishRangerCast)
        if (Ranger->RapidTimer <= 0.f && Player->CastLeft > DeltaTime)
        {
            Ranger->RapidTimer += PlayerSpells[PlayerSpell_RangerB].CastTime / (float)RAPID_FIRE_ARROWS;
            LooseRapidFireArrow(AppState, Slot, Player);
        }
    }
    Ranger->FocusHold = Maximum(0.f, Ranger->FocusHold - DeltaTime);
    if (!Run->FightingRoom && Ranger->FocusHold <= 0.f)
    {
        Ranger->Focus = Maximum(0.f, Ranger->Focus - RANGER_FOCUS_DRAIN * DeltaTime);
    }
    u32 Index = (u32)(Slot - AppState->Players);
    bool32 Deadeye = Ranger->Focus >= RANGER_FOCUS_MOST &&
        RoleRank(Slot, PlayerRole_Ranger, RangerTalent_Deadeye) > 0;
    Slot->ClassMeter = (u8)(Ranger->Focus + 0.5f);
    Slot->ClassFlags = (u8)((RangerMarkedFoe(AppState, Slot) ? RANGER_FLAG_MARK : 0) |
                            (RangerTrapDown(&Run->Ranger, Index) ? RANGER_FLAG_TRAP : 0) |
                            (Deadeye ? RANGER_FLAG_DEADEYE : 0) |
                            (Rapid ? RANGER_FLAG_RAPID : 0));
}

// NOTE(zoubir): once a tick, from UpdateRoleEffects: what the class's
// spells left behind, for every player of the class
internal void
UpdateRangerEffects(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Role == PlayerRole_Ranger)
        {
            UpdateRangerSlot(AppState, Run, Slot, DeltaTime);
        }
        else
        {
            // NOTE(zoubir): a player who left the class leaves its Focus
            // and mark behind
            Slot->Ranger.Focus = 0.f;
            Slot->Ranger.MarkSeconds = 0.f;
        }
    }
    UpdateRangerArrows(AppState, &Run->Ranger, DeltaTime);
    UpdateRangerVolleys(AppState, &Run->Ranger, DeltaTime);
    UpdateRangerTraps(AppState, &Run->Ranger, DeltaTime);
}
