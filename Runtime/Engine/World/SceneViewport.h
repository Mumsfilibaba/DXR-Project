#pragma once
#include "World.h"
#include "Application/IViewport.h"
#include "Application/Widgets/ViewportWidget.h"

class ENGINE_API FSceneViewport : public IViewport
{
public:
    FSceneViewport(const TWeakPtr<FViewportWidget>& InViewport);
    ~FSceneViewport();

    /**
     * @brief Creates the RHIViewport for this SceneViewport
     * 
     * @return Returns true if the creation of the RHIViewport was successful
     */
    bool InitializeRHI();

    /**
     * @brief Releases the RHIViewport
     */
    void ReleaseRHI();

    /**
     * @brief Updates the viewport each frame, this updates parts of the scene-view that requires
     * to know about the current size of the viewport. For example update the camera-projection.
     */
    void Tick();

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

    virtual void SetViewportWidget(const TSharedPtr<FViewportWidget>& InViewport) override
    {
        Viewport = InViewport;
    }

    virtual TSharedPtr<FViewportWidget> GetViewportWidget() override
    {
        return Viewport.IsValid() ? Viewport.ToSharedPtr() : nullptr;
    }

    virtual TSharedPtr<const FViewportWidget> GetViewportWidget() const override
    {
        return Viewport.IsValid() ? TSharedPtr<const FViewportWidget>(Viewport) : nullptr;
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
    TWeakPtr<FViewportWidget>      Viewport;
    TSharedRef<FRHIViewport> RHIViewport;
    FWorld*                  World;
};
