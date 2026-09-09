#pragma once
#include "Application/InputHandler.h"

class APPLICATION_API FDockInputHandler final : public FInputHandler
{
public:

    /**
     * @brief Creates the handler and registers it with the application.
     *
     * @return The handler, which is only registered when an application exists to register it with.
     */
    static TSharedPtr<FDockInputHandler> Register();

    /**
     * @brief Unregisters a handler and ends whatever drag it was watching.
     *
     * @param InputHandler The handler to remove.
     */
    static void Unregister(const TSharedPtr<FDockInputHandler>& InputHandler);

public:
    FDockInputHandler() = default;
    virtual ~FDockInputHandler() = default;

    // FInputHandler Interface
    virtual bool OnKeyDown(const FKeyEvent& KeyEvent) override final;
    virtual bool OnMouseMove(const FCursorEvent& CursorEvent) override final;
    virtual bool OnMouseButtonUp(const FCursorEvent& CursorEvent) override final;

private:
    static void UpdateDrag(const IntVector2& ScreenPosition);
    static void FinishDrag();
};
