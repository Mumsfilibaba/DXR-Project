#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Application/Console/ConsoleElement.h"
#include "Engine/EngineUI/BaseConsoleWidget.h"

class FOverlayConsole
{
public:
    FOverlayConsole();
    ~FOverlayConsole();

    /**
     * @brief Builds the console, attaches it as the overlay of the engine window and starts listening for the toggle key.
     *
     * @return True when the font loaded and the console was attached.
     */
    bool Initialize();

    /** @brief Detaches the console from the window and stops listening for input. */
    void Release();

private:
    void HandleKeyEvent(const FKeyEvent& KeyEvent);

    TSharedPtr<FConsoleElement>      ConsoleElement;
    TSharedPtr<IFontFace>            Font;
    TSharedPtr<FConsoleInputHandler> InputHandler;
};
