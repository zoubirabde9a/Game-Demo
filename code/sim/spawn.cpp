/* Spawners: one Add<Unit> function per entity kind. Each one sets the
   entity type, size, health, sprites and collision rules. To add a new
   unit, add its spawner here and its update logic in sim/. */

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
    Player->MaxHp = 100.f;
    Player->Hp = Player->MaxHp;
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
    if (Caster->Type == EntityType_Player)
    {
        Entity->HasOwner = true;
        Entity->OwnerSlot = Caster->PlayerIndex;
    }
    AddCollisionRule(AppState, Arena, Entity->ID, Caster->ID, false);
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
           world *World, memory_arena *Arena, v3 Position,
           monster_kind Kind)
{
    monster_stats *Stats = GetMonsterStats(Kind);
    bool32 Flies = Stats->FlyHeight > 0.f;
    world_entity *Entity =
        AddEntity(AppState, World, Arena,
                  EntityType_Monster,
                  Position,
                  Flies ? AppState->BatCollision : AppState->PlayerCollision);

    Entity->MonsterKind = Kind;
    Entity->Tint = Stats->Tint;
    Entity->AnimationState = {};
    Entity->ShadowTexture = {AssetType_Shadow};
    Entity->MaxHp = Stats->MaxHp;
    Entity->Hp = Entity->MaxHp;

    if (Flies)
    {
        Entity->Dimensions = {45 * 0.7f, 25 * 0.7f};
        Entity->Texture = {AssetType_Familiar};
        Entity->AnimationSet = &AppState->FamiliarAnimationSet;
    }
    else
    {
        Entity->Dimensions = {48, 48};
        Entity->Texture = {AssetType_Zoubir};
        Entity->AnimationSet = &AppState->ZoubirAnimationSet;
    }

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
    if (Owner->Type == EntityType_Player)
    {
        Entity->HasOwner = true;
        Entity->OwnerSlot = Owner->PlayerIndex;
    }
    AddCollisionRule(AppState, Arena, Entity->ID, Owner->ID, false);
    return Entity;
}

