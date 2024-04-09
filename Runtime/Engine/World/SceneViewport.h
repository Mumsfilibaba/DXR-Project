#pragma once
#include "World.h"
#include "Application/Viewport.h"
#include "Core/Containers/SharedPtr.h"

class ENGINE_API FSceneViewport : public IViewport
{
public:
    FSceneViewport(const TWeakPtr<FViewport>& InViewport);
    ~FSceneViewport();

    virtual FResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent) override;

    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent) override;
    
    virtual FResponse OnKeyUp(const FKeyEvent& KeyEvent) override;
    
    virtual FResponse OnKeyChar(const FKeyEvent&) override { return FResponse::Unhandled(); }

    virtual FResponse OnMouseMove(const FCursorEvent& MouseEvent) override;
    
    virtual FResponse OnMouseButtonDown(const FCursorEvent& MouseEvent) override;
    
    virtual FResponse OnMouseButtonUp(const FCursorEvent& MouseEvent) override;
    
    virtual FResponse OnMouseScroll(const FCursorEvent& MouseEvent) override;
    
    virtual FResponse OnMouseDoubleClick(const FCursorEvent& MouseEvent) override;

    virtual FResponse OnFocusLost() override;
    
    virtual FResponse OnFocusGained() override { return FResponse::Unhandled(); }
    
    virtual FResponse OnMouseLeft() override { return FResponse::Unhandled(); }
    
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

    void SetWorld(FWorld* InWorld) 
    { 
        World = InWorld; 
    }

    FWorld* GetWorldOwner() const 
    {
        return World; 
    }

    FPlayerController* GetFirstPlayerController()
    {
        if (World)
        {
            return World->GetFirstPlayerController();
        }

        return nullptr;
    }

private:
    TWeakPtr<FViewport> Viewport;
    FWorld*             World;
};
