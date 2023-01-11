#include "SceneViewport.h"

FSceneViewport::FSceneViewport(const FViewportInitializer& InInitializer)
    : FViewport(InInitializer)
    , Scene(nullptr)
{ }

FSceneViewport::~FSceneViewport()
{
    Scene = nullptr;
}

bool FSceneViewport::OnKeyEvent(const FKeyEvent& KeyEvent)
{ 
    return false;
}

bool FSceneViewport::OnKeyTyped(FKeyCharEvent KeyTypedEvent) 
{ 
    return false;
}

bool FSceneViewport::OnMouseMove(const FMouseMovedEvent& MouseEvent)
{ 
    return false; 
}

bool FSceneViewport::OnMouseButtonEvent(const FMouseButtonEvent& MouseEvent) 
{ 
    return false;
}

bool FSceneViewport::OnMouseScrolled(const FMouseScrolledEvent& MouseEvent)
{ 
    return false;
}

bool FSceneViewport::OnHighPrecisionMouseInput(const FHighPrecisionMouseEvent& MouseEvent) 
{ 
    return false;
}
