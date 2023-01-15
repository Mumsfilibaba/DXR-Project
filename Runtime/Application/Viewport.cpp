#include "Viewport.h"
#include "Application.h"
#include "Project/ProjectManager.h"
#include "RHI/RHIInterface.h"

FViewport::FViewport(const FViewportInitializer& InInitializer)
    : RHIViewport(nullptr)
    , Window(nullptr)
    , Initializer(InInitializer)
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
        FProjectManager::Get().GetProjectName().GetCString(),
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
    if (RHIViewport)
    {
        return true;
    }

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
    ResizedEvent.Broadcast(this, ResizeEvent);
    return false;
}

bool FViewport::OnViewportClosed()
{
    ClosedEvent.Broadcast(this);
    return false;
}
