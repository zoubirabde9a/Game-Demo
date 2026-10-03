#if !defined(UI_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

#define UI_H

struct app_state;
struct font;

enum ui_element_type
{
    UIE_Invalid,
    UIE_EditBox,
    UIE_Button,
    UIE_TextLabel,
    UIE_TilePickerWidget,
    UIE_Count
};

struct ui_container
{
    float X, Y, Width, Height;    
};

struct ui_state
{
    font *Font;
    char Text[128];
    u32 TextCount;
    union
    {
        // EditBox
        struct
        {
            v4 Padding;
        };
        
        // TextLabel
        struct
        {
            u32 TextJustification;
        };

        // Button
        struct
        {
            char ButtonText[64];
            bool32 IsPressed;
        };
        
        // TilePickerWidget
        struct
        {
            loaded_texture *TileMap;
            u32 TileMapNumTilesX;
            u32 TileMapNumTilesY;
            
            u32 NumTilesX;
            u32 NumTilesY;
            u32 TileWidth;
            u32 TileHeight;

            u32 OffsetX;
            u32 OffsetY;
            
            u32 SelectedTileIndices[12][12];
            u32 SelectedTileCountX;
            u32 SelectedTileCountY;
            v4 SelectedRectangle;

            bool32 MousePressedInside;
            v2 RelativeClickPosition;
        };
    };
};

struct ui_element
{
    int Type;
    float X, Y;
    v4 DrawRect;
    ui_state *State;
    
    float Width, Height;
    
};
struct ui_element_cursor
{
    u32 Pos;
    float Offset;
};

struct ui_context
{
    render_context *RenderContext;
    app_input *Input;
    app_state *AppState;
        
    ui_container *AllocatedContainers;
    u32 AllocatedContainersCount;
    u32 ContainersCount;

    v2 ContainerOffset;
    
    ui_state *AllocatedStates;
    u32 AllocatedStatesCount;
    u32 StatesCount;
    
    ui_element *AllocatedElements;
    u32 AllocatedElementsCount;
    u32 ElementsCount;

    ui_element *HighlightedElement;
    ui_state *SelectedState;
    
    ui_element_cursor ElementCursor;
    u32 ElementCursorCharOffset;
}; 

#endif
