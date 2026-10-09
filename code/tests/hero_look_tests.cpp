/* Hero look tests: the new hero look's frame picker
   (client/heroes/hero_looks.cpp) sends every animation, facing, swing,
   spell, dash and hit to the cell the sheet layout names, left mirrored
   from right; and every class's sheet in web/heroes_v2 is a 512-pixel
   sheet with something drawn in every cell the picker can show.
   Included by sim_tests.cpp, which calls RunHeroLookTests. */

internal bool32
SameHeroCell(hero_look_cell A, u32 Row, u32 Column, bool32 Mirrored)
{
    bool32 Result = A.Row == Row && A.Column == Column && (A.Mirrored != 0) == (Mirrored != 0);
    return Result;
}

internal hero_look_cell
HeroCellAt(u32 Row, u32 Column)
{
    hero_look_cell Result = {};
    Result.Row = Row;
    Result.Column = Column;
    return Result;
}

internal hero_look_cell
PickHeroCell(animation_type Type, animation_direction Facing, u32 Slot,
             u32 Swing = PlayerSwing_Cut, u32 Spell = PlayerSpell_None,
             bool32 Dashing = false, bool32 Hurt = false)
{
    hero_look_input In = {};
    In.Type = Type;
    In.Facing = Facing;
    In.SlotIndex = Slot;
    In.Swing = Swing;
    In.Spell = Spell;
    In.Dashing = Dashing;
    In.Hurt = Hurt;
    return HeroLookCell(&In);
}

internal void
TestHeroLookCellsFollowTheLayout()
{
    // NOTE(zoubir): a few cells named by hand from the layout
    Check(SameHeroCell(PickHeroCell(AnimationType_Stand, AnimationDirection_Down, 0), 0, 0, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_Stand, AnimationDirection_Down, 5), 0, 1, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_Move, AnimationDirection_Right, 2), 5, 10, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_Move, AnimationDirection_Left, 2), 5, 10, true));
    Check(SameHeroCell(PickHeroCell(AnimationType_Stop, AnimationDirection_Up, 3), 11, 1, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_Attack, AnimationDirection_Down, 1), 1, 9, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_Attack, AnimationDirection_Down, 1,
                                    PlayerSwing_Backhand), 2, 1, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_Attack, AnimationDirection_Up, 5,
                                    PlayerSwing_Heavy), 12, 13, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_Attack, AnimationDirection_Right, 0,
                                    PlayerSwing_Thrust), 8, 0, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_Cast, AnimationDirection_Down, 1), 3, 9, false));
    // NOTE(zoubir): a long raise keeps its last two frames going; the
    // others hold their last
    Check(SameHeroCell(PickHeroCell(AnimationType_Cast, AnimationDirection_Down, 7,
                                    PlayerSwing_Cut, PlayerSpell_Meteor), 4, 3, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_Cast, AnimationDirection_Down, 8,
                                    PlayerSwing_Cut, PlayerSpell_Meteor), 4, 2, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_Cast, AnimationDirection_Right, 9,
                                    PlayerSwing_Cut, PlayerSpell_Shockwave), 9, 11, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_JumpUp, AnimationDirection_Down, 0), 15, 0, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_JumpDown, AnimationDirection_Right, 0), 15, 6, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_Move, AnimationDirection_Up, 0, PlayerSwing_Cut,
                                    PlayerSpell_None, true), 15, 12, false));
    Check(SameHeroCell(PickHeroCell(AnimationType_Stand, AnimationDirection_Left, 0, PlayerSwing_Cut,
                                    PlayerSpell_None, false, true), 15, 8, true));
    // NOTE(zoubir): a swing or a cast keeps its own frames through a dash
    // or a hit
    Check(SameHeroCell(PickHeroCell(AnimationType_Attack, AnimationDirection_Down, 0, PlayerSwing_Cut,
                                    PlayerSpell_None, true, true), 1, 8, false));

    // NOTE(zoubir): everything lands on the sheet, left is right mirrored
    for(u32 Type = 0; Type < AnimationType_Count; Type++)
    {
        for(u32 Slot = 0; Slot < 24; Slot++)
        {
            for(u32 Swing = 0; Swing < PlayerSwing_Count; Swing++)
            {
                for(u32 Spell = 0; Spell < PlayerSpell_Count; Spell++)
                {
                    for(u32 Flags = 0; Flags < 4; Flags++)
                    {
                        hero_look_cell Right =
                            PickHeroCell((animation_type)Type, AnimationDirection_Right, Slot,
                                         Swing, Spell, Flags & 1, Flags & 2);
                        hero_look_cell Left =
                            PickHeroCell((animation_type)Type, AnimationDirection_Left, Slot,
                                         Swing, Spell, Flags & 1, Flags & 2);
                        Check(Right.Row < HERO_LOOK_CELLS && Right.Column < HERO_LOOK_CELLS);
                        Check(!Right.Mirrored && Left.Mirrored);
                        Check(Left.Row == Right.Row && Left.Column == Right.Column);
                        for(u32 Facing = 0; Facing < AnimationDirection_Count; Facing++)
                        {
                            hero_look_cell Cell =
                                PickHeroCell((animation_type)Type, (animation_direction)Facing, Slot,
                                             Swing, Spell, Flags & 1, Flags & 2);
                            Check(Cell.Row < HERO_LOOK_CELLS && Cell.Column < HERO_LOOK_CELLS);
                        }
                    }
                }
            }
        }
    }

    // NOTE(zoubir): the top-left cell is index row 15 on the GPU
    v4 Uvs = HeroLookCellUvs(HeroCellAt(0, 0), 512, 512);
    v4 Expected = GetTextureUvsFromIndex(512, 512, 16, 16, 15 * 16);
    Check(Uvs.X == Expected.X && Uvs.Y == Expected.Y && Uvs.Z == Expected.Z && Uvs.W == Expected.W);
    hero_look_cell Mirrored = HeroCellAt(15, 3);
    Mirrored.Mirrored = true;
    Uvs = HeroLookCellUvs(Mirrored, 512, 512);
    Expected = GetTextureUvsFromIndex(512, 512, 16, 16, 3);
    Check(Uvs.X == Expected.Z && Uvs.Z == Expected.X && Uvs.Y == Expected.Y);
}

// NOTE(zoubir): every cell the picker can show, as a 16 by 16 map
internal void
MarkPickableHeroCells(bool32 *Pickable)
{
    for(u32 Type = 0; Type < AnimationType_Count; Type++)
    {
        for(u32 Facing = 0; Facing < AnimationDirection_Count; Facing++)
        {
            for(u32 Slot = 0; Slot < 24; Slot++)
            {
                for(u32 Swing = 0; Swing < PlayerSwing_Count; Swing++)
                {
                    for(u32 Spell = 0; Spell < PlayerSpell_Count; Spell++)
                    {
                        for(u32 Flags = 0; Flags < 4; Flags++)
                        {
                            hero_look_cell Cell =
                                PickHeroCell((animation_type)Type, (animation_direction)Facing,
                                             Slot, Swing, Spell, Flags & 1, Flags & 2);
                            Pickable[Cell.Row * HERO_LOOK_CELLS + Cell.Column] = true;
                        }
                    }
                }
            }
        }
    }
}

// NOTE(zoubir): the tests run from build\, the sheets are in web\heroes_v2
internal void
TestHeroLookSheetsHaveEveryFrame()
{
    static bool32 Pickable[HERO_LOOK_CELLS * HERO_LOOK_CELLS];
    MarkPickableHeroCells(Pickable);
    u32 Missing = 0;
    u32 Empty = 0;
    for(u32 Role = 0; Role < PlayerRole_Count; Role++)
    {
        char Path[128];
        snprintf(Path, sizeof(Path), "../web/heroes_v2/%s.png", HeroSkinFileNames[Role]);
        FILE *Handle = fopen(Path, "rb");
        u8 *File = 0;
        long Size = 0;
        if (Handle)
        {
            fseek(Handle, 0, SEEK_END);
            Size = ftell(Handle);
            fseek(Handle, 0, SEEK_SET);
            File = (u8 *)malloc((size_t)Size);
            Size = (long)fread(File, 1, (size_t)Size, Handle);
            fclose(Handle);
        }
        int Width = 0, Height = 0, Channels = 0;
        u8 *Pixels = File ? stbi_load_from_memory(File, (int)Size, &Width, &Height, &Channels, 4) : 0;
        if (!Pixels || Width != HERO_SHEET_PIXELS || Height != HERO_SHEET_PIXELS)
        {
            printf("  %s: no 512-pixel sheet\n", Path);
            ++Missing;
        }
        else
        {
            u32 Cell = HERO_SHEET_PIXELS / HERO_LOOK_CELLS;
            for(u32 Index = 0; Index < HERO_LOOK_CELLS * HERO_LOOK_CELLS; Index++)
            {
                if (!Pickable[Index])
                {
                    continue;
                }
                u32 Row = Index / HERO_LOOK_CELLS;
                u32 Column = Index % HERO_LOOK_CELLS;
                if (!SheetCellHasPixels((u32 *)Pixels, HERO_SHEET_PIXELS, Cell, Column, Row))
                {
                    if (Empty++ < 5)
                    {
                        printf("  %s: cell row %u column %u is empty\n", Path, Row, Column);
                    }
                }
            }
        }
        stbi_image_free(Pixels);
        free(File);
    }
    Check(Missing == 0);
    Check(Empty == 0);
}

// NOTE(zoubir): both looks' sheets, with every other texture and sound
// drawn by code, fit the slots the assets keep for them
// (ReserveGeneratedAssets asserts it, but only once the game starts)
internal void
TestGeneratedAssetsFit()
{
    u32 Used = MonsterKind_Count + ShotStyle_Count + 1 + TerrainProp_Count + HazardStyle_Count +
        ArrayCount(SoundEffects) + 2 * PlayerRole_Count;
    printf("  generated assets: %u of %u\n", Used, (u32)MAX_GENERATED_ASSETS);
    Check(Used <= MAX_GENERATED_ASSETS);
}

internal void
RunHeroLookTests()
{
    printf("TestGeneratedAssetsFit\n");
    TestGeneratedAssetsFit();
    printf("TestHeroLookCellsFollowTheLayout\n");
    TestHeroLookCellsFollowTheLayout();
    printf("TestHeroLookSheetsHaveEveryFrame\n");
    TestHeroLookSheetsHaveEveryFrame();
}
