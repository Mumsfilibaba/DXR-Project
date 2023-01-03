#include "Viewport.h"

FViewport::FViewport(const FViewportInitializer &InInitializer)
    : Viewport(nullptr), Window(nullptr), ClosedEvent(), ResizedEvent()
{
}

bool FViewport::Create()
{
    return true;
}

bool FViewport::CreateRHI()
{
    return false;
}

void FViewport::Destroy()
{
    Window.Reset();
}

void FViewport::DestroyRHI()
{
    Viewport.Reset();
}

bool FViewport::OnViewportResized(const FWindowResizeEvent &ResizeEvent)
{
    ResizedEvent.Broadcast(this);
    return false;
}

bool FViewport::OnViewportClosed()
{
    ClosedEvent.Broadcast(this);
    return false;
}
