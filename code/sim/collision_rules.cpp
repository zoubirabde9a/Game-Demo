/* Collision queries: tile collision map lookups and per-entity-pair
   collision rules (a hash of pairs that should or should not collide). */

inline bool32
GetCollisionValue(world *World, u32 TileX, u32 TileY)
{
    Assert(TileX < World->NumCollisionX);
    Assert(TileY < World->NumCollisionY);
    
    u32 Index = TileX + TileY * World->NumCollisionX;
    bool32 Result = World->CollisionMap[Index];
    return Result;
}

inline bool32
IsPositionColliding(world *World, cannonical_position Position)
{
    bool32 Result = GetCollisionValue(World, Position.TileX,
                                      Position.TileY);
    return Result;
}

internal void
ClearCollisionRulesFor(app_state *AppState, u32 Entity)
{
    for(u32 BucketIndex = 0;
        BucketIndex < ArrayCount(AppState->CollisionRuleHash);
        BucketIndex++)
    {
        for(pairwise_collision_rule **Rule = &AppState->CollisionRuleHash[BucketIndex];
            *Rule;
            )
        {
            if ((*Rule)->EntityA == Entity ||
                (*Rule)->EntityB == Entity)
            {
                pairwise_collision_rule *RemovedRule = *Rule;                
                *Rule = (*Rule)->Next;
                RemovedRule->Next = AppState->FirstFreeCollisionRule;
                AppState->FirstFreeCollisionRule =
                    RemovedRule;
            }
            else
            {
                Rule = &((*Rule)->Next);
            }
        }
    }
}

internal void
AddCollisionRule(app_state *AppState, memory_arena *Arena,
                 u32 EntityA, u32 EntityB,
                 bool32 Tag)
{   
    if (EntityA > EntityB)
    {
        u32 Tmp = EntityA;
        EntityA = EntityB;
        EntityB = Tmp;
    }
    
    // Hash on Entity A
    // TODO(zoubir): Better Hash Function 02
    u32 HashValue =
        EntityA & (ArrayCount(AppState->CollisionRuleHash) - 1);

    
    pairwise_collision_rule *FoundRule = 0;
    for(pairwise_collision_rule *Rule = AppState->CollisionRuleHash[HashValue];
        Rule;
        Rule = Rule->Next)
    {
        if (Rule->EntityA == EntityA &&
            Rule->EntityB == EntityB)
        {
            FoundRule = Rule;
            break;
        }
    }

    if (FoundRule)
    {
        FoundRule->Tag = Tag;
    }
    else
    {
        pairwise_collision_rule *NewRule;
        
        if (AppState->FirstFreeCollisionRule)
        {
            NewRule = AppState->FirstFreeCollisionRule;
            AppState->FirstFreeCollisionRule =
                AppState->FirstFreeCollisionRule->Next;
        }
        else
        {
            NewRule = AllocateStruct(Arena, pairwise_collision_rule);
        }
        
        NewRule->EntityA = EntityA;
        NewRule->EntityB = EntityB;
        NewRule->Tag = Tag;
        NewRule->Next = AppState->CollisionRuleHash[HashValue];
        AppState->CollisionRuleHash[HashValue] = NewRule;
    }
}

internal bool32
CanCollide(app_state *AppState, world_entity *A,
              world_entity *B)
{
    bool32 Result = false;

    if(A != B)
    {
        if(A->ID > B->ID)
        {
            world_entity *Tmp = A;
            A = B;
            B = Tmp;
        }

        // TODO(zoubir): Property-based logic goes here
        Result = true;
            
        // TODO(zoubir): Better Hash Function 02        
        u32 HashBucket = A->ID & (ArrayCount(AppState->CollisionRuleHash) - 1);
        for(pairwise_collision_rule *Rule = AppState->CollisionRuleHash[HashBucket];
            Rule;
            Rule = Rule->Next)
        {
            if((Rule->EntityA == A->ID) &&
               (Rule->EntityB == B->ID))
            {
                Result = Rule->Tag;
                break;
            }
        }
    }
    
    return(Result);
}

// NOTE(zoubir): which entity types block or hit each other at all;
// pairs not listed here pass through
internal void
SetupCollisionTable(app_state *AppState)
{
    SetCollision(AppState, EntityType_Familiar,
                 EntityType_StaticObject, true);
    
    SetCollision(AppState, EntityType_Familiar,
                 EntityType_Monster, true);
    
    SetCollision(AppState, EntityType_Familiar,
                 EntityType_Tiled, true);
    
    SetCollision(AppState, EntityType_Player,
                 EntityType_Player, true);
    
    SetCollision(AppState, EntityType_Player,
                 EntityType_StaticObject, true);
    
    SetCollision(AppState, EntityType_Player,
                 EntityType_Tiled, true);
    
    SetCollision(AppState, EntityType_Player,
                 EntityType_Monster, true);

    SetCollision(AppState, EntityType_Monster,
                 EntityType_StaticObject, true);

    SetCollision(AppState, EntityType_Monster,
                 EntityType_FireBall, true);
    
    SetCollision(AppState, EntityType_Monster,
                 EntityType_Monster, true);

    SetCollision(AppState, EntityType_StaticObject,
                 EntityType_StaticObject, true);

    SetCollision(AppState, EntityType_Sword,
                 EntityType_Monster, true);

    SetCollision(AppState, EntityType_Sword,
                 EntityType_Player, true);

    SetCollision(AppState, EntityType_Player,
                 EntityType_FireBall, true);

    // NOTE(zoubir): walls, trees and rocks stop fireballs; UpdateFireBall
    // removes one that was blocked or deflected
    SetCollision(AppState, EntityType_FireBall,
                 EntityType_StaticObject, true);
    SetCollision(AppState, EntityType_FireBall,
                 EntityType_Tiled, true);

    // NOTE(zoubir): walls stop monster shots; players are hit by distance
    // in UpdateMonsterShot so a shot never shoves anyone
    SetCollision(AppState, EntityType_MonsterShot,
                 EntityType_StaticObject, true);
    SetCollision(AppState, EntityType_MonsterShot,
                 EntityType_Tiled, true);
}

// NOTE(zoubir): collision shapes, shared by every entity of a kind;
// half sizes in world units, sitting on the ground
internal void
SetupCollisionVolumes(app_state *AppState, memory_arena *ConstantsArena)
{
    AppState->TileObjectCollision =
        MakeSimpleGroundedCollisionVolume(ConstantsArena, {96.f*0.5f, 76.f*0.5f, 24.f});
    AppState->TreeCollision =
        MakeGroundedTreeCollisionVolume(ConstantsArena);
    AppState->WallCollision = 
        MakeSimpleGroundedCollisionVolume(ConstantsArena, {ARENA_TILE_SIZE*.5f, ARENA_TILE_SIZE*.5f, ARENA_TILE_SIZE*.5f});
    AppState->PlayerCollision = 
        MakeSimpleGroundedCollisionVolume(ConstantsArena, {15, 4, 19.f});
    AppState->FamiliarCollision = 
        MakeSimpleGroundedCollisionVolume(ConstantsArena, {11, 6, 3.f});        
    // NOTE(zoubir): tall enough that fireballs cast at hand height hit
    AppState->BatCollision = 
        MakeSimpleGroundedCollisionVolume(ConstantsArena, {11, 6, 10.f});
    AppState->FireBallCollision = 
        MakeSimpleGroundedCollisionVolume(ConstantsArena, {9, 9, 0.f});        
    AppState->SwordCollision = 
        MakeSimpleGroundedCollisionVolume(ConstantsArena, {31, 31, 31.f});
}
