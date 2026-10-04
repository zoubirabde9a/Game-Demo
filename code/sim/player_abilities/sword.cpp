/* Sword (right click): a swing toward the aim that hits each thing in its
   slice once (UpdateSword, update.cpp; reach and width in entity.h). The
   player steps a little that way. A plain cut's damage, shove and timing
   are in player_stats.cpp.

   Swings chain into a combo: a swing that starts within
   SWORD_COMBO_WINDOW of the last one takes the next cut of the chain
   (the first rows of SwordCuts), and the last one is a finisher that
   throws the target up and stuns it. A pause starts the chain over. The
   cuts after the chain are swung only by combo moves (combos.cpp). */

// NOTE(zoubir): a little longer than the swing interval plus a queued
// click's wait, so steady clicking keeps the chain going
#define SWORD_COMBO_WINDOW 0.45f

// NOTE(zoubir): one cut of the sword: what it does to each target, the
// arc clients draw around the swinger (fx_bursts.cpp), which grows step
// by step so the chain can be seen building, and the slice it hits (0 for
// SWORD_REACH and SWORD_HALF_ANGLE)
struct sword_cut
{
    hit Hit;
    sim_burst Arc;
    float Reach;
    float HalfAngle;
};

// NOTE(zoubir): the rows of SwordCuts. The first SWORD_CHAIN_LENGTH are the
// chain plain swings walk through; the rest only combos swing
// (combos.cpp)
enum sword_cut_index
{
    SwordCut_First,
    SwordCut_Second,
    SwordCut_Finisher,
    SwordCut_Lunge,
    SwordCut_Skewer,
    SwordCut_Count
};
#define SWORD_CHAIN_LENGTH 3

// NOTE(zoubir): the lunge's thrust: about twice the reach in a narrow
// slice, so it hits what the dash is carrying the player into
#define SWORD_LUNGE_REACH 84.f
#define SWORD_LUNGE_HALF_ANGLE 0.45f

// NOTE(zoubir): a cut's damage and shove as multiples of a plain cut's
// (player_stats.cpp)
#define CUT_DAMAGE(Scale) ((Scale) * PlayerStats.SwordDamage)
#define CUT_SHOVE(Scale) ((Scale) * PlayerStats.SwordShove)

global_variable sword_cut SwordCuts[SwordCut_Count] =
{
    // NOTE(zoubir): a target already in the air is knocked up a little
    // instead of only away, so swings can juggle it
    {{CUT_DAMAGE(1.f), CUT_SHOVE(1.f), 0.f, 220.f, 0.f, SimBurst_Count},
     SimBurst_SwingArc, 0.f, 0.f},
    {{CUT_DAMAGE(1.f), CUT_SHOVE(1.25f), 0.f, 240.f, 0.f, SimBurst_Count},
     SimBurst_SwingArcBack, 0.f, 0.f},
    // NOTE(zoubir): the finisher
    {{CUT_DAMAGE(1.4f), CUT_SHOVE(1.8f), 300.f, 340.f, 0.7f, SimBurst_Finisher},
     SimBurst_SwingArcFinisher, 0.f, 0.f},
    // NOTE(zoubir): Lunge (dash, attack): harder, and a long shove along
    // the thrust
    {{CUT_DAMAGE(1.5f), CUT_SHOVE(2.f), 0.f, 240.f, 0.2f, SimBurst_Impact},
     SimBurst_Count, SWORD_LUNGE_REACH, SWORD_LUNGE_HALF_ANGLE},
    // NOTE(zoubir): Skewer (jump, dash, attack): the lunge from the air,
    // throwing what it hits straight up and holding it there for a juggle
    {{CUT_DAMAGE(1.5f), 60.f, 380.f, 380.f, 0.8f, SimBurst_Finisher},
     SimBurst_Count, SWORD_LUNGE_REACH, SWORD_LUNGE_HALF_ANGLE},
};
#undef CUT_DAMAGE
#undef CUT_SHOVE

// NOTE(zoubir): the survivor is thrown away from the swinger (or the blade
// when the swinger is unknown), so a hit is felt
internal void
SwordHit(app_state *AppState, world *World, world_entity *Sword,
         world_entity *Target)
{
    v2 From = Sword->Position.XY;
    world_entity *Swinger = Sword->HasOwner ?
        AppState->Players[Sword->OwnerSlot].Entity : 0;
    if (Swinger && Swinger->IsPresent)
    {
        From = Swinger->Position.XY;
    }
    // NOTE(zoubir): a thrust shoves along itself, not out of its line
    v2 Away = Sword->ComboStep == SwordCut_Lunge ? Sword->CastingDirection :
        NormalizeOr(Target->Position.XY - From, Sword->CastingDirection);
    u32 Cut = Sword->ComboStep < SwordCut_Count ? Sword->ComboStep :
        SwordCut_Finisher;
    ApplyHit(AppState, World, Target, &SwordCuts[Cut].Hit, Away,
             Sword, Sword->OwnerSlot);
}

// NOTE(zoubir): a sword of cut Cut toward Dir, without touching the chain
internal world_entity *
SwingSwordCut(app_state *AppState, world *World, memory_arena *Arena,
              world_entity *Player, v2 Dir, u32 Cut)
{
    v3 SwordPosition = Player->Position +
        V3(SWORD_OFFSET * Dir.X, SWORD_OFFSET * Dir.Y, 0.f);
    world_entity *Sword = AddSword(AppState, World, Arena, SwordPosition,
                                   Player, DominantFacing(Dir));
    Sword->CastingDirection = Dir;
    Sword->ComboStep = Cut;
    Sword->SwordReach = SwordCuts[Cut].Reach;
    Sword->SwordHalfAngle = SwordCuts[Cut].HalfAngle;
    // NOTE(zoubir): a combo's cut has none; the combo draws its own
    if (SwordCuts[Cut].Arc != SimBurst_Count)
    {
        EmitBurst(&AppState->Events, SwordCuts[Cut].Arc,
                  (u8)Player->PlayerIndex, Player->Position, ATan2(Dir.Y, Dir.X));
    }
    return Sword;
}

// NOTE(zoubir): the swinger steps toward Dir this tick, at
// SWORD_STEP_SCALE of a run
#define SWORD_STEP_SCALE 0.6f

inline void
StepIntoSwing(player_tick *Tick, v2 Dir)
{
    Tick->Acceleration *= SWORD_STEP_SCALE;
    Tick->DDPlayer.XY = Dir;
}

internal void
SpawnSwordSwing(app_state *AppState, world *World, memory_arena *Arena,
                world_entity *Player, v2 Dir, player_tick *Tick)
{
    StepIntoSwing(Tick, Dir);
    Player->ComboStep = Player->ComboTimer > 0.f ?
        (Player->ComboStep + 1) % SWORD_CHAIN_LENGTH : 0;
    Player->ComboTimer = SWORD_COMBO_WINDOW;
    SwingSwordCut(AppState, World, Arena, Player, Dir, Player->ComboStep);
}
