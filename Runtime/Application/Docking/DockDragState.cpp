#include "Application/Docking/DockDragState.h"
#include "Application/Docking/DockingArea.h"
#include "Core/Containers/UniquePtr.h"

TUniquePtr<FDockDragState> FDockDragState::DockDragState = nullptr;

FDockDragState& FDockDragState::Get()
{
    if (!DockDragState)
    {
        DockDragState = MakeUniquePtr<FDockDragState>();
    }

    return *DockDragState;
}

bool FDockDragState::IsInitialized()
{
    return DockDragState.IsValid();
}

void FDockDragState::Shutdown()
{
    if (DockDragState)
    {
        DockDragState->CancelDrag();
        DockDragState.Reset();
    }
}

FDockDragState::FDockDragState()
    : DraggedPanelId()
    , SourceArea(nullptr)
    , TargetArea(nullptr)
    , TargetPanelId()
    , TargetDirection(EDockDirection::Center)
    , CursorPosition()
    , RegisteredAreas()
{
}

FDockDragState::~FDockDragState() = default;

void FDockDragState::BeginDrag(const String& PanelId, FDockingArea* InSourceArea, const IntVector2& ScreenPosition)
{
    if (PanelId.IsEmpty())
    {
        return;
    }

    DraggedPanelId = PanelId;
    SourceArea     = InSourceArea;
    CursorPosition = ScreenPosition;

    UpdateDrag(ScreenPosition);
}

void FDockDragState::UpdateDrag(const IntVector2& ScreenPosition)
{
    CursorPosition = ScreenPosition;

    if (!IsDragging())
    {
        return;
    }

    ClearTarget();

    for (FDockingArea* Area : RegisteredAreas)
    {
        String PanelId;
        
        EDockDirection Direction = EDockDirection::Center;
        if (Area->HitTestDropTarget(ScreenPosition, PanelId, Direction))
        {
            TargetArea      = Area;
            TargetPanelId   = PanelId;
            TargetDirection = Direction;
            return;
        }
    }
}

void FDockDragState::EndDrag()
{
    if (!IsDragging())
    {
        return;
    }

    const String         PanelId   = DraggedPanelId;
    FDockingArea* const  Area      = TargetArea;
    FDockingArea* const  Origin    = SourceArea;
    const String         TargetId  = TargetPanelId;
    const EDockDirection Direction = TargetDirection;

    DraggedPanelId.Clear();

    SourceArea = nullptr;

    ClearTarget();

    if (!Area)
    {
        return;
    }

    if (Origin && Origin != Area)
    {
        String                     Label;
        TSharedPtr<FVisualElement> Panel;

        if (Origin->GetPanelRegistration(PanelId, Label, Panel))
        {
            Area->RegisterPanel(PanelId, Label, Panel);
        }

        Origin->UnregisterPanel(PanelId);
    }

    Area->DockPanel(PanelId, TargetId, Direction);
}

void FDockDragState::CancelDrag()
{
    DraggedPanelId.Clear();

    SourceArea = nullptr;

    ClearTarget();
}

void FDockDragState::RegisterArea(FDockingArea* Area)
{
    if (Area && !RegisteredAreas.Contains(Area))
    {
        RegisteredAreas.Add(Area);
    }
}

void FDockDragState::UnregisterArea(FDockingArea* Area)
{
    RegisteredAreas.Remove(Area);

    if (SourceArea == Area)
    {
        SourceArea = nullptr;
    }

    if (TargetArea == Area)
    {
        ClearTarget();
    }
}

void FDockDragState::ClearTarget()
{
    TargetArea      = nullptr;
    TargetDirection = EDockDirection::Center;

    TargetPanelId.Clear();
}
