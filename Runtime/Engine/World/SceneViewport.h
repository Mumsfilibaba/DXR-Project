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

public:

    // IViewport Interface
    virtual FEventResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent) override;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override;
    virtual FEventResponse OnKeyUp(const FKeyEvent& KeyEvent) override;
    virtual FEventResponse OnKeyChar(const FKeyEvent&) override;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnFocusLost() override;
    virtual FEventResponse OnFocusGained() override;

    virtual TSharedRef<FRHIViewport> GetViewportRHI() const override
    {
        return RHIViewport;
    }

    virtual void SetViewportWidget(const TSharedPtr<FViewport>& InViewport) override
    {
        Viewport = InViewport;
    }

    virtual TSharedPtr<FViewport> GetViewportWidget() override
    {
        return Viewport.IsValid() ? Viewport.ToSharedPtr() : nullptr;
    }

    virtual TSharedPtr<const FViewport> GetViewportWidget() const override
    {
        return Viewport.IsValid() ? TSharedPtr<const FViewport>(Viewport) : nullptr;
    }

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
    TWeakPtr<FViewport>        Viewport;
    TSharedRef<FRHIViewport>   RHIViewport;
    FWorld*                    World;
};
