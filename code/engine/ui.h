#if !defined(UI_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

#define UI_H

#include "ui/theme.h"

struct app_state;
struct font;

// NOTE(zoubir): one font per job, loaded by LoadUIFonts. Any of them is
// 0 when no font file was found, so check before drawing text
struct font_set
{
    font *Small;   // hints, labels over players, small print
    font *Body;    // HUD text, buttons, edit boxes
    font *Title;   // screen headings, the respawn countdown
    font *Strong;  // bold numbers on the HUD: cooldown seconds, health
};

enum ui_element_type
{
    UIE_Invalid,
    UIE_EditBox,
    UIE_Button,
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
        
        // Button
        struct
        {
            char ButtonText[64];
            bool32 IsPressed;
            // NOTE(zoubir): drawn as the current choice in a group
            bool32 IsSelected;
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
