/* Immediate-mode widgets. Each frame: UIBegin, then Do* calls (buttons,
   edit boxes, the tile picker) that handle input at once and
   queue an element, then UIEnd, which draws every queued element and
   flushes the pass. */
#include "ui.h"

#include "ui/context.cpp"
#include "ui/fonts.cpp"
#include "ui/text.cpp"
#include "ui/panel.cpp"

internal void
UIBegin(render_context *RenderContext,
        memory_arena *TransientArena,
        app_input *Input, app_state *AppState,
        ui_context *UIContext,
        u32 NumberOfElements, u32 NumberOfContainers)
{
    Assert(UIContext->ContainersCount == 0);
    Assert(UIContext->ElementsCount == 0);
    Assert(UIContext->StatesCount == 0);
    
    UIContext->RenderContext = RenderContext;
    UIContext->Input = Input;
    UIContext->AppState = AppState;
    
    u32 NumberOfVertices = NumberOfElements * 6;
    SetupBatchRenderer(RenderContext, TransientArena, NumberOfElements);
    RenderBegin(RenderContext, NumberOfVertices, RENDER_ORDER_NO_ORDER);
    
    if (NumberOfElements > UIContext->AllocatedElementsCount)
    {
        UIContext->AllocatedElements =
            AllocateArray(TransientArena, NumberOfElements,
                                          ui_element);
        UIContext->AllocatedElementsCount = NumberOfElements;
    }
    if (NumberOfContainers > UIContext->AllocatedContainersCount)
    {
        UIContext->AllocatedContainers =
            AllocateArray(TransientArena, NumberOfContainers,
                                   ui_container);
        UIContext->AllocatedContainersCount = NumberOfContainers;
    }

    UIContext->ContainerOffset = V2(0.f);
    UIContext->HighlightedElement = 0;
}

#include "ui/button.cpp"
#include "ui/edit_box.cpp"
#include "ui/tile_picker.cpp"

inline void
UIEnd(ui_context *UIContext)
{
    render_context *RenderContext = UIContext->RenderContext;
    
    ui_element *Elements = UIContext->AllocatedElements;
    for(u32 ElementIndex = 0;
        ElementIndex < UIContext->ElementsCount;
        ElementIndex++)
    {
        ui_element *Element = &Elements[ElementIndex];
        switch(Element->Type)
        {
            case UIE_Button:
            {
                DrawButton(Element, UIContext);
                break;
            }
            case UIE_EditBox:
            {
                DrawEditBox(Element, UIContext);
                break;
            }
            case UIE_TilePickerWidget:
            {
                DrawTilePickerWidget(Element, UIContext);
                break;
            }
            default:
            {
                InvalidCodePath;
                break;
            }
        }
    }
        
    RenderFlush(RenderContext);
    UIContext->ElementsCount = 0;
    UIContext->ContainersCount = 0;
}
