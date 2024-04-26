#pragma once
#include "Core/Containers/SharedRef.h"
#include "CoreApplication/Generic/GenericWindow.h"

@class FCocoaWindow;
@class FCocoaWindowView;

class FMacApplication;

class COREAPPLICATION_API FMacWindow final : public FGenericWindow
{
public:
    FMacWindow(FMacApplication* InApplication);
    ~FMacWindow();
    
    virtual bool Initialize(const FGenericWindowInitializer& InInitializer) override final;
    virtual void Show(bool bFocusOnActivate) override final;
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
    virtual float GetWindowDpiScale() const override final;
    virtual uint32 GetWidth() const override final;
    virtual uint32 GetHeight() const override final;
    virtual void SetPlatformHandle(void* InPlatformHandle) override final;

    virtual void* GetPlatformHandle() const override final
    {
        return reinterpret_cast<void*>(Window);
    }

    virtual void SetStyle(FWindowStyle InStyle) override final;
    
    FCocoaWindow* GetWindow() const 
    { 
        return Window; 
    }

    FMacApplication* GetApplication() const 
    { 
        return Application;
    }
    
private:
    FMacApplication*  Application;
    FCocoaWindow*     Window;
    FCocoaWindowView* WindowView;
};
