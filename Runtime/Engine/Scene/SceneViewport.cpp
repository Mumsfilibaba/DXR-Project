#include "SceneViewport.h"

FSceneViewport::FSceneViewport(const TWeakPtr<FViewport>& InViewport)
    : IViewport()
    , Viewport(InViewport)
    , Scene(nullptr)
{ }

FSceneViewport::~FSceneViewport()
{
    Scene = nullptr;
}

bool FSceneViewport::OnKeyDown(const FKeyEvent& KeyEvent)
{
    return false;
}

bool FSceneViewport::OnKeyUp(const FKeyEvent& KeyEvent)
{
    return false;
}

bool FSceneViewport::OnKeyChar(FKeyCharEvent KeyCharEvent)
{
    return false;
}

bool FSceneViewport::OnMouseMove(const FMouseMovedEvent& MouseEvent)
{
    return false;
}

bool FSceneViewport::OnMouseDown(const FMouseButtonEvent& MouseEvent)
{
    return false;
}

bool FSceneViewport::OnMouseUp(const FMouseButtonEvent& MouseEvent)
{
    return false;
}

bool FSceneViewport::OnMouseScroll(const FMouseScrolledEvent& MouseEvent)
{
    return false;
}

bool FSceneViewport::OnMouseEntered()
{
    return false;
}

bool FSceneViewport::OnMouseLeft()
{
    return false;
}

bool FSceneViewport::OnWindowResized(const FWindowResizedEvent& InResizeEvent)
{
    return false;
}

bool FSceneViewport::OnWindowMove(const FWindowMovedEvent& InMovedEvent)
{
    return false;
}

bool FSceneViewport::OnWindowFocusGained()
{
    return false;
}

bool FSceneViewport::OnWindowFocusLost()
{
    return false;
}

bool FSceneViewport::OnWindowClosed()
{
    return false;
}
