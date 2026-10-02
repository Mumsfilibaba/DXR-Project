#include "Application/Menus/MenuInputHandler.h"
#include "Application/Menus/MenuStack.h"
#include "Application/Menus/ToolTipService.h"
#include "Application/Application.h"

void FMenuInputHandler::OnUnregistered()
{
    FMenuStack::Get().DismissAll();
    FToolTipService::Get().DismissToolTip();
}

void FMenuInputHandler::Tick(float DeltaSeconds)
{
    FMenuStack::Get().Tick(DeltaSeconds);
    FToolTipService::Get().Tick(DeltaSeconds);
}

FMenuInputHandler::FMenuInputHandler()
    : FInputHandler()
{
}

FMenuInputHandler::~FMenuInputHandler() = default;

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
