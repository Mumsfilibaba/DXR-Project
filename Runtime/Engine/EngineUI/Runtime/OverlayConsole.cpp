#include "Engine/EngineUI/Runtime/OverlayConsole.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/Paths.h"
#include "Application/Application.h"
#include "Application/Text/TrueTypeFontFace.h"
#include "Engine/Engine.h"

static constexpr int32 GConsoleFontPixelHeight = 16;

FOverlayConsole::FOverlayConsole()
    : Console(nullptr)
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

    TSharedPtr<FWindow> EngineWindow = FEngine::Get()->GetEngineWindow();
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

    FConsole::FDesc Desc;
    Desc.Font = Font;

    Console = FConsole::Create(Desc);
    if (!Console)
    {
        return false;
    }

    EngineWindow->SetOverlay(Console);

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

    if (Console && FEngine::Get())
    {
        if (TSharedPtr<FWindow> EngineWindow = FEngine::Get()->GetEngineWindow())
        {
            if (EngineWindow->GetOverlay() == Console)
            {
                EngineWindow->SetOverlay(nullptr);
            }
        }
    }

    InputHandler.Reset();
    Console.Reset();
    Font.Reset();
}

void FOverlayConsole::HandleKeyEvent(const FKeyEvent& KeyEvent)
{
    if (!Console || !KeyEvent.IsDown() || KeyEvent.IsRepeat())
    {
        return;
    }

    if (!FConsole::IsToggleKey(KeyEvent.GetKey()))
    {
        return;
    }

    Console->Toggle();

    const bool bIsOpen = Console->IsOpen();
    if (bIsOpen)
    {
        FApplication::Get().SetFocusElement(Console->GetInput());
    }
    else if (FEngine::Get())
    {
        FApplication::Get().SetFocusElement(FEngine::Get()->GetViewport());
    }
}
