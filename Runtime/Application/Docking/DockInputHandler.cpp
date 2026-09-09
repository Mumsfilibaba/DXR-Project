#include "Application/Application.h"
#include "Application/Docking/DockDragState.h"
#include "Application/Docking/DockInputHandler.h"
#include "Application/Docking/DockWindowManager.h"
#include "Application/Input/Keys.h"

TSharedPtr<FDockInputHandler> FDockInputHandler::Register()
{
    TSharedPtr<FDockInputHandler> InputHandler = MakeSharedPtr<FDockInputHandler>();
    if (FApplication::IsInitialized())
    {
        FApplication::Get().RegisterInputHandler(InputHandler);
    }

    return InputHandler;
}

void FDockInputHandler::Unregister(const TSharedPtr<FDockInputHandler>& InputHandler)
{
    if (InputHandler && FApplication::IsInitialized())
    {
        FApplication::Get().UnregisterInputHandler(InputHandler);
    }

    if (FDockDragState::IsInitialized())
    {
        FDockDragState::Get().CancelDrag();
    }
}

bool FDockInputHandler::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (KeyEvent.GetKey() != Keys::Escape || !FDockDragState::Get().IsDragging())
    {
        return false;
    }

    FinishDrag();
    return true;
}

bool FDockInputHandler::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (!FDockDragState::Get().IsDragging())
    {
        return false;
    }

    UpdateDrag(CursorEvent.GetScreenPosition());
    return true;
}

bool FDockInputHandler::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (!FDockDragState::Get().IsDragging())
    {
        return false;
    }

    UpdateDrag(CursorEvent.GetScreenPosition());
    FinishDrag();
    return true;
}

void FDockInputHandler::UpdateDrag(const IntVector2& ScreenPosition)
{
    FDockDragState& DragState = FDockDragState::Get();
    DragState.UpdateDrag(ScreenPosition);

    if (FDockWindowManager::IsInitialized())
    {
        FDockWindowManager& Manager = FDockWindowManager::Get();
        Manager.MoveDecorator(ScreenPosition);
        Manager.UpdateDropPreview();
    }
}

void FDockInputHandler::FinishDrag()
{
    FDockDragState::Get().EndDrag();

    if (FApplication::IsInitialized())
    {
        FApplication::Get().ReleaseMouseCapture();
    }
}
