#pragma once
#include "Scene.h"
#include "Application/Widgets/Viewport.h"
#include "Core/Containers/SharedPtr.h"

class ENGINE_API FSceneViewport : public IViewport
{
public:
    FSceneViewport(const TWeakPtr<FViewportWidget>& InViewport);
    ~FSceneViewport();

    virtual FResponse OnControllerButtonUp    (const FControllerEvent& ControllerEvent) override;
    virtual FResponse OnControllerButtonDown  (const FControllerEvent& ControllerEvent) override;
    virtual FResponse OnControllerButtonAnalog(const FControllerEvent& ControllerEvent) override;

    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent) override;
    virtual FResponse OnKeyUp  (const FKeyEvent& KeyEvent) override;
    virtual FResponse OnKeyChar(const FKeyEvent& KeyEvent) override;

    virtual FResponse OnMouseMove       (const FMouseEvent& MouseEvent) override;
    virtual FResponse OnMouseButtonDown (const FMouseEvent& MouseEvent) override;
    virtual FResponse OnMouseButtonUp   (const FMouseEvent& MouseEvent) override;
    virtual FResponse OnMouseEntered    (const FMouseEvent& MouseEvent) override;
    virtual FResponse OnMouseScroll     (const FMouseEvent& MouseEvent) override;
    virtual FResponse OnMouseLeft       (const FMouseEvent& MouseEvent) override;
    virtual FResponse OnMouseDoubleClick(const FMouseEvent& MouseEvent) override;

    virtual void SetViewportWidget(const TSharedPtr<FViewportWidget>& InViewport) override
    {
        Viewport = InViewport;
    }

    virtual TSharedPtr<FViewportWidget> GetViewportWidget()
    { 
        return Viewport.IsValid() ? Viewport.ToSharedPtr() : nullptr;
    }

    virtual TSharedPtr<const FViewportWidget> GetViewportWidget() const
    {
        return Viewport.IsValid() ? TSharedPtr<const FViewportWidget>(Viewport) : nullptr;
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
    TWeakPtr<FViewportWidget> Viewport;
    FScene*                   Scene;
};