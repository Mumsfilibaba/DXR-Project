#include "Window.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Application/Application.h"

FWindow::FWindow()
    : FWidget()
    , NativeWindow(nullptr)
    , Title()
    , WindowMode(EWindowMode::None)
    , Width{0}
    , Height{0}
    , bIsVisible{false}
    , bIsMaximized{false}
{ }

bool FWindow::Initialize(const FInitializer& Initializer)
{
    TSharedPtr<FGenericApplication> PlatfromApplication = FWindowedApplication::Get().GetPlatformApplication();
    if (!PlatfromApplication)
    {
        LOG_ERROR("[FWindow]: PlatformApplication is not initialized");
        return false;
    }

    NativeWindow = PlatfromApplication->CreateWindow();
    if (!NativeWindow)
    {
        LOG_ERROR("[FWindow]: Failed to create NativeWindow");
        return false;
    }

    if (Initializer.Width == 0 || Initializer.Height == 0 || Initializer.WindowMode == EWindowMode::None)
    {
        LOG_ERROR("[FWindow]: Width, Height or WindowMode is not Initialized");
        return false;
    }

    // TODO: Control these somehow
    const FWindowStyle WindowStyle
    { 
        EWindowStyleFlag::Titled |
        EWindowStyleFlag::Closable |
        EWindowStyleFlag::Resizeable |
        EWindowStyleFlag::Maximizable |
        EWindowStyleFlag::Minimizable 
    };

    WindowMode = Initializer.WindowMode;
    Width      = Initializer.Width;
    Height     = Initializer.Height;

    return NativeWindow->Initialize(Title, Width, Height, 0, 0, WindowStyle);
}

DISABLE_UNREFERENCED_VARIABLE_WARNING

void FWindow::Paint(const FRectangle& AssignedBounds)
{
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

FOverlay::FScopedSlotInitilizer FWindow::AddOverlaySlot()
{
    if (!Overlay)
    {
        Overlay = NewWidget(FOverlay);
    }

    CHECK(Overlay != nullptr);
    return Overlay->AddSlot();
}

void FWindow::RemoveOverlayWidget(const TSharedPtr<FWidget>& InWidget)
{
    if (Overlay)
    {
        Overlay->RemoveWidget(InWidget);
    }
}

void FWindow::ShowWindow(bool bMaximized)
{
    if (NativeWindow)
    {
        NativeWindow->Show(bMaximized);
    }

    bIsVisible = true;
}

void FWindow::CloseWindow()
{
    if (NativeWindow)
    {
        NativeWindow->Destroy();
    }

    bIsVisible = false;
}

void FWindow::Minimize()
{
    if (NativeWindow)
    {
        NativeWindow->Minimize();
    }
}

void FWindow::Maximize()
{
    if (NativeWindow)
    {
        NativeWindow->Maximize();
    }
}

void FWindow::Restore()
{
    if (NativeWindow)
    {
        NativeWindow->Restore();
    }
}

FResponse FWindow::OnWindowResized(const FWindowResizedEvent& ResizeEvent)
{
    Width  = ResizeEvent.GetWidth();
    Height = ResizeEvent.GetHeight();
    
    OnWindowResizeEvent.Broadcast(ResizeEvent);
    return FResponse::Unhandled();
}

FResponse FWindow::OnWindowMoved(const FWindowMovedEvent& InMoveEvent)
{
    OnWindowMovedEvent.Broadcast(InMoveEvent);
    return FResponse::Unhandled();
}

FResponse FWindow::OnWindowClosed()
{
    OnWindowClosedEvent.Broadcast();
    return FResponse::Unhandled();
}

void FWindow::SetWindowMode(EWindowMode InWindowMode)
{
    CHECK(InWindowMode != EWindowMode::Borderless && InWindowMode != EWindowMode::None);

    // TODO: Send this directly to the native window
    if (NativeWindow)
    {
        if (WindowMode != InWindowMode)
        {
            NativeWindow->ToggleFullscreen();
        }
    }

    WindowMode = InWindowMode;
}
