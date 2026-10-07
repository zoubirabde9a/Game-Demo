/* Monster rendering that only the client needs: turning each kind's draw
   function into a sprite-sheet texture at startup. What is drawn over
   the monsters each frame is in monster_telegraphs.cpp. */

inline u32
MonsterSheetWidth(monster_def *Def)
{
    return Def->FrameSize * MONSTER_SHEET_COLUMNS;
}

inline u32
MonsterSheetHeight(monster_def *Def)
{
    return Def->FrameSize * MonsterRow_Count;
}

// NOTE(zoubir): Pixels holds MonsterSheetWidth * MonsterSheetHeight u32s
internal void
BuildMonsterSheet(monster_kind Kind, u32 *Pixels)
{
    monster_def *Def = GetMonsterDef(Kind);
    u32 Width = MonsterSheetWidth(Def);
    u32 Height = MonsterSheetHeight(Def);
    ZeroSize(Pixels, Width * Height * sizeof(u32));

    animation_type RowTypes[MonsterRow_Count] =
        {
            AnimationType_Stand,
            AnimationType_Move,
            AnimationType_Cast,
            AnimationType_Attack,
            AnimationType_Stop,
            AnimationType_JumpDown,
        };
    for(u32 Row = 0; Row < MonsterRow_Count; Row++)
    {
        u32 FrameCount = Def->FrameCounts[Row];
        Assert(FrameCount <= MONSTER_SHEET_COLUMNS);
        for(u32 Frame = 0; Frame < FrameCount; Frame++)
        {
            sprite_canvas Canvas = CanvasFrame(Pixels, Width, Def->FrameSize,
                                               Frame, Row);
            monster_pose Pose = {};
            Pose.Anim = RowTypes[Row];
            Pose.Frame = Frame;
            Pose.FrameCount = FrameCount;
            Pose.t = (float)Frame / (float)FrameCount;
            // NOTE(zoubir): one-shot rows (windup, attack) should reach
            // their last pose on the last frame instead of looping back
            if (Row == MonsterRow_Windup || Row == MonsterRow_Attack)
            {
                Pose.t = FrameCount > 1 ?
                    (float)Frame / (float)(FrameCount - 1) : 1.f;
            }
            Pose.Wave = Sin(2.f * Pi32 * Pose.t);
            Pose.Wave2 = Cos(2.f * Pi32 * Pose.t);
            MonsterDrawFunctions[Kind](&Canvas, Pose);
        }
    }
}

// NOTE(zoubir): call once after InitializeAssets
internal void
AddMonsterTextures(assets *Assets, open_gl *OpenGL, memory_arena *TempArena)
{
    ReserveGeneratedAssets(Assets, AssetType_Monster, MonsterKind_Count);
    for(u32 KindIndex = 0; KindIndex < MonsterKind_Count; KindIndex++)
    {
        monster_def *Def = GetMonsterDef((monster_kind)KindIndex);
        u32 Width = MonsterSheetWidth(Def);
        u32 Height = MonsterSheetHeight(Def);
        temporary_memory Temp = BeginTemporaryMemory(TempArena);
        u32 *Pixels = AllocateArray(TempArena, Width * Height, u32);
        BuildMonsterSheet((monster_kind)KindIndex, Pixels);
        // NOTE(zoubir): feet sit on the 7/8 line like the player sprite
        AddGeneratedTexture(Assets, OpenGL, {AssetType_Monster, KindIndex},
                            Pixels, Width, Height,
                            MONSTER_SHEET_COLUMNS, MonsterRow_Count,
                            V2(0.5f, 0.875f));
        EndTemporaryMemory(Temp);
    }

    ReserveGeneratedAssets(Assets, AssetType_MonsterShot, ShotStyle_Count);
    for(u32 Style = 0; Style < ShotStyle_Count; Style++)
    {
        u32 Width = SHOT_FRAME_SIZE * SHOT_FRAMES;
        temporary_memory Temp = BeginTemporaryMemory(TempArena);
        u32 *Pixels = AllocateArray(TempArena, Width * SHOT_FRAME_SIZE, u32);
        BuildShotSheet((monster_shot_style)Style, Pixels);
        AddGeneratedTexture(Assets, OpenGL, {AssetType_MonsterShot, Style},
                            Pixels, Width, SHOT_FRAME_SIZE,
                            SHOT_FRAMES, 1, V2(0.5f, 0.5f));
        EndTemporaryMemory(Temp);
    }

    // NOTE(zoubir): the ground tiles, one row per terrain kind
    {
        ReserveGeneratedAssets(Assets, AssetType_TerrainAtlas, 1);
        u32 Width = TERRAIN_ATLAS_COLUMNS * TERRAIN_TILE_PIXELS;
        u32 Height = TerrainKind_Count * TERRAIN_TILE_PIXELS;
        temporary_memory Temp = BeginTemporaryMemory(TempArena);
        u32 *Pixels = AllocateArray(TempArena, Width * Height, u32);
        BuildTerrainAtlas(Pixels);
        AddGeneratedTexture(Assets, OpenGL, {AssetType_TerrainAtlas, 0},
                            Pixels, Width, Height, TERRAIN_ATLAS_COLUMNS,
                            TerrainKind_Count, V2(0.f, 0.f));
        EndTemporaryMemory(Temp);
    }

    // NOTE(zoubir): boulders and dead trees; slot 0 (no prop) and the tree
    // slot stay empty, trees use the packed tree texture
    ReserveGeneratedAssets(Assets, AssetType_TerrainProp, TerrainProp_Count);
    for(u32 Prop = 0; Prop < TerrainProp_Count; Prop++)
    {
        u32 Size = TERRAIN_PROP_PIXELS;
        temporary_memory Temp = BeginTemporaryMemory(TempArena);
        u32 *Pixels = AllocateArray(TempArena, Size * Size, u32);
        ZeroSize(Pixels, Size * Size * sizeof(u32));
        sprite_canvas Canvas = CanvasFrame(Pixels, Size, Size, 0, 0);
        DrawTerrainProp(&Canvas, (terrain_prop)Prop);
        AddGeneratedTexture(Assets, OpenGL, {AssetType_TerrainProp, Prop},
                            Pixels, Size, Size, 1, 1, V2(0.5f, 0.875f));
        EndTemporaryMemory(Temp);
    }

    ReserveGeneratedAssets(Assets, AssetType_MonsterHazard, HazardStyle_Count);
    for(u32 Style = 0; Style < HazardStyle_Count; Style++)
    {
        u32 Width = HAZARD_FRAME_SIZE * HAZARD_FRAMES;
        temporary_memory Temp = BeginTemporaryMemory(TempArena);
        u32 *Pixels = AllocateArray(TempArena, Width * HAZARD_FRAME_SIZE, u32);
        BuildHazardSheet((monster_hazard_style)Style, Pixels);
        AddGeneratedTexture(Assets, OpenGL, {AssetType_MonsterHazard, Style},
                            Pixels, Width, HAZARD_FRAME_SIZE,
                            HAZARD_FRAMES, 1, V2(0.5f, 0.5f));
        EndTemporaryMemory(Temp);
    }
}
