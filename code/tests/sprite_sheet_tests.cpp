/* Sprite sheet tests: every frame a monster's animation table can show
   lands on a cell of its generated sheet that has something drawn in it.
   The sheet is built row 0 first and reaches the GPU bottom up, so the
   cell row an animation names (its FirstIndex / MONSTER_SHEET_COLUMNS)
   is sheet row MonsterRow_Count - 1 - that. When the table read rows the
   other way, standing monsters drew an empty row and vanished.
   Included by sim_tests.cpp, which calls RunSpriteSheetTests. */

// NOTE(zoubir): whether the sheet's cell at Column, Row (row 0 first, as
// BuildMonsterSheet writes it) has any pixel that is not clear
internal bool32
SheetCellHasPixels(u32 *Pixels, u32 Width, u32 FrameSize, u32 Column, u32 Row)
{
    for(u32 Y = 0; Y < FrameSize; Y++)
    {
        u32 *Line = Pixels + (Row * FrameSize + Y) * Width + Column * FrameSize;
        for(u32 X = 0; X < FrameSize; X++)
        {
            if (Line[X] >> 24)
            {
                return true;
            }
        }
    }
    return false;
}

internal void
TestMonsterAnimationsShowDrawnCells()
{
    memory_index Size = Megabytes(8);
    u32 *Pixels = (u32 *)calloc(1, Size);
    test_world Test = CreateTestWorld();
    monster_population *Monsters = CreateMonsterPopulation(&Test.Arena, 0, 1);
    u32 Empty = 0;
    for(u32 Kind = 0; Kind < MonsterKind_Count; Kind++)
    {
        monster_def *Def = GetMonsterDef((monster_kind)Kind);
        u32 Width = MonsterSheetWidth(Def);
        Check(Width * MonsterSheetHeight(Def) * sizeof(u32) <= Size);
        BuildMonsterSheet((monster_kind)Kind, Pixels);
        animation_set *Set = &Monsters->AnimationSets[Kind];
        for(u32 Type = 0; Type < AnimationType_Count; Type++)
        {
            animation_slot *Slot = &Set->Animations[Type][AnimationDirection_Right];
            for(u32 Frame = 0; Frame < Slot->IndicesCount; Frame++)
            {
                u32 Index = Slot->FirstIndex + Frame;
                u32 Column = Index % MONSTER_SHEET_COLUMNS;
                u32 Row = MonsterRow_Count - 1 - Index / MONSTER_SHEET_COLUMNS;
                if (!SheetCellHasPixels(Pixels, Width, Def->FrameSize, Column, Row))
                {
                    if (Empty++ < 5)
                    {
                        printf("  %s: animation %u frame %u shows an empty cell\n",
                               Def->Name, Type, Frame);
                    }
                }
            }
        }
    }
    Check(Empty == 0);
    DestroyTestWorld(&Test);
    free(Pixels);
}

internal void
RunSpriteSheetTests()
{
    printf("TestMonsterAnimationsShowDrawnCells\n");
    TestMonsterAnimationsShowDrawnCells();
}
