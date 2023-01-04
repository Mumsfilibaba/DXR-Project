#include "Viewport.h"
#include "RHI/RHIResources.h"
// TODO: This needs to be moved to another module
#include "Engine/Project/ProjectManager.h"

FViewport::FViewport(const FViewportInitializer& InInitializer)
    : Viewport(nullptr)
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

    Window = Application.CreateWindow();
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
        return false
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

    Resources.MainWindowViewport = RHICreateViewport(ViewportDesc);
    if (!Resources.MainWindowViewport)
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
    Viewport.Reset();
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
