#include "Viewport.h"
#include "Application.h"
#include "RHI/RHIInterface.h"
// TODO: This needs to be moved to another module
#include "Engine/Project/ProjectManager.h"

FViewport::FViewport(const FViewportInitializer& InInitializer)
    : RHIViewport(nullptr)
    , Window(nullptr)
    , ClosedEvent()
    , ResizedEvent()
{ }

bool FViewport::Create()
{
    const EWindowStyleFlag Style =
        EWindowStyleFlag::Titled |
        EWindowStyleFlag::Closable |
        EWindowStyleFlag::Minimizable |
        EWindowStyleFlag::Maximizable |
        EWindowStyleFlag::Resizeable;

    Window = FApplication::Get().CreateWindow();
    if (Window && Window->Initialize(
        FProjectManager::GetProjectName(),
        Initializer.Width, 
        Initializer.Height, 
        0,
        0, 
        Style))
    {
        Window->Show(false);
        return true;
    }
    else
    {
        return false;
    }
}

bool FViewport::CreateRHI()
{
    FRHIViewportDesc ViewportDesc(
        Window->GetPlatformHandle(),
        EFormat::R8G8B8A8_Unorm,
        EFormat::Unknown,
        Initializer.Width,
        Initializer.Height);

    RHIViewport = RHICreateViewport(ViewportDesc);
    if (!RHIViewport)
    {
        return false;
    }

    return true;
}

void FViewport::Destroy()
{
    Window.Reset();
}

void FViewport::DestroyRHI()
{
    RHIViewport.Reset();
}

void FViewport::ToggleFullscreen()
{
    if (Window)
    {
        Window->ToggleFullscreen();
    }
}

bool FViewport::OnViewportResized(const FWindowResizeEvent& ResizeEvent)
{
    ResizedEvent.Broadcast(this);
    return false;
}

bool FViewport::OnViewportClosed()
{
    ClosedEvent.Broadcast(this);
    return false;
}
