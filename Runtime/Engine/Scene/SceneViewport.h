#pragma once
#include "Scene.h"
#include "Application/Viewport.h"
#include "Core/Containers/SharedPtr.h"

class ENGINE_API FSceneViewport : public IViewport
{
public:
    FSceneViewport(const TWeakPtr<FViewport>& InViewport);
    ~FSceneViewport();

    virtual FResponse OnControllerAnalog    (const FControllerEvent& ControllerEvent) override;
    virtual FResponse OnControllerButtonDown(const FControllerEvent& ControllerEvent) override;
    virtual FResponse OnControllerButtonUp  (const FControllerEvent& ControllerEvent) override;

    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent) override;
    virtual FResponse OnKeyUp  (const FKeyEvent& KeyEvent) override;
    
    virtual FResponse OnKeyChar(const FKeyEvent&) override { return FResponse::Unhandled(); }

    virtual FResponse OnMouseMove       (const FMouseEvent& MouseEvent) override;
    virtual FResponse OnMouseButtonDown (const FMouseEvent& MouseEvent) override;
    virtual FResponse OnMouseButtonUp   (const FMouseEvent& MouseEvent) override;
    virtual FResponse OnMouseScroll     (const FMouseEvent& MouseEvent) override;
    virtual FResponse OnMouseDoubleClick(const FMouseEvent& MouseEvent) override;

    virtual FResponse OnFocusLost()    override;
    virtual FResponse OnFocusGained()  override { return FResponse::Unhandled(); }
    virtual FResponse OnMouseLeft()    override { return FResponse::Unhandled(); }
    virtual FResponse OnMouseEntered() override { return FResponse::Unhandled(); }

    virtual void SetViewport(const TSharedPtr<FViewport>& InViewport) override
    {
        Viewport = InViewport;
    }

    virtual TSharedPtr<FViewport> GetViewport() override
    { 
        return Viewport.IsValid() ? Viewport.ToSharedPtr() : nullptr;
    }

    virtual TSharedPtr<const FViewport> GetViewport() const override
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

    FPlayerController* GetFirstPlayerController()
    {
        if (Scene)
        {
            return Scene->GetFirstPlayerController();
        }

        return nullptr;
    }

private:
    TWeakPtr<FViewport> Viewport;
    FScene*             Scene;
};