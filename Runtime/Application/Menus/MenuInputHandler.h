#pragma once
#include "Application/InputHandler.h"
#include "Application/InputService.h"

class APPLICATION_API FMenuInputHandler final : public FInputHandler, public TApplicationInputService<FMenuInputHandler>
{
public:

    /** @brief Closes every menu and the tool tip, which nothing routes input to once the handler is gone. */
    static void OnUnregistered();

    /**
     * @brief Advances both services, which is what opens a delayed submenu or a tool tip.
     *
     * @param DeltaSeconds Time since the last call.
     */
    static void Tick(float DeltaSeconds);

public:
    FMenuInputHandler();
    virtual ~FMenuInputHandler();

    // FInputHandler Interface
    virtual bool OnKeyDown(const FKeyEvent& KeyEvent) override final;
    virtual bool OnMouseMove(const FCursorEvent& CursorEvent) override final;
    virtual bool OnMouseButtonDown(const FCursorEvent& CursorEvent) override final;
};
