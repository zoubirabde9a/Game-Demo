/* Sword (right click): a swing toward the aim that hits each thing in its
   slice once (UpdateSword, update.cpp; reach and width in entity.h). The
   player lunges a little that way. Timing is its row in
   spawn_actions.cpp.

   Swings chain into a combo: a swing that starts within
   SWORD_COMBO_WINDOW of the last one takes the next hit of SwordCombo,
   and the last one is a finisher that throws the target up and stuns it.
   A pause starts the chain over. */

// NOTE(zoubir): a little longer than the swing interval plus a queued
// click's wait, so steady clicking keeps the chain going
#define SWORD_COMBO_WINDOW 0.45f

// NOTE(zoubir): one step of the combo: what it does to each target, and
// the arc clients draw around the swinger (fx_bursts.cpp), which grows
// step by step so the chain can be seen building
struct sword_combo_step
{
    player_hit Hit;
    sim_burst Arc;
};

global_variable sword_combo_step SwordCombo[] =
{
    {{SWORD_DAMAGE, SWORD_KNOCKBACK, 0.f, 0.f, SimBurst_Count},
     SimBurst_SwingArc},
    {{SWORD_DAMAGE, 1.25f * SWORD_KNOCKBACK, 0.f, 0.f, SimBurst_Count},
     SimBurst_SwingArcBack},
    // NOTE(zoubir): the finisher
    {{1.4f * SWORD_DAMAGE, 1.8f * SWORD_KNOCKBACK, 300.f, 0.7f, SimBurst_Finisher},
     SimBurst_SwingArcFinisher},
};

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
    v2 Away = Target->Position.XY - From;
    float Distance = Length(Away);
    Away = Distance > 0.f ? Away * (1.f / Distance) : Sword->CastingDirection;
    u32 Last = ArrayCount(SwordCombo) - 1;
    u32 Step = Sword->ComboStep < Last ? Sword->ComboStep : Last;
    ApplyPlayerHit(AppState, World, Target, &SwordCombo[Step].Hit, Away,
                   Sword->OwnerSlot, Sword);
}

internal void
SpawnSwordSwing(app_state *AppState, world *World, memory_arena *Arena,
                world_entity *Player, v2 Dir, player_tick *Tick)
{
    Tick->Acceleration *= 0.6f;
    Tick->DDPlayer.XY = Dir;
    v3 SwordPosition = Player->Position +
        V3(SWORD_OFFSET * Dir.X, SWORD_OFFSET * Dir.Y, 0.f);
    Player->ComboStep = Player->ComboTimer > 0.f ?
        (Player->ComboStep + 1) % ArrayCount(SwordCombo) : 0;
    Player->ComboTimer = SWORD_COMBO_WINDOW;
    world_entity *Sword = AddSword(AppState, World, Arena, SwordPosition,
                                   Player, DominantFacing(Dir));
    Sword->CastingDirection = Dir;
    Sword->ComboStep = Player->ComboStep;
    EmitBurst(&AppState->Events, SwordCombo[Player->ComboStep].Arc,
              (u8)Player->PlayerIndex, Player->Position, ATan2(Dir.Y, Dir.X));
}
