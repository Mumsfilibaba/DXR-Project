#include "Engine/EngineUI/Runtime/OverlayConsole.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/Paths.h"
#include "Application/Application.h"
#include "Application/Text/TrueTypeFontFace.h"
#include "Engine/Engine.h"

static constexpr int32 GConsoleFontPixelHeight = 16;

FOverlayConsole::FOverlayConsole()
    : ConsoleElement(nullptr)
    , Font(nullptr)
    , InputHandler(nullptr)
{
}

FOverlayConsole::~FOverlayConsole()
{
    Release();
}

bool FOverlayConsole::Initialize()
{
    if (!FApplication::IsInitialized() || !FEngine::Get())
    {
        return false;
    }

    TSharedPtr<FWindowElement> EngineWindow = FEngine::Get()->GetEngineWindow();
    if (!EngineWindow)
    {
        LOG_ERROR("[FOverlayConsole]: There is no engine window to attach the console to");
        return false;
    }

    Font = FTrueTypeFontFace::CreateFromFile(Paths::GetAssetDir() + "/Editor/Fonts/consola.ttf", GConsoleFontPixelHeight);
    if (!Font)
    {
        return false;
    }

    FConsoleElement::FInitializer Initializer;
    Initializer.Font = Font;

    ConsoleElement = FConsoleElement::Create(Initializer);
    if (!ConsoleElement)
    {
        return false;
    }

    EngineWindow->SetOverlay(ConsoleElement);

    InputHandler = MakeSharedPtr<FConsoleInputHandler>();
    InputHandler->KeyConsumption = EConsoleKeyConsumption::ToggleKeyOnly;
    InputHandler->HandleKeyEventDelegate.BindRaw(this, &FOverlayConsole::HandleKeyEvent);
    FApplication::Get().RegisterInputHandler(InputHandler);

    return true;
}

void FOverlayConsole::Release()
{
    if (FApplication::IsInitialized() && InputHandler)
    {
        FApplication::Get().UnregisterInputHandler(InputHandler);
    }

    if (ConsoleElement && FEngine::Get())
    {
        if (TSharedPtr<FWindowElement> EngineWindow = FEngine::Get()->GetEngineWindow())
        {
            if (EngineWindow->GetOverlay() == ConsoleElement)
            {
                EngineWindow->SetOverlay(nullptr);
            }
        }
    }

    InputHandler.Reset();
    ConsoleElement.Reset();
    Font.Reset();
}

void FOverlayConsole::HandleKeyEvent(const FKeyEvent& KeyEvent)
{
    if (!ConsoleElement || !KeyEvent.IsDown() || KeyEvent.IsRepeat())
    {
        return;
    }

    if (!FConsoleElement::IsToggleKey(KeyEvent.GetKey()))
    {
        return;
    }

    ConsoleElement->Toggle();

    const bool bIsOpen = ConsoleElement->IsOpen();
    if (bIsOpen)
    {
        FApplication::Get().SetFocusElement(ConsoleElement->GetInputElement());
    }
    else if (FEngine::Get())
    {
        FApplication::Get().SetFocusElement(FEngine::Get()->GetViewportElement());
    }
}
