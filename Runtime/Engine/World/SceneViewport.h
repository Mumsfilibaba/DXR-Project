#pragma once
#include "Application/IViewport.h"
#include "Application/Elements/ViewportElement.h"
#include "Engine/World/World.h"

class FWindowElement;

class ENGINE_API FSceneViewport : public IViewport
{
public:
    FSceneViewport(const TWeakPtr<FViewportElement>& InViewport);
    ~FSceneViewport();

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

    virtual FRHISwapChainRef GetRHISwapChain() const override
    {
        return RHISwapChain;
    }

    virtual void SetViewportElement(const TSharedPtr<FViewportElement>& InViewport) override
    {
        Viewport = InViewport;
    }

    virtual TSharedPtr<FViewportElement> GetViewportElement() override
    {
        return Viewport.IsValid() ? Viewport.ToSharedPtr() : nullptr;
    }

    virtual TSharedPtr<const FViewportElement> GetViewportElement() const override
    {
        return Viewport.IsValid() ? TSharedPtr<const FViewportElement>(Viewport) : nullptr;
    }

    /**
     * @brief Creates the RHISwapChain for this SceneViewport
     * 
     * @return Returns true if the creation of the RHISwapChain was successful
     */
    bool InitializeRHI();

    /** @brief Releases the RHISwapChain */
    void ReleaseRHI();

    /** @brief Update viewport-dependent scene state, such as the active camera projection. */
    void Tick();

    void SetPlayerInputEnabled(bool bEnabled);
    IntVector2 ConsumeHighPrecisionMouseDelta();

    bool CaptureMouse();
    void ReleaseMouse();

    bool IsMouseCaptured() const
    {
        return bMouseCaptured;
    }

    bool IsPlayerInputEnabled() const
    {
        return bPlayerInputEnabled;
    }
    
    FPlayerController* GetFirstPlayerController()
    {
        return World ? World->GetFirstPlayerController() : nullptr;
    }

    void SetWorld(FWorld* InWorld) 
    { 
        World = InWorld; 
    }

    FWorld* GetWorld() const 
    {
        return World; 
    }

private:
    TSharedPtr<FWindowElement> GetCaptureWindow() const;
    FRectangle                 GetCaptureRect() const;

    FWorld*                    World;
    TWeakPtr<FViewportElement> Viewport;
    FRHISwapChainRef           RHISwapChain;
    IntVector2                 HighPrecisionMouseDelta;
    IntVector2                 MouseRestorePosition;
    bool                       bPlayerInputEnabled;
    bool                       bMouseCaptured;
    bool                       bCursorWasVisible;
    bool                       bDiscardCaptureWarpDelta;
};
