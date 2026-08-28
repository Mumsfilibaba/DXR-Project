#pragma once
#include "Application/InputHandler.h"

class APPLICATION_API FMenuInputHandler final : public FInputHandler
{
public:

    /**
     * @brief Creates the handler and registers it with the application.
     *
     * @return The handler, which is only registered when an application exists to register it with.
     */
    static TSharedPtr<FMenuInputHandler> Register();

    /**
     * @brief Unregisters a handler and closes whatever it was keeping open.
     *
     * @param InputHandler The handler to remove.
     */
    static void Unregister(const TSharedPtr<FMenuInputHandler>& InputHandler);

    /**
     * @brief Advances both services, which is what opens a delayed submenu or a tool tip.
     *
     * @param DeltaSeconds Time since the last call.
     */
    static void Tick(float DeltaSeconds);

public:
    FMenuInputHandler() = default;
    virtual ~FMenuInputHandler() = default;

    // FInputHandler Interface
    virtual bool OnKeyDown(const FKeyEvent& KeyEvent) override final;
    virtual bool OnMouseMove(const FCursorEvent& CursorEvent) override final;
    virtual bool OnMouseButtonDown(const FCursorEvent& CursorEvent) override final;
};
