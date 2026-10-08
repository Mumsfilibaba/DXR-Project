#include "Application/Application.h"
#include "Application/Docking/DockDragState.h"
#include "Application/Docking/DockingArea.h"
#include "Application/Elements/Window.h"
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
    , DraggedPanelLabel()
    , DraggedPanelContent(nullptr)
    , SourceArea(nullptr)
    , SourceScreenBounds()
    , TargetArea(nullptr)
    , TargetPanelId()
    , TargetDirection(EDockDirection::Center)
    , TargetTabIndex(-1)
    , CursorPosition()
    , CursorWindow(nullptr)
    , RegisteredAreas()
    , OnDragBeganDelegate()
    , OnDropOutsideDelegate()
    , OnDragEndedDelegate()
{
}

FDockDragState::~FDockDragState() = default;

void FDockDragState::BeginDrag(const String& PanelId, FDockingArea* InSourceArea, const IntVector2& ScreenPosition)
{
    if (PanelId.IsEmpty())
    {
        return;
    }

    DraggedPanelId      = PanelId;
    DraggedPanelLabel   = PanelId;
    DraggedPanelContent = nullptr;
    SourceArea          = InSourceArea;
    SourceScreenBounds  = FRectangle();
    CursorPosition      = ScreenPosition;

    if (InSourceArea)
    {
        SourceScreenBounds = InSourceArea->GetPanelScreenBounds(PanelId);

        String                     Label;
        TSharedPtr<FVisualElement> Panel;

        if (InSourceArea->GetPanelRegistration(PanelId, Label, Panel))
        {
            DraggedPanelLabel   = Label.IsEmpty() ? PanelId : Label;
            DraggedPanelContent = Panel;
        }

        InSourceArea->UnregisterPanel(PanelId);
    }

    OnDragBeganDelegate.ExecuteIfBound(GetDraggedPanel(), ScreenPosition);

    UpdateDrag(ScreenPosition);
}

void FDockDragState::InvalidateDropZones() const
{
    for (FDockingArea* const Area : RegisteredAreas)
    {
        if (Area)
        {
            Area->UpdateDropPreview();
            Area->InvalidatePaint();
        }
    }
}

void FDockDragState::UpdateDrag(const IntVector2& ScreenPosition)
{
    CursorPosition = ScreenPosition;

    CursorWindow = FApplication::IsInitialized() ? FApplication::Get().FindWindowUnderCursor().Get() : nullptr;

    if (!IsDragging())
    {
        InvalidateDropZones();
        return;
    }

    ClearTarget();

    for (int32 Index = RegisteredAreas.Size() - 1; Index >= 0; --Index)
    {
        FDockingArea* const Area = RegisteredAreas[Index];
        if (!Area->IsInCursorWindow())
        {
            continue;
        }

        String PanelId;
        
        EDockDirection Direction = EDockDirection::Center;
        int32          TabIndex  = -1;

        if (Area->HitTestDropTarget(ScreenPosition, PanelId, Direction, TabIndex))
        {
            TargetArea      = Area;
            TargetPanelId   = PanelId;
            TargetDirection = Direction;
            TargetTabIndex  = TabIndex;
            break;
        }
    }

    InvalidateDropZones();
}

void FDockDragState::EndDrag()
{
    if (!IsDragging())
    {
        return;
    }

    const FDockDragPanel Panel        = GetDraggedPanel();
    FDockingArea* const  Area         = TargetArea;
    const String         TargetId     = TargetPanelId;
    const EDockDirection Direction    = TargetDirection;
    const int32          TabIndex     = TargetTabIndex;
    const IntVector2     DropPosition = CursorPosition;

    DraggedPanelId.Clear();
    DraggedPanelLabel.Clear();
    
    DraggedPanelContent = nullptr;
    SourceArea          = nullptr;

    ClearTarget();
    InvalidateDropZones();

    if (Area)
    {
        Area->RegisterPanel(Panel.PanelId, Panel.Label, Panel.Content);
        Area->DockPanel(Panel.PanelId, TargetId, Direction, TabIndex);
        Area->SettleDrop(Panel.PanelId);
    }
    else
    {
        OnDropOutsideDelegate.ExecuteIfBound(Panel, DropPosition);
    }

    OnDragEndedDelegate.ExecuteIfBound();
}

void FDockDragState::CancelDrag()
{
    const bool bWasDragging = IsDragging();
    if (bWasDragging && SourceArea)
    {
        SourceArea->RegisterPanel(DraggedPanelId, DraggedPanelLabel, DraggedPanelContent);
        SourceArea->DockPanel(DraggedPanelId, String(), EDockDirection::Center);
    }

    DraggedPanelId.Clear();
    DraggedPanelLabel.Clear();

    DraggedPanelContent = nullptr;
    SourceArea          = nullptr;

    ClearTarget();
    InvalidateDropZones();

    if (bWasDragging)
    {
        OnDragEndedDelegate.ExecuteIfBound();
    }
}

FDockDragPanel FDockDragState::GetDraggedPanel() const
{
    FDockDragPanel Panel;
    Panel.PanelId            = DraggedPanelId;
    Panel.Label              = DraggedPanelLabel;
    Panel.Content            = DraggedPanelContent;
    Panel.SourceArea         = SourceArea;
    Panel.SourceScreenBounds = SourceScreenBounds;

    return Panel;
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

void FDockDragState::SetOnDragBegan(const FOnDockDragBegan& InOnDragBegan)
{
    OnDragBeganDelegate = InOnDragBegan;
}

void FDockDragState::SetOnDropOutside(const FOnDockDropOutside& InOnDropOutside)
{
    OnDropOutsideDelegate = InOnDropOutside;
}

void FDockDragState::SetOnDragEnded(const FOnDockDragEnded& InOnDragEnded)
{
    OnDragEndedDelegate = InOnDragEnded;
}

void FDockDragState::ClearTarget()
{
    TargetArea      = nullptr;
    TargetDirection = EDockDirection::Center;
    TargetTabIndex  = -1;

    TargetPanelId.Clear();
}
