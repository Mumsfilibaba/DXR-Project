#include "Application/Menus/MenuInputHandler.h"
#include "Application/Menus/MenuStack.h"
#include "Application/Menus/ToolTipService.h"
#include "Application/Application.h"

TSharedPtr<FMenuInputHandler> FMenuInputHandler::Register()
{
    TSharedPtr<FMenuInputHandler> InputHandler = MakeSharedPtr<FMenuInputHandler>();
    if (FApplication::IsInitialized())
    {
        FApplication::Get().RegisterInputHandler(InputHandler);
    }

    return InputHandler;
}

void FMenuInputHandler::Unregister(const TSharedPtr<FMenuInputHandler>& InputHandler)
{
    if (InputHandler && FApplication::IsInitialized())
    {
        FApplication::Get().UnregisterInputHandler(InputHandler);
    }

    FMenuStack::Get().DismissAll();
    FToolTipService::Get().DismissToolTip();
}

void FMenuInputHandler::Tick(float DeltaSeconds)
{
    FMenuStack::Get().Tick(DeltaSeconds);
    FToolTipService::Get().Tick(DeltaSeconds);
}

bool FMenuInputHandler::OnKeyDown(const FKeyEvent& KeyEvent)
{
    FToolTipService::Get().DismissToolTip();
    return FMenuStack::Get().HandleKeyDown(KeyEvent);
}

bool FMenuInputHandler::OnMouseMove(const FCursorEvent& CursorEvent)
{
    FToolTipService::Get().NotifyCursorMoved(CursorEvent.GetScreenPosition());
    return false;
}

bool FMenuInputHandler::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    FToolTipService::Get().DismissToolTip();
    return FMenuStack::Get().DismissOnClickOutside(CursorEvent.GetScreenPosition());
}
