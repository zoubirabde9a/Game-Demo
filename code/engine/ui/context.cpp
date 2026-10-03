/* The UI context: widget state slots, the elements queued for drawing
   this frame, containers (a box that offsets and clips what is inside),
   clipping, and the colours and inner boxes all widgets share. */

inline ui_container *
UIContextGetCurrentContainer(ui_context *UIContext)
{
    Assert(UIContext->ContainersCount > 0);
    return &UIContext->AllocatedContainers[UIContext->ContainersCount - 1];
}

inline ui_element *
UIContextInsertElement(ui_context *UIContext)
{
    ui_element *NewElement =
        &UIContext->AllocatedElements[UIContext->ElementsCount++];
    return NewElement;
}

inline ui_element *
UIContextInsertElement(ui_context *UIContext,
                       ui_element_type Type, float X, float Y,
                       v4 DrawRect, ui_state *State,
                       float Width, float Height)
{
    ui_element *Element =
        UIContextInsertElement(UIContext);
    Element->Type = Type;
    Element->X = X;
    Element->Y = Y;
    Element->DrawRect = DrawRect;
    Element->State = State;
    Element->Width = Width;
    Element->Height = Height;
    
    return Element;
}

inline ui_state *
UIContextGetState(ui_context *UIContext, u32 StateIndex)
{
    ui_state *State =
        &UIContext->AllocatedStates[StateIndex];
    return State;
}

internal ui_context *
UIContextCreate(memory_arena *Arena, u32 NumberOfStates)
{
    //TODO(zoubir): is the memory initialized at zero ?
    ui_context *UIContext = AllocateStruct(Arena, ui_context);
        
    UIContext->AllocatedStates =
        AllocateArray(Arena, NumberOfStates, ui_state);
    UIContext->AllocatedStatesCount = NumberOfStates;    

    return UIContext;
}

inline void
BeginContainer(ui_context *UIContext, float X, float Y,
               float Width, float Height)
{
    Assert(UIContext->AllocatedContainersCount > UIContext->ContainersCount);
    ui_container *NewContainer =
        &UIContext->AllocatedContainers[UIContext->ContainersCount++];
    NewContainer->X = X;
    NewContainer->Y = Y;
    NewContainer->Width = Width;
    NewContainer->Height = Height;
    
    UIContext->ContainerOffset.X += X;
    UIContext->ContainerOffset.Y += Y;
}
inline void
EndContainer(ui_context *UIContext)
{
    Assert(UIContext->ContainersCount > 0);
    ui_container *LastContainer =
        &UIContext->AllocatedContainers[--UIContext->ContainersCount];
    UIContext->ContainerOffset.X -= LastContainer->X;
    UIContext->ContainerOffset.Y -= LastContainer->Y;
}

inline v4
ClipRectangle1(v2 Pos0, v2 Pos1, v2 Clip0, v2 Clip1)
{

    v4 Result;
    
    if (Pos0.X < Clip0.X)
    {
        Result.X = Clip0.X;
    }
    else
    {
        Result.X = Pos0.X;
    }
    
    if (Pos0.Y < Clip0.Y)
    {
        Result.Y = Clip0.Y;
    }
    else
    {
        Result.Y = Pos0.Y;
    }

    if (Pos1.X > Clip1.X)
    {
        Result.Z = Clip1.X - Result.X;
    }
    else
    {
        Result.Z = Pos1.X - Pos0.X;
    }
    
    if (Pos1.Y > Clip1.Y)
    {
        Result.W = Clip1.Y - Result.Y;
    }
    else
    {
        Result.W = Pos1.Y - Pos0.Y;
    }

    return Result;
}

#define RGBA8_UI_FILL (0xFF303030)
#define RGBA8_UI_FILL_HOT (0xFF505050)
#define RGBA8_UI_FIELD (0xFF181818)
#define RGBA8_UI_BORDER (0xFF808080)
#define RGBA8_UI_FOCUS (0xFF30C8F0)

// NOTE(zoubir): the text area inside a widget, and its clip rectangle
inline v4
UIInnerRect(ui_element *Element, v4 Padding)
{
    v4 Result = V4(Element->X + Padding.X, Element->Y + Padding.Z,
                   Element->Width - Padding.X - Padding.Y,
                   Element->Height - Padding.Z - Padding.W);
    return Result;
}

inline v4
UIInnerClip(ui_element *Element, v4 Padding)
{
    v4 Result = V4(Element->DrawRect.X + Padding.X,
                   Element->DrawRect.Y + Padding.Z,
                   Element->DrawRect.Z - Padding.X - Padding.Y,
                   Element->DrawRect.W - Padding.Z - Padding.W);
    return Result;
}
