#pragma once
#include "Application/Application.h"

template<typename HandlerType>
struct TApplicationInputService
{
    /**
     * @brief Creates the handler and registers it with the application.
     *
     * @return The handler, which is only registered when an application exists to register it with.
     */
    static TSharedPtr<HandlerType> Register()
    {
        TSharedPtr<HandlerType> InputHandler = MakeSharedPtr<HandlerType>();
        if (FApplication::IsInitialized())
        {
            FApplication::Get().RegisterInputHandler(InputHandler);
        }

        return InputHandler;
    }

    /**
     * @brief Unregisters a handler and lets it release what it was keeping open.
     *
     * @param InputHandler The handler to remove.
     */
    static void Unregister(const TSharedPtr<HandlerType>& InputHandler)
    {
        if (InputHandler && FApplication::IsInitialized())
        {
            FApplication::Get().UnregisterInputHandler(InputHandler);
        }

        HandlerType::OnUnregistered();
    }
};
