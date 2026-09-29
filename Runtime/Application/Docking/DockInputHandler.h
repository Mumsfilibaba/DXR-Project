#pragma once
#include "Application/InputHandler.h"
#include "Application/InputService.h"

class APPLICATION_API FDockInputHandler final : public FInputHandler, public TApplicationInputService<FDockInputHandler>
{
public:

    /** @brief Cancels whatever drag the handler was watching, which nothing would finish once it is gone. */
    static void OnUnregistered();

public:
    FDockInputHandler();
    virtual ~FDockInputHandler();

    // FInputHandler Interface
    virtual bool OnKeyDown(const FKeyEvent& KeyEvent) override final;
    virtual bool OnMouseMove(const FCursorEvent& CursorEvent) override final;
    virtual bool OnMouseButtonUp(const FCursorEvent& CursorEvent) override final;

private:
    static void UpdateDrag(const IntVector2& ScreenPosition);
    static void FinishDrag();
};
