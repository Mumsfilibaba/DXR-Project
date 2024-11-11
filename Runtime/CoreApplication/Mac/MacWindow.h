#pragma once
#include "Core/Containers/SharedRef.h"
#include "CoreApplication/Generic/GenericWindow.h"

@class FCocoaWindow;
@class FCocoaWindowView;
@class NSScreen;

class FMacApplication;

class COREAPPLICATION_API FMacWindow final : public FGenericWindow
{
public:
    
    // Creates a new FMacWindow and returns a smart pointer that controls the window instead
    // of creating a new raw pointer.
    static TSharedRef<FMacWindow> Create(FMacApplication* InApplication);
    
public:
    ~FMacWindow();
    
    virtual bool Initialize(const FGenericWindowInitializer& InInitializer) override final;
    virtual void Show(bool bFocus) override final;
    virtual void Minimize() override final;
    virtual void Maximize() override final;
    virtual void Destroy() override final;
    virtual void Restore() override final;
    virtual void ToggleFullscreen() override final;
    virtual bool IsActiveWindow() const override final;
    virtual bool IsValid() const override final;
    virtual bool IsMinimized() const override final;
    virtual bool IsMaximized() const override final;
    virtual bool IsChildWindow(const TSharedRef<FGenericWindow>& ChildWindow) const override final;
    virtual void SetWindowFocus() override final;
    virtual void SetTitle(const FString& Title) override final;
    virtual void GetTitle(FString& OutTitle) const override final;
    virtual void SetWindowPos(int32 x, int32 y) override final;
    virtual void SetWindowOpacity(float Alpha) override final;
    virtual void SetWindowShape(const FWindowShape& Shape, bool bMove) override final;
    virtual void GetWindowShape(FWindowShape& OutWindowShape) const override final;
    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const override final;
    virtual float GetWindowDPIScale() const override final;
    virtual uint32 GetWidth() const override final;
    virtual uint32 GetHeight() const override final;
    virtual void SetStyle(EWindowStyleFlags InStyle) override final;
    virtual void SetPlatformHandle(void* InPlatformHandle) override final;

    virtual void* GetPlatformHandle() const override final
    {
        return reinterpret_cast<void*>(CocoaWindow);
    }
    
    FCocoaWindow* GetCocoaWindow() const
    {
        return CocoaWindow;
    }

    FMacApplication* GetApplication() const
    {
        return Application;
    }
    
    // Sets the cached window position. This is used in order to avoid sending multiple events
    // that reports the same position.
    void SetCachedPosition(const FIntVector2& InPosition)
    {
        Position = InPosition;
    }
    
    const FIntVector2& GetCachedPosition() const
    {
        return Position;
    }
    
private:
    FMacWindow(FMacApplication* InApplication);

    FMacApplication*  Application;
    FCocoaWindow*     CocoaWindow;
    FCocoaWindowView* CocoaWindowView;
    FIntVector2       Position;
};
