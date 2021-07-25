/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: zoubir $
   ======================================================================== */


#include "app.h"
#include "stdio.h"
#include "string.h"

#include "random.cpp"
#include "utility.cpp"

#include "asset.cpp"
#include "render.cpp"
#include "ui.cpp"
#include "world.cpp"
#include "entity.cpp"
#include "opengl.cpp"
#include "audio.cpp"

#include "app_ui.h"



render_vertex VertexC(float X, float Y, float Z,
                      float U, float V)
 {
     
     render_vertex Result;
     Result.X = X;
     Result.Y = Y;
     Result.Z = Z;
     Result.Color = RGBA8_WHITE;
     Result.U = U;
     Result.V = V;
     
     return Result;
 }


#if 0
void PrintTest(render_context *RenderContext, app_memory *Memory,
               float X, float Y, char *text)
{
   // assume orthographic projection with units = screen pixels, origin at top left
   while (*text) {
      if (*text >= 32 && *text < 128) {
         stbtt_aligned_quad q;
         stbtt_GetBakedQuad((stbtt_bakedchar *)Memory->Font->Glyphs,
                            Memory->FontBitmapWidth,
                            Memory->FontBitmapHeight,
                             *text-32, &X,&Y,&q,1);//1=opengl & d3d10+,0=d3d9
         float QuadWidth = q.x1 - q.x0;
         float QuadHeight = q.y1 - q.y0;
         float UX = q.s0;
         float UY = q.t0;
         float TW = q.s1;
         float TH = q.t1;
         float ImageWidth = 512.f;
         float ImageHeight = 512.f;
         float QuadX = ImageWidth * UX;
         float QuadY = ImageHeight * UY;
         QuadWidth = (q.s1 - q.s0) * 512;
         QuadHeight = (q.t1 - q.t0) * 512;
         #if 0
         RenderQuadTexture(RenderContext, QuadX, QuadY,
                           QuadWidth, QuadHeight,
                           0.f, 0.f, 1.f, 1.f, RGBA8_WHITE, 1.f);
         #endif
         #if 1
         RenderQuadTexture(RenderContext, q.x0, q.y0,
                           QuadWidth, QuadHeight,
                           UX, UY, TW, TH, RGBA8_WHITE, 1.f);
         #endif
      }
      ++text;
   }
}
#endif
// old code 
#if 0
internal void *
Allocate(thread_context *Thread, app_memory *Memory, u32 Bytes)
{
    // TODO(zoubir): add a case where more memory is needed
    // if must be
    void *Result = 0;
    // look for a free block
    app_memory_block_header **FreeBlocks = Memory->PermanentStorageFreeBlocks;
    for(u32 CurrentFreeBlockIndex = 0;
        CurrentFreeBlockIndex < Memory->PermanentStorageFreeBlocksCount;
        CurrentFreeBlockIndex++)
    {
        app_memory_block_header *ThisFreeBlock = FreeBlocks[CurrentFreeBlockIndex];
        if (ThisFreeBlock->Size >= Bytes)
        {
            // Allocate This Block
            Result = ThisFreeBlock + 1;
            u32 RemainingBytes = ThisFreeBlock->Size - Bytes;
            if (RemainingBytes > sizeof(app_memory_block_header))
            {
                // Reduce Remaining Block size
                app_memory_block_header *NewFreeBlock =
                    (app_memory_block_header *)(RemainingBytes - sizeof(app_memory_block_header));
                
                NewFreeBlock->Size = ThisFreeBlock->Size
                    - Bytes - sizeof(app_memory_block_header);
                NewFreeBlock->FreeBlockIndex = CurrentFreeBlockIndex;
                NewFreeBlock->Previous = ThisFreeBlock;
                
                ThisFreeBlock->Size = Bytes;
                ThisFreeBlock->FreeBlockIndex = 0;

                return Result;
            }
            else
            {
                // Take the block and remove the header from the freeblocks array
                for(int FreeBlockIndex = CurrentFreeBlockIndex;
                    FreeBlockIndex >= 0;
                    FreeBlockIndex--)
                {
                    
                }
                // adjust the FreeBlockIndex for the remaining
                // Free Blocks 
                for(int FreeBlockIndex = CurrentFreeBlockIndex + 1;
                    FreeBlockIndex < Memory->PermanentStorageFreeBlocksCount;
                    FreeBlockIndex++)
                {
                    
                }
                
                return Result;
            }
        }
    }
#if 0    
    // see if we have enough memory left through nextBlockHeader
    void *NextFreeBlockHeader = Memory->PermanentStorageNextFreeBlockHeader;
    if (NextFreeBlockHeader && NextFreeBlockHeader->Size >= Bytes)
    {
        // use the current pointer to allocate
    }
    else
    {
        
    }

    
    for(u32 CurrentFreeBlockIndex = 0;
        CurrentFreeBlockIndex < Memory->PermanentStorageFreeBlocksCount;
        CurrentFreeBlockIndex++)
    {
        app_memory_free_block *ThisFreeBlock =
            &Memory->PermanentStorageFreeBlocks[CurrentFreeBlockIndex];
        if (ThisFreeBlock->Size > Bytes)
        {
            // take free block, reduce size and offset Ptr
            Result = ThisFreeBlock->Ptr;

            ThisFreeBlock->Ptr = (void *)((size_t)ThisFreeBlock->Ptr + (size_t)Bytes);
            ThisFreeBlock->Size = ThisFreeBlock->Size - Bytes;

            return Result;
        }
        else if (ThisFreeBlock->Size == Bytes)
        {
            // take this free block, and remove element
            Result = ThisFreeBlock->Ptr;
            
            // move all element By 1 Position
            for(int FreeBlockIndex = CurrentFreeBlockIndex;
                FreeBlockIndex > 0;
                FreeBlockIndex--)
            {
                Memory->PermanentStorageFreeBlocks[FreeBlockIndex] =
                    Memory->PermanentStorageFreeBlocks[FreeBlockIndex - 1];
            }
            Memory->PermanentStorageFreeBlocks++;
            Memory->PermanentStorageFreeBlocksCount--;

            return Result;
        }
    }
#endif
    return Result;
}

internal void
Free(thread_context *Thread, app_memory *Memory, void *Ptr)
{
    
}
#endif

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

internal world_entity *
AddPlayer(app_state *AppState,
          world *World, memory_arena *Arena, v3 Position)
{
    assets *Assets = &AppState->Assets;
    world_entity *Player =
        AddEntity(AppState, World, Arena,
                  EntityType_Player, Position,
                  AppState->PlayerCollision);
    
    Player->Position = Position;
    Player->Dimensions = V2(48.F, 48.f);
    Player->AnimationState = {};
    Player->Texture = {AssetType_Zoubir};
    Player->ShadowTexture = {AssetType_Shadow};
    Player->Direction = {0, -1};
//    Player->TextureOrigin = {0.5f, 0.875f};

    
    Player->AnimationSet = &AppState->ZoubirAnimationSet;;
#if 0        
    animation_slot *MoveUpAnimation =
        GetAnimation(PlayerAnimationSet,
                     AnimationType_Move,
                     AnimationDirection_Up);
    MoveUpAnimation->FirstIndex = 244;
    MoveUpAnimation->IndicesCount = 4;
        
    animation_slot *MoveDownAnimation =
        GetAnimation(PlayerAnimationSet,
                     AnimationType_Move,
                     AnimationDirection_Down);
    MoveDownAnimation->FirstIndex = 180;
    MoveDownAnimation->IndicesCount = 4;
        
    animation_slot *MoveRightAnimation =
        GetAnimation(PlayerAnimationSet,
                     AnimationType_Move,
                     AnimationDirection_Right);
    MoveRightAnimation->FirstIndex = 212;
    MoveRightAnimation->IndicesCount = 4;
        
    animation_slot *MoveLeftAnimation = 
        GetAnimation(PlayerAnimationSet,
                     AnimationType_Move,
                     AnimationDirection_Left);
    MoveLeftAnimation->FirstIndex = 212;
    MoveLeftAnimation->IndicesCount = 4;
    MoveLeftAnimation->Reversed = true;
        
    animation_slot *StandUpAnimation = 
        GetAnimation(PlayerAnimationSet,
                     AnimationType_Stand,
                     AnimationDirection_Up);
    StandUpAnimation->FirstIndex = 0;
    StandUpAnimation->IndicesCount = 1;
        
    animation_slot *StandDownAnimation =
        GetAnimation(PlayerAnimationSet,
                     AnimationType_Stand,
                     AnimationDirection_Down);
    StandDownAnimation->FirstIndex = 12;
    StandDownAnimation->IndicesCount = 1;
        
    animation_slot *StandRightAnimation =
        GetAnimation(PlayerAnimationSet,
                     AnimationType_Stand,
                     AnimationDirection_Right);
    StandRightAnimation->FirstIndex = 4;
    StandRightAnimation->IndicesCount = 1;
        
    animation_slot *StandLeftAnimation =
        GetAnimation(PlayerAnimationSet,
                     AnimationType_Stand,
                     AnimationDirection_Left);
    StandLeftAnimation->FirstIndex = 8;
    StandLeftAnimation->IndicesCount = 1;
#endif
    return Player;
}

internal world_entity *
AddSword(app_state *AppState,
         world *World, memory_arena *Arena, v3 Position,
         world_entity *Caster, animation_direction AnimationDirection)
{
    assets *Assets = &AppState->Assets;
    world_entity *Entity =
        AddEntity(AppState, World, Arena,
                  EntityType_Sword, Position,
                  AppState->SwordCollision);
    
    Entity->Position = Position;
    Entity->Dimensions = {32, 32};
    Entity->AnimationState = {};
    Entity->Texture = {AssetType_Sword};
    Entity->ShadowTexture = {};
//    Player->TextureOrigin = {0.5f, 0.875f};
    Entity->TimeLeft = 0.18f;
    Entity->AnimationDirection = AnimationDirection;
    
    Entity->AnimationSet = &AppState->SwordAnimationSet;
        #if 0
    animation_slot *SlashAnimation =
        GetAnimation(AnimationSet,
                     AnimationType_Stand,
                     AnimationDirection_Right);
    SlashAnimation->FirstIndex = 0;
    SlashAnimation->IndicesCount = 3;
    #endif

//    AddCollisionRule();
    


    return Entity;
}

internal world_entity *
AddWall(app_state *AppState,
          world *World, memory_arena *Arena, v3 Position)
{
    assets *Assets = &AppState->Assets;
    
    float WallWidth = (float)World->TileWidth;
    float WallHeight = (float)World->TileHeight;
    float WallDepth = (float)World->TileDepth;
    
    world_entity *Wall =
        AddEntity(AppState, World, Arena,
                  EntityType_StaticObject, Position,
                  AppState->WallCollision);
    
    Wall->Dimensions = {WallWidth, WallHeight};
//    Wall->TextureOrigin = {0.5f, 0.5f};
//    Wall->Uvs = GetTextureUvsFromIndex(Wall->Texture->Width,
//                                       Wall->Texture->Height,
//                                       3, 4, 4);
    
    return Wall;
}

internal world_entity *
AddTree(app_state *AppState,
          world *World, memory_arena *Arena, v3 Position)
{
    assets *Assets = &AppState->Assets;
    
    float Width = 122.f;
    float Height = 159.f;
    
    world_entity *Entity =
        AddEntity(AppState, World, Arena,
                  EntityType_StaticObject, Position,
                  AppState->TreeCollision);

    Entity->Dimensions = {Width, Height};
    Entity->Texture = {AssetType_Tree};
//    Entity->TextureOrigin = {0.5f, 0.931f};
    Entity->Uvs = {0.f, 0.f, 1.f, 1.f};
    
    return Entity;
}

internal world_entity *
AddTileEntity(app_state *AppState,
              world *World, memory_arena *Arena,
              u32 TileX, u32 TileY, u32 TileZ)
{
    assets *Assets = &AppState->Assets;

    u32 NumTilesX = 3;
    u32 NumTilesY = 3;
    u32 NumTilesZ = 2;

    v2 TextureDims = V2((NumTilesX * World->TileWidth),
                        ((NumTilesZ + NumTilesY) * World->TileHeight));

    v3 HalfDims = V3((World->TileWidth * NumTilesX / 2),
                     (World->TileHeight * NumTilesY / 2),
                     (World->TileDepth * NumTilesZ / 2));

    v3 Position = V3(TileX * World->TileWidth + HalfDims.X,
                     TileY * World->TileHeight + HalfDims.Y,
                     (float)(TileZ * World->TileWidth));
    
    world_entity *Entity =
        AddEntity(AppState, World, Arena,
                  EntityType_Tiled, Position,
                  AppState->TileObjectCollision);

    Entity->Dimensions = {TextureDims.X, TextureDims.Y};
    Entity->Texture = {AssetType_TileMap};
    //
//    Entity->TextureOrigin =
//        V2(0.5f, 0.67f);
    
    Entity->NumTilesX = NumTilesX;
    Entity->NumTilesY = NumTilesY;
    Entity->NumTilesZ = NumTilesZ;

    u32 TileMapOffsetX = 5;
    u32 TileMapOffsetY = 178;
    u32 TileMapNumTilesX = 8;
    for(u32 IndexY = 0;
        IndexY <  NumTilesY + NumTilesZ - 1;
        IndexY++)
    {
        u32 TileMapIndexY = TileMapOffsetY - IndexY;
        for(u32 IndexX = 0;
            IndexX < NumTilesX;
            IndexX++)
        {
            u32 TileMapIndexX = TileMapOffsetX + IndexX;
            u32 Index = TileMapIndexX +
                (TileMapIndexY * TileMapNumTilesX);
            Entity->TileIndices[IndexX + IndexY * NumTilesX] =
                Index;
        }
    }
    
    return Entity;
}

internal world_entity *
AddMonster(app_state *AppState,
          world *World, memory_arena *Arena, v3 Position)
{
    assets *Assets = &AppState->Assets;
    world_entity *Entity =
        AddEntity(AppState, World, Arena,
                  EntityType_Monster,
                  Position, AppState->PlayerCollision);
    
    Entity->Dimensions = {48, 48};
    Entity->AnimationState = {};
    Entity->Texture = {AssetType_Zoubir};
    Entity->ShadowTexture = {AssetType_Shadow};
//    Entity->TextureOrigin = {0.5f, 0.875f};
    Entity->Hp = 100.f;
    
    Entity->AnimationSet = &AppState->ZoubirAnimationSet;
        
    return Entity;
}

internal world_entity *
AddFamiliar(app_state *AppState,
          world *World, memory_arena *Arena,
            world_entity *Player)
{
    v3 Position = Player->Position + v3{40, 40, 0};
    
    assets *Assets = &AppState->Assets;
    world_entity *Entity =
        AddEntity(AppState, World, Arena,
                  EntityType_Familiar, Position,
                  AppState->FamiliarCollision);
    
    Entity->Dimensions = {45 * 0.5f, 25 * 0.5f};
    Entity->AnimationState = {};
    Entity->Texture = {AssetType_Familiar};
    Entity->ShadowTexture = {AssetType_Shadow};
//    Entity->TextureOrigin = {0.5f, 0.5f};
    Entity->FollowingEntity = Player;
    
    Entity->AnimationSet = &AppState->FamiliarAnimationSet;
#if 0        
    animation_slot *MoveRightAnimation =
        GetAnimation(EntityAnimationSet,
                     AnimationType_Move,
                     AnimationDirection_Right);
    MoveRightAnimation->FirstIndex = 0;
    MoveRightAnimation->IndicesCount = 1;
        
    animation_slot *MoveLeftAnimation = 
        GetAnimation(EntityAnimationSet,
                     AnimationType_Move,
                     AnimationDirection_Left);
    MoveLeftAnimation->FirstIndex = 1;
    MoveLeftAnimation->IndicesCount = 1;
        
    animation_slot *StandRightAnimation =
        GetAnimation(EntityAnimationSet,
                     AnimationType_Stand,
                     AnimationDirection_Right);
    StandRightAnimation->FirstIndex = 0;
    StandRightAnimation->IndicesCount = 1;
        
    animation_slot *StandLeftAnimation =
        GetAnimation(EntityAnimationSet,
                     AnimationType_Stand,
                     AnimationDirection_Left);
    StandLeftAnimation->FirstIndex = 1;
    StandLeftAnimation->IndicesCount = 1;
    #endif
    return Entity;
}

internal world_entity *
AddFireBall(app_state *AppState,
          world *World, memory_arena *Arena,
            world_entity *Owner, v3 CastPosition,
            v3 Velocity)
{
    assets *Assets = &AppState->Assets;

    float Width = 18.f;
    float Height = 18.f;
    
    world_entity *Entity =
        AddEntity(AppState, World, Arena,
                  EntityType_FireBall, CastPosition,
                  AppState->FireBallCollision);

    Entity->Velocity = Velocity;
    Entity->Dimensions = {Width, Height};
    Entity->AnimationState = {};
    Entity->Texture = {AssetType_FireBall};
    Entity->ShadowTexture = {AssetType_Shadow};
//    Entity->TextureOrigin = {0.5f, 0.5f};
    Entity->DistanceRemaining = 300.f;
    Entity->TimeLeft = 1.0f;
    
    Entity->AnimationSet = &AppState->FireballAnimationSet;
    return Entity;
}

internal void
UpdatePlayer(world_entity *Player, world *World,
             memory_arena *Arena,
             app_input *Input, app_state *AppState,
             v3 CameraOffset,
             float *AnimationSpeedRate,
             animation_type *AnimationType,
             animation_direction *AnimationDirection)
{
    float DeltaTime = Input->DeltaTime;
    v3 DDPlayer = {};
    float PlayerAcceleration = 56000.f;
    *AnimationType = AnimationType_Stand;
    *AnimationDirection =
        Player->AnimationState.LastAnimationDirection;
    *AnimationSpeedRate = 1.f;
    v2 MouseWorldPosition =
        V2((float)Input->MouseX, (float)Input->MouseY) +
        CameraOffset.XY;

    bool32 Move = 0;
    bool32 Up = Input->ButtonZ.EndedDown;
    bool32 Down = Input->ButtonS.EndedDown;
    bool32 Right = Input->ButtonD.EndedDown;
    bool32 Left = Input->ButtonQ.EndedDown;

    v2 Dir = {};
    float CastingTimeEventLinger = 0.15f;    
    float MoveEventLinger = 0.15f;    

    if (Up)
    {
        Dir.Y = -1.f;
        Move = true;        
    }
    if (Down)
    {
        Dir.Y = 1.f;
        Move = true;
    }
    if (Right)
    {
        Dir.X = 1.f;
        Move = true;
    }
    if (Left)
    {
        Dir.X = -1.f;
        Move = true;
    }

    if (!Up &&
        !Down &&
        (Right || Left))
    {
        Dir.Y = 0.f;
    }
    
    if (!Right &&
        !Left &&
        (Up || Down))
    {        
        Dir.X = 0.f;
    }
    
    if (Move)
    {
        for(u32 DelayedMoveIndex = 0;
            DelayedMoveIndex < 1;
            DelayedMoveIndex ++)
        {
            AddPlayerDelayedMove(AppState, MoveEventLinger, Dir);
        }
    }

    Move = 0;            
    bool32 Jumping = Player->Velocity.Z != 0.f;
    if (Jumping)
    {
        Player->State = EntityState_Jumping;
    }
    
        
    if (Input->RightButton.Pressed)
    {
        AddPlayerDelayedAttack(AppState, CastingTimeEventLinger, Player->Direction);
    }

    if (Input->LeftButton.Pressed)
    {       
        AddPlayerDelayedCast(AppState, CastingTimeEventLinger, Player->Direction);
    }
    

    if (Player->State == EntityState_Attacking)
    {
        
        if (IsAnimationFinished(Player->AnimationSet,
                    &Player->AnimationState,
                    AnimationType_Attack,
                                *AnimationDirection))
        {
            
            Player->State = EntityState_Standing;
        }
    }
            
    if (Player->State == EntityState_Casting &&// IsSet(Player, EntityFlag_Casting) &&
        IsAnimationFinished(Player->AnimationSet,
                            &Player->AnimationState,
                            AnimationType_Cast,
                            *AnimationDirection))
    {
        Player->State = EntityState_Standing;
//        ClearFlag(Player, EntityFlag_Casting);
    }
    
    bool32 Halted = 0;
    for(u32 DelayedInputIndex = 0;
        DelayedInputIndex < AppState->PlayerDelayedInputCount;)
    {
        player_delayed_input *DelayedInput =
            &AppState->PlayerDelayedInput[DelayedInputIndex];
        bool32 RemoveEvent = 0;
        bool32 Consumed = 0;
        if (Halted)
        {                
            DelayedInput->TimeRemaining -= Input->DeltaTime;
            if (DelayedInput->TimeRemaining <= 0.f)
            {
                RemoveEvent = true;
            }
            else
            {
                DelayedInputIndex++;
            }
        }
        else
        {
            if (DelayedInput->TimeRemaining > 0.f)
            {
                if (DelayedInput->Type == PDI_Attack)
                {
                    if (Player->State != EntityState_Attacking)//!IsSet(Player, EntityFlag_Casting))
                    {
                        Player->State = EntityState_Attacking;
//                    AddFlags(Player, EntityFlag_Casting);

                        v2 DirVec = Player->Direction;
//                        v2 DirVec = DelayedInput->Dir;
                        Player->CastingDirection = DirVec;

                        Player->AnimationState.SlotIndex = 0;
                        PlayerAcceleration *= 0.6f;
                        DDPlayer.XY = DirVec;
                        v3 SwordPosition = Player->Position + V3(16.f * DirVec.X, 16 * DirVec.Y, 0.f);
                        animation_direction SwordAnimationDirection =
                            AnimationDirection_Right;
                        
                        if (DirVec.X == 1.f)
                        {
                            SwordAnimationDirection =
                                AnimationDirection_Right;                            
                        }
                        if (DirVec.X == -1.f)
                        {
                            SwordAnimationDirection =
                                AnimationDirection_Left;    
                        }
                        if (DirVec.Y == 1.f)
                        {
                            SwordAnimationDirection =
                                AnimationDirection_Down;                            
                        }
                        if (DirVec.Y == -1.f)
                        {
                            SwordAnimationDirection =
                                AnimationDirection_Up;    
                        }
                        
                        AddSword(AppState, World, Arena, SwordPosition,
                                 Player, SwordAnimationDirection);
                        PlaySound(AppState, {AssetType_Dash});
                        Consumed = true;
                    }
                }
                else if (DelayedInput->Type == PDI_Move)
                {
                    
                    Player->Direction = DelayedInput->Dir;
                    Move = true;
                    Consumed = true;
                }
                else if (DelayedInput->Type == PDI_Cast)
                {

                    float FireBallVelocityScaler = 450.f;
                    float HandOffsetZ = 30;
                    v2 FireBallPosition =
                        Player->Position.XY +
                        DelayedInput->Dir * V2(32.f, 32.f);
                    v2 FireBallVelocity =
                        FireBallVelocityScaler * DelayedInput->Dir;
                    
                    *AnimationType = AnimationType_Cast;
                    world_entity *FireBall = AddFireBall(AppState, World, Arena,
                                                       Player,
                                                       V3(FireBallPosition.X, FireBallPosition.Y, HandOffsetZ),
                                                       V3(FireBallVelocity.X, FireBallVelocity.Y, 0.f));
                    FireBall->AnimationSpeed = 1.f;
                    FireBall->AnimationType = AnimationType_Move;
                    FireBall->AnimationDirection = AnimationDirection_Right;
                    
                    if (DelayedInput->Dir.X == 1.f)
                    {                        
                        FireBall->AnimationDirection = AnimationDirection_Right;
                    }
                    if (DelayedInput->Dir.X == -1.f)
                    {                        
                        FireBall->AnimationDirection = AnimationDirection_Left;                        
                    }
                    if (DelayedInput->Dir.Y == 1.f)
                    {                        
                        FireBall->AnimationDirection = AnimationDirection_Down;                        
                    }
                    if (DelayedInput->Dir.Y == -1.f)
                    {
                        FireBall->AnimationDirection = AnimationDirection_Up;                        
                    }
                    
                    PlaySound(AppState, {AssetType_FireCast});
                    Player->State = EntityState_Standing;
                    Player->State = EntityState_Casting;
//                    AddFlags(Player, EntityFlag_Casting);
                    Player->AnimationState.SlotIndex = 0;
                    Consumed = true;
                }
            }
            else
            {
                Consumed = true;
            }
            
            if (Consumed)
            {
                RemoveEvent = true;
                Halted = true;
            }
            else
            {                    
                DelayedInput->TimeRemaining -= Input->DeltaTime;
                DelayedInputIndex++;
            }

        }
        if (RemoveEvent)
        {
            AppState->PlayerDelayedInput[DelayedInputIndex] =
                AppState->PlayerDelayedInput[--AppState->PlayerDelayedInputCount];
        }
    }
    
    if (Player->State == EntityState_Attacking ||
        Player->State == EntityState_Casting)//IsSet(Player, EntityFlag_Casting))
    {
        Move = 0;
    }
    
    if (Move)
    {
        Player->State = EntityState_Moving;
    }

    // Stop
    if (!Move &&
        Player->State == EntityState_Moving &&
        !(Player->State == EntityState_Casting))//IsSet(Player, EntityFlag_Casting))
    {
        Player->State = EntityState_Stopping;
        Player->AnimationState.SlotIndex = 0;
    }
    else
    {

    }
    
    if (Input->SpaceButton.Pressed)
    {
        Player->State = EntityState_Jumping;
        Player->Velocity.Z = 230.f;
        PlaySound(AppState, {AssetType_ZoubirAudio});
    }
    
    if (Input->AltButton.Pressed)
    {
        PlayerAcceleration *= 10;
        PlaySound(AppState, {AssetType_Dash});
    }

    // Animation
    if (Player->State == EntityState_Stopping &&
        IsAnimationFinished(Player->AnimationSet,
                            &Player->AnimationState,
                            AnimationType_Stop,
                            *AnimationDirection))
    {
        Player->State = EntityState_Standing;
    }
    
    if (Player->State == EntityState_Stopping)
    {
        Assert(!Move);
        if (Player->Direction.Y == -1.f)
        {            
            *AnimationType = AnimationType_Stop;
            *AnimationDirection = AnimationDirection_Up;
        }
        if (Player->Direction.Y == 1.f)
        {        
            *AnimationType = AnimationType_Stop;
            *AnimationDirection = AnimationDirection_Down;
        }
        if (Player->Direction.X == 1.f)
        {
            *AnimationType = AnimationType_Stop;
            *AnimationDirection = AnimationDirection_Right;
        }
        if (Player->Direction.X == -1.f)
        {
            *AnimationType = AnimationType_Stop;
            *AnimationDirection = AnimationDirection_Left;
        }
                
    }
    else if (Player->State == EntityState_Attacking)
    {
        if (Player->CastingDirection.Y == -1.f)
        {            
            *AnimationType = AnimationType_Attack;
            *AnimationDirection = AnimationDirection_Up;
        }
        if (Player->CastingDirection.Y == 1.f)
        {        
            *AnimationType = AnimationType_Attack;
            *AnimationDirection = AnimationDirection_Down;
        }
        if (Player->CastingDirection.X == 1.f)
        {
            *AnimationType = AnimationType_Attack;
            *AnimationDirection = AnimationDirection_Right;
        }
        if (Player->CastingDirection.X == -1.f)
        {
            *AnimationType = AnimationType_Attack;
            *AnimationDirection = AnimationDirection_Left;
        }
    }
    else
    {
        // Stand And Move
        if (Player->State == EntityState_Casting)//IsSet(Player, EntityFlag_Casting))
        {
            *AnimationType = AnimationType_Cast;
        }
        else if (Move)
        {
            *AnimationType = AnimationType_Move;             
        }
        else   
        {
            *AnimationType = AnimationType_Stand;
        }
        
        if (Player->Direction.Y == -1.f)
        {            
            if (Move)
            {
                DDPlayer.Y = -1;
            }
            *AnimationDirection = AnimationDirection_Up;
        }
        if (Player->Direction.Y == 1.f)
        {
            
            if (Move)
            {
                DDPlayer.Y = 1;
            }
            *AnimationDirection = AnimationDirection_Down;
        }
        if (Player->Direction.X == 1.f)
        {
            
            if (Move)
            {
                DDPlayer.X = 1;
            }
            *AnimationDirection = AnimationDirection_Right;
        }
        if (Player->Direction.X == -1.f)
        {
            
            if (Move)
            {
                DDPlayer.X = -1.f;
            }
            *AnimationDirection = AnimationDirection_Left;
        }
    
        if (Input->ButtonJ.Pressed)
        {
            playing_sound *BattleTheme =
                PlaySound(AppState, {AssetType_BattleTheme});
//            ChangeVolume(BattleTheme, 5, {});
        }
    
    }


#if 1
    float DDPlayerLengthSq = LengthSq(DDPlayer);
    if (DDPlayerLengthSq > 1.f)
    {
        DDPlayer *= 1.f / SquareRoot(DDPlayerLengthSq);
    }
#else
    if (DDPlayer.X != 0 && DDPlayer.Y != 0)
    {
        DDPlayer *= .707106781187f;
    }
#endif
    
    

    DDPlayer *= PlayerAcceleration * DeltaTime;
    // Drag
    DDPlayer -= (10.f * Player->Velocity);
    //Gravity
    DDPlayer.Z = -1000.f;
    
    
//    float DeltaZ = 0.5f * DDPlayer.Z * Square(DeltaTime) +
//        Player->Velocity.Z * DeltaTime;    
//    Player->Velocity.Z = DDPlayer.Z * DeltaTime +
//        Player->Velocity.Z;    
//    Player->Position.Z += DeltaZ * DeltaTime;
//    Player->Position.Z = Maximum(0.f, Player->Position.Z);
    
    float MaxDistance = 10000.f;
    MoveEntity(Player, World, Arena, Input, AppState,
               DDPlayer, &MaxDistance);



    if (Player->Velocity.Z > 0.f)
    {
        *AnimationType = AnimationType_JumpUp;
    }
    if (Player->Velocity.Z < 0.f)
    {
        *AnimationType = AnimationType_JumpDown;
    }

}

internal void
UpdateFamiliar(world_entity *Entity, world *World,
             memory_arena *Arena,
             app_input *Input, app_state *AppState,
             float *AnimationSpeed,
             animation_type *AnimationType,
             animation_direction *AnimationDirection)
{
    float DeltaTime = Input->DeltaTime;
    v3 DDEntity = {};
    float EntityAcceleration = 56000.f;
    *AnimationType = AnimationType_Stand;
    *AnimationDirection =
        Entity->AnimationState.LastAnimationDirection;
    *AnimationSpeed = 1.f;
    
    Assert(Entity->FollowingEntity);
    v3 FollowingPos = Entity->FollowingEntity->Position;
    v3 EntityPos = Entity->Position;
    
    DDEntity = FollowingPos - EntityPos;
    
    float DDEntityAbsoluteX = Absolute(DDEntity.X);
    float DDEntityAbsoluteY = Absolute(DDEntity.Y);

        if (DDEntity.X > 0)
        {
            *AnimationDirection = AnimationDirection_Right;
        }
        else
        {
            *AnimationDirection = AnimationDirection_Left;
        }
    
    float DDEntityLength = Length(DDEntity);
    if (DDEntityLength > 1.f)
    {
        DDEntity *= 1.f / DDEntityLength;
    }

    DDEntity *= EntityAcceleration * DeltaTime;
    DDEntity -= (10.f * Entity->Velocity);

    Entity->tFlying += DeltaTime * 5;
    float TwoPi = 2.f * Pi32;
    if (Entity->tFlying > TwoPi)
    {
        Entity->tFlying -= TwoPi;
    }
    Entity->Position.Z = 30 + 4.f * Sin(Entity->tFlying);
    
#if 0        
    float DeltaZ = 0.5f * DDEntityZ * Square(DeltaTime) +
        Entity->VelocityZ * DeltaTime;
    Entity->VelocityZ = DDEntityZ * DeltaTime +
        Entity->VelocityZ;
    Entity->Z += DeltaZ * DeltaTime;
    Entity->Z = Maximum(0.f, Entity->Z);
#endif
    
    float MaxDistanceFromFollowingEntity = 45.f;
    if (DDEntityLength > MaxDistanceFromFollowingEntity)
    {        
        float MaxDistance =  DDEntityLength -
            MaxDistanceFromFollowingEntity;
        MoveEntity(Entity, World, Arena, Input, AppState,
                   DDEntity, &MaxDistance);
    }
}

internal void
UpdateFireBall(world_entity *Entity, world *World,
               memory_arena *Arena,
               app_input *Input, app_state *AppState)
{
    float DeltaTime = Input->DeltaTime;
    v3 DDEntity = {};
    float DeltaZ = 0.5f * DDEntity.Z * Square(DeltaTime) +
        Entity->Velocity.Z * DeltaTime;    
    Entity->Velocity.Z = DDEntity.Z * DeltaTime +
        Entity->Velocity.Z;
    
    Entity->Position.Z += DeltaZ * DeltaTime;
    
    DDEntity.X -= (0.75f * Entity->Velocity.X);
    DDEntity.Y -= (0.75f * Entity->Velocity.Y);
    
    if (Entity->Position.Z <= 0.f ||
        Entity->DistanceRemaining <= 0.f ||
        Entity->TimeLeft <= 0.f)
    {
        RemoveEntity(World, Entity);
    }
    else
    {
        MoveEntity(Entity, World, Arena, Input, AppState,
                   DDEntity, &Entity->DistanceRemaining);
        Entity->TimeLeft -= DeltaTime;
    }
}

//TODO(zoubir): check if its 3D compatible
inline v3
CenterCamera(v3 Position, float MinX, float MinY,
             float MaxX, float MaxY, u32 WindowWidth,
             u32 WindowHeight)
{
    v3 Result = Position;
    
    Result.X -= (float)(WindowWidth / 2);
    Result.Y -= (float)(WindowHeight / 2);
    
    Result.X = Minimum(Result.X, MaxX - WindowWidth);
    Result.Y = Minimum(Result.Y, MaxY - WindowHeight);
    
    Result.X = Maximum(Result.X, MinX);
    Result.Y = Maximum(Result.Y, MinY);
    
    return Result;
}


internal void
DrawTileEntity(render_context *RenderContext,
               app_state *AppState,
               render_program TextureProgram,
               assets *Assets,
               world *World,
               world_entity *Entity, v3 CameraOffset)
{
    open_gl *OpenGL = AppState->OpenGL;
    v2 TextureOrigin = V2(0.5f, 0.67f);
        
    v2 EntityCameraPosition = Entity->Position.XY - CameraOffset.XY;
    v2 EntityTexturePosition = EntityCameraPosition -
        TextureOrigin *
        V2(Entity->NumTilesX,
           Entity->NumTilesY + Entity->NumTilesZ - 1) *
        V2(World->TileWidth, World->TileHeight);
    EntityTexturePosition.Y -= Entity->Position.Z;
    
    float SortingValue =
        Entity->Position.Y - Entity->Collision->TotalVolume.HalfDims.Y;
    loaded_texture *Texture = GetTexture(Assets, OpenGL, AppState, Entity->Texture);
    if (Texture)
    {        
        ColorRGBA8 Color;
        Color.ColorU32 = RGBA8_WHITE;
        Color.A = 255;
        BeginBatch(RenderContext, Texture->ID,
                   SortingValue, TextureProgram);
        
        for(u32 TileY = 0;
            TileY < (Entity->NumTilesY + Entity->NumTilesZ - 1);
            TileY++)
        {
            for(u32 TileX = 0;
                TileX < Entity->NumTilesX;
                TileX++)
            {
                u32 TileValue =
                    Entity->TileIndices[TileX + TileY *
                                        (Entity->NumTilesX)];
                //TODO(zoubir): remove TextureTileNum X/Y
                // and replace them with the world
                // tileWidth and TileHeight
                u32 TextureTileNumX = Texture->Width /
                    World->TileWidth;
                u32 TextureTileNumY = Texture->Height /
                    World->TileHeight;
                v4 Uvs =
                    GetTextureUvsFromIndex(Texture->Width,
                                           Texture->Height,
                                           TextureTileNumX,
                                           TextureTileNumY,
                                           TileValue);

                v2 Offset = V2(TileX * World->TileWidth,
                               TileY * World->TileHeight);
                
                RenderQuadTexture(RenderContext,
                                  EntityTexturePosition.X + Offset.X,
                                  EntityTexturePosition.Y + Offset.Y,
                                  (float)World->TileWidth,
                                  (float)World->TileHeight,
                                  Uvs,
                                  Color.ColorU32, Entity->Position.Z);
            }
        }
        
        EndBatch(RenderContext);
        
#if 0
        for(u32 VolumeIndex = 0;
            VolumeIndex < Entity->Collision->VolumesCount;
            VolumeIndex++)
        {
            entity_collision_volume *Volume =
                Entity->Collision->Volumes + VolumeIndex;
            
        v2 CollisionRectPosition = EntityCameraPosition -
            Volume->HalfDims.XY;
        CollisionRectPosition.Y -= Entity->Position.Z -
            Volume->HalfDims.Z + Volume->Offset.Z;
    
        DrawRectangle3D(RenderContext,
                        CollisionRectPosition.X,
                        CollisionRectPosition.Y,
                        Volume->HalfDims.X * 2,
                        Volume->HalfDims.Y * 2,
                        Volume->HalfDims.Z * 2,
                        RGBA8_RED, SortingValue);
        }
    #endif
    }
}
internal void
DrawEntity(render_context *RenderContext,
           app_state *AppState,
           render_program TextureProgram,
           assets *Assets,
           world_entity *Entity, v3 CameraOffset)
{
    open_gl *OpenGL = AppState->OpenGL;
    loaded_texture *Texture = 0;
    zas_texture_info *TextureInfo = 0;
    v2 EntityCameraPosition = Entity->Position.XY - CameraOffset.XY;
    //TODO(zoubir): figure out if we need this check
    // inside assets
    float SortingValue =
        Entity->Position.Y;
    
    if (Entity->Texture.Type)
    {
        TextureInfo = &GetAssetInfo(Assets, Entity->Texture)->Texture;
        Texture = GetTexture(Assets, OpenGL, AppState, Entity->Texture);
        v2 EntityTexturePosition = EntityCameraPosition -
            TextureInfo->Origin * Entity->Dimensions;
        EntityTexturePosition.Y -= Entity->Position.Z;
        
        if (Texture)
        {
            BeginBatch(RenderContext, Texture->ID,
                       SortingValue, TextureProgram);

            ColorRGBA8 Color;
            Color.ColorU32 = RGBA8_WHITE;
            Color.A = 255;
            RenderQuadTexture(RenderContext,
                              EntityTexturePosition.X,
                              EntityTexturePosition.Y,
                              Entity->Dimensions.X,
                              Entity->Dimensions.Y,
                              Entity->Uvs,
                              Color.ColorU32,
                              Entity->Position.Z);
            EndBatch(RenderContext);
        }
        {
            // Hp
            if (Entity->Hp > 0.f && Entity->Hp <= 100.f)
            {
                DrawRectangle(RenderContext,
                              EntityTexturePosition.X +
                              Entity->Dimensions.X * 0.25f,
                              EntityTexturePosition.Y -
                              10,
                              (Entity->Hp / 100.f) *
                              (Entity->Dimensions.X * 0.5f),
                              1,
                              RGBA8_WHITE, SortingValue);
            }
#if 0
            DrawRectangle(RenderContext,
                          EntityTexturePosition.X,
                          EntityTexturePosition.Y,
                          Entity->Dimensions.X,
                          Entity->Dimensions.Y,
                          RGBA8_WHITE, SortingValue);
#endif

        
#if 0
            entity_collision_volume *Volume =
                &Entity->Collision->TotalVolume;
            
            v2 CollisionRectPosition = EntityCameraPosition -
                Volume->HalfDims.XY;
            CollisionRectPosition.Y -= Entity->Position.Z -
                Volume->HalfDims.Z +
                Volume->Offset.Z;
    
            DrawRectangle3D(RenderContext,
                            CollisionRectPosition.X,
                            CollisionRectPosition.Y,
                            Volume->HalfDims.X * 2,
                            Volume->HalfDims.Y * 2,
                            Volume->HalfDims.Z * 2,
                            RGBA8_RED, SortingValue);
#endif
        }

    }
    
#if 0        
    for(u32 VolumeIndex = 0;
        VolumeIndex < Entity->Collision->VolumesCount;
        VolumeIndex++)
    {
        entity_collision_volume *Volume =
            Entity->Collision->Volumes + VolumeIndex;
            
        v2 CollisionRectPosition = EntityCameraPosition -
            Volume->HalfDims.XY;
        CollisionRectPosition.Y -= Entity->Position.Z -
            Volume->HalfDims.Z +
            Volume->Offset.Z;
    
        DrawRectangle3D(RenderContext,
                        CollisionRectPosition.X,
                        CollisionRectPosition.Y,
                        Volume->HalfDims.X * 2,
                        Volume->HalfDims.Y * 2,
                        Volume->HalfDims.Z * 2,
                        RGBA8_RED, SortingValue);
    }
        
#endif
            
    loaded_texture *ShadowTexture = 0;
    zas_texture_info *ShadowTextureInfo = 0;
    if (Entity->ShadowTexture.Type)
    {
        ShadowTexture = GetTexture(Assets, OpenGL, AppState, Entity->ShadowTexture);
        ShadowTextureInfo = &GetAssetInfo(Assets, Entity->ShadowTexture)->Texture;
        
        v2 ShadowDims = {28, 14};
        ColorRGBA8 ShadowColor;
        ShadowColor.ColorU32 = RGBA8_WHITE;
        if (Entity->Position.Z < 200)
        {
            float Diff = Entity->Position.Z - Entity->GroundZ;
            ShadowColor.A = (u8)(255 - (Diff) - 55);
            if (Diff > 1.f)
            {
                ShadowDims *= (1.f - (Entity->Position.Z - Entity->GroundZ) / 200.f);
            }
        }
        else
        {
            ShadowColor.A = 0;
            ShadowDims = {};
        }
        v2 ShadowPosition = EntityCameraPosition -
            ShadowTextureInfo->Origin * ShadowDims;
        ShadowPosition.Y -= Entity->GroundZ;
    
        if (ShadowTexture)
        {
            BeginBatch(RenderContext, ShadowTexture->ID,
                       Entity->Position.Y, TextureProgram);

                            
            RenderQuadTexture(RenderContext,
                              ShadowPosition.X,
                              ShadowPosition.Y,
                              ShadowDims.X,
                              ShadowDims.Y,
                              {0.f, 0.f, 1.f, 1.f},
                              ShadowColor.ColorU32, 0.f);
            EndBatch(RenderContext);
        }
    
    }
       
    
    
}

internal void
DoEntityAnimation(world_entity *Entity, assets *Assets, app_state *AppState,
                  float DeltaTime, float AnimationSpeedRate,
                  animation_type AnimationType, animation_direction AnimationDirection)
{
    open_gl *OpenGL = AppState->OpenGL;
    loaded_texture *Texture = GetTexture(Assets, OpenGL,
                                         AppState, Entity->Texture);
    zas_texture_info *TextureInfo = &GetAssetInfo(Assets, Entity->Texture)->Texture;
    if (Texture)
    {
        Entity->Uvs = DoAnimation(&Entity->AnimationState, 
                                      Texture->Width,
                                      Texture->Height,
                                      TextureInfo->NumTilesX,
                                      TextureInfo->NumTilesY,
                                      DeltaTime,
                                      AnimationSpeedRate,
                                      Entity->AnimationSet,
                                      AnimationType,
                                      AnimationDirection);
    }
                

}

//TODO(zoubir): remove the collision and tiles in
// world_position
extern "C" APP_UPDATE_AND_RENDER(AppUpdateAndRender)
{
    app_state *AppState = (app_state *)Memory->PermanentStorage;
    transient_state *TransientState = (transient_state *)Memory->TransientStorage;
    memory_arena *MemoryArena = &AppState->MemoryArena;
    memory_arena *TransientArena = &TransientState->MemoryArena;
    memory_arena *ConstantsArena = &AppState->ConstantsArena;    
    
    assets *Assets = &AppState->Assets;
    audio_state *AudioState = &AppState->AudioState;
    render_context *RenderContext = &Thread->RenderContext;
    open_gl *OpenGL = RenderContext->OpenGL;

    static u32 VAO = 0;
    static u32 VBO = 0;
    static loaded_texture TextureL = {};
    
    if (!AppState->IsInitialized)
    {
        Platform = Memory->PlatformApi;
        AppState->OpenGL = RenderContext->OpenGL;
        
        InitializeArena(MemoryArena,
                        (memory_index *)(AppState + 1),
                        Memory->PermanentStorageSize - sizeof(app_state));
        
        InitializeArena(TransientArena,
                        (memory_index *)(TransientState + 1),
                        Memory->TransientStorageSize - sizeof(transient_state));
        SubArena(ConstantsArena, MemoryArena, Kilobytes(64));
        AppState->WorkQueue = Memory->WorkQueue;
        InitializeAudio(AudioState);        
        AppInitOpenGL(TransientArena, AppState, Thread, Memory);
        VAO = RenderContext->VAO;
        VBO = RenderContext->VBO;
#if 1
        u8 *C = (u8 *)malloc(1024 * 1024 * 4);
        for(u32 Index = 0;
            Index < 1024 * 1024 * 4;
            Index++)
        {
            C[Index] = 255;
        }
        TextureL = LoadOpenglTexture(Assets, OpenGL, 2 << 6, (2 << 6) + 1, GL_RGBA,
                                     (2 << 6) * ((2 << 6) + 1) * 4, C, TEXTURE_SOFT_FILTER);        
        free(C);
        
#endif
        InitializeAssets(Assets, OpenGL, AppState, MemoryArena);
                               
        temporary_memory TempMem = BeginTemporaryMemory(TransientArena);
        // Texture Loading        
        AppState->TextureCache = TextureCacheCreate(MemoryArena, 8, 20 * 8);

        AppState->DefaultFont =
            CreateFont(OpenGL, MemoryArena,
                       24.f, 512, 512,
                       "c:/windows/fonts/times.ttf");

        AppState->UIContext =
            UIContextCreate(MemoryArena, UI_COUNT);
        // TODO(zoubir): make this struct a pointer
        // to a struct
        u32 const TileWidth = 32;
        u32 const TileHeight = 32;
        u32 const TileDepth = 32 * 1;

        u32 const CollisionWidth = TileWidth;
        u32 const CollisionHeight = TileHeight;
        u32 const CollisionDepth = TileDepth;
        
        u32 const DesiredTilesX = 80;
        u32 const DesiredTilesY = 40;
        u32 const DesiredTilesZ = 12;
        
        u32 const CollisionToTilesX =
            TileWidth / CollisionWidth;
        u32 const CollisionToTilesY =
            TileHeight / CollisionHeight;
    
        AppState->TileObjectCollision =
            MakeSimpleGroundedCollisionVolume(ConstantsArena, {96.f*0.5f, 76.f*0.5f, 24.f});
        AppState->TreeCollision =
            MakeGroundedTreeCollisionVolume(ConstantsArena);
        AppState->WallCollision = 
            MakeSimpleGroundedCollisionVolume(ConstantsArena, {TileWidth*.5f, TileHeight*.5f, TileDepth *.5f});
        AppState->PlayerCollision = 
            MakeSimpleGroundedCollisionVolume(ConstantsArena, {15, 4, 19.f});
        AppState->FamiliarCollision = 
            MakeSimpleGroundedCollisionVolume(ConstantsArena, {11, 6, 3.f});        
        AppState->FireBallCollision = 
            MakeSimpleGroundedCollisionVolume(ConstantsArena, {9, 9, 0.f});        
        AppState->SwordCollision = 
            MakeSimpleGroundedCollisionVolume(ConstantsArena, {31, 31, 31.f});

        // Zoubir Animation
        {
            
            animation_set *Set =
                &AppState->ZoubirAnimationSet;
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

            AddAnimation(Set, ConstantsArena,
                         AnimationType_Stop,
                         AnimationDirection_Up,
                         248, 4, 0.08f);
            AddAnimation(Set, ConstantsArena,
                         AnimationType_Stop,
                         AnimationDirection_Down,
                         184, 4, 0.08f);
            AddAnimation(Set, ConstantsArena,
                         AnimationType_Stop,
                         AnimationDirection_Right,
                         216, 4, 0.08f);
            AddAnimation(Set, ConstantsArena,
                         AnimationType_Stop,
                         AnimationDirection_Left,
                         216, 4, 0.08f, true);
            
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
        
        world *World = &AppState->World;
        World->TileWidth = TileWidth;
        World->TileHeight = TileHeight;
        World->TileDepth = TileDepth;
        
        World->CollisionWidth = World->TileWidth;
        World->CollisionHeight = World->TileHeight;
        World->CollisionDepth = World->TileDepth;
        
        World->TilesPerChunkX = 16;
        World->TilesPerChunkY = 16;
        World->TilesPerChunkZ = 4;

        World->MaxEntityVelocity = {1.f, 1.f, 1.f};
        
        
        Assert(World->CollisionWidth <= World->TileWidth);
        Assert(World->CollisionHeight <= World->TileHeight);
        
        u32 const CollisionNumX = DesiredTilesX * CollisionToTilesX;
        u32 const CollisionNumY = DesiredTilesY * CollisionToTilesY;
        bool32 *CollisionTiles =
            AllocateArray(MemoryArena,
                          CollisionNumX * CollisionNumY,
                          bool32);
        World->CollisionMap = CollisionTiles;
        World->NumCollisionX = CollisionNumX;
        World->NumCollisionY = CollisionNumY;
    
        u32 const TileNumX = DesiredTilesX;
        u32 const TileNumY = DesiredTilesY;
        u32 const TileNumZ = DesiredTilesZ;
        
        asset_id TMT = {AssetType_TileMap};
        tile_map *TileMap = &World->TileMap;
        TileMap->Texture = TMT;
        World->NumTilesX = TileNumX;
        World->NumTilesY = TileNumY;
        World->NumTilesZ = TileNumZ;
        tile *Tiles = AllocateArray(MemoryArena,
                                    TileNumX * TileNumY,
                                    tile);

        for(u32 CurrentTile = 0;
            CurrentTile < (World->NumCollisionX *
                           World->NumCollisionY);
            CurrentTile++)
        {
            CollisionTiles[CurrentTile] = CurrentTile % (17) == 2;
            if ((CurrentTile < World->NumCollisionX) ||
                (CurrentTile > (World->NumCollisionX *
                                 World->NumCollisionY) - World->NumCollisionX) ||
                (CurrentTile % World->NumCollisionX) == 0 ||
                ((CurrentTile + 1) % World->NumCollisionX) == 0)
            {
                CollisionTiles[CurrentTile] = true;
                #if 1
                u32 X = CurrentTile % World->NumCollisionX;
                u32 Y = CurrentTile / World->NumCollisionX;
                v3 Pos;
                Pos.X = X * (float)World->TileWidth +
                    (float)World->TileWidth / 2.f;
                Pos.Y = Y * (float)World->TileHeight +
                    (float)World->TileHeight / 2.f;
                Pos.Z = 0.f;
                AddWall(AppState, World, MemoryArena, Pos);
                #endif
            }            
        }
    
        u32 TileIndex = 1592;
        //u32 TileIndex = 10;
        for(u32 CurrentTile = 0;
            CurrentTile < (World->NumTilesX * World->NumTilesY);
            CurrentTile++)
        {
            if (CollisionTiles[CurrentTile])
            {
//                Tiles[CurrentTile].Index = 4;
                Tiles[CurrentTile].Index = TileIndex;
            }
            else
            {            
                Tiles[CurrentTile].Index = TileIndex;
                u32 CurrentTileY = CurrentTile / World->NumTilesX;
                u32 CurrentTileX = CurrentTile % World->NumTilesX;
                if (CurrentTileX % World->TilesPerChunkX == 0 ||
                    CurrentTileY % World->TilesPerChunkY == 0)
                {
                    Tiles[CurrentTile].Index = 1567;
                    //Tiles[CurrentTile].Index = 6;
                }
                if (CurrentTile % 70 == 0)
                {
                    u32 X = CurrentTile % World->NumTilesX;
                    u32 Y = CurrentTile / World->NumTilesX;
                    v3 Pos;
                    Pos.X = X * (float)World->TileWidth +
                        (float)World->TileWidth / 2.f;
                    Pos.Y = Y * (float)World->TileHeight +
                        (float)World->TileHeight / 2.f;
                    Pos.Z = 0.f;
                    AddTree(AppState, World, MemoryArena, Pos);
                }
            }
        }




        TileMap->Tiles = Tiles;

        world_entity *Player = AddPlayer(AppState, World,
                                         MemoryArena, {350, 300});
        AppState->Player = Player;
        
        #if 1
        world_entity *Familiar =
            AddFamiliar(AppState, World, MemoryArena, Player);
        #endif
        
        for(u32 MonsterIndex = 0;
            MonsterIndex < 10;
            MonsterIndex++)
        {
            AddMonster(AppState, World,
                       MemoryArena,
                       {((MonsterIndex % 3) * 32.f + MonsterIndex) * 32.f,
                               600 +
                               (float)((MonsterIndex % 2) * 5 + MonsterIndex) * 4,
                               0.f});
        }

        random_series Series = Seed(67);
        u32 PosX = 12;
        u32 PosY = 6;
        for(u32 EntityIndex = 0;
            EntityIndex < 10;
            EntityIndex++)
        {
            u32 DirX = RandomChoice(&Series, 2);
            u32 DirY = RandomChoice(&Series, 2);
            PosX += DirX * 5;
            PosY += DirY * 5;
            AddTileEntity(AppState, World, MemoryArena, PosX, PosY, 0);
        }
        
        
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
        EndTemporaryMemory(TempMem);
        AppState->IsInitialized = true;
        
    }
    
    render_program *TextureProgram = &RenderContext->TextureProgram;
    render_program *LineProgram = &RenderContext->LineProgram;

    LoadOpenglTexturesFromQueue(Assets, OpenGL, &AppState->OpenglTextureQueue);
    
    ui_context *UIContext = AppState->UIContext;
    
    temporary_memory FrameTemporaryMemory =
        BeginTemporaryMemory(TransientArena);
    
    texture_cache *TextureCache = AppState->TextureCache;

//    loaded_texture *ZoubirTexture = GetTexture(Assets, OpenGL,
//                                               AppState, {AssetType_Zoubir});
    
    font *DefaultFont = AppState->DefaultFont;

    float Right = (float)Window->Width;
    float Left = 0.f;
    float Top = 0.f;
    float Bottom = (float)Window->Height;
    float Far = 100000.f;
    float Near = 0.f;

    //https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix/opengl-perspective-projection-matrix
    mat4 ProjectionMatrix = OrthoMatrix(Left, Right, Bottom, Top, Near, Far);

#if 0
    static float A = 0.f;

    A+= 0.01f;

    mat4 ProjectionMatrix =
        PerspectiveMatrix(Window->Width / (float)Window->Height, -Pi32 * 0.6f, 0.f, 1.f) *
        TranslationMatrix(-0.5f, -0.5f, 0.f) *
        ScaleMatrix(-1.f / Window->Width * 2, 1.f / Window->Height * 2, 0.001f);
#endif
    
    TextureProgram->ProjectionMatrix = &ProjectionMatrix;
    LineProgram->ProjectionMatrix = &ProjectionMatrix;
    
    Assert(&Input->controllers[0].terminator - &Input->controllers[0].buttons[0] ==
           ArrayCount(Input->controllers[0].buttons));
    Assert(Memory->PermanentStorageSize > sizeof(app_state));

    for(int controllerIndex = 0;
        controllerIndex < ArrayCount(Input->controllers);
        controllerIndex++)
    {
        app_controller_input *thisController = GetController(Input, controllerIndex);

    }
//    glViewport(0, 0, Window->Width, Window->Height);
    
    
    OpenGL->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    OpenGL->glClearColor(1.0f, 0.5f, 0.5f, 1.0f);


#if 1
    world *World = &AppState->World;
    tile_map *TileMap = &World->TileMap;
    world_entity *Player = AppState->Player;

    float TileMapWidth = (float)(World->NumTilesX * World->TileWidth);
    float TileMapHeight = (float)(World->NumTilesY * World->TileHeight);
    u32 WindowHalfWidth = Window->Width / 2;
    u32 WindowHalfHeight = Window->Height / 2;

    AppState->TargetCamera =
        CenterCamera(Player->Position,
                     0.f, 0.f,
                     TileMapWidth,
                     TileMapHeight,
                     Window->Width,
                     Window->Height);

    #if 0
    float LerpValue = 0.95f;
    float DeltaTime = Input->DeltaTime;
    v3 CameraOffset = AppState->CameraOffset =
        AppState->CameraOffset +
        (AppState->TargetCamera - AppState->CameraOffset) *
        LerpValue * DeltaTime;
    #else
    v3 CameraOffset = AppState->CameraOffset =
        AppState->TargetCamera;
    #endif
    
    CameraOffset.X = (float)((u32)CameraOffset.X);
    CameraOffset.Y = (float)((u32)CameraOffset.Y);
    
    v3 CameraDims = {(float)Window->Width,
                     (float)Window->Height,
                     1.f};
    v3 CameraGrabEntitySafetyMargin = {32, 32};
    v3 CameraMinPos = CameraOffset;
    v3 CameraMaxPos = CameraOffset + CameraDims;
    CameraMinPos -= CameraGrabEntitySafetyMargin;
    CameraMaxPos += CameraGrabEntitySafetyMargin;
    
    u32 MinChunkX;
    u32 MinChunkY;
    u32 MinChunkZ;
    
    u32 MaxChunkX;
    u32 MaxChunkY;
    u32 MaxChunkZ;
    
    rectangle3 Box = RectMinMax(CameraMinPos,
                                CameraMaxPos);
    GetChunksFromBox(World,
                     Box,
                     &MinChunkX, &MinChunkY, &MinChunkZ,
                     &MaxChunkX, &MaxChunkY, &MaxChunkZ);
    
    
    #if 1
    {
    u32 BatchesCount =(World->NumTilesX *
                       World->NumTilesY + World->EntityCount * 2) ;
    u32 VerticesCount = 6 * BatchesCount;
    SetupBatchRenderer(RenderContext, TransientArena, BatchesCount);
    RenderBegin(RenderContext, VerticesCount, RENDER_ORDER_BACK_TO_FRONT);
    
    loaded_texture *TileMapTexture = GetTexture(Assets, OpenGL,
                                                AppState, TileMap->Texture);
    // Drawing Tile Map
#if 1
    if (TileMapTexture)
    {
    BeginBatch(RenderContext, TileMapTexture->ID, 0.f, *TextureProgram);    
    u32 TextureTileNumX = TileMapTexture->Width / 32;
    u32 TextureTileNumY = TileMapTexture->Height / 32;
    for(u32 TileYIndex = 0;
        TileYIndex < World->NumTilesY;
        TileYIndex++)
    {
        for(u32 TileXIndex = 0;
            TileXIndex < World->NumTilesX;
            TileXIndex++)
        {
            tile *ThisTile =
                &TileMap->Tiles[TileXIndex +
                              TileYIndex * World->NumTilesX];
            
            v4 TileUvs = GetTextureUvsFromIndex(TileMapTexture->Width,
                                            TileMapTexture->Height,
                                            TextureTileNumX,
                                            TextureTileNumY,
                                            ThisTile->Index);
            RenderQuadTexture(RenderContext,
                              (float)TileXIndex * World->TileWidth -
                              CameraOffset.X,
                              (float)TileYIndex * World->TileHeight -
                              CameraOffset.Y,
                              (float)World->TileWidth,
                              (float)World->TileHeight,
                              TileUvs,
                              RGBA8_WHITE,
                              0.f);
        }
    }    
    #endif
            EndBatch(RenderContext);
    }
    }
    #endif
    
#if 1
    // TODO(zoubir): IMPORTANT
    // this shit is not updating entities
    // that are not in camera bounds
    u32 UpdateID = AppState->UpdateID++;
    
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *ThisEntity =
            &World->Entities[EntityIndex];
        if (ThisEntity->IsPresent)
        {
                        
//                        Assert(ThisEntity->CollisionHalfDims.Z * 2.f <
//                               World->MaxEntityHalfDims.Z);

//                        Assert(ThisEntity->CollisionHalfDims.Z * 2.f >
//                               World->MaxEntityVelocity.Z);

                        
        if (ThisEntity->UpdateID != UpdateID)
        {
            ThisEntity->UpdateID = UpdateID;
        switch(ThisEntity->Type)
        {
            case EntityType_Player:
            {
                // NOTE(zoubir): player code    
                float AnimationSpeedRate;
                animation_type AnimationType;
                animation_direction AnimationDirection;
    
                UpdatePlayer(ThisEntity, World, MemoryArena, Input, AppState,
                             CameraOffset,
                             &AnimationSpeedRate,
                             &AnimationType,
                             &AnimationDirection);

                DoEntityAnimation(ThisEntity, Assets, AppState,
                                  Input->DeltaTime, AnimationSpeedRate,
                                  AnimationType, AnimationDirection);
                DrawEntity(RenderContext, AppState,
                           *TextureProgram,
                           Assets,
                           ThisEntity, CameraOffset);

                break;
            }                    
            case EntityType_Sword:
            {
                
                ThisEntity->TimeLeft -= Input->DeltaTime;                
                if (ThisEntity->TimeLeft > 0.f)
                {
                    float AnimationSpeedRate = 1.f;
                    animation_type AnimationType = AnimationType_Stand;
                    animation_direction AnimationDirection =
                        ThisEntity->AnimationDirection;
    
                    DoEntityAnimation(ThisEntity, Assets, AppState,
                                      Input->DeltaTime, AnimationSpeedRate,
                                      AnimationType, AnimationDirection);
                    DrawEntity(RenderContext, AppState,
                               *TextureProgram,
                               Assets,
                               ThisEntity, CameraOffset);
                }
                break;
            }                    
            case EntityType_Familiar:
            {
                // NOTE(zoubir): player code    
                float AnimationSpeedRate;
                animation_type AnimationType;
                animation_direction AnimationDirection;
    
                UpdateFamiliar(ThisEntity, World, MemoryArena, Input, AppState,
                               &AnimationSpeedRate,
                               &AnimationType,
                               &AnimationDirection);

                DoEntityAnimation(ThisEntity, Assets, AppState,
                                  Input->DeltaTime, AnimationSpeedRate,
                                  AnimationType, AnimationDirection);

                DrawEntity(RenderContext, AppState, *TextureProgram,
                           Assets,
                           ThisEntity, CameraOffset);
                break;
            }                    
            case EntityType_FireBall:
            {
                // NOTE(zoubir): player code    
                float AnimationSpeedRate = ThisEntity->AnimationSpeed;
                animation_type AnimationType = ThisEntity->AnimationType;
                animation_direction AnimationDirection = ThisEntity->AnimationDirection;
    
                UpdateFireBall(ThisEntity, World, MemoryArena, Input, AppState);

                DoEntityAnimation(ThisEntity, Assets, AppState,
                                  Input->DeltaTime, AnimationSpeedRate,
                                  AnimationType, AnimationDirection);
                
                DrawEntity(RenderContext, AppState,
                           *TextureProgram,
                           Assets,
                           ThisEntity, CameraOffset);
                break;
            }                    
            case EntityType_StaticObject:
            {
                                
                DrawEntity(RenderContext, AppState,
                           *TextureProgram,
                           Assets,
                           ThisEntity, CameraOffset);
                break;
            }                    
            case EntityType_Tiled:
            {
                                
                DrawTileEntity(RenderContext, AppState, *TextureProgram,
                               Assets,
                               World, ThisEntity, CameraOffset);
                break;
            }
            case EntityType_Monster:
            {
                //Gravity
                float Gravity = -1000.f;
                float MaxDistance = 10000;
                MoveEntity(ThisEntity, World, MemoryArena, Input, AppState,
                           {0.f, 0.f, Gravity}, &MaxDistance);
                
                float AnimationSpeedRate = 1.f;
                animation_type AnimationType = AnimationType_Stand;
                animation_direction AnimationDirection =
                    AnimationDirection_Down;

                DoEntityAnimation(ThisEntity, Assets, AppState,
                                  Input->DeltaTime, AnimationSpeedRate,
                                  AnimationType, AnimationDirection);
                
                DrawEntity(RenderContext, AppState,
                           *TextureProgram,
                           Assets,
                           ThisEntity, CameraOffset);
                break;
            }
            case EntityType_Count:
            case EntityType_Invalid:
            {
                break;
            }
        }
        }
        }
    }
    RenderFlush(RenderContext);

#endif



//    EndTemporaryMemory(FrameTemporaryMemory);
//    FrameTemporaryMemory = BeginTemporaryMemory(TransientArena);
    
    if (Input->ButtonF3.Pressed)
    {
        AppState->TileEditing = !AppState->TileEditing;
    }
#if 1
    u32 NumberOfElements = 200;
    UIBegin(RenderContext, TransientArena, Input, AppState,
            UIContext, NumberOfElements, 4);
    if (AppState->TileEditing)
    {
        float ContainerWidth = 300;
        float ContainerHeight = 600;
        float ContainerX = Window->Width - ContainerWidth - 60;
        float ContainerY = (Window->Height - ContainerHeight) / 2;
        
        BeginContainer(UIContext, ContainerX, ContainerY,
                       ContainerWidth, ContainerHeight);

        DrawRectangle(RenderContext, ContainerX, ContainerY,
                      ContainerWidth, ContainerHeight,
                      RGBA8_YELLOW, 0.f);

        ui_state *TilePickerWidget = &AppState->TilePickerWidget;
        DoTilePickerWidget(&AppState->TilePickerWidget, AppState,
                           UIContext, 0.f, 0.f, 200.f, 200.f,
                           GetTexture(Assets, OpenGL,
                                      AppState, {AssetType_TileMap}), 32, 32,
                           8, 200);
        float MouseXF = (float)Input->MouseX;
        float MouseYF = (float)Input->MouseY;
        
        float TileWidthF = (float)World->TileWidth;
        float TileHeightF = (float)World->TileHeight;

        float X = (float)((u32)((MouseXF + CameraOffset.X) /
                                TileWidthF));
        float Y = (float)((u32)((MouseYF + CameraOffset.Y) /
                                TileHeightF));
        X *= TileWidthF;
        Y *= TileHeightF;
        X -= CameraOffset.X;
        Y -= CameraOffset.Y;

        loaded_texture *TileMapTexture = AppState->TilePickerWidget.TileMap;
        if (TileMapTexture)
        {
        for(u32 IndexY = 0;
            IndexY < TilePickerWidget->SelectedTileCountY;
            IndexY++)
        {
            for(u32 IndexX = 0;
                IndexX < TilePickerWidget->SelectedTileCountX;
                IndexX++)
            {
                u32 IndexInTexture =
                    AppState->TilePickerWidget.SelectedTileIndices[IndexX]
                    [IndexY];
                u32 NumTilesX = AppState->TilePickerWidget.TileMapNumTilesX;
                u32 NumTilesY = AppState->TilePickerWidget.TileMapNumTilesY;
        
                v4 Uvs = GetTextureUvsFromIndex(TileMapTexture->Width,
                                                TileMapTexture->Height,
                                                NumTilesX, NumTilesY,
                                                IndexInTexture);

                u32 TileOffsetX = IndexX * World->TileWidth;
                u32 TileOffsetY = IndexY * World->TileHeight;
                
                BeginBatch(RenderContext, TileMapTexture->ID, 0.f, *TextureProgram);        
                RenderQuadTexture(RenderContext,
                                  X + IndexX * TileWidthF,
                                  Y + IndexY * TileHeightF,
                                  TileWidthF,
                                  TileHeightF,
                                  Uvs, RGBA8_WHITE, 0.f);
                EndBatch(RenderContext);
                if (Input->LeftButton.EndedDown &&
                    !IsMouseOnRectangle(Input->MouseX, Input->MouseY,
                                        ContainerX, ContainerY,
                                        ContainerWidth,
                                        ContainerHeight))
                {
                    u32 TileXIndex =
                        ((u32)(CameraOffset.X + Input->MouseX + TileOffsetX) /
                         World->TileWidth);
                    u32 TileYIndex =
                        ((u32)(CameraOffset.Y + Input->MouseY + TileOffsetY) /
                         World->TileHeight);
                    TileMap->Tiles[TileXIndex +
                                   (TileYIndex * World->NumTilesX)].Index =
                        IndexInTexture;
                }
            }
        }
        }
        float TilesRectangleWidth =
            (float)(TilePickerWidget->SelectedTileCountX
                    * World->TileWidth);
        float TilesRectangleHeight =
            (float)(TilePickerWidget->SelectedTileCountY *
                    World->TileHeight);
        DrawRectangle(RenderContext, X, Y,
                      TilesRectangleWidth,
                      TilesRectangleHeight,
                      RGBA8_YELLOW, 0.f);
        EndContainer(UIContext);
        
    }
    #if 0
    for(int Number = 0;
        Number < 9;
        Number++)
    {
        if (Input->NumbersButtons[Number].EndedDown)
        {
            ui_state *Button1 = UIContextGetState(UIContext, UI_BUTTON1);
            if (DoButton(Button1, AppState, UIContext, -100 + Number * 50.f, 10.f, 200.f,
                         30.f, "Hjglo"))
            {
        
            }
        }
    }
    if (DoButton(UI_BUTTON2, AppState, UIContext, 400.f, 0.f, 200.f,
                       30.f, "Hjglo"))
    {
        if (DoButton(UI_BUTTON3, AppState, UIContext, 200.f, 200.f,
                           33.f, 33.f, "Helgo"))
        {
            
        }        
    }

    DoEditBox(UI_EDITBOX1, AppState, UIContext, 200.f, 300.f,
              100.f, 33.f);

    DoEditBox(UI_EDITBOX2, AppState, UIContext, 240.f, 300.f,
              100.f, 33.f);
    //TODO(zoubir): remove all local_persist
    //bad for hot loading
    local_persist ui_state EditBox3 = {};
    DoEditBox(&EditBox3, AppState, UIContext, 240.f, 200.f,
              100.f, 33.f);
    
    local_persist ui_state TextLabel = {};
    DoTextLabel(&TextLabel, AppState, UIContext, 240.f, 400.f,
                100.f, 33.f, "Hi Sir", TEXT_JUSTIFICATION_MIDDLE);
    #endif
    
    UIEnd(UIContext);
    
    for(int buttonIndex = 0;
        buttonIndex < ArrayCount(Input->mouseButtons);
        buttonIndex++)
    {
        if (Input->mouseButtons[buttonIndex].EndedDown)
        {
//            RenderPlayer(Buffer, 100 * buttonIndex, 100);
        }
    }

#endif
    #endif
    EndTemporaryMemory(FrameTemporaryMemory);

    
    EvictAssetsAsNecessary(OpenGL, &AppState->Assets);

}

extern "C" APP_GET_SOUND_SAMPLES(AppGetSoundSamples)
{
    app_state *AppState = (app_state *)Memory->PermanentStorage;
    transient_state *TransientState = (transient_state *)Memory->TransientStorage;
    memory_arena *Arena = &TransientState->MemoryArena;

    audio_state *AudioState = &AppState->AudioState;
    
    temporary_memory TempMem = BeginTemporaryMemory(Arena);
    OutputAudio(AudioState, SoundBuffer, Arena);
    EndTemporaryMemory(TempMem);
}

#if 0
int WINAPI WinMain(      
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{  
  return 0;
}
#endif
