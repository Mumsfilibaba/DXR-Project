#pragma once
#include "Scene.h"
#include "Application/Viewport.h"
#include "Core/Containers/SharedPtr.h"

class ENGINE_API FSceneViewport
    : public IViewport
{
public:
    FSceneViewport(const TWeakPtr<FViewport>& InViewport);
    ~FSceneViewport();

    virtual bool OnKeyDown(const FKeyEvent& KeyEvent);
    virtual bool OnKeyUp(const FKeyEvent& KeyEvent);
    virtual bool OnKeyChar(FKeyCharEvent KeyCharEvent);

    virtual bool OnMouseMove(const FMouseMovedEvent& MouseEvent);
    virtual bool OnMouseDown(const FMouseButtonEvent& MouseEvent);
    virtual bool OnMouseUp(const FMouseButtonEvent& MouseEvent);
    virtual bool OnMouseScroll(const FMouseScrolledEvent& MouseEvent);
    virtual bool OnMouseEntered();
    virtual bool OnMouseLeft();

    virtual bool OnWindowResized(const FWindowResizedEvent& InResizeEvent);
    virtual bool OnWindowMove(const FWindowMovedEvent& InMovedEvent);
    virtual bool OnWindowFocusGained();
    virtual bool OnWindowFocusLost();
    virtual bool OnWindowClosed();
    
    virtual TSharedPtr<FViewport> GetViewport() 
    { 
        return Viewport.IsValid() ? Viewport.ToSharedPtr() : nullptr;
    }

    virtual TSharedPtr<const FViewport> GetViewport() const
    {
        return Viewport.IsValid() ? TSharedPtr<const FViewport>(Viewport) : nullptr;
    }

    void SetScene(FScene* InScene) 
    { 
        Scene = InScene; 
    }

    FScene* GetScene() const 
    { 
        return Scene; 
    }

private:
    TWeakPtr<FViewport> Viewport;
    FScene*             Scene;
};