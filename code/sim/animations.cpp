/* Animation tables: which sprite-sheet frames each unit plays for each
   action and facing. Frame indices point into the unit's texture; the
   last argument of AddAnimation mirrors the frames for the left side. */

internal void
SetupAnimationSets(app_state *AppState, memory_arena *ConstantsArena)
{
    // Zoubir Animation
    {
        
        animation_set *Set =
            &AppState->ZoubirAnimationSet;
        // NOTE(zoubir): the speed at which the walk frames play at their
        // listed rate; faster walking plays them faster. It matches the
        // stride drawn in the frames, so it stays put when the player gets
        // faster: the top run speed of 260 (PlayerStats.RunSpeed,
        // player_stats.cpp) plays them 2.8 times as fast (MoveCycleRate,
        // entity.cpp)
        Set->MoveSpeed = 93.f;
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Move,
                     AnimationDirection_Up,
                     244, 4, 0.08f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Move,
                     AnimationDirection_Down,
                     180, 4, 0.08f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Move,
                     AnimationDirection_Right,
                     212, 4, 0.08f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Move,
                     AnimationDirection_Left,
                     212, 4, 0.08f, true);
        
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stand,
                     AnimationDirection_Up,
                     241, 1, 0.33f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stand,
                     AnimationDirection_Down,
                     177, 1, 0.33f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stand,
                     AnimationDirection_Right,
                     209, 1, 0.33f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stand,
                     AnimationDirection_Left,
                     209, 1, 0.33f, true);

        animation_slot *AttackAnimations[4];
        AttackAnimations[0] =
            AddAnimation(Set, ConstantsArena,
                         AnimationType_Attack,
                         AnimationDirection_Up,
                         80, 6, 0.03f);
        AttackAnimations[1] =
            AddAnimation(Set, ConstantsArena,
                         AnimationType_Attack,
                         AnimationDirection_Down,
                         32, 6, 0.03f);
        AttackAnimations[2] =
            AddAnimation(Set, ConstantsArena,
                         AnimationType_Attack,
                         AnimationDirection_Right,
                         64, 6, 0.03f);
        AttackAnimations[3] =
            AddAnimation(Set, ConstantsArena,
                         AnimationType_Attack,
                         AnimationDirection_Left,
                         64, 6, 0.03f, true);
        for(u32 AttackAnimationIndex = 0;
            AttackAnimationIndex < ArrayCount(AttackAnimations);
            AttackAnimationIndex++)
        {
            
            animation_slot *Animation =
                AttackAnimations[AttackAnimationIndex];
//                Animation->FramesTimeInSeconds[5] = 0.2f;
        }

        // NOTE(zoubir): the skid, about as long as the body takes to stop
        // from a run and settle (0.07 s, player_stats.cpp); at 0.08 a frame
        // it played on for a quarter second after the player stood still
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stop,
                     AnimationDirection_Up,
                     248, 4, 0.04f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stop,
                     AnimationDirection_Down,
                     184, 4, 0.04f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stop,
                     AnimationDirection_Right,
                     216, 4, 0.04f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stop,
                     AnimationDirection_Left,
                     216, 4, 0.04f, true);
        
        AddAnimation(Set, ConstantsArena,
                     AnimationType_JumpUp,
                     AnimationDirection_Up,
                     167, 1, 0.33f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_JumpUp,
                     AnimationDirection_Down,
                     103, 1, 0.33f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_JumpUp,
                     AnimationDirection_Right,
                     135, 1, 0.33f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_JumpUp,
                     AnimationDirection_Left,
                     135, 1, 0.33f, true);
        
        AddAnimation(Set, ConstantsArena,
                     AnimationType_JumpDown,
                     AnimationDirection_Up,
                     168, 1, 0.33f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_JumpDown,
                     AnimationDirection_Down,
                     104, 1, 0.33f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_JumpDown,
                     AnimationDirection_Right,
                     136, 1, 0.33f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_JumpDown,
                     AnimationDirection_Left,
                     136, 1, 0.33f, true);
        
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Cast,
                     AnimationDirection_Up,
                     160, 4, 0.1f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Cast,
                     AnimationDirection_Down,
                     96, 4, 0.1f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Cast,
                     AnimationDirection_Right,
                     128, 4, 0.1f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Cast,
                     AnimationDirection_Left,
                     128, 4, 0.1f, true);
    }
    
    //Familiar Animation
    {
        
        animation_set *Set =
            &AppState->FamiliarAnimationSet;
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Move,
                     AnimationDirection_Right,
                     0, 1, 0.15f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Move,
                     AnimationDirection_Left,
                     0, 1, 0.15f, true);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stand,
                     AnimationDirection_Right,
                     0, 1, 0.15f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stand,
                     AnimationDirection_Left,
                     0, 1, 0.15f, true);
    }

    //FireBall Animation
    {
        
        animation_set *Set =
            &AppState->FireballAnimationSet;
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Move,
                     AnimationDirection_Up,
                     0, 4, 0.15f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Move,
                     AnimationDirection_Down,
                     12, 4, 0.15f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Move,
                     AnimationDirection_Right,
                     4, 4, 0.15f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Move,
                     AnimationDirection_Left,
                     4, 4, 0.15f, true);

    }

    //Sword Animation
    {
        
        animation_set *Set =
            &AppState->SwordAnimationSet;
        
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stand,
                     AnimationDirection_Right,
                     0, 3, 0.06f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stand,
                     AnimationDirection_Left,
                     0, 3, 0.06f, true);
        
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stand,
                     AnimationDirection_Up,
                     0, 3, 0.06f);
        AddAnimation(Set, ConstantsArena,
                     AnimationType_Stand,
                     AnimationDirection_Down,
                     0, 3, 0.06f);

    }
}
