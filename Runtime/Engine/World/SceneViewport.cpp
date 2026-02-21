#include "Core/Misc/OutputDeviceLogger.h"
#include "Application/Application.h"
#include "Application/Widgets/ViewportWidget.h"
#include "Engine/World/Actors/PlayerInput.h"
#include "Engine/World/SceneViewport.h"
#include "RHI/RHI.h"
#include "RendererCore/RenderSettings.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FSceneViewport::FSceneViewport(const TWeakPtr<FViewportWidget>& InViewport)
    : IViewport()
    , Viewport(InViewport)
    , RHISwapChain(nullptr)
    , World(nullptr)
{
}

FSceneViewport::~FSceneViewport()
{
    CHECK(RHISwapChain == nullptr);

    Viewport = nullptr;
    World    = nullptr;
}

bool FSceneViewport::InitializeRHI()
{
    TSharedPtr<FViewportWidget> ViewportWidget;
    if (Viewport.IsExpired())
    {
        LOG_INFO("No valid viewport");
        return false;
    }
    else
    {
        ViewportWidget = Viewport.ToSharedPtr();
    }

    TSharedPtr<FWindowWidget> WindowWidget = FApplication::Get().FindWindowWidget(ViewportWidget);
    if (!WindowWidget)
    {
        return false;
    }

    const FIntVector2 WindowSize = WindowWidget->GetSize();
    FRHISwapChainInfo SwapChainInfo;
    SwapChainInfo.Width        = static_cast<uint16>(WindowSize.X);
    SwapChainInfo.Height       = static_cast<uint16>(WindowSize.Y);
    SwapChainInfo.WindowHandle = WindowWidget->GetPlatformWindow()->GetPlatformHandle();
    SwapChainInfo.ColorFormat  = RenderSettings::GetBackBufferFormat();
    SwapChainInfo.bFramePacing = true;

    FRHISwapChainRef NewSwapChain = FRHI::Get()->CreateSwapChain(SwapChainInfo);
    if (!NewSwapChain)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        RHISwapChain = NewSwapChain;
    }

    return true;
}

void FSceneViewport::ReleaseRHI()
{
    CHECK(RHISwapChain->GetRefCount() == 1);
    RHISwapChain.Reset();
}

void FSceneViewport::Tick()
{
    if (World)
    {
        FCamera* Camera = World->GetCamera();
        if (Camera && Viewport.IsValid())
        {
            const FRectangle& ViewportArea = Viewport->GetContentRectangle();
            Camera->UpdateProjectionMatrix(static_cast<float>(ViewportArea.Width), static_cast<float>(ViewportArea.Height));
        }
    }
}

FEventResponse FSceneViewport::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnAxisEvent(AnalogGamepadEvent.GetAnalogSource(), AnalogGamepadEvent.GetAnalogValue());
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent.GetKey(), KeyEvent.IsDown(), KeyEvent.IsRepeat());
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnKeyUp(const FKeyEvent& KeyEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent.GetKey(), KeyEvent.IsDown(), KeyEvent.IsRepeat());
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnKeyChar(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        // PlayerController->GetPlayerInput()->OnCursorEvent(CursorEvent);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(CursorEvent.GetKey(), CursorEvent.IsDown(), false);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(CursorEvent.GetKey(), CursorEvent.IsDown(), false);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        // PlayerController->GetPlayerInput()->OnCursorEvent(CursorEvent);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseDoubleClick(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(CursorEvent.GetKey(), true, false);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent)
{
    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnFocusLost()
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->ClearInputStates();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnFocusGained() 
{
    return FEventResponse::Unhandled();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
