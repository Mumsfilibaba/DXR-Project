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

    virtual bool Initialize(
        const FString& InTitle,
        uint32         InWidth,
        uint32         InHeight,
        int32          x,
        int32          y,
        FWindowStyle   Style) override final;

    virtual void Show(bool bMaximized) override final;

    virtual void Minimize() override final;

    virtual void Maximize() override final;

    virtual void Close() override final;

    virtual void Restore() override final;

    virtual void ToggleFullscreen() override final;

    virtual bool IsValid() const override final { return (WindowHandle != nullptr); }
    
    virtual bool IsActiveWindow() const override final;

    virtual void SetTitle(const FString& Title) override final;
    
    virtual void GetTitle(FString& OutTitle) override final;

    virtual void SetWindowShape(const FWindowShape& Shape, bool bMove) override final;
    
    virtual void GetWindowShape(FWindowShape& OutWindowShape) const override final;

    virtual uint32 GetWidth() const override final;
    
    virtual uint32 GetHeight() const override final;

    virtual void  SetPlatformHandle(void* InPlatformHandle) override final;
    
    virtual void* GetPlatformHandle() const override final 
    { 
        return reinterpret_cast<void*>(WindowHandle); 
    }

public:
    FCocoaWindow* GetWindowHandle() const 
    { 
        return WindowHandle; 
    }

    FMacApplication* GetApplication() const 
    { 
        return Application;
    }
    
private:
    FMacApplication* Application{nullptr};
    FCocoaWindow*    WindowHandle{nullptr};
};
