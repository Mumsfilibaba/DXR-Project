#pragma once
#include "World.h"
#include "Application/IViewport.h"
#include "Application/Widgets/Viewport.h"

class ENGINE_API FSceneViewport : public IViewport
{
public:
    FSceneViewport(const TWeakPtr<FViewport>& InViewport);
    ~FSceneViewport();

    // RHI Resource
    bool InitializeRHI();
    void ReleaseRHI();

    FRHIViewportRef GetRHIViewport() const
    {
        return RHIViewport;
    }

public:

    // IViewport Interface
    virtual FResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent) override;
    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent) override;
    virtual FResponse OnKeyUp(const FKeyEvent& KeyEvent) override;
    virtual FResponse OnKeyChar(const FKeyEvent&) override { return FResponse::Unhandled(); }
    virtual FResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;
    virtual FResponse OnMouseScroll(const FCursorEvent& CursorEvent) override;
    virtual FResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent) override;
    virtual FResponse OnMouseLeft(const FCursorEvent& CursorEvent) override { return FResponse::Unhandled(); }
    virtual FResponse OnMouseEntered(const FCursorEvent& CursorEvent) override { return FResponse::Unhandled(); }
    virtual FResponse OnFocusLost() override;
    virtual FResponse OnFocusGained() override { return FResponse::Unhandled(); }

    virtual void SetViewportWidget(const TSharedPtr<FViewport>& InViewport) override
    {
        Viewport = InViewport;
    }

    virtual TSharedPtr<FViewport>       GetViewportWidget()       override { return Viewport.IsValid() ? Viewport.ToSharedPtr() : nullptr; }
    virtual TSharedPtr<const FViewport> GetViewportWidget() const override { return Viewport.IsValid() ? TSharedPtr<const FViewport>(Viewport) : nullptr; }

public:
    void SetWorld(FWorld* InWorld) 
    { 
        World = InWorld; 
    }

    FWorld* GetWorld() const 
    {
        return World; 
    }

    FPlayerController* GetFirstPlayerController()
    {
        return World ? World->GetFirstPlayerController() : nullptr;
    }

private:
    TSharedRef<FGenericWindow> Window;
    TWeakPtr<FViewport>        Viewport;
    TSharedRef<FRHIViewport>   RHIViewport;
    FWorld*                    World;
};
